/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Adrian Sai-wah Tam
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }

#include "dctcp.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "ns3/packet.h"

NS_LOG_COMPONENT_DEFINE ("Dctcp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Dctcp);

TypeId
Dctcp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Dctcp")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<Dctcp> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&Dctcp::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
		    BooleanValue (false),
		    MakeBooleanAccessor (&Dctcp::m_limitedTx),
		    MakeBooleanChecker ())
    .AddAttribute ("ShiftG", "Exponential weight factor for alpha, the loss rate",
                   UintegerValue (4), /* g=1/2^4 */
                   MakeUintegerAccessor (&Dctcp::m_shiftG),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&Dctcp::m_cWnd))
  ;
  return tid;
}

Dctcp::Dctcp (void)
  : m_retxThresh (3), // mute valgrind, actual value set by the attribute system
    m_inFastRec (false),
    m_limitedTx (false), // mute valgrind, actual value set by the attribute system
    m_ece (false),
    m_cwr (false),
    m_ackedBytesEcn (0),
    m_ackedBytesTotal (0),
    m_alpha (0),
    m_nextSeq (0),
    m_ceState (false)
{
  NS_LOG_FUNCTION (this);
}

Dctcp::Dctcp (const Dctcp& sock)
  : TcpSocketBase (sock),
    m_cWnd (sock.m_cWnd),
    m_ssThresh (sock.m_ssThresh),
    m_initialCWnd (sock.m_initialCWnd),
    m_retxThresh (sock.m_retxThresh),
    m_inFastRec (false),
    m_limitedTx (sock.m_limitedTx),
    m_ece (false),
    m_cwr (false),
    m_ackedBytesEcn (0),
    m_ackedBytesTotal (0),
    m_alpha (0),
    m_nextSeq (sock.m_nextTxSequence),
    m_ceState (false)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

Dctcp::~Dctcp (void)
{
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
Dctcp::Listen (void)
{
  NS_LOG_FUNCTION (this);
  InitializeCwnd ();
  return TcpSocketBase::Listen ();
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
Dctcp::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  InitializeCwnd ();
  return TcpSocketBase::Connect (address);
}

/** Limit the size of in-flight data by cwnd and receiver's rxwin */
uint32_t
Dctcp::Window (void)
{
  NS_LOG_FUNCTION (this);
  //XXX: Disabling receiver window limiting.
  // We don't want to be host limited anyways
  //return std::min (m_rWnd.Get (), m_cWnd.Get ());
  return m_cWnd.Get ();
}

Ptr<TcpSocketBase>
Dctcp::Fork (void)
{
  return CopyObject<Dctcp> (this);
}

/** New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
Dctcp::NewAck (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("Dctcp receieved ACK for seq " << seq <<
                " cwnd " << m_cWnd <<
                " ssthresh " << m_ssThresh);

  // Check for exit condition of fast recovery
  if (m_inFastRec && seq < m_recover)
    { // Partial ACK, partial window deflation (RFC2582 sec.3 bullet #5 paragraph 3)
      m_cWnd -= seq - m_txBuffer.HeadSequence ();
      m_cWnd += m_segmentSize;  // increase cwnd
      NS_LOG_INFO ("Partial ACK in fast recovery: cwnd set to " << m_cWnd);
      TcpSocketBase::NewAck (seq); // update m_nextTxSequence and send new data if allowed by window
      DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
      return;
    }
  else if (m_inFastRec && seq >= m_recover)
    { // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
      m_cWnd = std::min (m_ssThresh, BytesInFlight () + m_segmentSize);
      m_inFastRec = false;
      NS_LOG_INFO ("Received full ACK. Leaving fast recovery with cwnd set to " << m_cWnd);
    }

  // Increase of cwnd based on current phase (slow start or congestion avoidance)
  if (m_cWnd < m_ssThresh)
    { // Slow start mode, add one segSize to cWnd. Default m_ssThresh is 65535. (RFC2001, sec.1)
      m_cWnd += m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }
  else
    { // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
      // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
      double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();
      adder = std::max (1.0, adder);
      m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }

  // Complete newAck processing
  TcpSocketBase::NewAck (seq);
}

/** Cut cwnd and enter fast recovery mode upon triple dupack */
void
Dctcp::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
      m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_recover = m_highTxMark;
      m_inFastRec = true;
      NS_LOG_INFO ("Triple dupack. Enter fast recovery mode. Reset cwnd to " << m_cWnd <<
                   ", ssthresh to " << m_ssThresh << " at fast recovery seqnum " << m_recover);
      DoRetransmit ();
    }
  else if (m_inFastRec)
    { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      m_cWnd += m_segmentSize;
      NS_LOG_INFO ("Dupack in fast recovery mode. Increase cwnd to " << m_cWnd);
      SendPendingData (m_connected);
    }
  else if (!m_inFastRec && m_limitedTx && m_txBuffer.SizeFromSequence (m_nextTxSequence) > 0)
    { // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
      NS_LOG_INFO ("Limited transmit");
      uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
      m_nextTxSequence += sz;                    // Advance next tx sequence
    };
}

