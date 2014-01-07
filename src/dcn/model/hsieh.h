/* The Hsieh hash function
 * http://www.azillionmonkeys.com/qed/hash.html
 */

#ifndef __HSIEH_H__
#define __HSIEH_H__
#include <stdint.h>

/* This function was called "SuperFastHash" by Hsieh */
uint32_t HsiehHash (const char * data, int len);

#endif /* __HSIEH_H__ */
