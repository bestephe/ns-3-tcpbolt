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

#include "tcp-admctrl.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "tcp-option-ts.h"

NS_LOG_COMPONENT_DEFINE ("TcpAdmCtrl");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpAdmCtrl);

TypeId
TcpAdmCtrl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpAdmCtrl")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpAdmCtrl> ()
  ;
  return tid;
}

TcpAdmCtrl::TcpAdmCtrl (void)
  : m_lastTS (0), m_ece (false), m_cwr (false)
{
  NS_LOG_FUNCTION (this);
}

TcpAdmCtrl::TcpAdmCtrl (const TcpAdmCtrl& sock)
  : TcpNewReno (sock), m_lastTS (0), m_ece (false), m_cwr (false)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpAdmCtrl::~TcpAdmCtrl (void)
{
}

Ptr<TcpSocketBase>
TcpAdmCtrl::Fork (void)
{
  return CopyObject<TcpAdmCtrl> (this);
}

/** Overloading TcpSocketBase::DoForwardUp to intercept the ECN mark in Ipv4Header */
void
TcpAdmCtrl::DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                          Ptr<Ipv4Interface> incomingInterface)
{
  TcpHeader tcpHeader;
  packet->PeekHeader (tcpHeader);
  if (header.GetEcn() == Ipv4Header::CE && !m_admCtrlCallback.IsNull ())
    { // ECN-marked, if it is a SYN packet, hold it back for a while
      if (tcpHeader.GetFlags () & TcpHeader::SYN)
        {
          NS_LOG_LOGIC("Marked handshake packet found.");
          m_bufPacket = packet;
          m_bufHeader = header;
          m_bufPort = port;
          m_bufIface = incomingInterface;
          m_admCtrlCallback (this, header.GetSource(), port);
          return;
        }
      else
        { // Non handshake packet: Cut down window
          m_ece = true;
        }
    }
  if ((tcpHeader.GetFlags () & TcpHeader::ECE) && ! m_cwr)
    {
      // Peer echoed ECN. Reduce window
      m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_cwr = true;
    }
  else if (!(tcpHeader.GetFlags () & TcpHeader::ECE) && m_cwr)
    { // Peer acknowledged my window reduction. Reset CWR flag
      m_cwr = false;
    };
  if (tcpHeader.GetFlags () & TcpHeader::CWR)
    { // Peer reduced window. Reset ECE flag
      m_ece = false;
    }
  // Run the normal DoForwardUp() function
  TcpSocketBase::DoForwardUp(packet, header, port, incomingInterface);
}

void
TcpAdmCtrl::SetAdmCtrlCallback(Callback<void, Ptr<TcpAdmCtrl>, Ipv4Address, uint16_t> cb)
{
  m_admCtrlCallback = cb;
}

void
TcpAdmCtrl::ResumeConnection (void)
{
  NS_LOG_FUNCTION(this);
  TcpSocketBase::DoForwardUp(m_bufPacket, m_bufHeader, m_bufPort, m_bufIface);
  m_bufPacket = 0;
  m_bufIface = 0;
}

void
TcpAdmCtrl::AddOptions (TcpHeader& h)
{
  Ptr<TcpOptionTS> op = CreateObject<TcpOptionTS> ();
  op->SetTimestamp (uint32_t (Now ().GetNanoSeconds () & std::numeric_limits<uint32_t>::max ()));
  op->SetEcho (m_lastTS);
  h.AppendOption (op);
  // Put flags into headers
  if (m_ece || m_cwr)
    {
      uint8_t flags = h.GetFlags ();
      if (m_ece) flags |= TcpHeader::ECE;
      if (m_cwr) flags |= TcpHeader::CWR;
      h.SetFlags (flags);
    }
}

void
TcpAdmCtrl::ReadOptions (const TcpHeader& h)
{
  Ptr<TcpOption> opt = h.GetOption (8);
  if (opt == NULL) return;
  Ptr<TcpOptionTS> op = opt->GetObject<TcpOptionTS> ();

  m_lastTS = op->GetTimestamp ();
  if (! m_inFastRec) return;
  // RFC 3626 algorithm:
  //  echoTS is 'greater than' m_retxTS if
  //    echoTS > m_retxTX && echoTS - m_retxTS <= MAXVALUE/2
  // OR echoTS < m_retxTS && m_retxTS - echoTS > MAXVALUE/2
  uint32_t echoTS  = op->GetEcho ();
  static const uint32_t halfMax = std::numeric_limits<uint32_t>::max ()/2;
  if (((echoTS > m_retxTS) && (echoTS - m_retxTS) <= halfMax)
      || ((echoTS < m_retxTS) && (m_retxTS - echoTS) > halfMax))
    { // The retransmitted packet is probably lost again!
      DoRetransmit ();
    }
}

void
TcpAdmCtrl::DoRetransmit (void)
{
  m_retxTS = uint32_t (Now ().GetNanoSeconds () & std::numeric_limits<uint32_t>::max ());
  TcpSocketBase::DoRetransmit ();
}

} // namespace ns3
