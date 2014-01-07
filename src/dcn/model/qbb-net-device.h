/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Adrian S. Tam
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
 */

#ifndef QBB_NET_DEVICE_H
#define QBB_NET_DEVICE_H

#include "ns3/point-to-point-net-device.h"
#include "ns3/fivetuple.h"
#include "ns3/event-id.h"

namespace ns3 {

/**
 * \class QbbNetDevice
 * \brief A Device for a IEEE 802.1Qbb Network Link.
 */
class QbbNetDevice : public PointToPointNetDevice 
{
public:
  //TODO: Lets cut out multiple queues for now
  static const unsigned qCnt = 8;	// Number of queues/priorities used

  static TypeId GetTypeId (void);

  QbbNetDevice ();
  virtual ~QbbNetDevice ();

  /**
   * Attach a queue to the PointToPointNetDevice.
   *
   * The PointToPointNetDevice "owns" a queue that implements a queueing 
   * method such as DropTail or RED.  The queue must be an infinite queue
   *
   * @see Queue
   * @see DropTailQueue
   * @param queue Ptr to the new queue.
   */
  virtual void SetQueue (Ptr<Queue> queue);

  /**
   * Receive a packet from a connected PointToPointChannel.
   *
   * This is to intercept the same call from the PointToPointNetDevice
   * so that the pause messages are honoured without letting
   * PointToPointNetDevice::Receive(p) know
   *
   * @see PointToPointNetDevice
   * @param p Ptr to the received packet.
   */
  virtual void Receive (Ptr<Packet> p);

  /**
   * Send a packet to the channel by putting it to the queue
   * of the corresponding priority class
   *
   * @param packet Ptr to the packet to send
   * @param dest Unused
   * @param protocolNumber Protocol used in packet
   */
  virtual bool Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

  /**
   * Get the size of Tx buffer available in the device
   *
   * @return buffer available in bytes
   */
  virtual uint32_t GetTxAvailable() const;

  /**
   * TracedCallback hooks
   */
  virtual void ConnectWithoutContext(const CallbackBase& callback);
  virtual void DisconnectWithoutContext(const CallbackBase& callback);

  virtual int32_t PrintStatus(std::ostream& os);

protected:
  virtual void DoDispose(void);

  /// Reset the channel into READY state and try transmit again
  virtual void TransmitComplete(void);

  /// Look for an available packet and send it using TransmitStart(p)
  virtual void DequeueAndTransmit(void);

  /// Resume a paused queue and call DequeueAndTransmit()
  virtual void Resume(unsigned qIndex);

  /// Wrapper to Resume() as an schedulable function
  void PauseFinish(unsigned qIndex);

  /**
   * Function to generate PAUSE message if necessary
   *
   * @param qIndex The queue index to check
   */
  void CheckQueueFull(unsigned qIndex);

  /// Tell if an address is local to this node
  bool IsLocal(const Ipv4Address& addr) const;

  /**
   * The queues for each priority class.
   * @see class Queue
   * @see class InfiniteQueue
   */
  std::vector<Ptr<Queue> > m_queues;

  TracedCallback<Ptr<NetDevice>, uint32_t> m_sendCb;	//< Callback chain for notifying Tx buffer available

  bool m_qbbEnabled;	//< Qbb behaviour enabled
  uint32_t m_threshold;	//< Threshold for generating Pause
  uint32_t m_pausetime;	//< Time for each Pause
  uint32_t m_buffersize;//< Total size of Tx buffer
  uint32_t m_bufferUsage;	//< Occupancy at the buffer
  bool m_paused[qCnt];	//< Whether a queue paused
  unsigned m_lastQ;	//< Last accessed queue
  EventId m_resumeEvt[qCnt];  //< Keeping the next resume event
  EventId m_recheckEvt[qCnt]; //< Keeping the next recheck queue full event
#if 1
  std::list<uint32_t> m_arrival[qCnt];
#else
  std::vector<double> m_arrival[qCnt];
#endif

};

} // namespace ns3

#endif // QBB_NET_DEVICE_H

