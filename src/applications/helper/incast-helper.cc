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
 * Author: Geoge Riley <riley@ece.gatech.edu>
 * Adapted from OnOffHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "incast-helper.h"
#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/random-variable.h"
#include "ns3/string.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("IncastHelper");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (IncastHelper);

sgi::hash_map<Ptr<Socket>, Time, PtrHash> IncastHelper::m_sockets;
sgi::hash_map<Ptr<IncastAggregator>, Time, PtrHash> IncastHelper::m_incasts;

#if 0
If for every t > 0 the number of arrivals in the time interval [0,t] follows the Poisson distribution with mean λ t, then the sequence of inter-arrival times are independent and identically distributed exponential random variables having mean 1 / λ.[20]
#endif

TypeId
IncastHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IncastHelper")
    .SetParent<Object> ()
    .AddConstructor<IncastHelper> ()
    //.AddAttribute ("SruSizes",
    //               "The list of SRU sizes",
    //               ObjectVectorValue (std::vector (1, UintegerValue (64e3))),
    //               MakeObjectVectorAccessor (&QbbNetDevice::m_sruSizes),
    //               MakeObjectVectorChecker<uint32_t> ())
    //.AddAttribute ("SenderSizes",
    //               "The list of number of senders per incast",
    //               ObjectVectorValue (std::vector (1, UintegerValue (0))),
    //               MakeObjectVectorAccessor (&QbbNetDevice::m_senderSizes),
    //               MakeObjectVectorChecker<uint32_t> ())
    .AddAttribute ("IncastsPerSecond",
                   "The average number of incasts per second",
                   UintegerValue (10),
                   MakeUintegerAccessor (&IncastHelper::m_incastsPerSecond),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TimeAllowed",
                   "The time allowed for each incast send",
                   TimeValue (Seconds (-1)),
                   MakeTimeAccessor (&IncastHelper::m_timeAllowed),
                   MakeTimeChecker ())
    .AddAttribute ("Protocol",
                   "The transport protocol to use",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&IncastHelper::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("Trace", "True to enable tracing",
                   BooleanValue (false),
                   MakeBooleanAccessor (&IncastHelper::m_trace),
                   MakeBooleanChecker ())
    .AddAttribute ("TraceName",
                   "The unique portion of the trace name",
                   StringValue ("incast-helper"),
                   MakeStringAccessor (&IncastHelper::m_fileName),
                   MakeStringChecker ())
  ;
  return tid;
}

IncastHelper::IncastHelper ()
{
  NS_LOG_FUNCTION (this);

  //m_aggFactory.SetTypeId ("ns3::PriorityIncastAggregator");
  m_aggFactory.SetTypeId ("ns3::IncastAggregator");
  m_senderFactory.SetTypeId ("ns3::PriorityIncastSender");
}

void
IncastHelper::SetAggs (NodeContainer aggs)
{
  NS_LOG_FUNCTION (this);

  m_aggs = aggs;
}

void
IncastHelper::SetSenders (NodeContainer& senders,
    std::vector<Ipv4Address> senderAddrs)
{
  NS_LOG_FUNCTION (this);

  m_senders = senders;
  m_senderAddrs = senderAddrs;
}

void
IncastHelper::SetSruSizes (std::vector<uint32_t> sruSizes)
{
  NS_LOG_FUNCTION (this);

  m_sruSizes = sruSizes;
}

void
IncastHelper::SetSenderSizes (std::vector<uint32_t> senderSizes)
{
  NS_LOG_FUNCTION (this);

  m_senderSizes = senderSizes;
}

void
IncastHelper::SetIncastAttribute (std::string name, const AttributeValue &value)
{
  NS_LOG_FUNCTION (this);

  m_aggFactory.Set (name, value);
  m_senderFactory.Set (name, value);
}

