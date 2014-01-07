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

#ifndef INCAST_SENDER_H
#define INCAST_SENDER_H

#include "ns3/application.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/inet-socket-address.h"
#include "ns3/sgi-hashmap.h"

namespace ns3 {

class PtrHash : public std::unary_function<Ptr<Object>, size_t> {
public:
  size_t operator() (Ptr<Object> const &x) const;
};

/**
 * \ingroup applications
 * \defgroup incast IncastSender
 *
 * A part of the incast application. This component is the `sender' part of
 * the incast application, namely, it accepts data from remote peers. Because
 * of the connection-oriented assumption on the sockets, only SOCK_STREAM and
 * SOCK_SEQPACKET sockets are supported, i.e. TCP but not UDP.
 */
class IncastSender : public Application
{
public:
  static TypeId GetTypeId (void);

  IncastSender ();

  virtual ~IncastSender ();

protected:
  virtual void DoDispose (void);
//private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);    // Called at time specified by Start

  // Incast variables
  uint16_t        m_port;         //< TCP port for incast applications
  TypeId          m_tid;          //< Socket's TypeId
  Ptr<Socket>     m_listensock;   //< Listening TCP socket
  typedef sgi::hash_map<Ptr<Socket>, uint32_t, PtrHash> SockCounts;
  SockCounts      m_sockSrus;     //< Number of bytes to send per socket
  SockCounts      m_sockCounts;   //< Number of bytes sent per socket

//private:
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  virtual void SetupSocket (Ptr<Socket> socket);
  virtual void HandleSend (Ptr<Socket> socket, uint32_t n);
  void HandleRead (Ptr<Socket> socket);
  void HandleClose (Ptr<Socket> socket);
};

} // namespace ns3

#endif /* INCAST_SENDER_H */
