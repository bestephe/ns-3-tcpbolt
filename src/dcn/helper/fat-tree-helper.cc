/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Polytechnic Institute of NYU
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
 * Author: Fan Wang <amywangfan1985@yahoo.com.cn>
 */

#define __STDC_LIMIT_MACROS 1
#include <sstream>
#include <stdint.h>
#include <stdlib.h>

#include "fat-tree-helper.h"

#include "ns3/hash-function-impl.h"
#include "ns3/log.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/queue.h"
#include "ns3/uinteger.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/ipv4-address-generator.h"
#include "ns3/ipv4-hash-routing-helper.h"
#include "ns3/config.h"
#include "ns3/abort.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("FatTreeHelper");

NS_OBJECT_ENSURE_REGISTERED (FatTreeHelper);

unsigned FatTreeHelper::m_size = 0;  // Defining static variable

TypeId
FatTreeHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FatTreeHelper")
    .SetParent<Object> ()
    .AddAttribute ("HeDataRate",
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("100Mbps")),
                   MakeDataRateAccessor (&FatTreeHelper::m_heRate),
                   MakeDataRateChecker ())
    .AddAttribute ("EaDataRate",
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("100Mbps")),
                   MakeDataRateAccessor (&FatTreeHelper::m_eaRate),
                   MakeDataRateChecker ())
    .AddAttribute ("AcDataRate",
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("100Mbps")),
                   MakeDataRateAccessor (&FatTreeHelper::m_acRate),
                   MakeDataRateChecker ())
    .AddAttribute ("HeDelay", "Transmission delay through the channel",
                   TimeValue (NanoSeconds (500)),
                   MakeTimeAccessor (&FatTreeHelper::m_heDelay),
                   MakeTimeChecker ())
    .AddAttribute ("EaDelay", "Transmission delay through the channel",
                   TimeValue (NanoSeconds (500)),
                   MakeTimeAccessor (&FatTreeHelper::m_eaDelay),
                   MakeTimeChecker ())
    .AddAttribute ("AcDelay", "Transmission delay through the channel",
                   TimeValue (NanoSeconds (500)),
                   MakeTimeAccessor (&FatTreeHelper::m_acDelay),
                   MakeTimeChecker ())
    .AddAttribute ("PktSpray",
                   "True to enable per-packet random routing",
                   BooleanValue (false),
                   MakeBooleanAccessor (&FatTreeHelper::m_pktSpray),
                   MakeBooleanChecker ())
 ;

  return tid;
}

FatTreeHelper::FatTreeHelper(unsigned N)
{
  m_size = N;
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_channelFactory.SetTypeId ("ns3::PointToPointChannel");
  m_rpFactory.SetTypeId ("ns3::PointToPointNetDevice");
  m_cpFactory.SetTypeId ("ns3::PointToPointNetDevice");
  //XXX: TODO: Set to a QbbNetDevice if lossless is turned on
}

FatTreeHelper::~FatTreeHelper()
{
}

void 
FatTreeHelper::SetQueue (std::string type,
                         std::string n1, const AttributeValue &v1,
                         std::string n2, const AttributeValue &v2,
                         std::string n3, const AttributeValue &v3,
                         std::string n4, const AttributeValue &v4)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void 
FatTreeHelper::SetDevice (std::string type,
                          std::string n1, const AttributeValue &v1,
                          std::string n2, const AttributeValue &v2,
                          std::string n3, const AttributeValue &v3,
                          std::string n4, const AttributeValue &v4)
{
  m_rpFactory.SetTypeId (type);
  m_rpFactory.Set (n1, v1);
  m_rpFactory.Set (n2, v2);
  m_rpFactory.Set (n3, v3);
  m_rpFactory.Set (n4, v4);
  m_cpFactory.SetTypeId (type);
  m_cpFactory.Set (n1, v1);
  m_cpFactory.Set (n2, v2);
  m_cpFactory.Set (n3, v3);
  m_cpFactory.Set (n4, v4);
}

