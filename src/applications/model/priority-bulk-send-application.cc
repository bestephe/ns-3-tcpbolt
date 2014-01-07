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
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-base.h" // For setting priority
#include "ns3/packet.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "priority-bulk-send-application.h"

NS_LOG_COMPONENT_DEFINE ("PriorityBulkSendApplication");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PriorityBulkSendApplication);

TypeId
PriorityBulkSendApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityBulkSendApplication")
    .SetParent<BulkSendApplication> ()
    .AddConstructor<PriorityBulkSendApplication> ()
  ;
  return tid;
}

// Private helpers

void PriorityBulkSendApplication::SendData (void)
{
  NS_LOG_FUNCTION (this);

  while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more
      uint32_t toSend = m_sendSize;
      double priority;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (m_sendSize, m_maxBytes - m_totBytes);
          //priority = (double) m_maxBytes - m_totBytes;
          //XXX: The priority will now be updated automatically by tcp-socket-base.cc
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
      m_sockStop(m_socket, m_totBytes);
    }
}

} // Namespace ns3