/** Retransmit timeout */
void
Dctcp::Retransmit (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
  m_inFastRec = false;

  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT) return;
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark) return;

  // According to RFC2581 sec.3.1, upon RTO, ssthresh is set to half of flight
  // size and cwnd is set to 1*MSS, then the lost packet is retransmitted and
  // TCP back to slow start
  m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
  m_cWnd = m_segmentSize;
  m_nextTxSequence = m_txBuffer.HeadSequence (); // Restart from highest Ack
  NS_LOG_INFO ("RTO. Reset cwnd to " << m_cWnd <<
               ", ssthresh to " << m_ssThresh << ", restart from seqnum " << m_nextTxSequence);
  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  DoRetransmit ();                          // Retransmit the packet
}

void
Dctcp::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "Dctcp::SetSegSize() cannot change segment size after connection started.");
  m_segmentSize = size;
}

void
Dctcp::SetSSThresh (uint32_t threshold)
{
  m_ssThresh = threshold;
}

uint32_t
Dctcp::GetSSThresh (void) const
{
  return m_ssThresh;
}

void
Dctcp::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "Dctcp::SetInitialCwnd() cannot change initial cwnd after connection started.");
  m_initialCWnd = cwnd;
}

uint32_t
Dctcp::GetInitialCwnd (void) const
{
  return m_initialCWnd;
}

void 
Dctcp::InitializeCwnd (void)
{
  /*
   * Initialize congestion window, default to 1 MSS (RFC2001, sec.1) and must
   * not be larger than 2 MSS (RFC2581, sec.3.1). Both m_initiaCWnd and
   * m_segmentSize are set by the attribute system in ns3::TcpSocket.
   */
  m_cWnd = m_initialCWnd * m_segmentSize;
}

//XXX: DCTCP Specific functions
void
Dctcp::DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this);

  /* Check if the IP Header encountered congestion */
  CheckCe (header);

  /* Check for ECE, which signals congestions */
  TcpHeader tcpHeader;
  packet->PeekHeader (tcpHeader);
  if ((tcpHeader.GetFlags () & TcpHeader::ECE) || tcpHeader.GetEce ())
    {
      // Peer echoed ECN. Reduce window
      NS_LOG_LOGIC ("Received a peer echoed ECN. Reducing window.");
      NS_LOG_UNCOND ("Received a peer echoed ECN. Reducing window.");

      //XXX: Remove ECE flag because it breaks the rest of the TCP Stack
#if 0
      uint8_t flags = tcpHeader.GetFlags ();
      std::ostringstream oss = AsciiToBinary (flags);
      NS_LOG_UNCOND ("current flags: " << std::bitset<8> (flags));
      flags = flags & (~TcpHeader::ECE);
      NS_LOG_UNCOND ("new flags: " << flags);
      tcpHeader.SetFlags (flags);
      //tcpHeader.SetFlags (tcpHeader.GetFlags () & (~TcpHeader::ECE));
#endif

      uint32_t ssThreshOld;
      uint32_t cWndOld;

      cWndOld = m_cWnd;
      m_cWnd = std::max (2 * m_segmentSize, cWndOld - ((cWndOld * m_alpha)>>11));
      ssThreshOld = m_ssThresh;
      m_ssThresh = m_cWnd;

      NS_LOG_INFO ("Updated cwnd and ssthresh.  alpha=" << m_alpha <<
        ", cwnd old=" << cWndOld << ", cwnd new=" << m_cWnd <<
        ", ssthresh old=" << ssThreshOld << ", ssthresh new=" << m_ssThresh);
    }

  // Run the normal DoForwardUp() function
  TcpSocketBase::DoForwardUp (packet, header, port, incomingInterface);
}

