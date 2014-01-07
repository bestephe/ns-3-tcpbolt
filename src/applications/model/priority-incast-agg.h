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

#ifndef PRIORITY_INCAST_AGGREGATOR_H
#define PRIORITY_INCAST_AGGREGATOR_H

#include "ns3/application.h"
#include "ns3/incast-agg.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/inet-socket-address.h"
#include "ns3/traced-callback.h"

//class TcpAdmCtrl;

namespace ns3 {

/**
 * \ingroup applications
 * \defgroup incast PriorityIncastAggregator
 *
 * A part of the incast application. This component is the `aggregator' part of
 * the incast application, namely, it accepts data from remote peers. Because
 * of the connection-oriented assumption on the sockets, only SOCK_STREAM and
 * SOCK_SEQPACKET sockets are supported, i.e. TCP but not UDP.
 */
class PriorityIncastAggregator : public IncastAggregator
{
public:
  static TypeId GetTypeId (void);
protected:
  virtual void HandleSend (Ptr<Socket> socket, uint32_t n);
};

} // namespace ns3

#endif /* PRIORITY_INCAST_AGGREGATOR_H */
