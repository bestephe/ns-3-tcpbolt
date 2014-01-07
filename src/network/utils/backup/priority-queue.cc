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

#include <iostream> //XXX
#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/hash-function-impl.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-header.h"
#include "priority-queue.h"
#include "priority-tag.h"
#include "order-tag.h"

NS_LOG_COMPONENT_DEFINE ("PriorityQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PriorityQueue);

#if 0
class CompareIpTos : public Object, public std::less<Ptr<Packet> > {
   public:
      bool operator()(Ptr<Packet> x, Ptr<Packet> y) {
        Ipv4Header xhdr, yhdr;
        x->PeekHeader(xhdr);
        y->PeekHeader(yhdr);
        //if (xhdr.GetProtocol()
        //XXX: Do better than this....
#if 0
        if (xhdr.GetProtocol() != 6 || yhdr.GetProtocol() != 6)
          {
            NS_LOG_ERROR("Non-IP traffic in Priority Queue");
            return true;
          }
#endif
        return xhdr.GetTos () < yhdr.GetTos ();
      }
} compareIpTos;
#endif 

#if 0
/* Asks the question: "Is x less than y?" */
bool
ComparePriorityTag::operator()(Ptr<Packet> x, Ptr<Packet> y) const
{
        PriorityTag xtag, ytag;
        OrderTag xOtag, yOtag;
        bool xfound = x->PeekPacketTag (xtag);
        bool yfound = y->PeekPacketTag (ytag);
        if (xfound == false || yfound == false)
          {
            //NS_LOG_UNCOND ("Non-Priority-Tag traffic in ComparePriorityTag");
            NS_LOG_INFO ("Non-Priority-Tag traffic in ComparePriorityTag");
          }
        bool xOfound = x->PeekPacketTag (xOtag);
        bool yOfound = y->PeekPacketTag (yOtag);
        NS_ASSERT (xOfound && yOfound);
#if 0
        else
          {
            NS_LOG_INFO ("Priority-Tags found");
          }
#endif

        if (xfound == false && yfound == false) {
            // Lower order is higher priority, so x is less if the priority is greater
            return xOtag.GetOrder () > yOtag.GetOrder ();
        } else if (xfound == false) {
            return false; //No priority is highest priority so x is greater
        } else if (yfound == false) {
            return true; //No priority is highest priority so x is less
        } else if (xtag.GetPriority () == ytag.GetPriority ()) {
            // Lower time is higher priority, so x is less if the priority is greater
            //NS_LOG_UNCOND ("CmpPri: xtime=" << xtag.GetTime () << ", ytime=" << ytag.GetTime ());
            //if (xtag.GetTime () == ytag.GetTime ())
                //NS_LOG_UNCOND ("x and y times are the same!");
            return xOtag.GetOrder () > yOtag.GetOrder ();
        } else {
            // Lower number is higher priority, so x is less if the priority is greater
            return xtag.GetPriority () > ytag.GetPriority ();
        }
}
//ComparePriorityTag comparePriorityTag;
#endif

/* Asks the question: "Is x less than y?" */
bool
ComparePriorityTag::operator()(Ptr<Packet> const x, Ptr<Packet> const y)
{
        flowid xid = flowid (x);
        flowid yid = flowid (y);
        double xpri = m_priTable[xid];
        double ypri = m_priTable[yid];

        OrderTag xOtag, yOtag;
        bool xOfound = x->PeekPacketTag (xOtag);
        bool yOfound = y->PeekPacketTag (yOtag);
        NS_ASSERT (xOfound && yOfound);

        if (xpri == ypri) {
            // Lower time is higher priority, so x is less if the priority is greater
            return xOtag.GetOrder () > yOtag.GetOrder ();
        } else {
            // Lower number is higher priority, so x is less if the priority is greater
            return xpri > ypri;
        }
}

TypeId PriorityQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::PriorityQueue")
    .SetParent<Queue> ()
    .AddConstructor<PriorityQueue> ()
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this PriorityQueue.",
                   UintegerValue (100 * 65535),
                   MakeUintegerAccessor (&PriorityQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

PriorityQueue::PriorityQueue () :
  Queue (),
  m_container (),
  m_compare (),
  m_packets (m_container, m_compare),
  m_bytesInQueue (0),
  m_numPackets (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

PriorityQueue::~PriorityQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

//TODO: Data with no priority (ACKs) should be still transmitted in FIFO order
bool 
PriorityQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  /* Increment the number of packets. */
  m_numPackets++;
  NS_ASSERT (m_numPackets != std::numeric_limits<uint64_t>::max ());

  /* Assign an order to the packet */
  OrderTag otag;
  bool found = p->PeekPacketTag (otag);
  NS_ASSERT (!found);
  otag.SetOrder (m_numPackets);
  p->AddPacketTag (otag);

  /* Update the priority for the flow */
  PriorityTag ptag;
  found = p->PeekPacketTag (ptag);
  double priority = (found) ? ptag.GetPriority () : 0;
  flowid tuple = flowid (p);
  m_flowPri[tuple] = priority;
  m_compare.SetPriTable (m_flowPri);

  /* Enqueue the packet */
  m_bytesInQueue += p->GetSize ();
  m_packets.push(p);

  /* Drop packets until we are at the max size */
  while (m_bytesInQueue > m_maxBytes)
    {
      NS_LOG_LOGIC ("Queue full (packet exceeded max bytes) -- droppping pkt");
      Ptr<Packet> toDrop = m_packets.popMin ();
      uint32_t size = toDrop->GetSize ();
      Drop (toDrop);
      m_bytesInQueue -= size;
      /*
       * XXX: Queue::Enqueue expects that only the enqueued packets can be
       *  dropped, and updates m_nBytes, m_nPackets only based on the newly
       *  enqueued packet succeeds, which it always does in this function.
       *  Similarly, these values are only decremented on dequeue, which
       *  our dropped packet will never be, so we need to update these values.
       */
      m_nBytes -= size;
      m_nPackets--;

      //XXX: Debugging
      PriorityTag priTag;

    }

  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  /*
   * Always claim success, which causes m_nBytes, m_nPackets,
   * m_nTotalReceivedBytes, and m_nTotalReceivedPackets to be incremented.
   */
  return true;
}

Ptr<Packet>
PriorityQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  //XXX
  std::ostringstream oss;
  Ptr<Packet> min = m_packets.findMin ();
  PriorityTag minTag;
  if (min->PeekPacketTag (minTag))
    {
      oss << "Min pri: " << minTag.GetPriority ();
    }
  else
    {
      oss << "Min packet with no priority: ";
    }
  NS_LOG_LOGIC (oss.str ());

  Ptr<Packet> p = m_packets.pop ();
  m_bytesInQueue -= p->GetSize ();

  OrderTag otag;
  p->RemovePacketTag (otag);
  NS_LOG_LOGIC ("dequeue order: " << otag.GetOrder ());

  //XXX
  PriorityTag tag;
  oss.str ("");
  if (p->PeekPacketTag (tag))
    {
      oss << "dequeue pri: " << tag.GetPriority ();
    }
  else
    {
      oss << "Packet with no priority: ";
    }
  NS_LOG_LOGIC (oss.str ());

  NS_LOG_UNCOND (this << "Dequeued packet w/ pri: " << tag.GetPriority () << " and order: " << otag.GetOrder ());

  NS_LOG_LOGIC ("Popped " << p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

Ptr<const Packet>
PriorityQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.findMax ();

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

size_t FlowIdHash::operator() (flowid const &x) const
{ 
  HashHsieh hash;
  return hash (0, x);
}

} // namespace ns3