void
IncastHelper::Start (Time start)
{
  NS_LOG_FUNCTION (this);

  // Setup all of the member variables
  if (m_timeAllowed.IsPositive ())
    {
      NS_LOG_UNCOND ("Setting the timeAllowed to " << m_timeAllowed);
      m_senderFactory.SetTypeId ("ns3::DeadlineIncastSender");
      m_senderFactory.Set ("TimeAllowed", TimeValue (m_timeAllowed));
    }
  m_aggFactory.Set ("Protocol", TypeIdValue (m_tid));
  m_senderFactory.Set ("Protocol", TypeIdValue (m_tid));
  m_nextIncastVar = ExponentialVariable (1.0 / m_incastsPerSecond);

  // Setup logging
  if (m_trace)
    {
      AsciiTraceHelper asciiTraceHelper;
      std::ostringstream oss;
      oss << "sockets." << m_fileName << "-" << SeedManager::GetRun () << ".fct";
      m_sockStream = asciiTraceHelper.CreateFileStream (oss.str ());
      oss.str ("");
      oss << "incasts." << m_fileName << "-" << SeedManager::GetRun () << ".fct";
      m_incastStream = asciiTraceHelper.CreateFileStream (oss.str ());
    }

  // Install the senders
  for (NodeContainer::Iterator i = m_senders.Begin ();
        i != m_senders.End (); ++i)
    {
        Ptr<Node> sender = *i;
        Ptr<Application> senderApp = m_senderFactory.Create<Application> ();
        sender->AddApplication(senderApp);
        m_aggApps.Add (senderApp);
        senderApp->SetStartTime (start);
    }

  // Install the aggregators
  double nextAggDelay = m_nextIncastVar.GetValue ();
  NS_LOG_LOGIC ("Next incast starting at " <<  Simulator::Now () + Seconds (nextAggDelay) <<
    " (delay of " << nextAggDelay << ")");
  Simulator::Schedule (Seconds (nextAggDelay), &IncastHelper::StartAgg, this);

#if 0
  for (NodeContainer::Iterator i = m_aggs.Begin ();
        i != m_aggs.End (); ++i)
    {
        Ptr<Node> agg = *i;
        Ptr<IncastAggregator> aggApp = m_aggFactory.Create<IncastAggregator> ();
        aggApp->SetSenders (GetRandomSenders ());
        aggApp->SetRoundFinishCallback (
          MakeCallback (&IncastHelper::HandleRoundFinish, this));
        agg->AddApplication(aggApp);
        m_aggApps.Add (aggApp);
        aggApp->SetStartTime (start);
        if (m_trace)
          {
            aggApp->TraceConnectWithoutContext ("SockStart", MakeBoundCallback (&IncastHelper::SockStart, m_sockStream));
            aggApp->TraceConnectWithoutContext ("SockStop", MakeBoundCallback (&IncastHelper::SockStop, m_sockStream));
            aggApp->TraceConnectWithoutContext ("Start", MakeBoundCallback (&IncastHelper::IncastStart, m_incastStream));
            aggApp->TraceConnectWithoutContext ("Stop", MakeBoundCallback (&IncastHelper::IncastStop, m_incastStream));
          }
    }
#endif
}

void
IncastHelper::Stop (Time stop)
{
  NS_LOG_FUNCTION (this);
  //XXX: No-op for now.
}

std::vector<Ipv4Address>
IncastHelper::GetRandomSenders (uint32_t numSenders)
{
  NS_LOG_FUNCTION (this << numSenders);

  std::vector<Ipv4Address> senders;

  //TODO: It might be worth guaranteeing that there are no dups in senders
  for (uint32_t i = 0; i < numSenders; i++)
    {
      int senderI = UniformVariable ().GetInteger (0, m_senderAddrs.size () - 1);      Ipv4Address addr = m_senderAddrs.at (senderI);
      senders.push_back (addr);
    }
  return senders;
}

