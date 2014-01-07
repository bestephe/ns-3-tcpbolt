/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * Published by the Free Software Foundation;
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

// Network topology
//
//       n0 ----------- n1
//            10 Gbps
//             12 us
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include "ns3/dcn-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("QbbTest");

void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  NS_LOG_INFO ("cwnd: " << Simulator::Now ().GetSeconds () << "\t" << newval);
}

void
RttTracer (Time oldval, Time newval)
{
  NS_LOG_INFO ("rtt: " << Simulator::Now ().GetSeconds () << "\t" << newval);
}

void
TraceTcp (void)
{
  //Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
  //Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
#if 0
  Config::MatchContainer matches = Config::LookupMatches ("/NodeList/1/$ns3::TcpL4Protocol");
  for (uint32_t i = 0; i < matches.GetN (); i++)
    {
      NS_LOG_UNCOND(matches.GetMatchedPath (i));
      Ptr<TcpL4Protocol> tcpl4 = matches.Get (i)->GetObject<TcpL4Protocol> ();
      ObjectVectorValue socks;
      tcpl4->GetAttribute ("SocketList", socks);
      NS_LOG_UNCOND ("socks size: " << socks.GetN ());
    }
#endif
}

int
main (int argc, char *argv[])
{
  LogComponentEnable("QbbTest", LOG_LEVEL_ALL);
  LogComponentEnableAll(LOG_PREFIX_TIME);
  LogComponentEnableAll(LOG_PREFIX_FUNC);

  //GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  Packet::EnablePrinting (); //XXX: Necessary!

  bool tracing = false;
  uint32_t maxBytes = 0;
  uint32_t threshold = 25e3;
  uint32_t queueBytes = 225e3;   // Number of bytes per queue port
  double simLength = 1;         // Number of seconds in simulation

//
// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.AddValue ("queue", "Queue bytes", queueBytes);
  cmd.AddValue ("simLength", "Seconds in simulation", simLength);
  cmd.Parse (argc, argv);

  uint32_t segmentSize = 1440;
  //int initCwnd = 100000;
  //double minRto = 40e-6;

  const char* socketSpec = "ns3::TcpSocketFactory";

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (segmentSize));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1e7)); //Large value
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1e7)); //Large value
  //Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (initCwnd));
  //Config::SetDefault ("ns3::TcpSocket::SlowStartThreshold", UintegerValue(1)); // Disable slow-start
  //Config::SetDefault ("ns3::RttEstimator::InitialEstimation",
    //StringValue ("12us"));
  //Config::SetDefault ("ns3::RttEstimator::MinRTO", TimeValue ( Seconds (minRto)));
  //XXX: Reenable DCTCP
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (Dctcp::GetTypeId ()));
  Config::SetDefault ("ns3::Ipv4L3Protocol::ECN", BooleanValue (true));
  Config::SetDefault ("ns3::TcpSocketBase::Blocking", BooleanValue (true)); //Necessary for QBB

//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Build star topology.");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("40us"));
  //pointToPoint.SetQueue("ns3::DropTailQueue",
  //  "Mode", EnumValue(DropTailQueue::BYTES),
  //  "MaxBytes", UintegerValue(queueBytes));
  pointToPoint.SetDevice ("ns3::QbbNetDevice",
    "QbbThreshold", UintegerValue (queueBytes),
    "PauseTime", UintegerValue (10));
  //pointToPoint.SetQueue ("ns3::InfiniteQueue");
  pointToPoint.SetQueue ("ns3::InfiniteSimpleRedEcnQueue",
    "Th", UintegerValue (threshold));
  PointToPointStarHelper star (6, pointToPoint);
  InternetStackHelper internet;
  star.InstallStack (internet);
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  NodeContainer hosts = star.m_spokes;
  Ipv4InterfaceContainer hostIfs = star.m_spokeInterfaces;
  pointToPoint.EnablePcapAll ("QbbTest");

  uint16_t port = 1337;  // well-known echo port number

  // Send bulk send traffic
  BulkSendHelper bulkSendHelper (socketSpec,
                                 InetSocketAddress (hostIfs.GetAddress (5), port));
  bulkSendHelper.SetAttribute ("SendSize", UintegerValue (8192));
  bulkSendHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  //ApplicationContainer apps = bulkSendHelper.Install (hosts);

  // Start the sources
  //XXX: Disable source 1 for now //for (uint32_t i = 0; i < 2; ++i)
  for (uint32_t i = 0; i < 2; ++i)
    {
      //apps.Get (i)->SetStartTime (Seconds (simLength / 6.0));  // some Application method
      //apps.Get (i)->SetStartTime (Seconds (0));  // some Application method
      ApplicationContainer app = bulkSendHelper.Install (hosts.Get (i));
      app.Start (Seconds (0));
    }
  ApplicationContainer app = bulkSendHelper.Install (hosts.Get (1));
  app.Start (Seconds (0));


  // Create the sinks
  for (unsigned i = 5; i < hosts.GetN (); i++) {
          PacketSinkHelper sink (socketSpec, Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
          ApplicationContainer app = sink.Install (hosts.Get (i));
          app.Start (Seconds (0));
  };


//
// Set up tracing if enabled
//
//  if (tracing)
//    {
//      AsciiTraceHelper ascii;
//      //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
//      pointToPoint.EnablePcapAll ("tcp-bulk-send", false);
//    }

  LogComponentEnable ("Dctcp", LOG_LEVEL_ALL);
//  LogComponentEnable ("SimpleRedEcnQueue", LOG_LEVEL_ALL);
//  LogComponentEnable ("InfiniteSimpleRedEcnQueue", LOG_LEVEL_ALL);
//  LogComponentEnable ("DropTailQueue", LOG_LEVEL_ALL);
//  LogComponentEnable ("TcpNewReno", LOG_LEVEL_LOGIC);
//  LogComponentEnable ("QbbNetDevice", LOG_LEVEL_ALL);
//  LogComponentEnable ("Node", LOG_LEVEL_ALL);

  Simulator::Schedule (Seconds(0.00001), &TraceTcp);

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (simLength));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
