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
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "persistent-bulk-send-application.h"

NS_LOG_COMPONENT_DEFINE ("PersistentBulkSendApplication");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PersistentBulkSendApplication);

TypeId
PersistentBulkSendApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PersistentBulkSendApplication")
    .SetParent<Application> ()
    .AddConstructor<PersistentBulkSendApplication> ()
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&PersistentBulkSendApplication::m_sendSize),
                   MakeUintegerChecker<uint64_t> (1))
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&PersistentBulkSendApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&PersistentBulkSendApplication::m_txTrace))
    .AddTraceSource ("Start", "The transfer starts",
                     MakeTraceSourceAccessor (&PersistentBulkSendApplication::m_sockStart))
    .AddTraceSource ("Stop", "The transfer stops",
                     MakeTraceSourceAccessor (&PersistentBulkSendApplication::m_sockStop))
  ;
  return tid;
}


PersistentBulkSendApplication::PersistentBulkSendApplication ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0)
{
  NS_LOG_FUNCTION (this);
}

PersistentBulkSendApplication::~PersistentBulkSendApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
PersistentBulkSendApplication::SetRemotes (std::vector<Address> remotes)
{
  NS_LOG_FUNCTION (this);
  m_peers = remotes;
}

void
PersistentBulkSendApplication::SetSizes (std::vector<uint64_t> sizes)
{
  NS_LOG_FUNCTION (this);
  m_sizes = sizes;
}

Ptr<Socket>
PersistentBulkSendApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
PersistentBulkSendApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void PersistentBulkSendApplication::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      //StartNewSocket ();
      Simulator::Schedule (Seconds (UniformVariable ().GetValue(0, 1000e-6)), &PersistentBulkSendApplication::StartNewSocket, this);
    }
  if (m_connected)
    {
      SendData ();
    }
}

void PersistentBulkSendApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("PersistentBulkSendApplication found null socket to close in StopApplication");
    }
}


// Private helpers
void
PersistentBulkSendApplication::StartNewSocket (void)
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
      NS_FATAL_ERROR ("Using PersistentBulkSend with an incompatible socket type. "
                      "PersistentBulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                      "In other words, use TCP instead of UDP.");
    }

  m_socket->Bind ();
  m_sockStart(m_socket);
  m_socket->Connect (peer);
  m_socket->ShutdownRecv ();
  m_socket->SetConnectCallback (
    MakeCallback (&PersistentBulkSendApplication::ConnectionSucceeded, this),
    MakeCallback (&PersistentBulkSendApplication::ConnectionFailed, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&PersistentBulkSendApplication::ConnectionClosed, this),
    MakeCallback (&PersistentBulkSendApplication::ConnectionClosed, this));
  m_socket->SetSendCallback (
    MakeCallback (&PersistentBulkSendApplication::DataSend, this));
}

void PersistentBulkSendApplication::SendData (void)
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
      Simulator::Schedule (Seconds (UniformVariable ().GetValue (0, 1000e-6)), &PersistentBulkSendApplication::StartNewSocket, this);
      //StartNewSocket ();
    }
}

void PersistentBulkSendApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("PersistentBulkSendApplication Connection succeeded");
  m_connected = true;
  SendData ();
}

void PersistentBulkSendApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("PersistentBulkSendApplication, Connection Failed");
}

void PersistentBulkSendApplication::ConnectionClosed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_sockStop(m_socket, m_totBytes);
}

void PersistentBulkSendApplication::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);

  if (m_connected)
    { // Only send new data if the connection has completed
      Simulator::ScheduleNow (&PersistentBulkSendApplication::SendData, this);
    }
}



} // Namespace ns3
