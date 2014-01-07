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

#ifndef TCP_ADMCTRL_H
#define TCP_ADMCTRL_H

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
class TcpAdmCtrl : public TcpNewReno
{
public:
  static TypeId GetTypeId (void);
  /**
   * Create an unbound tcp socket.
   */
  TcpAdmCtrl (void);
  TcpAdmCtrl (const TcpAdmCtrl& sock);
  virtual ~TcpAdmCtrl (void);
  void SetAdmCtrlCallback(Callback<void, Ptr<TcpAdmCtrl>, Ipv4Address, uint16_t> cb);
  void ResumeConnection (void);

protected:
  virtual Ptr<TcpSocketBase> Fork (void);
  virtual void DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface);
  virtual void AddOptions (TcpHeader& h);
  virtual void ReadOptions (const TcpHeader& h);
  virtual void DoRetransmit (void);

private:
  Ptr<Packet>        m_bufPacket;
  Ipv4Header         m_bufHeader;
  uint16_t           m_bufPort;
  Ptr<Ipv4Interface> m_bufIface;
  uint32_t           m_lastTS;
  uint32_t           m_retxTS;
  bool               m_ece; // Should send ECE flag
  bool               m_cwr; // Should send CWR flag

  Callback<void, Ptr<TcpAdmCtrl>, Ipv4Address, uint16_t> m_admCtrlCallback;
};

} // namespace ns3

#endif /* TCP_NEWRENO_H */