void
Dctcp::ReceivedAck (Ptr<Packet> packet, const TcpHeader& t)
{
  uint32_t ackedBytes = 0;

  NS_LOG_FUNCTION (this << t);

  if (t.GetAckNumber () == m_txBuffer.HeadSequence ())
    { // Case 2: Potentially a duplicated ACK
      if (t.GetAckNumber () < m_nextTxSequence)
        {
          NS_LOG_LOGIC ("DupAck of " << t.GetAckNumber ());
          /* Count duplicate ACKs for Retransmission packets and so on as MSS size */
          ackedBytes = m_segmentSize;
        }
      // otherwise, the ACK is precisely equal to the nextTxSequence
      NS_ASSERT (t.GetAckNumber () <= m_nextTxSequence);
    }
  else if (t.GetAckNumber () > m_txBuffer.HeadSequence ())
    { // Case 3: New ACK
      ackedBytes = t.GetAckNumber () - m_txBuffer.HeadSequence ();
    }

  /* Update byte counts */
  if (t.GetFlags () & TcpHeader::ECE)
    {
      m_ackedBytesEcn += ackedBytes;
    }
  m_ackedBytesTotal += ackedBytes;

  /* Expired RTT */
  if (!(t.GetAckNumber () < m_nextSeq))
    {
      /* For avoiding denominator == 1 */
      if (m_ackedBytesTotal == 0) m_ackedBytesTotal = 1;

      uint32_t alphaOld = m_alpha; 

      /* alpha = (1-g) * alpha + g * F */
      m_alpha = alphaOld - (alphaOld >> m_shiftG)
        + (m_ackedBytesEcn << (10 - m_shiftG)) / m_ackedBytesTotal;  
      
      if (m_alpha > 1024) m_alpha = 1024; /* round to 0-1024 */

      NS_LOG_INFO("bytesEcn=" << m_ackedBytesEcn << ", bytesTotal=" <<
        m_ackedBytesTotal << ", alpha: old=" << alphaOld << ", new=" << m_alpha);
      
      m_ackedBytesEcn = 0;
      m_ackedBytesTotal = 0;
      m_nextSeq = m_nextTxSequence;
    }

  // Complete ReceivedAck processing
  TcpSocketBase::ReceivedAck (packet, t);
}

void
Dctcp::CheckCe (Ipv4Header header)
{
  if (header.GetEcn() == Ipv4Header::CE)
    {
      NS_LOG_LOGIC ("Marked packet found.");
      NS_LOG_UNCOND ("Marked packet found.");
      
      /* State has changed from CE=0 to CE=1 and we have not yet
       * sent the delayed ACK for the non CE data, so send the ACK. */
      if (m_ceState == false && m_delAckCount > 0)
        {
          NS_ASSERT (m_ece == false);
          SendEmptyPacket (TcpHeader::ACK);
        }
      
      m_ceState = true;
      m_ece = true;
    }
  else
    {
      /* State has changed from CE=1 to CE=0 and we have not yet
       * sent the delayed ACK for the CE data, so send the ACK. */
      if (m_ceState == true && m_delAckCount > 0)
        {
          NS_ASSERT (m_ece == true);
          SendEmptyPacket (TcpHeader::ACK);
        }

      m_ceState = false;
      m_ece = false;
    }
}

void
Dctcp::AddOptions (TcpHeader& h)
{
  // Put flags into headers
  if (m_ece || m_cwr)
    {
      uint8_t flags = h.GetFlags ();
      if (m_ece) flags |= TcpHeader::ECE;
      if (m_cwr) flags |= TcpHeader::CWR;
      h.SetFlags (flags);
    }
}

} // namespace ns3
