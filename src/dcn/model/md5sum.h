/* md5sum.h - Header file of the C++ Wrapper for the MD5 implementation by L. Peter Deutsch
 * Copyright 2009 (c) New York University
 * Copyright 2006 (c) The CUPIDE Project
 * Programmer: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#ifndef __MD5SUM_H__
#define __MD5SUM_H__

#include "md5.h"
#include <string>

class md5sum {
protected:
	md5_state_t* state;
	md5_byte_t digestCode[16];
public:
	md5sum();
	~md5sum();
	void reset();
	md5_byte_t* digest(const char* s);
	inline md5_byte_t* digest(const std::string& s);
	md5_byte_t* getDigest();
	void append(const char*s);
	inline void append(const std::string& s);
};

inline md5sum& operator<<(md5sum& m, const char*s)
{
	m.append(s);
	return m;
};

inline md5sum& operator<<(md5sum& m, const std::string& s)
{
	return operator<<(m,s.c_str());
};

#endif /*  __MD5SUM_H__ */
