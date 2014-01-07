// -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*-
//
// Copyright (c) 2009 New York University
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//

#ifndef HASH_ROUTING_H
#define HASH_ROUTING_H

#include <list>
#include <set>
#include <stdint.h>
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/ipv4.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ref-count-base.h"
#include "fivetuple.h"
#include "hash-function.h"

namespace ns3 {

class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Ipv4RoutingTableEntry;
class Ipv4MulticastRoutingTableEntry;
class Node;

// Destination-based routing table
class DestRoute : public RefCountBase
{
public:
  DestRoute(Ipv4Address const& d, Ipv4Mask const& m, uint32_t i) : dest(d), mask(m), outPort(i) {};
  Ipv4Address dest;
  Ipv4Mask mask;
  uint32_t outPort;
};

typedef std::list<Ptr<DestRoute> > DestRouteTable;

// Flow-based routing table indexed by five-tuple
class FlowRoute : public RefCountBase
{
public:
  FlowRoute(const flowid& f, const Time& t, uint32_t i) : fid(f), last(t), outPort(i) {};
  flowid fid;
  Time last;
  uint32_t outPort;
};

typedef std::list<Ptr<FlowRoute> > FlowRouteTable;

// Congestion signal record
class CongRecord : public RefCountBase
{
public:
  CongRecord(const flowid& f, const Time& t, uint32_t c) : fid(f), last(t), count(c) {};
  flowid fid;
  Time last;
  uint32_t count;
  std::set<uint32_t> congPoints;
};

typedef std::list<Ptr<CongRecord> > CongRecordTable;

// Class for hash-based routing logic
class HashRouting : public Ipv4RoutingProtocol
{
public:
  static TypeId GetTypeId (void);
  HashRouting ();
  virtual ~HashRouting ();

  // Functions defined in Ipv4RoutingProtocol
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
      const Ipv4Header &header,
      Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  virtual bool RouteInput  (Ptr<const Packet> p,
      const Ipv4Header &header, Ptr<const NetDevice> idev,
      UnicastForwardCallback ucb, MulticastForwardCallback mcb,
      LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface) {};
  virtual void NotifyInterfaceDown (uint32_t interface) {};
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {};
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {};
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);

  /**
   * \brief Add a destination-based route to the routing table.
   *
   * \param dest The Ipv4Address network for this route.
   * \param mask The Ipv4Mask to extract the network.
   * \param interface The network interface index used to send packets to the
   * destination.
   *
   * \see Ipv4Address
   */
  void AddRoute (const Ipv4Address& dest, const Ipv4Mask& mask, uint32_t interface);

  /**
   * \brief Set the hash function to be used in the hash-based routing
   *
   * \param hash The pointer to a hash function object
   */
  void SetHashFunction (Ptr<HashFunction> hash) { m_hash = hash; };

  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;
protected:
  flowid GetTuple(Ptr<const Packet> packet, const Ipv4Header& header);
  uint32_t Lookup (flowid fid);
  bool IsLocal (const Ipv4Address& dest);

  /*
   * \brief Get the output port for the destination address
   *
   * This function is called by the RequestIfIndex() function when this node is
   * an edge switch.
   *
   * \param iph    The Ipv4Header of the packet to be routed
   * \param outPort   Output port number to be filled
   * \return true if a route is found, false otherwise
   */
  bool RequestDestRoute(const Ipv4Address& addr, uint32_t &outPort);

  /*
   * \brief Get the output port from flow-based routing table
   *
   * \param tuple    The flow id as five tuple
   * \param outPort   Output port number to be filled
   * \return true if a route is found, false otherwise
   */
  bool RequestFlowRoute(const flowid& tuple, uint32_t& outPort);

  /*
   * Given the flow Id, return a 32-but hash value
   *
   * \param tuple  The flow id as five tuple
   * \return 32-bit hash value specific to this incarnation of routing module
   */
  uint32_t Hash(const flowid& tuple) const;

  /*
   * Given an address, tell if it is an downward port on an edge switch.
   *
   * \return true if the address is an edge switch, false otherwise
   */
  bool IsEdge(const Ipv4Address& addr) const;

  /*
   * Check if rerouting shall be triggered for a flow
   *
   * \param fid  The flow id as five tuple
   * \param cp    The IP address of the congestion point
   * \return true if the flow shall be rerouted, false otherwise
   */
  bool NeedReroute(const flowid& fid, uint32_t cp);

  /*
   * Reroute a flow by adding/modifying an entry in the flow table
   *
   * \param fid  The flow id as five tuple
   * \param oldPort Original outgoing port number
   * \return New outgoing port number
   */
  uint32_t Reroute(const flowid& fid, uint32_t oldPort);

  /* Rerouting using the congestion information assisted algorithm
   */
  uint32_t IntelRoute(const flowid& fid, uint32_t oldPort);

  /*
   * Clean up everything to make this object virtually dead
   */
    virtual void DoDispose (void);
protected:
  Ptr<HashFunction> m_hash;

  DestRouteTable m_destRouteTable;
  FlowRouteTable m_flowRouteTable;
  CongRecordTable m_congRecord;

  std::set<uint32_t> localAddrCache;
  
  Ptr<Ipv4> m_ipv4;  // Hook to the Ipv4 object of this node
  bool m_intelRR;    // Intelligent reroute
  bool m_enableRR;  // Enable reroute
  uint32_t m_rrThresh;  // CN threshold for reroute
  Time m_lifetime;  // Lifetime of a CN record
};

} // Namespace ns3

#endif /* HASH_ROUTING_H */
