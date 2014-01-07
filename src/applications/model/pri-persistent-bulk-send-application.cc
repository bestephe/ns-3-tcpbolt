/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/random-variable.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-base.h" // For setting priority
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "pri-persistent-bulk-send-application.h"

NS_LOG_COMPONENT_DEFINE ("PriorityPersistentBulkSendApplication");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PriorityPersistentBulkSendApplication);

TypeId
PriorityPersistentBulkSendApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityPersistentBulkSendApplication")
    .SetParent<Application> ()
    .AddConstructor<PriorityPersistentBulkSendApplication> ()
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&PriorityPersistentBulkSendApplication::m_sendSize),
                   MakeUintegerChecker<uint64_t> (1))
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&PriorityPersistentBulkSendApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&PriorityPersistentBulkSendApplication::m_txTrace))
    .AddTraceSource ("Start", "The transfer starts",
                     MakeTraceSourceAccessor (&PriorityPersistentBulkSendApplication::m_sockStart))
    .AddTraceSource ("Stop", "The transfer stops",
                     MakeTraceSourceAccessor (&PriorityPersistentBulkSendApplication::m_sockStop))
  ;
  return tid;
}


PriorityPersistentBulkSendApplication::PriorityPersistentBulkSendApplication ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0)
{
  NS_LOG_FUNCTION (this);
}

PriorityPersistentBulkSendApplication::~PriorityPersistentBulkSendApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
PriorityPersistentBulkSendApplication::SetRemotes (std::vector<Address> remotes)
{
  NS_LOG_FUNCTION (this);
  m_peers = remotes;
}

void
PriorityPersistentBulkSendApplication::SetSizes (std::vector<uint64_t> sizes)
{
  NS_LOG_FUNCTION (this);
  m_sizes = sizes;
}

Ptr<Socket>
PriorityPersistentBulkSendApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
PriorityPersistentBulkSendApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void PriorityPersistentBulkSendApplication::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      //StartNewSocket ();
      Simulator::Schedule (Seconds (UniformVariable ().GetValue(0, 1000e-6)), &PriorityPersistentBulkSendApplication::StartNewSocket, this);
    }
  if (m_connected)
    {
      SendData ();
    }
}

void PriorityPersistentBulkSendApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("PriorityPersistentBulkSendApplication found null socket to close in StopApplication");
    }
}


// Private helpers
void
PriorityPersistentBulkSendApplication::StartNewSocket (void)
{
  NS_LOG_FUNCTION (this);

  /* Set the totBytes and the remote peer */
  m_totBytes = 0;
  int bytesI = UniformVariable ().GetInteger (0, m_sizes.size () - 1);
  m_maxBytes = m_sizes[bytesI];
  int peerI = UniformVariable ().GetInteger (0, m_peers.size () - 1);
  Address peer = m_peers[peerI];
  NS_LOG_LOGIC ("Sending " << m_maxBytes << " bytes to remote " << peer);

  m_socket = Socket::CreateSocket (GetNode (), m_tid);

  // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
  if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
      m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
    {
      NS_FATAL_ERROR ("Using PriorityPersistentBulkSend with an incompatible socket type. "
                      "PriorityPersistentBulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                      "In other words, use TCP instead of UDP.");
    }

  m_socket->Bind ();
  m_sockStart(m_socket);
  m_socket->Connect (peer);
  m_socket->ShutdownRecv ();
  m_socket->SetConnectCallback (
    MakeCallback (&PriorityPersistentBulkSendApplication::ConnectionSucceeded, this),
    MakeCallback (&PriorityPersistentBulkSendApplication::ConnectionFailed, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&PriorityPersistentBulkSendApplication::ConnectionClosed, this),
    MakeCallback (&PriorityPersistentBulkSendApplication::ConnectionClosed, this));
  m_socket->SetSendCallback (
    MakeCallback (&PriorityPersistentBulkSendApplication::DataSend, this));
}

void PriorityPersistentBulkSendApplication::SendData (void)
{
  NS_LOG_FUNCTION (this);

  while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more
      uint32_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (m_sendSize, m_maxBytes - m_totBytes);
        }

      double priority;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (m_sendSize, m_maxBytes - m_totBytes);
          //priority = (double) m_maxBytes - m_totBytes;
          //The priority is now updated after ACKs by tcp-socket-base.cc
          priority = (double) m_maxBytes;
        }
      else
        {
          priority = std::numeric_limits<double>::max ();
        }

      NS_LOG_LOGIC ("setting the priority to " << priority);
      Ptr<TcpSocketBase> tcpSock = m_socket->GetObject<TcpSocketBase> ();
      if (!tcpSock)
        {
          NS_LOG_ERROR ("Requires a TcpSocketBase socket!");
          NS_LOG_UNCOND ("Requires a TcpSocketBase socket!"); //XXX
        }
      tcpSock->SetAttribute("Priority", DoubleValue (priority));


      NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
      Ptr<Packet> packet = Create<Packet> (toSend);
      m_txTrace (packet);
      int actual = m_socket->Send (packet);
      if (actual > 0)
        {
          m_totBytes += actual;
        }
      // We exit this loop when actual < toSend as the send side
      // buffer is full. The "DataSent" callback will pop when
      // some buffer space has freed up.
      if ((unsigned)actual != toSend)
        {
          break;
        }
    }
  // Check if time to close (all sent)
  if (m_totBytes == m_maxBytes && m_connected)
    {
      m_socket->Close ();
      m_connected = false;
      Simulator::Schedule (Seconds (UniformVariable ().GetValue (0, 1000e-6)), &PriorityPersistentBulkSendApplication::StartNewSocket, this);
      //StartNewSocket ();
    }
}

void PriorityPersistentBulkSendApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("PriorityPersistentBulkSendApplication Connection succeeded");
  m_connected = true;
  SendData ();
}

void PriorityPersistentBulkSendApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("PriorityPersistentBulkSendApplication, Connection Failed");
}

void PriorityPersistentBulkSendApplication::ConnectionClosed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_sockStop(m_socket, m_totBytes);
}

void PriorityPersistentBulkSendApplication::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);

  if (m_connected)
    { // Only send new data if the connection has completed
      Simulator::ScheduleNow (&PriorityPersistentBulkSendApplication::SendData, this);
    }
}



} // Namespace ns3
