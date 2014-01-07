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
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "tcp-option-ts.h"
#include "ipv4-l3-protocol.h"

NS_LOG_COMPONENT_DEFINE ("Dctcp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Dctcp);

TypeId
Dctcp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Dctcp")
    .SetParent<TcpNewReno> ()
    .AddConstructor<Dctcp> ()
    .AddAttribute ("ShiftG", "Exponential weight factor for alpha, the loss rate",
                   UintegerValue (4), /* g=1/2^4 */
                   MakeUintegerAccessor (&Dctcp::m_shiftG),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ShiftGamma", "Exponential weight factor for gamma",
                   UintegerValue (2), /* g=1/2^2 */
                   MakeUintegerAccessor (&Dctcp::m_shiftG),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Deadline", "Time at which the flow should complete",
                   TimeValue (Seconds (-1)), /* Negative deadline means no deadline */
                   MakeTimeAccessor (&Dctcp::m_deadline),
                   MakeTimeChecker ())
  ;
  return tid;
}

Dctcp::Dctcp (void)
  : m_ece (false),
    m_cwr (false),
    m_ackedBytesEcn (0),
    m_ackedBytesTotal (0),
    m_alpha (0),
    m_gamma (1),
    m_nextSeq (0),
    m_ceState (false),
    m_deadline (Seconds (-1))
{
  NS_LOG_FUNCTION (this);

  //TODO: Make sure that the L3 protocol as _useEcn set 
  //Ptr<Ipv4L3Protocol> ipv4 = GetNode ()->GetObject<Ipv4L3Protocol> ();
  //ipv4->SetAttribute("ECN", BooleanValue (true));
  //Config::SetDefault ("ns3::Ipv4L3Protocol::ECN", BooleanValue (true));
}

Dctcp::Dctcp (const Dctcp& sock)
  : TcpNewReno (sock),
    m_ece (false),
    m_cwr (false),
    m_ackedBytesEcn (0),
    m_ackedBytesTotal (0),
    m_alpha (0),
    m_gamma (1),
    m_nextSeq (sock.m_nextTxSequence),
    m_ceState (false),
    m_deadline (Seconds (-1))
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");

  //TODO: Make sure that the L3 protocol as _useEcn set 
  //Ptr<Ipv4L3Protocol> ipv4 = GetNode ()->GetObject<Ipv4L3Protocol> ();
  //ipv4->SetAttribute("ECN", BooleanValue (true));
  //Config::SetDefault ("ns3::Ipv4L3Protocol::ECN", BooleanValue (true));
}

Dctcp::~Dctcp (void)
{
}

Ptr<TcpSocketBase>
Dctcp::Fork (void)
{
  return CopyObject<Dctcp> (this);
}

/** Overloading TcpSocketBase::DoForwardUp to intercept the ECN mark in
  * Ipv4Header. Processes the packet before the normal stack. */
void
Dctcp::DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                          Ptr<Ipv4Interface> incomingInterface)
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

      uint32_t scale;
      uint32_t ssThreshOld;
      uint32_t cWndOld;

      if (m_deadline.IsPositive ())
        {
          scale = static_cast<uint32_t> (std::pow((double)m_alpha, m_gamma));
          NS_LOG_LOGIC ("Gamma scaling alpha from " << m_alpha << " to " << scale);
        }
      else
        {
          scale = m_alpha;
        }
      if (scale > 1024)
        scale = 1024;

      cWndOld = m_cWnd;
      m_cWnd = std::max (2 * m_segmentSize, cWndOld - ((cWndOld * scale)>>11));
      ssThreshOld = m_ssThresh;
      ssThreshOld = ssThreshOld; //XXX: Damn compilers not caring about log info
      m_ssThresh = m_cWnd;

      NS_LOG_INFO ("Updated cwnd and ssthresh.  alpha=" << m_alpha <<
        ", cwnd old=" << cWndOld << ", cwnd new=" << m_cWnd <<
        ", ssthresh old=" << ssThreshOld << ", ssthresh new=" << m_ssThresh);
    }

  // Run the normal DoForwardUp() function
  TcpSocketBase::DoForwardUp (packet, header, port, incomingInterface);
}

/** Calculate the number of acked bytes and acked ecn bytes and update
  * alpha, if one RTT has passed, then perform normal Ack processing. */
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
      NS_LOG_LOGIC ("New Ack of " << ackedBytes << " bytes");
    }

  /* Update byte counts */
  if ((t.GetFlags () & TcpHeader::ECE) || t.GetEce ())
    {
      m_ackedBytesEcn += ackedBytes;
    }
  m_ackedBytesTotal += ackedBytes;
  NS_LOG_DEBUG("bytesEcn=" << m_ackedBytesEcn << ", bytesTotal=" << m_ackedBytesTotal);

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

      if (m_deadline.IsPositive ())
        {
          double gammaOld = m_gamma;
          double m_tc1, m_tc2;
          double newgamma, gamma1, gamma2;
          uint32_t cWnd_mss = m_cWnd / m_segmentSize;
          int64_t remaining = ((m_deadline - Simulator::Now ()).GetInteger ()/m_rtt->GetCurrentEstimate ().GetInteger ()) - 1;
          
          m_tc1 = std::sqrt( std::pow(((double)cWnd_mss + 1.0)/2.0,2.0) + 2.0 * (double)m_segmentSize) - ((double)cWnd_mss + 1.0)/2.0;
          if (remaining > 0 && m_tc1 > 0.0)
            gamma1 = m_tc1 / (double) remaining;
          else
            gamma1 = 0.0;
          
          m_tc2 = (double)m_segmentSize / (3.0 * (double)cWnd_mss / 4.0 + 0.5);
          if (remaining > 0 && m_tc2 > 0.0)
            gamma2 = m_tc2 / (double) remaining;
          else
            gamma2 = 0.0;
          
          newgamma = std::max(gamma1, gamma2);
          m_gamma = ((1 - 1/(1<<m_shiftGamma)) * m_gamma) + (1/(1<<m_shiftGamma) * newgamma);
          // Cap m_gamma
          if (m_gamma <= 0.4)
            m_gamma = 0.4;
          else if (m_gamma >= 2.0)
            m_gamma = 2.0;

          NS_LOG_INFO("Time remaining: " << (m_deadline - Simulator::Now ()) <<
            ", gammaOld: " << gammaOld << ", m_gamma: " << m_gamma);
          gammaOld = gammaOld;
        }
      else
        {
          NS_ASSERT (m_gamma == 1);
        }
      
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
      NS_LOG_LOGIC ("IP::CE Marked packet found.");
      
      /* State has changed from CE=0 to CE=1 and we have not yet
       * sent the delayed ACK for the non CE data, so send the ACK. */
      if (m_ceState == false && m_delAckCount > 0)
        {
          NS_ASSERT (m_ece == false);
          NS_LOG_LOGIC ("Sending immediate Ack because state changed to CE");
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
          NS_LOG_LOGIC ("Sending immediate Ack because state changed non-congested");
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
