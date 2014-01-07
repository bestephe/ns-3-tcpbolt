/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 New York University
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

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/internet-module.h"
#include "ns3/priority-incast-agg.h"

NS_LOG_COMPONENT_DEFINE ("PriorityIncastAggregator");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PriorityIncastAggregator);

TypeId
PriorityIncastAggregator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityIncastAggregator")
    .SetParent<IncastAggregator> ()
    .AddConstructor<PriorityIncastAggregator> ()
  ;
  return tid;
}

// Application Methods
#if 0
void PriorityIncastAggregator::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Do nothing if incast is running
  if (m_running) return;

  m_running = true;
  m_closeCount = 0;
  m_byteCount = 0;
  m_start (this->GetObject<PriorityIncastAggregator> ());

  // Connection initiator. Connect to each peer and wait for data.
  for (std::vector<Ipv4Address>::iterator i = m_senders.begin (); i != m_senders.end (); ++i)
    {
      Ptr<Socket> s = Socket::CreateSocket (GetNode (), m_tid);
      // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
      if (s->GetSocketType () != Socket::NS3_SOCK_STREAM &&
          s->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
          NS_FATAL_ERROR ("Only NS_SOCK_STREAM or NS_SOCK_SEQPACKET sockets are allowed.");
        }

      double priority = sizeof(m_sru);
      NS_LOG_LOGIC ("setting the priority to " << priority);
      Ptr<TcpSocketBase> tcpSock = s->GetObject<TcpSocketBase> ();
      if (!tcpSock) NS_LOG_ERROR ("Requires a TcpSocketBase socket!");
      tcpSock->SetAttribute("Priority", DoubleValue (priority));

      // Bind, connect, and wait for data
      NS_LOG_LOGIC ("Connect to " << *i);
      s->Bind ();
      m_sockStart (s);
      s->Connect (InetSocketAddress (*i, m_port));
      s->SetSendCallback (MakeCallback (&PriorityIncastAggregator::HandleSend, this));
      s->SetRecvCallback (MakeCallback (&PriorityIncastAggregator::HandleRead, this));
      s->SetCloseCallbacks (
        MakeCallback (&PriorityIncastAggregator::HandleClose, this),
        MakeCallback (&PriorityIncastAggregator::HandleClose, this));

#if 0
      // Not sure what TcpAdmCtrl is for...
      Ptr<TcpAdmCtrl> tcpAdm = s->GetObject<TcpAdmCtrl> ();
      if (tcpAdm != 0)
        tcpAdm->SetAdmCtrlCallback (MakeCallback (&PriorityIncastAggregator::HandleAdmCtrl, this));
#endif

      // Remember this socket, in case we need to terminate all of them prematurely
      m_sockets.push_back(s);
    }
}
#endif

void PriorityIncastAggregator::HandleSend (Ptr<Socket> socket, uint32_t n)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT(n > sizeof(m_sru));

  double priority = sizeof(m_sru);
  NS_LOG_LOGIC ("setting the priority to " << priority);
  Ptr<TcpSocketBase> tcpSock = socket->GetObject<TcpSocketBase> ();
  if (!tcpSock) NS_LOG_ERROR ("Requires a TcpSocketBase socket!");
  tcpSock->SetAttribute("Priority", DoubleValue (priority));

  Ptr<Packet> packet = Create<Packet> ((uint8_t *)&m_sru, sizeof(m_sru));
  int actual = socket->Send (packet);
  NS_ASSERT(actual == sizeof(m_sru));
  socket->SetSendCallback (MakeNullCallback<void, Ptr<Socket>, uint32_t> ());
  //socket->ShutdownSend ();
}

} // Namespace ns3