void
IncastHelper::StartAgg (void)
{
  NS_LOG_FUNCTION (this);

  // Pick the SRU for this incast
  uint32_t sru = m_sruSizes[UniformVariable ().GetInteger (0, m_sruSizes.size () - 1)];
  m_aggFactory.Set ("SRU", UintegerValue (sru));
  uint32_t numSenders = m_senderSizes[UniformVariable ().GetInteger (0, m_senderSizes.size () - 1)];
  NS_LOG_LOGIC ("Starting an incast with " << numSenders <<
    " senders and an SRU of " << sru);
  
  // Start an agg
  int aggI = UniformVariable ().GetInteger (0, m_aggs.GetN () - 1);
  Ptr<Node> agg = m_aggs.Get (aggI);
  Ptr<IncastAggregator> aggApp = m_aggFactory.Create<IncastAggregator> ();
  aggApp->SetSenders (GetRandomSenders (numSenders));
  aggApp->SetRoundFinishCallback (
    MakeCallback (&IncastHelper::HandleRoundFinish, this));
  agg->AddApplication(aggApp);
  m_aggApps.Add (aggApp);
  aggApp->SetStartTime (Simulator::Now ());
  if (m_trace)
    {
      aggApp->TraceConnectWithoutContext ("SockStart", MakeBoundCallback (&IncastHelper::SockStart, m_sockStream));
      aggApp->TraceConnectWithoutContext ("SockStop", MakeBoundCallback (&IncastHelper::SockStop, m_sockStream));
      aggApp->TraceConnectWithoutContext ("Start", MakeBoundCallback (&IncastHelper::IncastStart, m_incastStream));
      aggApp->TraceConnectWithoutContext ("Stop", MakeBoundCallback (&IncastHelper::IncastStop, m_incastStream));
    }

  NS_LOG_LOGIC ("Starting Agg app at node " << agg->GetId ());

  // Schedule the next agg
  double nextAggDelay = m_nextIncastVar.GetValue ();
  NS_LOG_LOGIC ("Next incast starting at " <<  Simulator::Now () + Seconds (nextAggDelay) <<
    " (delay of " << nextAggDelay << ")");
  Simulator::Schedule (Seconds (nextAggDelay), &IncastHelper::StartAgg, this);
}

void
IncastHelper::HandleRoundFinish (Ptr<IncastAggregator> aggApp)
{
  NS_LOG_FUNCTION (this);

  //TODO: Just stop the application for now
  aggApp->SetStopTime (Simulator::Now ());

  //XXX: start next round
  //aggApp->SetStartTime (Simulator::Now ());
  //XXX: Doesn't actually restart the 
  //aggApp->DoStart (); //Actually schedule the next round
}

void
IncastHelper::SockStart (Ptr<OutputStreamWrapper> stream, Ptr<Socket> s)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << s << " start");
  //*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << s << " start" << std::endl;
  m_sockets[s] = Simulator::Now ();
}

void
IncastHelper::SockStop (Ptr<OutputStreamWrapper> stream, Ptr<Socket> s, uint32_t totBytes)
{
  Time stop = Simulator::Now ();
  Time start = m_sockets[s];
  m_sockets.erase(s);
  double completionTime = (stop - start).GetSeconds ();
  //double bps = 8 * totBytes / completionTime;
  std::ostringstream oss;
  oss << "- [" << Simulator::Now ().GetSeconds () << ", ";
  oss << completionTime << ", " << totBytes << ", 1]";
  //NS_LOG_UNCOND (oss.str ());
  *stream->GetStream () << oss.str () << std::endl;
}

void
IncastHelper::IncastStart (Ptr<OutputStreamWrapper> stream, Ptr<IncastAggregator> app)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << app << " start");
  //*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << app << " start" << std::endl;
  m_incasts[app] = Simulator::Now ();
}

void
IncastHelper::IncastStop (Ptr<OutputStreamWrapper> stream, Ptr<IncastAggregator> app, uint32_t totBytes)
{
  Time stop = Simulator::Now ();
  Time start = m_incasts[app];
  m_incasts.erase(app);
  double completionTime = (stop - start).GetSeconds ();
  //double bps = 8 * totBytes / completionTime;
  UintegerValue sru;
  app->GetAttribute("SRU", sru);
  int numflows = totBytes / sru.Get ();
  std::ostringstream oss;
  oss << "- [" << Simulator::Now ().GetSeconds () << ", ";
  oss << completionTime << ", " << totBytes << ", " << numflows << "]";
  //NS_LOG_UNCOND (oss.str ());
  *stream->GetStream () << oss.str () << std::endl;
}

} // namespace ns3
