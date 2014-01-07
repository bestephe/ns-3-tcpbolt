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
#include "ns3/incast-agg.h"

NS_LOG_COMPONENT_DEFINE ("IncastAggregator");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (IncastAggregator);

TypeId
IncastAggregator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IncastAggregator")
    .SetParent<Application> ()
    .AddConstructor<IncastAggregator> ()
    .AddAttribute ("SRU",
                   "Size of one server request unit. Only for flow size computation",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&IncastAggregator::m_sru),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Port",
                   "TCP port number for the incast applications",
                   UintegerValue (5000),
                   MakeUintegerAccessor (&IncastAggregator::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Protocol", "The type of connection-oriented protocol to use.",
                   TypeIdValue (TcpNewReno::GetTypeId ()),
                   MakeTypeIdAccessor (&IncastAggregator::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("SockStart", "A transfer (socket) starts",
                     MakeTraceSourceAccessor (&IncastAggregator::m_sockStart))
    .AddTraceSource ("SockStop", "A transfer (socket) stops",
                     MakeTraceSourceAccessor (&IncastAggregator::m_sockStop))
    .AddTraceSource ("Start", "A transfer (socket) starts",
                     MakeTraceSourceAccessor (&IncastAggregator::m_start))
    .AddTraceSource ("Stop", "A transfer (socket) stops",
                     MakeTraceSourceAccessor (&IncastAggregator::m_stop))
  ;
  return tid;
}


IncastAggregator::IncastAggregator ()
  : m_running (false),
    m_closeCount (0),
    m_byteCount (0)
{
  NS_LOG_FUNCTION (this);
}

IncastAggregator::~IncastAggregator ()
{
  NS_LOG_FUNCTION (this);
}

void
IncastAggregator::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_sockets.clear ();
  // chain up
  Application::DoDispose ();
}

void
IncastAggregator::SetSenders (const std::vector<Ipv4Address>& n)
{
  NS_LOG_FUNCTION (this);
  m_senders = n;
}

// Application Methods
void IncastAggregator::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Do nothing if incast is running
  if (m_running) return;

  m_running = true;
  m_closeCount = 0;
  m_byteCount = 0;
  m_start (this->GetObject<IncastAggregator> ());

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

      // Bind, connect, and wait for data
      NS_LOG_LOGIC ("Connect to " << *i);
      s->Bind ();
      m_sockStart (s);
      s->Connect (InetSocketAddress (*i, m_port));
      s->SetSendCallback (MakeCallback (&IncastAggregator::HandleSend, this));
      s->SetRecvCallback (MakeCallback (&IncastAggregator::HandleRead, this));
      s->SetCloseCallbacks (
        MakeCallback (&IncastAggregator::HandleClose, this),
        MakeCallback (&IncastAggregator::HandleClose, this));

#if 0
      // Not sure what TcpAdmCtrl is for...
      Ptr<TcpAdmCtrl> tcpAdm = s->GetObject<TcpAdmCtrl> ();
      if (tcpAdm != 0)
        tcpAdm->SetAdmCtrlCallback (MakeCallback (&IncastAggregator::HandleAdmCtrl, this));
#endif

      // Remember this socket, in case we need to terminate all of them prematurely
      m_sockets.push_back(s);
    }
}

#if 0
void IncastAggregator::HandleAdmCtrl (Ptr<TcpAdmCtrl> sock, Ipv4Address addr, uint16_t port)
{
  NS_LOG_FUNCTION (this << addr << port);
  m_suspendedSocket.push_back(sock);
}
#endif

void IncastAggregator::HandleSend (Ptr<Socket> socket, uint32_t n)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT(n > sizeof(m_sru));
  Ptr<Packet> packet = Create<Packet> ((uint8_t *)&m_sru, sizeof(m_sru));
  int actual = socket->Send (packet);
  NS_ASSERT(actual == sizeof(m_sru));
  socket->SetSendCallback (MakeNullCallback<void, Ptr<Socket>, uint32_t> ());
  //socket->ShutdownSend ();
}

void IncastAggregator::HandleRead (Ptr<Socket> socket)
{
  // Discard all data being read
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  while (packet = socket->Recv ())
    {
      m_byteCount += packet->GetSize ();
    };
}

void IncastAggregator::HandleClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  ++m_closeCount;
  m_sockStop(socket, m_sru);
  m_sockets.remove (socket);
  if (m_closeCount == m_senders.size ())
    {
      // Start next round of incast
      m_running = false;
      m_closeCount = 0;
      Simulator::ScheduleNow (&IncastAggregator::RoundFinish, this);
    }
#if 0
  else if (!m_suspendedSocket.empty())
    {
      // Replace the terminated connection with a suspended one
      Ptr<TcpAdmCtrl> sock = m_suspendedSocket.front();
      m_suspendedSocket.pop_front();
      sock->ResumeConnection ();
    }
#endif
}

void IncastAggregator::RoundFinish (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_byteCount == m_sru * m_senders.size ());
  m_stop (this->GetObject<IncastAggregator> (), m_sru * m_senders.size ());
  if (!m_roundFinish.IsNull ())
    m_roundFinish (this->GetObject<IncastAggregator> ());
}

void IncastAggregator::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  for (std::list<Ptr<Socket> >::iterator i = m_sockets.begin (); i != m_sockets.end (); ++i)
    {
      (*i)->Close ();
    }
}

void IncastAggregator::SetRoundFinishCallback (Callback<void, Ptr<IncastAggregator> > cb)
{
  NS_LOG_FUNCTION (this);
  m_roundFinish = cb;
};

} // Namespace ns3
