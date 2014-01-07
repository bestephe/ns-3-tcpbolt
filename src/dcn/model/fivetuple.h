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

#ifndef __FIVETUPLE_H__
#define __FIVETUPLE_H__

#include <iostream>
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"

namespace ns3 {

// Use 128-bit to store the 104-bit (five-tuple) flow ID
class flowid {
	friend inline bool operator==(const flowid& f1, const flowid& f2);
	friend class CnHeader;
public:
	flowid() : hi(0), lo(0) {};
	flowid(Ptr<Packet> p);	// Set the flow Id from a packet
	flowid(char* id);	// Set the flow Id using a 16-byte char*
	flowid(const Ipv4Header& iph);	// Set partial flow Id using Ipv4Header
	flowid(uint32_t _saddr, uint32_t _daddr, uint8_t _proto, uint16_t _sport, uint16_t _dport);
	void SetSAddr(uint32_t saddr);
	void SetDAddr(uint32_t daddr);
	void SetSPort(uint16_t sport);
	void SetDPort(uint16_t dport);
	void Print(std::ostream& os) const;
	uint32_t GetSAddr() const;
	uint32_t GetDAddr() const;
	uint16_t GetSPort() const;
	uint16_t GetDPort() const;
	uint8_t GetProtocol() const;
	char* Write(char* s) const;
	operator char*() const;
protected:
	uint64_t hi,lo;
};

// Report equivalence of flow IDs
inline bool operator==(const flowid& f1, const flowid& f2)
{
	return ( (f1.hi == f2.hi) && (f1.lo == f2.lo) );
};

inline std::ostream& operator<<(std::ostream& os, const flowid& fid)
{
	fid.Print(os);
	return os;
};

}; // namespace ns3

#endif
