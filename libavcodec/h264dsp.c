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

/**
 * @file
 * H.264 / AVC / MPEG-4 part10 DSP functions.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include <stdint.h>

#include "libavutil/attributes.h"
#include "libavutil/avassert.h"

#include "avcodec.h"
#include "h264dsp.h"
#include "h264idct.h"
#include "startcode.h"
#include "libavutil/common.h"

#define BIT_DEPTH 8
#include "h264dsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 9
#include "h264dsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 10
#include "h264dsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 12
#include "h264dsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 14
#include "h264dsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 8
#include "h264addpx_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 16
#include "h264addpx_template.c"
#undef BIT_DEPTH

/*
DCT���任��ຯ���ĳ�ʼ��
FFmpeg H.264��������4x4DCT���任��Ҳ��Ϊ��IDCT������ຯ��ָ��λ��H264DSPContext�С�
��FFmpeg H.264��������ʼ����ʱ�򣬻����ff_h264dsp_init()����ϵͳ�����ö�H264DSPContext�е���ЩIDCT����ָ����и�ֵ
��H264DSPContext��ʵ���ϲ���������DCT���任��������������Hadamard���任��������·�˲������������ﲻ��ϸ���ۣ���
����򵥿�һ��ff_h264_pred_init()�Ķ��塣
*/

/*
��ff_h264dsp_init()�Ķ�����Կ������ú���ͨ�����á�H264_DSP(depth)�������C���԰汾��DCT���任������Hadamard���任������
��·�˲������ĳ�ʼ�����ں�����ĩβ�����ж�ϵͳ�����ԣ��������Ļ����ʼ��Ч�ʸ��ߵľ�������Ż��ĺ�����
��������չ����H264_DSP(8)���꿴��C���԰汾�����ĳ�ʼ�����̡�

	c->h264_idct_add= ff_h264_idct_add_8_c;  
    c->h264_idct8_add= ff_h264_idct8_add_8_c;  
    c->h264_idct_dc_add= ff_h264_idct_dc_add_8_c;  
    c->h264_idct8_dc_add= ff_h264_idct8_dc_add_8_c;  
    c->h264_idct_add16     = ff_h264_idct_add16_8_c;  
    c->h264_idct8_add4     = ff_h264_idct8_add4_8_c;  
    if (chroma_format_idc <= 1)  
        c->h264_idct_add8  = ff_h264_idct_add8_8_c;  
    else  
        c->h264_idct_add8  = ff_h264_idct_add8_422_8_c;  
    c->h264_idct_add16intra= ff_h264_idct_add16intra_8_c;  
    c->h264_luma_dc_dequant_idct= ff_h264_luma_dc_dequant_idct_8_c;  
    if (chroma_format_idc <= 1)  
        c->h264_chroma_dc_dequant_idct= ff_h264_chroma_dc_dequant_idct_8_c;  
    else  
        c->h264_chroma_dc_dequant_idct= ff_h264_chroma422_dc_dequant_idct_8_c;  
  
    c->weight_h264_pixels_tab[0]= weight_h264_pixels16_8_c;  
    c->weight_h264_pixels_tab[1]= weight_h264_pixels8_8_c;  
    c->weight_h264_pixels_tab[2]= weight_h264_pixels4_8_c;  
    c->weight_h264_pixels_tab[3]= weight_h264_pixels2_8_c;  
    c->biweight_h264_pixels_tab[0]= biweight_h264_pixels16_8_c;  
    c->biweight_h264_pixels_tab[1]= biweight_h264_pixels8_8_c;  
    c->biweight_h264_pixels_tab[2]= biweight_h264_pixels4_8_c;  
    c->biweight_h264_pixels_tab[3]= biweight_h264_pixels2_8_c;  
  
    c->h264_v_loop_filter_luma= h264_v_loop_filter_luma_8_c;  
    c->h264_h_loop_filter_luma= h264_h_loop_filter_luma_8_c;  
    c->h264_h_loop_filter_luma_mbaff= h264_h_loop_filter_luma_mbaff_8_c;  
    c->h264_v_loop_filter_luma_intra= h264_v_loop_filter_luma_intra_8_c;  
    c->h264_h_loop_filter_luma_intra= h264_h_loop_filter_luma_intra_8_c;  
    c->h264_h_loop_filter_luma_mbaff_intra= h264_h_loop_filter_luma_mbaff_intra_8_c;  
    c->h264_v_loop_filter_chroma= h264_v_loop_filter_chroma_8_c;  
    if (chroma_format_idc <= 1)  
        c->h264_h_loop_filter_chroma= h264_h_loop_filter_chroma_8_c;  
    else  
        c->h264_h_loop_filter_chroma= h264_h_loop_filter_chroma422_8_c;  
    if (chroma_format_idc <= 1)  
        c->h264_h_loop_filter_chroma_mbaff= h264_h_loop_filter_chroma_mbaff_8_c;  
    else  
        c->h264_h_loop_filter_chroma_mbaff= h264_h_loop_filter_chroma422_mbaff_8_c;  
    c->h264_v_loop_filter_chroma_intra= h264_v_loop_filter_chroma_intra_8_c;  
    if (chroma_format_idc <= 1)  
        c->h264_h_loop_filter_chroma_intra= h264_h_loop_filter_chroma_intra_8_c;  
    else  
        c->h264_h_loop_filter_chroma_intra= h264_h_loop_filter_chroma422_intra_8_c;  
    if (chroma_format_idc <= 1)  
        c->h264_h_loop_filter_chroma_mbaff_intra= h264_h_loop_filter_chroma_mbaff_intra_8_c;  
    else  
        c->h264_h_loop_filter_chroma_mbaff_intra= h264_h_loop_filter_chroma422_mbaff_intra_8_c;  
    c->h264_loop_filter_strength= ((void *)0);  


�ӡ�H264_DSP(8)����չ���Ľ�����Կ�����
��1��4x4���DCT���任����ָ��h264_idct_add()ָ��ff_h264_idct_add_8_c()
��2��ֻ����DCϵ����4x4���DCT���任����ָ��h264_idct_dc_add()ָ��ff_h264_idct_dc_add_8_c()
��3��16x16���DCT���任����ָ��h264_idct_add16()ָ��ff_h264_idct_add16_8_c()
��4��16x16��Intra���DCT���任����ָ��h264_idct_add16intra()ָ��ff_h264_idct_add16intra_8_c()
���Ľ���򵥷�����������������

*/

