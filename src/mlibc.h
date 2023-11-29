// Inigo Quilez - 2020
// The MIT License: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef _MLIBC_H_
#define _MLIBC_H_

#include <immintrin.h>

typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
typedef unsigned __int64  uint64_t;

//----------------------------

typedef void *(*mallocFunc)( void *obj, uint64_t amount );
typedef void  (*freeFunc  )( void *obj, void *ptr );
typedef void  (*memcpyFunc)( void *obj, void *dst, void *src, uint64_t len );
typedef void  (*memsetFunc)( void *obj, void *dst, uint8_t val, uint64_t len );
typedef void  (*printFunc )( void *obj, wchar_t *str );

void mlibc_init( void *obj, 
                 mallocFunc,
                 freeFunc,
                 memcpyFunc,
                 memsetFunc,
                 printFunc );

//----------------------------

void *mlibc_malloc( uint64_t amount );
void  mlibc_free( void *ptr );
void  mlibc_memcpy( void *dst, void *src, uint64_t len );
void  mlibc_memset( void *dst, uint8_t val, uint64_t len );
void  mlibc_print( wchar_t *str );



#endif