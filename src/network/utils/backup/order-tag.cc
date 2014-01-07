/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "order-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OrderTag");
NS_OBJECT_ENSURE_REGISTERED (OrderTag);

TypeId 
OrderTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OrderTag")
    .SetParent<Tag> ()
    .AddConstructor<OrderTag> ()
  ;
  return tid;
}
TypeId 
OrderTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
OrderTag::GetSerializedSize (void) const
{
  return sizeof(m_order) + 8;
}
void 
OrderTag::Serialize (TagBuffer buf) const
{
  buf.WriteU64 (m_order);
  //NS_LOG_UNCOND("Serialized m_order=" << m_order << ", time=" << m_time);
}
void 
OrderTag::Deserialize (TagBuffer buf)
{
  m_order = buf.ReadU64 ();
  //NS_LOG_UNCOND("Deserialized m_order=" << m_order << ", time=" << m_time);
}
void 
OrderTag::Print (std::ostream &os) const
{
  os << "Order=" << m_order << std::endl;
}
OrderTag::OrderTag ()
  : Tag () 
{
  //m_time = Simulator::Now ();
}

OrderTag::OrderTag (uint64_t ord)
  : Tag (),
    m_order (ord)
{
  //NS_LOG_UNCOND ("Arg constructor.  order=" << m_order);
  //m_time = Simulator::Now ();
}

void
OrderTag::SetOrder (uint64_t order)
{
  m_order = order;
  //NS_LOG_UNCOND ("Set order.  order=" << m_order);
  //m_time = Simulator::Now ();
  //NS_LOG_LOGIC ("Setting Time with order.  Time=" << m_time);
  //NS_LOG_UNCOND ("Setting Time with order.  Order=" << m_order << " Time=" << m_time);
}

uint64_t
OrderTag::GetOrder (void) const
{
  //NS_LOG_UNCOND ("Get order.  order=" << m_order);
  return m_order;
}

#if 0
void
OrderTag::SetTime (void)
{
    m_time = Simulator::Now ();
    NS_LOG_LOGIC ("Set Time.  Time=" << m_time);
    NS_LOG_UNCOND ("Set Time.  Time=" << m_time);
}

Time
OrderTag::GetTime (void) const
{
  return m_time;
}
#endif

} // namespace ns3

