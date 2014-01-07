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

#ifndef INCAST_HELPER_H
#define INCAST_HELPER_H

#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/incast-agg.h"
#include "ns3/incast-send.h"
//#include "ns3/priority-incast-agg.h"
#include "ns3/priority-incast-send.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/random-variable.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/trace-helper.h"
#include "ns3/traced-callback.h"
#include "ns3/application-container.h"

namespace ns3 {

/**
 * \brief A helper to make it easier to instantiate an ns3::IncastAggregator
 * on a set of nodes.
 */
class IncastHelper : public Object
{
public:
  static TypeId GetTypeId (void);

  /**
   * Create an IncastHelper to make it easier to work with IncastApplications
   *
   * \param protocol the name of the protocol to use to send traffic
   *        by the applications. This string identifies the socket
   *        factory type used to create sockets for the applications.
   *        A typical value would be ns3::UdpSocketFactory.
   * \param address the address of the remote node to send traffic
   *        to.
   */
  IncastHelper ();
  /*
  IncastHelper (std::string protocol, uint32_t sru,
    NodeContainer& aggs, NodeContainer& senders,
    std::vector<Ipv4Address> senderAddrs);
   */

  /**
   * Helper function used to set the underlying application attributes, 
   * _not_ the socket attributes.
   *
   * \param name the name of the application attribute to set
   * \param value the value of the application attribute to set
   */
  void SetIncastAttribute (std::string name, const AttributeValue &value);

  void Install (void);

  /**
   * \brief Arrange for all of the Applications in this container to Start()
   * at the Time given as a parameter.
   *
   * All Applications need to be provided with a starting simulation time and
   * a stopping simulation time.  The ApplicationContainer is a convenient 
   * place for allowing all of the contained Applications to be told to wake
   * up and start doing their thing (Start) at a common time.
   *
   * This method simply iterates through the contained Applications and calls
   * their Start() methods with the provided Time.
   *
   * \param start The Time at which each of the applications should start.
   */
  void Start (Time start);

  /**
   * \brief Arrange for all of the Applications in this container to Stop()
   * at the Time given as a parameter.
   *
   * All Applications need to be provided with a starting simulation time and
   * a stopping simulation time.  The ApplicationContainer is a convenient 
   * place for allowing all of the contained Applications to be told to shut
   * down and stop doing their thing (Stop) at a common time.
   *
   * This method simply iterates through the contained Applications and calls
   * their Stop() methods with the provided Time.
   *
   * \param stop The Time at which each of the applications should stop.
   */
  void Stop (Time stop);

  void SetAggs (NodeContainer aggs);

  void SetSenders (NodeContainer& senders, std::vector<Ipv4Address> senderAddrs);

  void SetSruSizes (std::vector<uint32_t> sruSizes);

  void SetSenderSizes (std::vector<uint32_t> senderSizes);

private:
  std::vector<Ipv4Address> GetRandomSenders (uint32_t numSenders);
  void StartAgg (void);
  void HandleRoundFinish (Ptr<IncastAggregator> aggApp);

  TypeId m_tid;
  std::vector<uint32_t> m_sruSizes;
  std::vector<uint32_t> m_senderSizes;
  Time m_timeAllowed;
  uint32_t m_incastsPerSecond; // Incasts per second
  ObjectFactory m_aggFactory;
  ObjectFactory m_senderFactory;

  NodeContainer m_aggs;     // Aggregator nodes
  ApplicationContainer m_aggApps; // Aggregator applications
  NodeContainer m_senders;  // Sender nodes
  ApplicationContainer m_senderApps; // Sender applications
  std::vector<Ipv4Address> m_senderAddrs;	// Sender Addresses
  ExponentialVariable m_nextIncastVar;

  //bool m_persistent;
  bool m_trace;
  std::string m_fileName;
  Ptr<OutputStreamWrapper> m_sockStream;
  Ptr<OutputStreamWrapper> m_incastStream;

  static sgi::hash_map<Ptr<Socket>, Time, PtrHash> m_sockets;
  static sgi::hash_map<Ptr<IncastAggregator>, Time, PtrHash> m_incasts;
  static void SockStart (Ptr<OutputStreamWrapper> stream, Ptr<Socket> s);
  static void SockStop (Ptr<OutputStreamWrapper> stream, Ptr<Socket> s, uint32_t totBytes);
  static void IncastStart (Ptr<OutputStreamWrapper> stream, Ptr<IncastAggregator> app);
  static void IncastStop (Ptr<OutputStreamWrapper> stream, Ptr<IncastAggregator> app, uint32_t totBytes);
};

} // namespace ns3

#endif /* INCAST_HELPER_H */

