/*
 * libtxc_dxtn
 * Version:  0.1
 *
 * Copyright (C) 2004  Roland Scheidegger   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include "txc_dxtn.h"

/* inefficient. To be efficient, it would be necessary to decode 16 pixels at once */

static void dxt135_decode_imageblock ( const GLubyte *img_block_src,
                              GLint i, GLint j, GLuint dxt_type, GLvoid *texel ) {
   GLchan *rgba = (GLchan *) texel;
   const GLubyte *c0_lo = img_block_src + 0;
   const GLubyte *c0_hi = img_block_src + 1;
   const GLubyte *c1_lo = img_block_src + 2;
   const GLubyte *c1_hi = img_block_src + 3;
   const GLubyte *bits_0 = img_block_src + 4;
   const GLubyte *bits_1 = img_block_src + 5;
   const GLubyte *bits_2 = img_block_src + 6;
   const GLubyte *bits_3 = img_block_src + 7;
   GLushort color0 = *c0_lo + *c0_hi * 256;
   GLushort color1 = *c1_lo + *c1_hi * 256;
   GLuint bits = *bits_0 + 256 * (*bits_1 + 256 * (*bits_2 + 256 * *bits_3));
   /* What about big/little endian? */
   GLubyte bit_pos = 2 * (j * 4 + i) ;
   GLubyte code = (GLubyte) ((bits >> bit_pos) & 3);
   switch (code) {
      case 0:
         rgba[RCOMP] = UBYTE_TO_CHAN( ((color0 >> 8) & 0xf8) * 255 / 0xf8 );
         rgba[GCOMP] = UBYTE_TO_CHAN( ((color0 >> 3) & 0xfc) * 255 / 0xfc );
         rgba[BCOMP] = UBYTE_TO_CHAN( ((color0 << 3) & 0xf8) * 255 / 0xf8 );
         rgba[ACOMP] = CHAN_MAX;
         break;
      case 1:
         rgba[RCOMP] = UBYTE_TO_CHAN( ((color1 >> 8) & 0xf8) * 255 / 0xf8 );
         rgba[GCOMP] = UBYTE_TO_CHAN( ((color1 >> 3) & 0xfc) * 255 / 0xfc );
         rgba[BCOMP] = UBYTE_TO_CHAN( ((color1 << 3) & 0xf8) * 255 / 0xf8 );
         rgba[ACOMP] = CHAN_MAX;
         break;
      case 2:
         if (color0 > color1) {
            rgba[RCOMP] = UBYTE_TO_CHAN( (((color0 >> 8) & 0xf8) * 255 / 0xf8 * 2 + ((color1 >> 8) & 0xf8) * 255 / 0xf8 ) / 3);
            rgba[GCOMP] = UBYTE_TO_CHAN( (((color0 >> 3) & 0xfc) * 255 / 0xfc * 2 + ((color1 >> 3) & 0xfc) * 255 / 0xfc ) / 3);
            rgba[BCOMP] = UBYTE_TO_CHAN( (((color0 << 3) & 0xf8) * 255 / 0xf8 * 2 + ((color1 << 3) & 0xf8) * 255 / 0xf8 ) / 3);
            rgba[ACOMP] = CHAN_MAX;
         }
         else {
            rgba[RCOMP] = UBYTE_TO_CHAN( (((color0 >> 8) & 0xf8) * 255 / 0xf8 + ((color1 >> 8) & 0xf8) * 255 / 0xf8 ) / 2);
            rgba[GCOMP] = UBYTE_TO_CHAN( (((color0 >> 3) & 0xfc) * 255 / 0xfc + ((color1 >> 3) & 0xfc) * 255 / 0xfc ) / 2);
            rgba[BCOMP] = UBYTE_TO_CHAN( (((color0 << 3) & 0xf8) * 255 / 0xf8 + ((color1 << 3) & 0xf8) * 255 / 0xf8 ) / 2);
            rgba[ACOMP] = CHAN_MAX;
         }
         break;
      case 3:
         /* don't understand the spec. Is the dxt_type switch necessary for other code cases too? */
         if ((dxt_type > 1) || (color0 > color1)) {
            rgba[RCOMP] = UBYTE_TO_CHAN( (((color0 >> 8) & 0xf8) * 255 / 0xf8 + ((color1 >> 8) & 0xf8) * 255 / 0xf8 * 2) / 3);
            rgba[GCOMP] = UBYTE_TO_CHAN( (((color0 >> 3) & 0xfc) * 255 / 0xfc + ((color1 >> 3) & 0xfc) * 255 / 0xfc * 2) / 3);
            rgba[BCOMP] = UBYTE_TO_CHAN( (((color0 << 3) & 0xf8) * 255 / 0xf8 + ((color1 << 3) & 0xf8) * 255 / 0xf8 * 2) / 3);
            rgba[ACOMP] = CHAN_MAX;
         }
         else {
            rgba[RCOMP] = 0;
            rgba[GCOMP] = 0;
            rgba[BCOMP] = 0;
            rgba[ACOMP] = CHAN_MAX;
            if (dxt_type == 1) rgba[ACOMP] = UBYTE_TO_CHAN(0);
         }
         break;
      default:
      /* CANNOT happen (I hope) */
         break;
   }
}



