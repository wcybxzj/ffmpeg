/*
 * H.26L/H.264/AVC/JVT/14496-10/... encoder/decoder
 * Copyright (c) 2003-2010 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/attributes.h"
#include "h264qpel.h"

#define pixeltmp int16_t
#define BIT_DEPTH 8
#include "h264qpel_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 9
#include "h264qpel_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 10
#include "h264qpel_template.c"
#undef BIT_DEPTH
#undef pixeltmp

#define pixeltmp int32_t
#define BIT_DEPTH 12
#include "h264qpel_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 14
#include "h264qpel_template.c"
#undef BIT_DEPTH

//��ʼ���ķ�֮һ�����˶�������صĺ�����
/*
ff_h264qpel_init()ͨ��SET_QPEL(8)��ʼ���ķ�֮�����˶�����C���԰汾�ĺ�����
�ں�����ĩβ��ϵͳ��������ã����֧�ֻ���Ż��Ļ���
�������������ff_h264qpel_init_x86()����ĺ�����ʼ����������Ż�֮����ķ�֮һ�����˶������ĺ�����
*/
/*
����չ����SET_QPEL(8)����һ�������������ݡ�

c->put_h264_qpel_pixels_tab[0][ 0] = put_h264_qpel16_mc00_8_c;   
c->put_h264_qpel_pixels_tab[0][ 1] = put_h264_qpel16_mc10_8_c;   
c->put_h264_qpel_pixels_tab[0][ 2] = put_h264_qpel16_mc20_8_c;   
c->put_h264_qpel_pixels_tab[0][ 3] = put_h264_qpel16_mc30_8_c;   
c->put_h264_qpel_pixels_tab[0][ 4] = put_h264_qpel16_mc01_8_c;   
c->put_h264_qpel_pixels_tab[0][ 5] = put_h264_qpel16_mc11_8_c;   
c->put_h264_qpel_pixels_tab[0][ 6] = put_h264_qpel16_mc21_8_c;   
c->put_h264_qpel_pixels_tab[0][ 7] = put_h264_qpel16_mc31_8_c;   
c->put_h264_qpel_pixels_tab[0][ 8] = put_h264_qpel16_mc02_8_c;   
c->put_h264_qpel_pixels_tab[0][ 9] = put_h264_qpel16_mc12_8_c;   
c->put_h264_qpel_pixels_tab[0][10] = put_h264_qpel16_mc22_8_c;   
c->put_h264_qpel_pixels_tab[0][11] = put_h264_qpel16_mc32_8_c;   
ʡ��


*/

av_cold void ff_h264qpel_init(H264QpelContext *c, int bit_depth)
{
#undef FUNCC
#define FUNCC(f, depth) f ## _ ## depth ## _c

//�����ú궨��д�ĺ�����FFmpeg��H.264�������кܳ���  
#define dspfunc2(PFX, IDX, NUM, depth)                                  \
    c->PFX ## _pixels_tab[IDX][ 0] = FUNCC(PFX ## NUM ## _mc00, depth); \
    c->PFX ## _pixels_tab[IDX][ 1] = FUNCC(PFX ## NUM ## _mc10, depth); \
    c->PFX ## _pixels_tab[IDX][ 2] = FUNCC(PFX ## NUM ## _mc20, depth); \
    c->PFX ## _pixels_tab[IDX][ 3] = FUNCC(PFX ## NUM ## _mc30, depth); \
    c->PFX ## _pixels_tab[IDX][ 4] = FUNCC(PFX ## NUM ## _mc01, depth); \
    c->PFX ## _pixels_tab[IDX][ 5] = FUNCC(PFX ## NUM ## _mc11, depth); \
    c->PFX ## _pixels_tab[IDX][ 6] = FUNCC(PFX ## NUM ## _mc21, depth); \
    c->PFX ## _pixels_tab[IDX][ 7] = FUNCC(PFX ## NUM ## _mc31, depth); \
    c->PFX ## _pixels_tab[IDX][ 8] = FUNCC(PFX ## NUM ## _mc02, depth); \
    c->PFX ## _pixels_tab[IDX][ 9] = FUNCC(PFX ## NUM ## _mc12, depth); \
    c->PFX ## _pixels_tab[IDX][10] = FUNCC(PFX ## NUM ## _mc22, depth); \
    c->PFX ## _pixels_tab[IDX][11] = FUNCC(PFX ## NUM ## _mc32, depth); \
    c->PFX ## _pixels_tab[IDX][12] = FUNCC(PFX ## NUM ## _mc03, depth); \
    c->PFX ## _pixels_tab[IDX][13] = FUNCC(PFX ## NUM ## _mc13, depth); \
    c->PFX ## _pixels_tab[IDX][14] = FUNCC(PFX ## NUM ## _mc23, depth); \
    c->PFX ## _pixels_tab[IDX][15] = FUNCC(PFX ## NUM ## _mc33, depth)

#define SET_QPEL(depth)                         \
    dspfunc2(put_h264_qpel, 0, 16, depth);      \
    dspfunc2(put_h264_qpel, 1,  8, depth);      \
    dspfunc2(put_h264_qpel, 2,  4, depth);      \
    dspfunc2(put_h264_qpel, 3,  2, depth);      \
    dspfunc2(avg_h264_qpel, 0, 16, depth);      \
    dspfunc2(avg_h264_qpel, 1,  8, depth);      \
    dspfunc2(avg_h264_qpel, 2,  4, depth)

    switch (bit_depth) {
    default:
        SET_QPEL(8);
        break;
    case 9:
        SET_QPEL(9);
        break;
    case 10:
        SET_QPEL(10);
        break;
    case 12:
        SET_QPEL(12);
        break;
    case 14:
        SET_QPEL(14);
        break;
    }

    if (ARCH_AARCH64)
        ff_h264qpel_init_aarch64(c, bit_depth);
    if (ARCH_ARM)
        ff_h264qpel_init_arm(c, bit_depth);
    if (ARCH_PPC)
        ff_h264qpel_init_ppc(c, bit_depth);
    if (ARCH_X86)
        ff_h264qpel_init_x86(c, bit_depth);
    if (ARCH_MIPS)
        ff_h264qpel_init_mips(c, bit_depth);
}
