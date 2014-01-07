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

#ifndef INFINITE_SIMPLE_RED_ECN_H
#define INFINITE_SIMPLE_RED_ECN_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/infinite-queue.h"
#include "ns3/queue.h"

namespace ns3 {

class TraceContainer;

/**
 * \ingroup queue
 *
 * \brief A FIFO packet queue that drops tail-end packets on overflow
 */
class InfiniteSimpleRedEcnQueue : public InfiniteQueue {
public:
  static TypeId GetTypeId (void);
  /**
   * \brief DropTailQueue Constructor
   *
   * Creates a droptail queue with a maximum size of 100 packets by default
   */
  InfiniteSimpleRedEcnQueue ();

  virtual ~InfiniteSimpleRedEcnQueue();

  /*
   * \brief Set the thresh limits of RED.
   *
   * \param min Minimum thresh in bytes or packets.
   * \param max Maximum thresh in bytes or packets.
   */
  void SetTh (uint32_t th);

protected:
  virtual bool DoEnqueue (Ptr<Packet> p);
  void Mark (Ptr<Packet> p);

  uint32_t m_th;
};

} // namespace ns3

#endif /* INFINITE_SIMPLE_RED_ECN_H */
