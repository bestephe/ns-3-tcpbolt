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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <assert.h>
#include <math.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/dcn-module.h"
#include "ns3/fat-tree-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OldiScript");

const uint16_t port = 1337;   // Discard port (RFC 863)

sgi::hash_map<Ptr<Socket>, Time, PtrHash> sockets;
//std::map<Ptr<Socket>, Time> sockets;

static void
SockStart (Ptr<OutputStreamWrapper> stream, Ptr<Socket> s)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << s << " start");
  //*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << s << " start" << std::endl;
  sockets[s] = Simulator::Now ();
}

static void
SockStop (Ptr<OutputStreamWrapper> stream, Ptr<Socket> s, uint32_t totBytes)
{
  Time stop = Simulator::Now ();
  Time start = sockets[s];
  sockets.erase(s);
  double completionTime = (stop - start).GetSeconds ();
  //double bps = 8 * totBytes / completionTime;
  std::ostringstream oss;
  oss << "- [" << Simulator::Now ().GetSeconds () << ", ";
  oss << completionTime << ", " << totBytes << ", 1]";
  //NS_LOG_UNCOND (oss.str ());
  *stream->GetStream () << oss.str () << std::endl;
}

#if 0
void
RttTracer (Time oldval, Time newval)
{
  NS_LOG_INFO ("rtt: " << Simulator::Now ().GetSeconds () << "\t" << newval);
}

