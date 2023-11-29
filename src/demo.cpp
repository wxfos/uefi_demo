// Inigo Quilez - 2020
// The MIT License: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "mlibc.h"

struct
{
    uint32_t   *mLut;
    uint32_t   *mTexture;
}gDemo; // i don't like having globals. TODO - put on cient's stack or heap, please

//===================
static int imin( int a, int b ) { return (a<b)?a:b; }

bool Demo_Init( int xres, int yres )
{
    gDemo.mTexture = (uint32_t*)mlibc_malloc( 256*256*4 );
    if( !gDemo.mTexture ) return false;

    gDemo.mLut = (uint32_t*)mlibc_malloc( xres*yres*4 );
    if( !gDemo.mLut ) return false;

    //--------------------------

    for( int j=0; j<256; j++ )
    for( int i=0; i<256; i++ )
    {
        const int r = i^(j>>1);
        const int g = (i>>1)^j;
        const int b = i^j;
        gDemo.mTexture[256*j+i] = 0xff000000 | (r<<16) | (g<<8) | b;
    }

    //--------------------------

    for( int j=0; j<yres; j++ )
    for( int i=0; i<xres; i++ )
    {
        const float x = float(2*i-xres)/float(yres);
        const float y = float(2*j-yres)/float(yres);

#if 0
        // tunnel
        const float r = mlibc_sqrtf(x*x+y*y);
        const float a = mlibc_atan2f(y,x);
        const float u = 3.0f*a/3.141592f;
        const float v = 1.0f/r;
        const int iu = int(1024.0f*u) & 1023;
        const int iv = int(1024.0f*v) & 1023;
        const int iw = imin(1023,int(512.0f*r)) & 1023;
#else
        // inversion
        float r2 = x*x + y*y;
        const float u = x/r2;
        const float v = y/r2;
        const int iu = int(1024.0f*u) & 1023;
        const int iv = int(1024.0f*v) & 1023;
        const int iw = imin(1023,int(512.0f*r2)) & 1023;
#endif

        gDemo.mLut[xres*j+i] = (iw<<20) | (iv<<10) + iu;
    }

    return true;
}

void Demo_DeInit( void )
{
    mlibc_free( gDemo.mTexture ); 
    mlibc_free( gDemo.mLut ); 
}

void Demo_Render( uint32_t *buffer, int xres, int yres, float iTime )
{
    const uint32_t ani = uint32_t(iTime*400.0f);
    const uint32_t fad = imin(int(255.0f*iTime/2.0f),255);

    const uint32_t numPixels = xres*yres;
    for( uint32_t i=0; i<numPixels; i++ )
    {
        // fetch deformation
        const uint32_t offs = gDemo.mLut[i];

        // decode
        uint32_t u = (offs      ) & 1023;
        uint32_t v = (offs >> 10) & 1023;
        uint32_t w = (offs >> 20);

        // animate
        v = (v+ani)&1023;

        // texture scale (should do bilinear filtering)
        u >>= 2;
        v >>= 2;
        const uint32_t col = gDemo.mTexture[(v<<8)+u];

        // extract colors
        uint32_t r = (col>>16)&255;
        uint32_t g = (col>> 8)&255;
        uint32_t b = (col>> 0)&255;

        // fade from black
        w = w*fad;

        // lut brightness (could be done with 2 mul/shifts instead of 3)
        r = (r*w)>>18; //18 = 10(w) + 8(fad)
        g = (g*w)>>18;
        b = (b*w)>>18;
            
        // output
        buffer[i] = 0xff000000 | (r<<16) | (g<<8) | b;
    }
}
 