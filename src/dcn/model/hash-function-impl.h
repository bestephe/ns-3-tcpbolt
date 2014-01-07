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

#ifndef __HASHROUTE_HASHFUNC_IMPL_H__
#define __HASHROUTE_HASHFUNC_IMPL_H__

#include "hash-function.h"

namespace ns3 {

#if 0
class HashMD5 : public HashFunction
{
public:
  virtual uint64_t operator() (uint32_t salt, flowid tuple) const;
  virtual ~HashMD5() {};
};
#endif

class HashHsieh : public HashFunction
{
public:
  virtual uint64_t operator() (uint32_t salt, flowid tuple) const;
  virtual ~HashHsieh() {};
};

class HashRandom : public HashFunction
{
public:
  virtual uint64_t operator() (uint32_t salt, flowid tuple) const;
  virtual ~HashRandom() {};

};

}; // namespace ns3

#endif
