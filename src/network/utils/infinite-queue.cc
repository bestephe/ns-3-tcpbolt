/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 New York University
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
 * Author: Adrian S. Tam <adrian.sw.tam@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/assert.h"
#include "infinite-queue.h"

NS_LOG_COMPONENT_DEFINE ("InfiniteQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (InfiniteQueue);

TypeId InfiniteQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::InfiniteQueue")
    .SetParent<Queue> ()
    .AddConstructor<InfiniteQueue> ()
    ;
  
  return tid;
}

InfiniteQueue::InfiniteQueue () :
  Queue (),
  m_packets ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

InfiniteQueue::~InfiniteQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool 
InfiniteQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  m_bytesInQueue += p->GetSize ();
  m_packets.push(p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  //NS_LOG_UNCOND ("Enqueue: " << m_bytesInQueue);

  //XXX
  if (m_bytesInQueue > 1000e3) {
    int *iptr = 0;
    int foo = *iptr;
    foo = foo;
  }

  return true;
}

Ptr<Packet>
InfiniteQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty()) 
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();
  m_packets.pop ();
  m_bytesInQueue -= p->GetSize ();

  NS_LOG_LOGIC ("Popped " << p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  //NS_LOG_UNCOND ("Dequeue: " << m_bytesInQueue);

  return p;
}

Ptr<const Packet>
InfiniteQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty()) 
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

} // namespace ns3


#ifdef RUN_SELF_TESTS

#include "ns3/test.h"

namespace ns3 {

class InfiniteQueueTest: public TestCase {
public:
  virtual bool DoRun (void);
  InfiniteQueueTest ();

};


InfiniteQueueTest::InfiniteQueueTest ()
  : TestCase("InfiniteQueue") {}


bool
InfiniteQueueTest::DoRun (void)
{
  Ptr<InfiniteQueue> queue = CreateObject<InfiniteQueue> ();
  
  Ptr<Packet> p1, p2, p3;
  p1 = Create<Packet> ();
  p2 = Create<Packet> ();
  p3 = Create<Packet> ();

  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 0, "There should be no packets in there");
  queue->Enqueue (p1);
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 1, "There should be one packet in there");
  queue->Enqueue (p2);
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 2, "There should be two packets in there");
  queue->Enqueue (p3);
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 3, "There should be three packets in there");

  Ptr<Packet> p;

  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p != 0), true, "I want to remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 2, "There should be two packets in there");
  NS_TEST_EXPECT_MSG_EQ (p->GetUid (), p1->GetUid (), "was this the first packet ?");

  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p != 0), true, "I want to remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 1, "There should be one packet in there");
  NS_TEST_EXPECT_MSG_EQ (p->GetUid (), p2->GetUid (), "Was this the second packet ?");

  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p != 0), true, "I want to remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), 0, "There should be no packets in there");
  NS_TEST_EXPECT_MSG_EQ (p->GetUid (), p3->GetUid (), "Was this the third packet ?");

  p = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((p == 0), true, "There are really no packets in there");

  return false;
}


static InfiniteQueueTest gInfiniteQueueTest;

}; // namespace ns3

#endif /* RUN_SELF_TESTS */
