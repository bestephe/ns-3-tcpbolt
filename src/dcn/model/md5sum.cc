/* md5sum.cc - C++ Wrapper for the MD5 implementation by L. Peter Deutsch
 * Copyright 2009 (c) New York University
 * Copyright 2006 (c) The CUPIDE Project
 * Programmer: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#include "md5.h"
#include "md5sum.h"
#include <string.h>
#include <string>

md5sum::md5sum()
{
	state = new md5_state_t;
	reset();
};

md5sum::~md5sum()
{
	if (state) { delete state; };
};

void md5sum::reset()
{
	md5_init(state);
};

inline md5_byte_t* md5sum::digest(const std::string& s)
{
	return digest(s.c_str());
};

md5_byte_t* md5sum::digest(const char* s)
{
	reset();
	md5_append(state, (const md5_byte_t*)s, strlen(s));
	md5_finish(state, digestCode);
	return digestCode;
};

md5_byte_t* md5sum::getDigest()
{
	md5_finish(state, digestCode);
	return digestCode;
};

inline void md5sum::append(const std::string& s)
{
	append(s.c_str());
};

void md5sum::append(const char*s)
{
	md5_append(state, (const md5_byte_t*)s, strlen(s));
};
