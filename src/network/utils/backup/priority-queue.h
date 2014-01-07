/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <string>
#include <queue>
#include "ns3/fivetuple.h"
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/MinMaxHeap.h"
#include "ns3/sgi-hashmap.h"

namespace ns3 {

class FlowIdHash : public std::unary_function<flowid, size_t> {
public:
  size_t operator() (flowid const &x) const;
};

class TraceContainer;

class ComparePriorityTag : public Object {
   public:
      bool operator()(Ptr<Packet> const x, Ptr<Packet> const y);
      void SetPriTable (sgi::hash_map<flowid, double, FlowIdHash>& priTable) { m_priTable = priTable; };
    private:
      sgi::hash_map<flowid, double, FlowIdHash> m_priTable;
};

/**
 * \ingroup queue
 *
 * \brief A FIFO packet queue that drops tail-end packets on overflow
 */
class PriorityQueue : public Queue {
public:
  static TypeId GetTypeId (void);

  PriorityQueue ();
  virtual ~PriorityQueue();

private:
  friend class PointToPointNetDevice;
  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;

  //TODO: Specify the Compare function
  std::vector<Ptr<Packet> > m_container;
  ComparePriorityTag m_compare;
  MinMaxHeap<Ptr<Packet>, std::vector<Ptr<Packet> >, ComparePriorityTag> m_packets;
  uint32_t m_maxBytes;
  uint32_t m_bytesInQueue;
  uint64_t m_numPackets;

  typedef sgi::hash_map<flowid, double, FlowIdHash> FlowPriorities;
  FlowPriorities m_flowPri;
};

} // namespace ns3

#endif /* PRIORITY_QUEUE_H */
