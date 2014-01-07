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

#ifndef __HASHROUTE_HASHFUNC_H__
#define __HASHROUTE_HASHFUNC_H__

#include <stdint.h>
#include "ns3/fivetuple.h"
#include "ns3/object.h"

namespace ns3 {

class HashFunction : public Object
{
public:
	virtual ~HashFunction() {};
	virtual uint64_t operator() (uint32_t salt, flowid tuple) const = 0;
};

}; // namespace ns3

#endif