/* Create the whole topology */
void
FatTreeHelper::Create()
{
  const unsigned N = m_size;
  const unsigned numST = 2*N;
  const unsigned numCore = N*N;
  const unsigned numAggr = numST * N;
  const unsigned numEdge = numST * N;
  const unsigned numHost = numEdge * N;
  const unsigned numTotal= numCore + numAggr + numEdge + numHost;

  /*
   * Create nodes and distribute them into different node container.
   * We create 5N^2+2N^3 nodes at a batch, where the first 4N^2 nodes are the
   * edge and aggregation switches. In each of the 2N subtrees, first N nodes are
   * edges and the remaining N are aggregations. The last N^2 nodes in the
   * first 5N^2 nodes are core switches. The last 2N^3 nodes are end hosts.
   */
  NS_LOG_LOGIC ("Creating fat-tree nodes.");
  m_node.Create(numTotal);

  for(unsigned j=0;j<2*N;j++) { // For every subtree
    for(unsigned i=j*2*N; i<=j*2*N+N-1; i++) { // First N nodes
      m_edge.Add(m_node.Get(i));
    }
    for(unsigned i=j*2*N+N; i<=j*2*N+2*N-1; i++) { // Last N nodes
      m_aggr.Add(m_node.Get(i));
    }
  };
  for(unsigned i=4*N*N; i<5*N*N; i++) {
    m_core.Add(m_node.Get(i));
  };
  for(unsigned i=5*N*N; i<numTotal; i++) {
    m_host.Add(m_node.Get(i));
  };

  /*
   * Connect nodes by adding netdevice and set up IP addresses to them.
   *
   * The formula is as follows. We have a fat tree of parameter N, and
   * there are six categories of netdevice, namely, (1) on host;
   * (2) edge towards host; (3) edge towards aggr; (4) aggr towards
   * edge; (5) aggr towards core; (6) on core. There are 2N^3 devices
   * in each category which makes up to 12N^3 netdevices. The IP addrs
   * are assigned in the subnet 10.0.0.0/8 with the 24 bits filled as
   * follows: (Assume N is representable in 6 bits)
   *
   * Address         Scheme
   *               | 7 bit      | 1 bit |  6 bit  | 2 bit | 8 bit   |
   * Host (to edge)| Subtree ID |   0   | Edge ID |  00   | Host ID |
   * Edge (to host)| Subtree ID |   0   | Edge ID |  10   | Host ID |
   * Edge (to aggr)| Subtree ID |   0   | Edge ID |  11   | Aggr ID |
   * Agg. (to edge)| Subtree ID |   0   | Edge ID |  01   | Aggr ID |
   *
   * Address         Scheme
   *               | 7 bit  | 1 bit | 2 bit |  6 bit  | 8 bit   |
   * Agg. (to core)| Subtree ID |   1   |  00   | Aggr ID | Core ID |
   * Core (to aggr)| Subtree ID |   1   |  01   | Core ID | Aggr ID |
   *
   * All ID are numbered from 0 onward. Subtree ID is numbered from left to
   * right according to the fat-tree diagram. Host ID is numbered from
   * left to right within the same attached edge switch. Edge and aggr
   * ID are numbered within the same subtree. Core ID is numbered with a
   * mod-N from left to right according to the fat-tree diagram.
   */
  NS_LOG_LOGIC ("Creating connections and set-up addresses.");
  Ipv4HashRoutingHelper hashHelper;
  if (m_pktSpray)
    hashHelper.SetHashFunction (CreateObject<HashRandom> ());
  InternetStackHelper internet;
  internet.SetRoutingHelper(hashHelper);
  internet.Install (m_node);
  // How to set a hash function to the hash routing????

  m_rpFactory.Set ("DataRate", DataRateValue(m_heRate));  /* Host to Edge */
  m_cpFactory.Set ("DataRate", DataRateValue(m_heRate));
  m_channelFactory.Set ("Delay", TimeValue(m_heDelay));
  for (unsigned j=0; j<numST; j++) { // For each subtree
    for(unsigned i=0; i<N; i++) { // For each edge
      for(unsigned m=0; m<N; m++) { // For each port of edge
        // Connect edge to host
        Ptr<Node> eNode = m_edge.Get(j*N+i);
        Ptr<Node> hNode = m_host.Get(j*N*N+i*N+m);
        NetDeviceContainer devices = InstallCpRp(eNode, hNode);
        // Set routing for end host: Default route only
        Ptr<HashRouting> hr = hashHelper.GetHashRouting(hNode->GetObject<Ipv4>());
        hr->AddRoute(Ipv4Address(0U), Ipv4Mask(0U), 1);
        // Set IP address for end host
        uint32_t address = (((((((10<<7)+j)<<7)+i)<<2)+0x0)<<8)+m;
        AssignIP(devices.Get(1), address, m_hostIface);
        // Set routing for edge switch
        hr = hashHelper.GetHashRouting(eNode->GetObject<Ipv4>());
        hr->AddRoute(Ipv4Address(address), Ipv4Mask(0xFFFFFFFFU), m+1);
        // Set IP address for edge switch
        address = (((((((10<<7)+j)<<7)+i)<<2)+0x2)<<8)+m;
        AssignIP(devices.Get(0), address, m_edgeIface);
      };
    };
  };
  m_cpFactory.Set ("DataRate", DataRateValue(m_eaRate));  /* Edge to Aggr */
  m_channelFactory.Set ("Delay", TimeValue(m_eaDelay));
  for (unsigned j=0; j<numST; j++) { // For each subtree
    for(unsigned i=0; i<N; i++) { // For each edge
      for(unsigned m=0; m<N; m++) { // For each aggregation
        // Connect edge to aggregation
        Ptr<Node> aNode = m_aggr.Get(j*N+m);
        Ptr<Node> eNode = m_edge.Get(j*N+i);
        NetDeviceContainer devices = InstallCpCp(aNode, eNode);
        // Set IP address for aggregation switch
        uint32_t address = (((((((10<<7)+j)<<7)+i)<<2)+0x1)<<8)+m;
        AssignIP(devices.Get(0), address, m_aggrIface);
        // Set routing for aggregation switch
        Ptr<HashRouting> hr = hashHelper.GetHashRouting(aNode->GetObject<Ipv4>());
        hr->AddRoute(Ipv4Address(address & 0xFFFFFC00U), Ipv4Mask(0xFFFFFC00U), i+1);
        // Set IP address for edge switch
        address = (((((((10<<7)+j)<<7)+i)<<2)+0x3)<<8)+m;
        AssignIP(devices.Get(1), address, m_edgeIface);
      } ;
    };
  };
  m_cpFactory.Set ("DataRate", DataRateValue(m_acRate));  /* Aggr to Core */
  m_channelFactory.Set ("Delay", TimeValue(m_acDelay));
  for(unsigned j=0; j<numST; j++) { // For each subtree
    for(unsigned i=0; i<N; i++) { // For each aggr
      for(unsigned m=0; m<N; m++) { // For each port of aggr
        // Connect aggregation to core
        Ptr<Node> cNode = m_core.Get(i*N+m);
        Ptr<Node> aNode = m_aggr.Get(j*N+i);
        NetDeviceContainer devices = InstallCpCp(cNode, aNode);
        // Set IP address for aggregation switch
        uint32_t address = (((((((10<<7)+j)<<3)+0x4)<<6)+i)<<8)+m;
        AssignIP(devices.Get(1), address, m_aggrIface);
        // Set routing for core switch
        Ptr<HashRouting> hr = hashHelper.GetHashRouting(cNode->GetObject<Ipv4>());
        hr->AddRoute(Ipv4Address(address & 0xFFFE0000U), Ipv4Mask(0xFFFE0000U), j+1);
        // Set IP address for core switch
        address = (((((((10<<7)+j)<<3)+0x5)<<6)+m)<<8)+i;
        AssignIP(devices.Get(0), address, m_coreIface);
      };
    };
  };

#ifdef NS3_LOG_ENABLE
  if (! g_log.IsEnabled(ns3::LOG_DEBUG)) {
    return;
  };
  for(unsigned i = 0; i< numTotal; i++) {
    Ptr<Ipv4> m_ipv4 = m_node.Get(i)->GetObject<Ipv4>();
    for(unsigned j=1; j<m_ipv4->GetNInterfaces(); j++) {
      for(unsigned k=0; k<m_ipv4->GetNAddresses(j); k++) {
        NS_LOG_DEBUG("Addr of node " << i << " iface " << j << ": " << m_ipv4->GetAddress(j,k));
      }
    }
  }
#endif
} // FatTreeHelper::Create()

