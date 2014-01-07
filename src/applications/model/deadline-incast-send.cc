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
#include "ns3/deadline-incast-send.h"
#include "ns3/tcp-socket-base.h" //XXX: for setting priority
#include "ns3/dctcp.h"

NS_LOG_COMPONENT_DEFINE ("DeadlineIncastSender");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DeadlineIncastSender);

TypeId
DeadlineIncastSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DeadlineIncastSender")
    .SetParent<IncastSender> ()
    .AddConstructor<DeadlineIncastSender> ()
    .AddAttribute ("TimeAllowed",
                   "The time allowed for each incast send",
                   TimeValue (Seconds (0.01)),
                   MakeTimeAccessor (&DeadlineIncastSender::m_timeAllowed),
                   MakeTimeChecker ())
  ;
  return tid;
}

void DeadlineIncastSender::SetupSocket (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this);
    Ptr<Dctcp> dctcp = socket->GetObject<Dctcp> ();
    Time deadline = Simulator::Now () + m_timeAllowed;
    NS_LOG_LOGIC ("Setting the deadline to " << deadline);
    dctcp->SetAttribute ("Deadline", TimeValue(deadline));
}

} // Namespace ns3
