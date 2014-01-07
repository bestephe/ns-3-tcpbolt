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
#include "ns3/incast-send.h"

NS_LOG_COMPONENT_DEFINE ("IncastSender");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (IncastSender);

TypeId
IncastSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IncastSender")
    .SetParent<Application> ()
    .AddConstructor<IncastSender> ()
    .AddAttribute ("Port",
                   "TCP port number for the incast applications",
                   UintegerValue (5000),
                   MakeUintegerAccessor (&IncastSender::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Protocol", "The type of connection-oriented protocol to use.",
                   TypeIdValue (TcpNewReno::GetTypeId ()),
                   MakeTypeIdAccessor (&IncastSender::m_tid),
                   MakeTypeIdChecker ())
  ;
  return tid;
}


IncastSender::IncastSender () : m_listensock (0)
{
  NS_LOG_FUNCTION (this);
}

IncastSender::~IncastSender ()
{
  NS_LOG_FUNCTION (this);
}

void
IncastSender::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_listensock = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void IncastSender::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Connection responder. Wait for connection and send data.
   m_sockCounts.clear ();
   m_sockSrus.clear ();
   //m_listensock  = GetNode ()->GetObject<TcpL4Protocol> ()->CreateSocket(m_tid);
   m_listensock = Socket::CreateSocket (GetNode (), m_tid);
   m_listensock->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_port));
   m_listensock->Listen ();
   m_listensock->SetAcceptCallback (
     MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
     MakeCallback (&IncastSender::HandleAccept, this));
   m_listensock->SetCloseCallbacks (
     MakeCallback (&IncastSender::HandleClose, this),
     MakeCallback (&IncastSender::HandleClose, this));
    
}

void IncastSender::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  //NS_ASSERT (m_sockSrus.find (socket) == m_sockCounts.end ());
  NS_ASSERT (m_sockSrus[socket] == 0);
  Ptr<Packet> packet;
  packet = socket->Recv ();
  uint32_t sru;
  NS_ASSERT (packet->GetSize () == sizeof(sru));
  packet->CopyData ((uint8_t *)&sru, sizeof(sru));
  NS_LOG_LOGIC ("Read an sru of " << sru);
  m_sockSrus[socket] = sru;
  SetupSocket (socket);

  socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  socket->SetSendCallback (MakeCallback (&IncastSender::HandleSend, this));
  HandleSend(socket, 10000);
}

void IncastSender::SetupSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
}

void IncastSender::HandleSend (Ptr<Socket> socket, uint32_t n)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sockCounts.find (socket) != m_sockCounts.end ());

  uint32_t sru = m_sockSrus[socket];
  if (sru <= 0)
    return;
  uint32_t sentCount = m_sockCounts[socket];
  while (sentCount < sru && socket->GetTxAvailable ())
    { // Not yet finish the one SRU data
      Ptr<Packet> packet = Create<Packet> (std::min(sru - sentCount, n));
      int actual = socket->Send (packet);
      if (actual > 0) 
        {
          sentCount += actual;
          m_sockCounts[socket] = sentCount;
        }
      NS_LOG_LOGIC(actual << " bytes sent");
    }
  // If finished, close the socket
  if (sentCount == sru)
    {
      Address sockAddr;
      socket->GetSockName (sockAddr);
      NS_LOG_LOGIC("Finished sending " << sru << "bytes. Socket closed for " << sockAddr);
      socket->Close ();
      //m_sockCounts.erase (socket);
    }
}

void IncastSender::HandleAccept (Ptr<Socket> socket, const Address& from)
{
  NS_LOG_FUNCTION (this << socket);
  InetSocketAddress addr = InetSocketAddress::ConvertFrom(from);
  addr = addr;
  NS_LOG_LOGIC ("Accepting connection from " << addr.GetIpv4() << ":" <<addr.GetPort());
  m_sockCounts[socket] = 0; /* Initialize the sentCount */
  m_sockSrus[socket] = 0; /* Initialize an invalid SRU */
  socket->SetRecvCallback (MakeCallback (&IncastSender::HandleRead, this));
  //socket->SetSendCallback (MakeCallback (&IncastSender::HandleSend, this));
}

void IncastSender::HandleClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_sockCounts.erase (socket);
  m_sockSrus.erase (socket);
}

void IncastSender::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if (m_listensock) m_listensock->Close ();
}

size_t PtrHash::operator() (Ptr<Object> const &x) const
{ 
  void *ptr = PeekPointer (x);
  sgi::hash<unsigned long> hash;
  return hash ((unsigned long) ptr);
}

} // Namespace ns3