void fetch_2d_texel_rgb_dxt1(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel)
{
   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * (8));
   dxt135_decode_imageblock(blksrc, (i&3), (j&3), 0, texel);
}


void fetch_2d_texel_rgba_dxt1(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel)
{
   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * (8));
   dxt135_decode_imageblock(blksrc, (i&3), (j&3), 1, texel);
}

void fetch_2d_texel_rgba_dxt3(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel) {

   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   GLchan *rgba = (GLchan *) texel;
   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * (16));
   dxt135_decode_imageblock(blksrc + 8, (i&3), (j&3), 2, texel);
   {
      GLubyte bit_pos = 4 * ((j&3) * 4 + (i&3));
      /* Simple 32bit version. */
      GLuint alpha_low = *blksrc | (*++blksrc << 8) | (*++blksrc << 16) | (*++blksrc << 24);
      GLuint alpha_high = *++blksrc | (*++blksrc << 8) | (*++blksrc << 16) | (*++blksrc << 24);
/*      GLuint alpha_low = *((GLuint *)blksrc)++;
      GLuint alpha_high = *(GLuint *)blksrc;*/
      if (bit_pos < 32)
         rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte)((((alpha_low >> bit_pos) & 15) * 255) / 15));
      else
         rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte)((((alpha_high >> (bit_pos - 32)) & 15) * 255) / 15));
   }
}

void fetch_2d_texel_rgba_dxt5(GLint srcRowStride, const GLubyte *pixdata,
                         GLint i, GLint j, GLvoid *texel) {

   /* Extract the (i,j) pixel from pixdata and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */

   GLchan *rgba = (GLchan *) texel;
   const GLubyte *blksrc = (pixdata + ((srcRowStride + 3) / 4 * (j / 4) + (i / 4)) * (16));
   dxt135_decode_imageblock(blksrc + 8, (i&3), (j&3), 2, texel);
   {
      GLubyte code;
      GLushort alpha0 = *blksrc++;
      GLushort alpha1 = *blksrc++;
      GLubyte bit_pos = 3 * ((j&3) * 4 + (i&3));
      /* simple 32bit version */
      GLuint bits_low = *blksrc | (*++blksrc << 8) | (*++blksrc << 16) | (*++blksrc << 24);
      GLuint bits_high = *++blksrc | (*++blksrc << 8);
/*      GLuint bits_low = *((GLuint *)blksrc)++;
      GLuint bits_high = *(GLuint *)blksrc;*/
      if (bit_pos < 30)
         code = (GLubyte) ((bits_low >> bit_pos) & 7);
      else if (bit_pos == 30)
         code = (GLubyte) ((bits_low >> 30) & 3) | ((bits_high << 2) & 4);
      else
         code = (GLubyte) ((bits_high >> (bit_pos - 32)) & 7);
      if (alpha0 > alpha1)
         switch (code) {
            case 0:
               rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte) alpha0);
               break;
            case 1:
               rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte) alpha1);
               break;
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
               rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte)
                  ((alpha0 * (8 - code) + (alpha1 * (code - 1))) / 7));
               break;
         }
      else
         switch (code) {
            case 0:
               rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte) alpha0);
               break;
            case 1:
               rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte) alpha1);
               break;
            case 2:
            case 3:
            case 4:
            case 5:
               rgba[ACOMP] = UBYTE_TO_CHAN((GLubyte)
                  ((alpha0 * (6 - code) + (alpha1 * (code - 1))) / 5));;
               break;
            case 6:
               rgba[ACOMP] = 0;
               break;
            case 7:
               rgba[ACOMP] = CHAN_MAX;
               break;
         }
   }
}
