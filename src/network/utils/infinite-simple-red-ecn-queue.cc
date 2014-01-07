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

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "infinite-simple-red-ecn-queue.h"

NS_LOG_COMPONENT_DEFINE ("InfiniteSimpleRedEcnQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (InfiniteSimpleRedEcnQueue);

TypeId InfiniteSimpleRedEcnQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::InfiniteSimpleRedEcnQueue")
    .SetParent<InfiniteQueue> ()
    .AddConstructor<InfiniteSimpleRedEcnQueue> ()
    .AddAttribute ("Th",
                   "Instantaneous mark length threshold in packets/bytes",
                   UintegerValue (5),
                   MakeUintegerAccessor (&InfiniteSimpleRedEcnQueue::m_th),
                   MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

InfiniteSimpleRedEcnQueue::InfiniteSimpleRedEcnQueue () :
  InfiniteQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

InfiniteSimpleRedEcnQueue::~InfiniteSimpleRedEcnQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}


bool 
InfiniteSimpleRedEcnQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p << "bytes: " << m_bytesInQueue + p->GetSize ());

  if (m_bytesInQueue + p->GetSize () >= m_th)
    {
      NS_LOG_LOGIC ("Queue (" << (m_bytesInQueue + p->GetSize ()) << ") above threshold (" << m_th << ") -- marking pkt");
      Mark (p);
    }

  return InfiniteQueue::DoEnqueue (p);
}

void
InfiniteSimpleRedEcnQueue::Mark (Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);
  PacketMetadata::ItemIterator metadataIterator = p->BeginItem();
  PacketMetadata::Item item;
  while (metadataIterator.HasNext())
    {
      item = metadataIterator.Next();
      NS_LOG_DEBUG(this << "item name: " << item.tid.GetName());
      if(item.tid.GetName() == "ns3::Ipv4Header")
        {
          NS_LOG_DEBUG("IP packet found");
          // This is a dirty hack to access the ECN byte: directly read/write from the buffer
          Buffer::Iterator i = item.current;
          i.Next (1); // Move to TOS byte
          uint8_t tos = i.ReadU8 ();
          uint8_t ecn = tos & 0x3;
          if (ecn == 1 || ecn == 2)
            {
              tos |= 0x3;
              i.Prev (1); // Go back to the TOS byte after ReadU8()
              i.WriteU8 (tos);
            }
          if ((tos & 0x3) == 0x3)
            {
              NS_LOG_LOGIC ("Marking IP packet");
            }
          else
            {
              NS_LOG_LOGIC ("Unable to mark packet");
            }
          break;
        }
    }
}

} // namespace ns3

