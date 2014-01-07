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
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/internet-module.h"
#include "ns3/priority-incast-send.h"
#include "ns3/priority-tag.h"
#include "ns3/tcp-socket-base.h" //XXX: for setting priority

NS_LOG_COMPONENT_DEFINE ("PriorityIncastSender");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PriorityIncastSender);

TypeId
PriorityIncastSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityIncastSender")
    .SetParent<IncastSender> ()
    .AddConstructor<PriorityIncastSender> ()
  ;
  return tid;
}

void PriorityIncastSender::HandleSend (Ptr<Socket> socket, uint32_t n)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sockCounts.find (socket) != m_sockCounts.end ());

  uint32_t sru = m_sockSrus[socket];
  uint32_t sentCount = m_sockCounts[socket];
  while (sentCount < sru && socket->GetTxAvailable ())
    { 
      //double priority = (double) sru - sentCount;
      //The priority is now updated after ACKs by tcp-socket-base.cc
      double priority = (double) sru;
      NS_LOG_LOGIC ("setting the priority to " << priority);
      Ptr<TcpSocketBase> tcpSock = socket->GetObject<TcpSocketBase> ();
      if (!tcpSock) NS_LOG_ERROR ("Requires a TcpSocketBase socket!");
      tcpSock->SetAttribute("Priority", DoubleValue (priority));
    
      // Not yet finish the one SRU data
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
      NS_LOG_LOGIC("Socket closed for " << sockAddr);
      socket->Close ();
      //m_sockCounts.erase (socket);
    }
}

} // Namespace ns3