void
TraceTcp (void)
{
  //Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
  Config::ConnectWithoutContext ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/RTT", MakeCallback (&RttTracer));
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
#endif

void
PrintTime (void)
{
    NS_LOG_UNCOND (Simulator::Now ());
    Simulator::Schedule (Seconds(0.005), &PrintTime);
}

int main (int argc, char *argv[])
{
	// Turn on logging
	LogComponentEnable("OldiScript", LOG_LEVEL_ALL);
	LogComponentEnableAll(LOG_PREFIX_TIME);
	LogComponentEnableAll(LOG_PREFIX_FUNC);
//	LogComponentPrintList();

        //XXX: Whoah.  Necessary for DCTCP to receive IP:CE
        Packet::EnablePrinting ();

	double linkBw = 10e9;		// Link bandwidth in bps
	double linkDelay = 20e-6;	// Link delay in seconds (5ns = 1m wire)
        uint32_t threshold = 22.5e3;
        uint32_t queueBytes = 225e3;      // Number of bytes per queue port
	unsigned size = 3;		// Fat tree size
        double loadFactor = 0.1;         // The load factor the incasts
        uint32_t initCwnd = 10;         // TCP Initial Congestion Window
        double minRto = 5000e-6;        // Minimum RTO
        //double minRto = -1;
        bool noSlowStart = 0;           // Disable initial TCP Slow Start
	bool dryrun=0;			// Fake the simulation or not
        bool pktspray = 0;              // Enable per-packet randomized routing
        bool star = 0;                  // Use a star topology
        bool dctcp = 0;                 // Use TCP or DCTCP
        bool d2tcp = 0;                 // Use D2TCP
        bool red = 0;
        bool pFabric = 0;
        bool lossless = 0;              // Use Lossless queues
	int pausetime = 10;		// Pause duration in us
        double simLength = 1;           // Number of seconds in simulation
        std::string simPrefix = "OLDI_sim";

	CommandLine cmd;
	cmd.AddValue("bw","Link bandwidth",linkBw);
	cmd.AddValue("delay","Link delay",linkDelay);
        cmd.AddValue("queue","Queue bytes",queueBytes);
	cmd.AddValue("dryrun","Fake the simulation",dryrun);
	cmd.AddValue("size","Fat tree size",size);
	cmd.AddValue("loadFactor","The incast load factor",loadFactor);
	cmd.AddValue("initCwnd","Maximum number of flows per node",initCwnd);
        cmd.AddValue("minRto","Minimum RTO",minRto);
	cmd.AddValue("noSlowStart","Disable the initial TCP Slow Start",noSlowStart);
        cmd.AddValue("pktspray","Enable per-packet randomized routing",pktspray);
        cmd.AddValue("star","Use a star topology instead of a fat tree",star);
        cmd.AddValue("dctcp","Use DCTCP instead of TCP",dctcp);
        cmd.AddValue("d2tcp","Use D2TCP instead of TCP",d2tcp);
        cmd.AddValue("red","Use RED/ECN with TCP",red);
        cmd.AddValue("pFabric","Use pFabric",pFabric);
        cmd.AddValue("lossless","Use Lossless queues",lossless);
        cmd.AddValue("simLength", "Seconds in simulation", simLength);
        cmd.AddValue("simprefix", "The prefix for the logging files", simPrefix);
	cmd.Parse (argc, argv);

        uint32_t segmentSize = 1460;
        double minRtt = (12 * linkDelay);
        double maxRtt = minRtt + 8 * (8 * queueBytes / linkBw);
        int lineCwnd = round((linkBw * minRtt) / 8 / segmentSize);
        NS_LOG_INFO("minRtt: " << minRtt);
        NS_LOG_INFO("maxRtt: " << maxRtt);
        NS_LOG_INFO("lineCwnd: " << lineCwnd);
        maxRtt = maxRtt; lineCwnd = lineCwnd;

        /* Set the simple parameters */
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (segmentSize));
        Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1e7)); //Large value
        Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1e7)); //Large value
	Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (initCwnd));
	Config::SetDefault ("ns3::HashRouting::EnableReroute", BooleanValue (false));
        Config::SetDefault ("ns3::Ipv4L3Protocol::ECN", BooleanValue (true));
        if (dctcp || d2tcp || red) {
            Config::SetDefault ("ns3::Ipv4L3Protocol::ECN", BooleanValue (true));
        }
        if (pktspray) {
            Config::SetDefault ("ns3::TcpNewReno::ReTxThreshold", UintegerValue(std::numeric_limits<uint32_t>::max () - 1)); // Disable fast-retransmit
        }
        if (noSlowStart) {
	    Config::SetDefault ("ns3::TcpSocket::SlowStartThreshold", UintegerValue(1)); // Disable slow-start
        }
        Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue ( Seconds (minRtt))); //TODO: What should the initial estimate be?
        if (minRto > 0) {
            //Config::SetDefault ("ns3::RttEstimator::MinRTO", TimeValue ( Seconds (minRto)));
        }

        /* Set the transport protocol */
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
        std::string queueType = "ns3::DropTailQueue";
        std::string n1 = "Mode";
        Ptr<AttributeValue> v1 = Create<EnumValue> (DropTailQueue::QUEUE_MODE_BYTES);
        std::string n2 = "MaxBytes";
        Ptr<AttributeValue> v2 = Create<UintegerValue> (queueBytes);
        std::string n3 = "";
        Ptr<AttributeValue> v3 = Create<EmptyAttributeValue> ();
        if (dctcp && red) {
            NS_LOG_UNCOND ("Enabling DCTCP and RED");
            Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (Dctcp::GetTypeId ()));
            queueType = "ns3::RedQueue";
            n2 = "QueueLimit";
            Config::SetDefault ("ns3::RedQueue::LinkBandwidth", DataRateValue (DataRate (linkBw)));
            Config::SetDefault ("ns3::RedQueue::LinkDelay", TimeValue (Seconds (linkDelay)));
            Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (threshold));
            Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (queueBytes));
        } else if (dctcp || d2tcp) {
            NS_LOG_UNCOND ("Enabling DCTCP");
            Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (Dctcp::GetTypeId ()));
            queueType = "ns3::SimpleRedEcnQueue";
            n3 = "Th";
            v3 = Create<UintegerValue> (threshold);
        }
        else if (red) {
            Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpAdmCtrl::GetTypeId ()));
            queueType = "ns3::RedQueue";
            n2 = "QueueLimit";
            Config::SetDefault ("ns3::RedQueue::LinkBandwidth", DataRateValue (DataRate (linkBw)));
            Config::SetDefault ("ns3::RedQueue::LinkDelay", TimeValue (Seconds (linkDelay)));
            Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (threshold));
            Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (queueBytes));
        }

        /* Set lossy or lossless */
        std::string deviceType = "ns3::PointToPointNetDevice";
        if (lossless) {
            deviceType = "ns3::QbbNetDevice";
            Config::SetDefault ("ns3::TcpSocketBase::Blocking", BooleanValue (true)); //Necessary for QBB
            Config::SetDefault ("ns3::QbbNetDevice::PauseTime", UintegerValue(pausetime));
            Config::SetDefault ("ns3::QbbNetDevice::QbbThreshold", UintegerValue(queueBytes));
            n1 = ""; //Disable Mode
            n2 = ""; //Disable MaxBytes
            if (dctcp || d2tcp) {
                queueType = "ns3::InfiniteSimpleRedEcnQueue";
            } else {
                queueType = "ns3::InfiniteQueue";
            }
        }

        /* Build the physical network */
        NodeContainer hosts; // Host nodes
	Ipv4InterfaceContainer hostIfs;	// Host interfaces
	const unsigned numHosts = size * size * size * 2;
        if (star) {
            NS_LOG_INFO ("Build star topology.");
            // Adjust to having 3x fewer links per RTT
            linkDelay = linkDelay * 3;
            //queueBytes = queueBytes * 3;

            PointToPointHelper pointToPoint;
            pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue(DataRate(linkBw)));
            pointToPoint.SetChannelAttribute ("Delay", TimeValue(Seconds(linkDelay)));
            pointToPoint.SetDevice(deviceType);
            pointToPoint.SetQueue(queueType,
              n1, *v1,
              n2, *v2,
              n3, *v3);
            PointToPointStarHelper star (numHosts, pointToPoint);
            InternetStackHelper internet;
            star.InstallStack (internet);
            star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));
            Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
            hosts = star.m_spokes;
            hostIfs = star.m_spokeInterfaces;
            //pointToPoint.EnablePcapAll ("oldi_sim");
        }
        else {
            NS_LOG_INFO ("Build fat tree topology.");
            Ptr<FatTreeHelper> fattree = CreateObject<FatTreeHelper>(size);
            fattree->SetAttribute("HeDataRate", DataRateValue(DataRate(linkBw)));
            fattree->SetAttribute("EaDataRate", DataRateValue(DataRate(linkBw)));
            fattree->SetAttribute("AcDataRate", DataRateValue(DataRate(linkBw)));
            fattree->SetAttribute("HeDelay", TimeValue(Seconds(linkDelay)));
            fattree->SetAttribute("EaDelay", TimeValue(Seconds(linkDelay)));
            fattree->SetAttribute("AcDelay", TimeValue(Seconds(linkDelay)));
            fattree->SetAttribute("PktSpray", BooleanValue(pktspray));
            fattree->SetDevice(deviceType);
            fattree->SetQueue(queueType,
              n1, *v1,
              n2, *v2,
              n3, *v3);
            fattree->Create();
            hosts = fattree->HostNodes ();
            hostIfs = fattree->HostInterfaces ();
        }

	NS_ASSERT_MSG(hosts.GetN() == numHosts,
		"Number of hosts mismatch.");
	NS_ASSERT_MSG(hostIfs.GetN() == numHosts,
		"Number of host interfaces mismatch.");

        std::vector<Ipv4Address> hostAddrs;
        for (uint32_t i = 0; i < hostIfs.GetN(); i++)
          {
            hostAddrs.push_back(hostIfs.GetAddress (i));
          }

	NS_LOG_INFO ("Create OLDI Application.");
        //XXX: Hardcode SRU and NumSenders for now
        std::vector<uint32_t> srus;
        srus.push_back(1<<11); srus.push_back(1<<12);
        srus.push_back(1<<13); //srus.push_back(1<<14);// srus.push_back(1<<15);
        std::vector<uint32_t> senders;
        senders.push_back(10); //senders.push_back(20);
        Ptr<IncastHelper> incastHelper = CreateObject<IncastHelper>();
        /* Load factor is E(flow size) * (flows per second) / (link rate * num hosts)
         * flow per second = (Load factor) * Link rate * hosts / E(flow size)
         * E(flow size) bits = (15360 bytes) * 8 * 15
         * flows per second per host = 542 if load factor = 0.1
         * XXX: Move this into incastHelper. */
        //incastHelper->SetAttribute("IncastsPerSecond", UintegerValue (600 * numHosts));
        incastHelper->SetAttribute("IncastsPerSecond", UintegerValue (200 * numHosts));
        if (d2tcp)
            incastHelper->SetAttribute("TimeAllowed", TimeValue (Seconds (0.005)));
        incastHelper->SetAttribute("Trace", BooleanValue (true));
        incastHelper->SetAttribute("TraceName", StringValue (simPrefix));
        incastHelper->SetAggs (hosts);
        incastHelper->SetSenders (hosts, hostAddrs);
        incastHelper->SetSruSizes(srus);
        incastHelper->SetSenderSizes(senders);
        incastHelper->Start (Seconds (0));

        // Create the bulk send sinks
	const char* socketSpec = "ns3::TcpSocketFactory";
	for (unsigned i = 0; i < numHosts; i++) {
		PacketSinkHelper sink (socketSpec, Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
		ApplicationContainer app = sink.Install (hosts.Get (i));
		app.Start (Seconds (0));
	};

        // Create the traces for the bulk send application
        std::ostringstream oss;
        oss << "bulkSend." << simPrefix << "-" << SeedManager::GetRun () << ".fct";
        AsciiTraceHelper asciiTraceHelper;
        Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (oss.str ());

        // Create the bulk send sources
        std::vector<uint64_t> bulkSizes;
        //bulkSizes.push_back (1<<17); bulkSizes.push_back ((1<<17) + (1<<16)); bulkSizes.push_back (1<<18); bulkSizes.push_back ((1<<18) + (1<<17)); bulkSizes.push_back (1<<19); bulkSizes.push_back (1<<20); bulkSizes.push_back (1<<21); bulkSizes.push_back (1<<22); bulkSizes.push_back(1<<23); bulkSizes.push_back (1<<24); bulkSizes.push_back (1<<25);
        for (int i = 1<<16; i < 1<<20; i += 1<<16) {
            bulkSizes.push_back (i);
        }
        bulkSizes.push_back (1<<20); bulkSizes.push_back (1<<21); bulkSizes.push_back (1<<22); bulkSizes.push_back(1<<23); bulkSizes.push_back (1<<24); bulkSizes.push_back (1<<25);
        std::vector<Address> remoteHostAddrs;
        for (unsigned i = 0; i < hostAddrs.size (); i++) {
            remoteHostAddrs.push_back(InetSocketAddress (hostAddrs[i], port));
        }
        ObjectFactory bulkSendFactory;
        bulkSendFactory.SetTypeId ("ns3::PersistentBulkSendApplication");
        bulkSendFactory.Set ("SendSize", UintegerValue (4192));
        bulkSendFactory.Set ("Protocol", StringValue (socketSpec));
        ApplicationContainer apps;
	for (unsigned i = 0; i < numHosts; i++) {
            Ptr<PersistentBulkSendApplication> bulkSend = bulkSendFactory.Create<PersistentBulkSendApplication> ();
            bulkSend->SetRemotes (remoteHostAddrs);
            bulkSend->SetSizes (bulkSizes);
            bulkSend->TraceConnectWithoutContext ("Start", MakeBoundCallback (&SockStart, stream));
            bulkSend->TraceConnectWithoutContext ("Stop", MakeBoundCallback (&SockStop, stream));
            hosts.Get (i)->AddApplication (bulkSend);
            apps.Add (bulkSend);
	};
        apps.Start (Seconds (0));

	// Collect packet trace
//	PointToPointHelper::EnablePcapAll ("FatTreeSimulation");
//	std::ofstream ascii;
//	ascii.open (outputfile);
//	PointToPointHelper::EnableAsciiAll (ascii);
	std::cout << std::setprecision(9) << std::fixed;
	std::clog << std::setprecision(9) << std::fixed;
//	fattree->EnableAsciiAll (Create<OutputStreamWrapper>(&std::cout));
//	LogComponentEnableAll(LOG_LEVEL_ALL);
//	LogComponentEnable("PriorityBulkSendApplication", LOG_LEVEL_ALL);
//        LogComponentEnable("PriorityQueue", LOG_LEVEL_ALL);
//        LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
//        LogComponentEnable("PriorityTag", LOG_LEVEL_ALL);
//	LogComponentEnable("Simulator", LOG_LEVEL_ALL);
//	LogComponentEnable("MapScheduler", LOG_LEVEL_ALL);
//	LogComponentEnable("Buffer", LOG_LEVEL_ALL);
//	LogComponentEnable("PointToPointNetDevice", LOG_LEVEL_ALL);
//        LogComponentEnable("IncastSender", LOG_LEVEL_ALL);
//        LogComponentEnable("IncastHelper", LOG_LEVEL_ALL);
//        LogComponentEnable("TcpSocketBase", LOG_LEVEL_ALL);
//        LogComponentEnable("Ipv4HashRoutingHelper", LOG_LEVEL_ALL);
//        LogComponentEnable("FatTreeHelper", LOG_LEVEL_ALL);
//        LogComponentEnable("HashRouting", LOG_LEVEL_ALL);
//        LogComponentEnable("PointToPointNetDevice", LOG_LEVEL_ALL);
//        LogComponentEnable("PointToPointHelper", LOG_LEVEL_ALL);
//        LogComponentEnable("PersistentBulkSendApplication", LOG_LEVEL_ALL);
//        LogComponentEnable("Dctcp", LOG_LEVEL_ALL);
//        LogComponentEnable("InfiniteSimpleRedEcnQueue", LOG_LEVEL_ALL);
//        LogComponentEnable("SimpleRedEcnQueue", LOG_LEVEL_ALL);
//        LogComponentEnable("IncastSender", LOG_LEVEL_ALL);
//        LogComponentEnable("IncastAggregator", LOG_LEVEL_ALL);
//        LogComponentEnable("TcpNewReno", LOG_LEVEL_ALL);

        //Simulator::Schedule (Seconds(0.00001), &TraceTcp);
        Simulator::Schedule (Seconds(0.05), &PrintTime);

	// Run the simulation
	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(simLength));
        if (!dryrun)
	    Simulator::Run ();
	Simulator::Destroy ();
	NS_LOG_INFO("Done.");
	return 0;
}
