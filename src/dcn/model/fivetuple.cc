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

#include "ns3/fivetuple.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("FiveTuple");

namespace ns3 {

// Given a packet, retrieve the flow id
flowid::flowid(Ptr<Packet> p)
{
	Ipv4Header iph;
	Ptr<Packet> q = p->Copy();
	q->RemoveHeader(iph);
	uint32_t saddr = iph.GetSource().Get();
	uint32_t daddr = iph.GetDestination().Get();
	uint8_t  proto = iph.GetProtocol();
	uint16_t sport = 0;
	uint16_t dport = 0;
	if (proto == 17) {
		UdpHeader udph;
		q->RemoveHeader(udph);
		sport = udph.GetSourcePort();
		dport = udph.GetDestinationPort();
	} else if (proto == 6) {
		TcpHeader tcph;
		q->RemoveHeader(tcph);
		sport = tcph.GetSourcePort();
		dport = tcph.GetDestinationPort();
	};

	// Set the bits, note the first 12 bit in lo is always zero
	hi = (static_cast<uint64_t>(saddr) << 32)
		| static_cast<uint64_t>(daddr);
	lo = (((static_cast<uint64_t>(proto) << 16)
		| static_cast<uint64_t>(sport)) << 16 )
		| static_cast<uint64_t>(dport);

	NS_LOG_INFO("(" << iph.GetSource() << ":" << sport << " " << (int)proto << " " << iph.GetDestination() << ":" << dport << ") -> " << (*this));
};

// Retrieve the flow id directly from five tuple
flowid::flowid(uint32_t saddr, uint32_t daddr, uint8_t proto, uint16_t sport, uint16_t dport)
{
	// Set the bits, note the first 12 bit in lo is always zero
	hi = (static_cast<uint64_t>(saddr) << 32)
		| static_cast<uint64_t>(daddr);
	lo = (((static_cast<uint64_t>(proto) << 16)
		| static_cast<uint64_t>(sport)) << 16 )
		| static_cast<uint64_t>(dport);
};

flowid::flowid(char* id)
{
	hi = lo = 0;
	for (int i=0;i<8;i++) {
		hi <<= 8;
		lo <<= 8;
		hi |= id[i];
		lo |= id[8+i];
	};
};

flowid::flowid(const Ipv4Header& iph)
{
	hi = lo = 0;
	uint32_t saddr = iph.GetSource().Get();
	uint32_t daddr = iph.GetDestination().Get();
	uint8_t  proto = iph.GetProtocol();

	// Set the bits, note the first 12 bit in lo is always zero
	hi = (static_cast<uint64_t>(saddr) << 32)
		| static_cast<uint64_t>(daddr);
	lo = static_cast<uint64_t>(proto) << 32;
};

void flowid::SetSPort(uint16_t sport)
{
	lo |= static_cast<uint64_t>(sport) << 16;
};

void flowid::SetDPort(uint16_t dport)
{
	lo |= static_cast<uint64_t>(dport);
};

void flowid::SetSAddr(uint32_t saddr)
{
	hi |= static_cast<uint64_t>(saddr) << 32;
};

void flowid::SetDAddr(uint32_t daddr)
{
	hi |= static_cast<uint64_t>(daddr);
};

uint32_t flowid::GetSAddr() const
{
	return (uint32_t)(hi>>32);
};

uint32_t flowid::GetDAddr() const
{
	return (uint32_t)(hi & 0x00000000FFFFFFFFLLU);
};

uint16_t flowid::GetSPort() const
{
	return (uint16_t)((lo<<32)>>48);
};

uint16_t flowid::GetDPort() const
{
	return (uint16_t)((lo<<48)>>48);
};

uint8_t flowid::GetProtocol() const
{
	return (uint8_t)((lo<<24)>>56);
};

void flowid::Print(std::ostream& os) const
{
	os << "[";
	for (int i=0; i<4; i++) {
		// Src IP
		os << (int)((hi<<(i*8))>>(7*8));
		os << ((i==3)?":":".");
	};
	os << GetSPort();	//Src Port
	os << " " << (int)GetProtocol() << " "; // Protocol
	for (int i=4; i<8; i++) {
		// Dest IP
		os << (int)((hi<<(i*8))>>(7*8));
		os << ((i==7)?":":".");
	};
	os << GetDPort() << "]";	//Dest Port
	//os << std::hex << " ("<< hi << ","<< lo << std::dec << ")";
};

flowid::operator char*() const
{
	static char s[16];
	return Write(s);
}

char* flowid::Write(char* s) const
{
	for (int i=0; i<8; i++) {
		s[i] = ((char*)(&hi))[i];
		s[i+8] = ((char*)(&lo))[i];
	};
	return s;
};

}; // namespace ns3