void
FatTreeHelper::AssignIP (Ptr<NetDevice> c, uint32_t address, Ipv4InterfaceContainer &con)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = c->GetNode ();
  NS_ASSERT_MSG (node, "FatTreeHelper::AssignIP(): Bad node");

  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ipv4, "FatTreeHelper::AssignIP(): Bad ipv4");

  int32_t ifIndex = ipv4->GetInterfaceForDevice (c);
  if (ifIndex == -1) {
    ifIndex = ipv4->AddInterface (c);
  };
  NS_ASSERT_MSG (ifIndex >= 0, "FatTreeHelper::AssignIP(): Interface index not found");

  Ipv4Address addr(address);
  Ipv4InterfaceAddress ifaddr(addr, 0xFFFFFFFF);
  ipv4->AddAddress (ifIndex, ifaddr);
  ipv4->SetMetric (ifIndex, 1);
  ipv4->SetUp (ifIndex);
  con.Add (ipv4, ifIndex);
  Ipv4AddressGenerator::AddAllocated (addr);
}


/* Functions to create nodes and connections */
NetDeviceContainer
FatTreeHelper::InstallCpRp (Ptr<Node> a, Ptr<Node> b)
{
  NetDeviceContainer container;

  Ptr<PointToPointNetDevice> devA = m_cpFactory.Create<PointToPointNetDevice> ();
  //devA->SetAttribute("QueueEq",UintegerValue(100));
  //devA->SetAttribute("QbbThreshold",UintegerValue(500));
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA); 
  Ptr<PointToPointNetDevice> devB = m_rpFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

