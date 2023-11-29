// Inigo Quilez - 2020
// The MIT License: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "mlibc.h"

struct
{
    void      *mObj;
    mallocFunc mMalloc;
    freeFunc   mFree;
    memcpyFunc mMemcpy;
    memsetFunc mMemset;
    printFunc  mPrint;
}mlibc;

void mlibc_init( void *obj, 
                 mallocFunc mallocf,
                 freeFunc   freef,
                 memcpyFunc memcpyf,
                 memsetFunc memsetf,
                 printFunc  printf )
{
    mlibc.mObj = obj;
    mlibc.mMalloc = mallocf;
    mlibc.mFree = freef;
    mlibc.mMemcpy = memcpyf;
    mlibc.mMemset = memsetf;
    mlibc.mPrint = printf;
}

void *mlibc_malloc( uint64_t amount ) { return mlibc.mMalloc( mlibc.mObj, amount ); }
void  mlibc_free( void *ptr ) { mlibc.mFree( mlibc.mObj, ptr ); }
void  mlibc_memcpy( void *dst, void *src, uint64_t len ) { mlibc.mMemcpy( mlibc.mObj, dst, src, len ); }
void  mlibc_memset( void *dst, uint8_t val, uint64_t len ) { mlibc.mMemset( mlibc.mObj, dst, val, len ); }
void  mlibc_print( wchar_t *str ) { mlibc.mPrint( mlibc.mObj, str ); }