/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008-2009 New York University
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
 * Author: Chang Liu <cliu02@students.poly.edu>
 * Author: Adrian S.-W. Tam <adrian.sw.tam@gmail.com>
 */
 
#include "hash-function-impl.h"
//#include "md5sum.h"
extern "C" {
#include "hsieh.h"
}
#include <stdlib.h>
#include <sstream>

namespace ns3 {

#if 0
uint64_t HashMD5::operator() (uint32_t salt, flowid tuple) const
{
	md5sum MD5;		// MD5 engine
	char x[4+17];		// Stringify the salt
	for (int i=0;i<4;i++) {
		x[i] = ((char*)(&salt))[i];
	};
	tuple.Write(&x[4]);
	MD5 << x;		// Feed salt and flowid into MD5

	// Convert 128-bit MD5 digest into 64-bit unsigned integer
	unsigned char* b = MD5.getDigest();
	uint64_t val = 0;
	for (int i=0; i<8; i++) {
		val <<= 8;
		val |= b[i] ^ b[15-i];
	};
	return val;
}
#endif

uint64_t HashHsieh::operator() (uint32_t salt, flowid tuple) const
{
	char x[4+17];		// 4-byte for salt, 16-byte/128-bit for tuple
	for (int i=0;i<4;i++) { // Stringify the salt
		x[i] = ((char*)(&salt))[i];
	};
	tuple.Write(&x[4]);
	return HsiehHash (x, 4+16);
}


uint64_t HashRandom::operator() (uint32_t salt, flowid tuple) const
{
	return random();
}

};
