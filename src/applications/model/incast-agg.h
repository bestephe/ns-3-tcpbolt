/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 New York University
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
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#ifndef INCAST_AGGREGATOR_H
#define INCAST_AGGREGATOR_H

#include "ns3/application.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/inet-socket-address.h"
#include "ns3/traced-callback.h"

//class TcpAdmCtrl;

namespace ns3 {

/**
 * \ingroup applications
 * \defgroup incast IncastAggregator
 *
 * A part of the incast application. This component is the `aggregator' part of
 * the incast application, namely, it accepts data from remote peers. Because
 * of the connection-oriented assumption on the sockets, only SOCK_STREAM and
 * SOCK_SEQPACKET sockets are supported, i.e. TCP but not UDP.
 */
class IncastAggregator : public Application
{
public:
  static TypeId GetTypeId (void);

  IncastAggregator ();

  virtual ~IncastAggregator ();

  /**
   * Associate this incast aggregator with the incast senders
   */
  void SetSenders (const std::vector<Ipv4Address>& n);

  /**
   * Set round finish callback
   */
  void SetRoundFinishCallback (Callback<void, Ptr<IncastAggregator> > cb);

protected:
  virtual void DoDispose (void);
//private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  // Incast variables
  uint32_t        m_sru;          //< Size of one SRU
  uint16_t        m_port;         //< TCP port for incast applications
  TypeId          m_tid;          //< Socket's TypeId
  bool            m_running;      //< Incast running
  uint32_t        m_closeCount;   //< Count of closed connections
  uint32_t        m_byteCount;
  std::list<Ptr<Socket> >  m_sockets;      //< Associated sockets
  std::vector<Ipv4Address> m_senders;      //< All incast senders
  //std::list<Ptr<TcpAdmCtrl> > m_suspendedSocket;       //< suspended TCP socket

  Callback<void, Ptr<IncastAggregator> >  m_roundFinish;  //< Round-finish callback
  TracedCallback<Ptr<Socket> > m_sockStart;
  TracedCallback<Ptr<Socket>, uint32_t> m_sockStop;
  TracedCallback<Ptr<IncastAggregator> > m_start;
  TracedCallback<Ptr<IncastAggregator>, uint32_t> m_stop;

//private:
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void HandleClose (Ptr<Socket> socket);
  void HandleSend (Ptr<Socket> socket, uint32_t n);
  //virtual void HandleSend (Ptr<Socket> socket, uint32_t n);
  void HandleRead (Ptr<Socket> socket);
  //void HandleAdmCtrl (Ptr<TcpAdmCtrl> sock, Ipv4Address addr, uint16_t port);
  void RoundFinish (void);
};

} // namespace ns3

#endif /* INCAST_AGGREGATOR_H */
