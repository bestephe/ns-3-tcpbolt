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
#include "priority-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PriorityTag");
NS_OBJECT_ENSURE_REGISTERED (PriorityTag);

TypeId 
PriorityTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityTag")
    .SetParent<Tag> ()
    .AddConstructor<PriorityTag> ()
  ;
  return tid;
}
TypeId 
PriorityTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
PriorityTag::GetSerializedSize (void) const
{
  return sizeof(m_priority) + 8;
}
void 
PriorityTag::Serialize (TagBuffer buf) const
{
  buf.WriteDouble (m_priority);
  //buf.WriteU64 ( (uint64_t) m_time.GetInteger ());
  //NS_LOG_UNCOND("Serialized m_priority=" << m_priority << ", time=" << m_time);
}
void 
PriorityTag::Deserialize (TagBuffer buf)
{
  m_priority = buf.ReadDouble ();
  //int64_t v = (int64_t) buf.ReadU64 ();
  //m_time = Time (v);
  //NS_LOG_UNCOND("Deserialized m_priority=" << m_priority << ", time=" << m_time);
}
void 
PriorityTag::Print (std::ostream &os) const
{
  os << "Priority=" << m_priority << std::endl;
}
PriorityTag::PriorityTag ()
  : Tag () 
{
  //m_time = Simulator::Now ();
}

PriorityTag::PriorityTag (double pri)
  : Tag (),
    m_priority (pri)
{
  //NS_LOG_UNCOND ("Arg constructor.  priority=" << m_priority);
  //m_time = Simulator::Now ();
}

void
PriorityTag::SetPriority (double priority)
{
  m_priority = priority;
  //NS_LOG_UNCOND ("Set priority.  priority=" << m_priority);
  //m_time = Simulator::Now ();
  //NS_LOG_LOGIC ("Setting Time with priority.  Time=" << m_time);
  //NS_LOG_UNCOND ("Setting Time with priority.  Priority=" << m_priority << " Time=" << m_time);
}

double
PriorityTag::GetPriority (void) const
{
  //NS_LOG_UNCOND ("Get priority.  priority=" << m_priority);
  return m_priority;
}

#if 0
void
PriorityTag::SetTime (void)
{
    m_time = Simulator::Now ();
    NS_LOG_LOGIC ("Set Time.  Time=" << m_time);
    NS_LOG_UNCOND ("Set Time.  Time=" << m_time);
}

Time
PriorityTag::GetTime (void) const
{
  return m_time;
}
#endif

} // namespace ns3

