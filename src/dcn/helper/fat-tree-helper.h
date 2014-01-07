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
 * Author: Fan Wang <amywangfan1985@yahoo.com.cn>
 */
#ifndef FAT_TREE_HELPER_H
#define FAT_TREE_HELPER_H

#include "ns3/boolean.h"
#include "ns3/type-id.h"
#include "ns3/node-container.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"
#include "ns3/ptr.h"
#include "ns3/trace-helper.h"

namespace ns3 {

class FatTreeHelper : public Object, public AsciiTraceHelperForDevice//, public PcapHelperForDevice
{
public:
  static TypeId GetTypeId (void);
  FatTreeHelper(unsigned n);
  virtual ~FatTreeHelper();
  void Create(void);

  void SetQueue (std::string type,
                 std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                 std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                 std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                 std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());
  void SetDevice (std::string type,
                  std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                  std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                  std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                  std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());

  // Functions to retrieve nodes and interfaces
  NodeContainer& AllNodes(void)  { return m_node; };
  NodeContainer& CoreNodes(void) { return m_core; };
  NodeContainer& AggrNodes(void) { return m_aggr; };
  NodeContainer& EdgeNodes(void) { return m_edge; };
  NodeContainer& HostNodes(void) { return m_host; };
  Ipv4InterfaceContainer& CoreInterfaces(void) { return m_coreIface; };
  Ipv4InterfaceContainer& AggrInterfaces(void) { return m_aggrIface; };
  Ipv4InterfaceContainer& EdgeInterfaces(void) { return m_edgeIface; };
  Ipv4InterfaceContainer& HostInterfaces(void) { return m_hostIface; };
private:
  // Aux functions
  //virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename);
  virtual void EnableAsciiInternal (Ptr<OutputStreamWrapper> stream,
    std::string prefix, Ptr<NetDevice> nd, bool explicitFilename);
  void  AssignIP (Ptr<NetDevice> c, uint32_t address, Ipv4InterfaceContainer &con);
  NetDeviceContainer  InstallCpCp (Ptr<Node> a, Ptr<Node> b);
  NetDeviceContainer  InstallCpRp (Ptr<Node> a, Ptr<Node> b);
#if 0
  static void AsciiRxEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet);
  static void AsciiEnqueueEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet);
  static void AsciiDequeueEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet);
  static std::string PathTranslate(const std::string& path);
#endif
  // Parameters
  static unsigned  m_size;    //< This is ugly, but necessary for PathTranslate()
  DataRate  m_heRate;
  DataRate  m_eaRate;
  DataRate  m_acRate;
  Time  m_heDelay;
  Time  m_eaDelay;
  Time  m_acDelay;
  bool m_pktSpray;
  NodeContainer  m_node;
  NodeContainer  m_core;
  NodeContainer  m_aggr;
  NodeContainer  m_host;
  NodeContainer  m_edge;
  Ipv4InterfaceContainer  m_hostIface;
  Ipv4InterfaceContainer  m_edgeIface;
  Ipv4InterfaceContainer  m_aggrIface;
  Ipv4InterfaceContainer  m_coreIface;
  ObjectFactory  m_queueFactory;
  ObjectFactory  m_channelFactory;
  ObjectFactory  m_cpFactory;
  ObjectFactory  m_rpFactory;
};
};

#endif
