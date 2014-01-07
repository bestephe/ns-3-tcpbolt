/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Adrian Sai-wah Tam
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

#ifndef DCTCP_H
#define DCTCP_H

#include "tcp-newreno.h"

namespace ns3 {

/**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief An implementation of a stream socket using TCP.
 *
 * This class contains the NewReno implementation of TCP, as of RFC2582.
 */
class Dctcp : public TcpNewReno
{
public:
  static TypeId GetTypeId (void);
  /**
   * Create an unbound tcp socket.
   */
  Dctcp (void);
  Dctcp (const Dctcp& sock);
  virtual ~Dctcp (void);

protected:
  virtual Ptr<TcpSocketBase> Fork (void);
  virtual void DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface);
  virtual void ReceivedAck (Ptr<Packet> packet, const TcpHeader& t); // Received an ACK packet
  virtual void AddOptions (TcpHeader& h);

private:
  void CheckCe (Ipv4Header header);

  bool               m_ece; // Should send ECE flag
  bool               m_cwr; // Should send CWR flag

  /* DCTCP Specific Parameters from the linux patch */
  uint32_t           m_ackedBytesEcn;
  uint32_t           m_ackedBytesTotal;
  uint32_t           m_alpha;
  double             m_gamma;
  SequenceNumber32   m_nextSeq;
  bool               m_ceState; /* false: last pkt was non-ce , true: last pkt was ce */
  uint32_t           m_shiftG;
  uint32_t           m_shiftGamma;
  Time               m_deadline;
};

} // namespace ns3

#endif /* DCTCP_H */