//��ʼ��DSP��صĺ�����������IDCT����·�˲������ȡ�
av_cold void ff_h264dsp_init(H264DSPContext *c, const int bit_depth,
                             const int chroma_format_idc)
{
#undef FUNC
#define FUNC(a, depth) a ## _ ## depth ## _c

#define ADDPX_DSP(depth) \
    c->h264_add_pixels4_clear = FUNC(ff_h264_add_pixels4, depth);\
    c->h264_add_pixels8_clear = FUNC(ff_h264_add_pixels8, depth)

    if (bit_depth > 8 && bit_depth <= 16) {
        ADDPX_DSP(16);
    } else {
        ADDPX_DSP(8);
    }

#define H264_DSP(depth) \
    c->h264_idct_add= FUNC(ff_h264_idct_add, depth);\
    c->h264_idct8_add= FUNC(ff_h264_idct8_add, depth);\
    c->h264_idct_dc_add= FUNC(ff_h264_idct_dc_add, depth);\
    c->h264_idct8_dc_add= FUNC(ff_h264_idct8_dc_add, depth);\
    c->h264_idct_add16     = FUNC(ff_h264_idct_add16, depth);\
    c->h264_idct8_add4     = FUNC(ff_h264_idct8_add4, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_idct_add8  = FUNC(ff_h264_idct_add8, depth);\
    else\
        c->h264_idct_add8  = FUNC(ff_h264_idct_add8_422, depth);\
    c->h264_idct_add16intra= FUNC(ff_h264_idct_add16intra, depth);\
    c->h264_luma_dc_dequant_idct= FUNC(ff_h264_luma_dc_dequant_idct, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_chroma_dc_dequant_idct= FUNC(ff_h264_chroma_dc_dequant_idct, depth);\
    else\
        c->h264_chroma_dc_dequant_idct= FUNC(ff_h264_chroma422_dc_dequant_idct, depth);\
\
    c->weight_h264_pixels_tab[0]= FUNC(weight_h264_pixels16, depth);\
    c->weight_h264_pixels_tab[1]= FUNC(weight_h264_pixels8, depth);\
    c->weight_h264_pixels_tab[2]= FUNC(weight_h264_pixels4, depth);\
    c->weight_h264_pixels_tab[3]= FUNC(weight_h264_pixels2, depth);\
    c->biweight_h264_pixels_tab[0]= FUNC(biweight_h264_pixels16, depth);\
    c->biweight_h264_pixels_tab[1]= FUNC(biweight_h264_pixels8, depth);\
    c->biweight_h264_pixels_tab[2]= FUNC(biweight_h264_pixels4, depth);\
    c->biweight_h264_pixels_tab[3]= FUNC(biweight_h264_pixels2, depth);\
\
    c->h264_v_loop_filter_luma= FUNC(h264_v_loop_filter_luma, depth);\
    c->h264_h_loop_filter_luma= FUNC(h264_h_loop_filter_luma, depth);\
    c->h264_h_loop_filter_luma_mbaff= FUNC(h264_h_loop_filter_luma_mbaff, depth);\
    c->h264_v_loop_filter_luma_intra= FUNC(h264_v_loop_filter_luma_intra, depth);\
    c->h264_h_loop_filter_luma_intra= FUNC(h264_h_loop_filter_luma_intra, depth);\
    c->h264_h_loop_filter_luma_mbaff_intra= FUNC(h264_h_loop_filter_luma_mbaff_intra, depth);\
    c->h264_v_loop_filter_chroma= FUNC(h264_v_loop_filter_chroma, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma= FUNC(h264_h_loop_filter_chroma, depth);\
    else\
        c->h264_h_loop_filter_chroma= FUNC(h264_h_loop_filter_chroma422, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma_mbaff= FUNC(h264_h_loop_filter_chroma_mbaff, depth);\
    else\
        c->h264_h_loop_filter_chroma_mbaff= FUNC(h264_h_loop_filter_chroma422_mbaff, depth);\
    c->h264_v_loop_filter_chroma_intra= FUNC(h264_v_loop_filter_chroma_intra, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma_intra= FUNC(h264_h_loop_filter_chroma_intra, depth);\
    else\
        c->h264_h_loop_filter_chroma_intra= FUNC(h264_h_loop_filter_chroma422_intra, depth);\
    if (chroma_format_idc <= 1)\
        c->h264_h_loop_filter_chroma_mbaff_intra= FUNC(h264_h_loop_filter_chroma_mbaff_intra, depth);\
    else\
        c->h264_h_loop_filter_chroma_mbaff_intra= FUNC(h264_h_loop_filter_chroma422_mbaff_intra, depth);\
    c->h264_loop_filter_strength= NULL;
    //������ɫλ���ʼ����ͬ�ĺ���  
    //һ��Ϊ8bit����ִ��H264_DSP(8)  
    switch (bit_depth) {
    case 9:
        H264_DSP(9);
        break;
    case 10:
        H264_DSP(10);
        break;
    case 12:
        H264_DSP(12);
        break;
    case 14:
        H264_DSP(14);
        break;
    default:
        av_assert0(bit_depth<=8);
        H264_DSP(8);
        break;
    }
	
    //�����������startcode��ʱ���õ�  
    //�����ﾹȻ�����г�  
    c->startcode_find_candidate = ff_startcode_find_candidate_c;

    //���ϵͳ֧�֣����ʼ����������Ż��ĺ���  
    if (ARCH_AARCH64) ff_h264dsp_init_aarch64(c, bit_depth, chroma_format_idc);
    if (ARCH_ARM) ff_h264dsp_init_arm(c, bit_depth, chroma_format_idc);
    if (ARCH_PPC) ff_h264dsp_init_ppc(c, bit_depth, chroma_format_idc);
    if (ARCH_X86) ff_h264dsp_init_x86(c, bit_depth, chroma_format_idc);
    if (ARCH_MIPS) ff_h264dsp_init_mips(c, bit_depth, chroma_format_idc);
}