NetDeviceContainer
FatTreeHelper::InstallCpCp (Ptr<Node> a, Ptr<Node> b)
{
  NetDeviceContainer container;

  Ptr<PointToPointNetDevice> devA = m_cpFactory.Create<PointToPointNetDevice> ();
  //devA->SetAttribute("QueueEq",UintegerValue(100));
  //devA->SetAttribute("QbbThreshold",UintegerValue(500));
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);
  Ptr<PointToPointNetDevice> devB = m_cpFactory.Create<PointToPointNetDevice> ();
  //devB->SetAttribute("QueueEq",UintegerValue(100));
  //devB->SetAttribute("QbbThreshold",UintegerValue(500));
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  Ptr<PointToPointChannel> channel = m_channelFactory.Create<PointToPointChannel> ();
  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

void 
FatTreeHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream, 
  std::string prefix, 
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<PointToPointNetDevice> device = nd->GetObject<PointToPointNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("PointToPointHelper::EnableAsciiInternal(): Device " << device << 
                   " not of type ns3::PointToPointNetDevice");
      return;
    }

  Packet::EnablePrinting ();

  if (stream == 0)
    {
      AsciiTraceHelper asciiTraceHelper;
      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, nd);
        }
      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<PointToPointNetDevice> (device, "MacRx", theStream);
      Ptr<Queue> queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue> (queue, "Enqueue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue> (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue> (queue, "Dequeue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<PointToPointNetDevice> (device, "PhyRxDrop", theStream);

      return;
    }

  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/PhyRxDrop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

#if 0
void
FatTreeHelper::AsciiEnqueueEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet)
{
  writer->WritePacket (AsciiWriter::ENQUEUE, PathTranslate(path), packet);
}

void
FatTreeHelper::AsciiDequeueEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet)
{
  writer->WritePacket (AsciiWriter::DEQUEUE, PathTranslate(path), packet);
}

void
FatTreeHelper::AsciiRxEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet)
{
  writer->WritePacket (AsciiWriter::RX, PathTranslate(path), packet);
}

std::string
FatTreeHelper::PathTranslate(const std::string& path)
{
  // path is of the form "/NodeList/nn/DeviceList/nn/$ns3::QbbNetDevice/...",
  // here we extract the node number, in a non-robust way for performance reason
  size_t pos1n = path.find('/',1) + 1;
  size_t pos2n = path.find('/',pos1n);
  unsigned nodeNum = atoi( path.substr(pos1n, pos2n-pos1n).c_str() );
  //if ( (std::istringstream(path.substr(pos1n, pos2n-pos1n)) >> nodeNum).fail() ) {
  //  NS_ABORT_MSG("Cannot find the node number from the context path: " << path);
  //};

  // here we extract the device number
  size_t pos1d = path.find('/',pos2n+1) + 1;
  size_t pos2d = path.find('/',pos1d);
  unsigned devNum = atoi( path.substr(pos1d, pos2d-pos1d).c_str() );
  //if ( (std::istringstream(path.substr(pos1d, pos2d-pos1d)) >> devNum).fail() ) {
  //  NS_ABORT_MSG("Cannot find the device number from the context path: " << path << ", what we have is " << path.substr(pos1d, pos2d-pos1d));
  //};

  // and prepare with the tail
  size_t pos1t = path.find('/',pos2d+1);
  size_t pos2t = path.size();
  std::string tail( path.substr(pos1t, pos2t-pos1t) );
  // convert the node number into location
  const unsigned N = m_size;
  std::ostringstream newpath;
  if (nodeNum < 4*N*N) {
    // Edge or aggregation nodes
    bool isEdge = ((nodeNum/N) % 2 == 0);
    unsigned subtreeNum = nodeNum / (2*N);
    unsigned switchNum = nodeNum % N;
    newpath << "/S" << subtreeNum << (isEdge? "E":"A") << switchNum << "D" << devNum << tail;
  } else if (nodeNum < 5*N*N) {
    // Core nodes
    unsigned coreNum = nodeNum - 4*N*N;
    newpath << "/C" << coreNum << "D" << devNum << tail;
  } else if (nodeNum < 5*N*N + 2*N*N*N) {
    // Host nodes
    unsigned subtreeNum = (nodeNum - 5*N*N) / (N*N);
    unsigned switchNum = ((nodeNum - 5*N*N) / N) % N;
    unsigned hostNum = (nodeNum - 5*N*N) % N;
    newpath << "/S" << subtreeNum << "E" << switchNum << "H" << hostNum << tail;
  } else {
    NS_ABORT_MSG("Node number exceeds range: " << path);
  };
  return newpath.str();
};
#endif

}//namespace
