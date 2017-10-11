/*
 * H.26L/H.264/AVC/JVT/14496-10/... decoder
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
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

#undef FUNC
#undef PIXEL_SHIFT

#if SIMPLE
#   define FUNC(n) AV_JOIN(n ## _simple_, BITS)
#   define PIXEL_SHIFT (BITS >> 4)
#else
#   define FUNC(n) n ## _complex
#   define PIXEL_SHIFT h->pixel_shift
#endif

#undef  CHROMA_IDC
#define CHROMA_IDC 1
#include "h264_mc_template.c"

#undef  CHROMA_IDC
#define CHROMA_IDC 2
#include "h264_mc_template.c"


/*
PS：在这里需要注意，FFmpeg H.264解码器中名称中包含“_template”的C语言文件中的函数都是使用类似于“FUNC(name)()”的方式书写的，
这样做的目的大概是为了适配各种各样的功能。
例如
在处理16bit的H.264码流的时候，FUNC(hl_decode_mb)()可以展开为hl_decode_mb_simple_16()函数；
同理，FUNC(hl_decode_mb)()在其他条件下也可以展开为hl_decode_mb_complex()函数。
*/
//hl是什么意思？high level？  
/*
* 宏块解码 
* 帧内宏块：帧内预测->残差DCT反变换 
* 帧间宏块：帧间预测（运动补偿）->残差DCT反变换 
*/


/*
下面简单梳理一下FUNC(hl_decode_mb)的流程（在这里只考虑亮度分量的解码，色度分量的解码过程是类似的）：
（1）预测
	a)如果是帧内预测宏块（Intra），
	  调用hl_decode_mb_predict_luma()进行帧内预测，得到预测数据。
	b)如果不是帧内预测宏块（Inter），
	  调用FUNC(hl_motion_420)()或者FUNC(hl_motion_422)()进行帧间预测（即运动补偿），得到预测数据。
（2）残差叠加
	a)调用hl_decode_mb_idct_luma()对DCT残差数据进行DCT反变换，
	  获得残差像素数据并且叠加到之前得到的预测数据上，得到最后的图像数据。
PS：该流程中有一个重要的贯穿始终的内存指针dest_y，其指向的内存中存储了解码后的亮度数据。
*/
static av_noinline void FUNC(hl_decode_mb)(const H264Context *h, H264SliceContext *sl)
{
    //序号：x（行）和y（列）  
    const int mb_x    = sl->mb_x;
    const int mb_y    = sl->mb_y;
    //宏块序号 mb_xy = mb_x + mb_y*mb_stride  
    const int mb_xy   = sl->mb_xy;
    //宏块类型  
    const int mb_type = h->cur_pic.mb_type[mb_xy];
	//这三个变量存储最后处理完成的像素值  
    uint8_t *dest_y, *dest_cb, *dest_cr;
    int linesize, uvlinesize /*dct_offset*/;
    int i, j;
    const int *block_offset = &h->block_offset[0];
    const int transform_bypass = !SIMPLE && (sl->qscale == 0 && h->ps.sps->transform_bypass);
    void (*idct_add)(uint8_t *dst, int16_t *block, int stride);
    const int block_h   = 16 >> h->chroma_y_shift;
    const int chroma422 = CHROMA422(h);
    //存储Y，U，V像素的位置：dest_y，dest_cb，dest_cr  
    //分别对应AVFrame的data[0]，data[1]，data[2]  
    dest_y  = h->cur_pic.f->data[0] + ((mb_x << PIXEL_SHIFT)     + mb_y * sl->linesize)  * 16;
    dest_cb = h->cur_pic.f->data[1] +  (mb_x << PIXEL_SHIFT) * 8 + mb_y * sl->uvlinesize * block_h;
    dest_cr = h->cur_pic.f->data[2] +  (mb_x << PIXEL_SHIFT) * 8 + mb_y * sl->uvlinesize * block_h;

    h->vdsp.prefetch(dest_y  + (sl->mb_x & 3) * 4 * sl->linesize   + (64 << PIXEL_SHIFT), sl->linesize,       4);
    h->vdsp.prefetch(dest_cb + (sl->mb_x & 7)     * sl->uvlinesize + (64 << PIXEL_SHIFT), dest_cr - dest_cb, 2);

    h->list_counts[mb_xy] = sl->list_count;

    //系统中包含了  
    //#define SIMPLE 1  
    //不会执行？  
    if (!SIMPLE && MB_FIELD(sl)) {
        linesize     = sl->mb_linesize = sl->linesize * 2;
        uvlinesize   = sl->mb_uvlinesize = sl->uvlinesize * 2;
        block_offset = &h->block_offset[48];
        if (mb_y & 1) { // FIXME move out of this function?
            dest_y  -= sl->linesize * 15;
            dest_cb -= sl->uvlinesize * (block_h - 1);
            dest_cr -= sl->uvlinesize * (block_h - 1);
        }
        if (FRAME_MBAFF(h)) {
            int list;
            for (list = 0; list < sl->list_count; list++) {
                if (!USES_LIST(mb_type, list))
                    continue;
                if (IS_16X16(mb_type)) {
                    int8_t *ref = &sl->ref_cache[list][scan8[0]];
                    fill_rectangle(ref, 4, 4, 8, (16 + *ref) ^ (sl->mb_y & 1), 1);
                } else {
                    for (i = 0; i < 16; i += 4) {
                        int ref = sl->ref_cache[list][scan8[i]];
                        if (ref >= 0)
                            fill_rectangle(&sl->ref_cache[list][scan8[i]], 2, 2,
                                           8, (16 + ref) ^ (sl->mb_y & 1), 1);
                    }
                }
            }
        }
    } else {
        linesize   = sl->mb_linesize   = sl->linesize;
        uvlinesize = sl->mb_uvlinesize = sl->uvlinesize;
        // dct_offset = s->linesize * 16;
    }
	
    //系统中包含了  
    //#define SIMPLE 1  
    //不会执行？ 
	if (!SIMPLE && IS_INTRA_PCM(mb_type)) {
        const int bit_depth = h->ps.sps->bit_depth_luma;
        if (PIXEL_SHIFT) {
            int j;
            GetBitContext gb;
            init_get_bits(&gb, sl->intra_pcm_ptr,
                          ff_h264_mb_sizes[h->ps.sps->chroma_format_idc] * bit_depth);

            for (i = 0; i < 16; i++) {
                uint16_t *tmp_y = (uint16_t *)(dest_y + i * linesize);
                for (j = 0; j < 16; j++)
                    tmp_y[j] = get_bits(&gb, bit_depth);
            }
            if (SIMPLE || !CONFIG_GRAY || !(h->flags & AV_CODEC_FLAG_GRAY)) {
                if (!h->ps.sps->chroma_format_idc) {
                    for (i = 0; i < block_h; i++) {
                        uint16_t *tmp_cb = (uint16_t *)(dest_cb + i * uvlinesize);
                        uint16_t *tmp_cr = (uint16_t *)(dest_cr + i * uvlinesize);
                        for (j = 0; j < 8; j++) {
                            tmp_cb[j] = tmp_cr[j] = 1 << (bit_depth - 1);
                        }
                    }
                } else {
                    for (i = 0; i < block_h; i++) {
                        uint16_t *tmp_cb = (uint16_t *)(dest_cb + i * uvlinesize);
                        for (j = 0; j < 8; j++)
                            tmp_cb[j] = get_bits(&gb, bit_depth);
                    }
                    for (i = 0; i < block_h; i++) {
                        uint16_t *tmp_cr = (uint16_t *)(dest_cr + i * uvlinesize);
                        for (j = 0; j < 8; j++)
                            tmp_cr[j] = get_bits(&gb, bit_depth);
                    }
                }
            }
        } else {
            for (i = 0; i < 16; i++)
                memcpy(dest_y + i * linesize, sl->intra_pcm_ptr + i * 16, 16);
            if (SIMPLE || !CONFIG_GRAY || !(h->flags & AV_CODEC_FLAG_GRAY)) {
                if (!h->ps.sps->chroma_format_idc) {
                    for (i = 0; i < 8; i++) {
                        memset(dest_cb + i * uvlinesize, 1 << (bit_depth - 1), 8);
                        memset(dest_cr + i * uvlinesize, 1 << (bit_depth - 1), 8);
                    }
                } else {
                    const uint8_t *src_cb = sl->intra_pcm_ptr + 256;
                    const uint8_t *src_cr = sl->intra_pcm_ptr + 256 + block_h * 8;
                    for (i = 0; i < block_h; i++) {
                        memcpy(dest_cb + i * uvlinesize, src_cb + i * 8, 8);
                        memcpy(dest_cr + i * uvlinesize, src_cr + i * 8, 8);
                    }
                }
            }
        }
    } else {
        //Intra类型  
        //Intra4x4或者Intra16x16  
		if (IS_INTRA(mb_type)) {
            if (sl->deblocking_filter)
                xchg_mb_border(h, sl, dest_y, dest_cb, dest_cr, linesize,
                               uvlinesize, 1, 0, SIMPLE, PIXEL_SHIFT);

            if (SIMPLE || !CONFIG_GRAY || !(h->flags & AV_CODEC_FLAG_GRAY)) {
                h->hpc.pred8x8[sl->chroma_pred_mode](dest_cb, uvlinesize);
                h->hpc.pred8x8[sl->chroma_pred_mode](dest_cr, uvlinesize);
            }
            //帧内预测-亮度  
            hl_decode_mb_predict_luma(h, sl, mb_type, SIMPLE,
                                      transform_bypass, PIXEL_SHIFT,
                                      block_offset, linesize, dest_y, 0);

            if (sl->deblocking_filter)
                xchg_mb_border(h, sl, dest_y, dest_cb, dest_cr, linesize,
                               uvlinesize, 0, 0, SIMPLE, PIXEL_SHIFT);
        } else {
			//Inter类型  
			//运动补偿	
            if (chroma422) {
                FUNC(hl_motion_422)(h, sl, dest_y, dest_cb, dest_cr,
                              h->h264qpel.put_h264_qpel_pixels_tab,
                              h->h264chroma.put_h264_chroma_pixels_tab,
                              h->h264qpel.avg_h264_qpel_pixels_tab,
                              h->h264chroma.avg_h264_chroma_pixels_tab,
                              h->h264dsp.weight_h264_pixels_tab,
                              h->h264dsp.biweight_h264_pixels_tab);
            } else {
                //“*_put”处理单向预测，“*_avg”处理双向预测，“weight”处理加权预测  
                //h->qpel_put[16]包含了单向预测的四分之一像素运动补偿所有样点处理的函数  
                //两个像素之间横向的点（内插点和原始的点）有4个，纵向的点有4个，组合起来一共16个  
                //h->qpel_avg[16]情况也类似 

				/*
				FUNC(hl_motion_420)()用于对YUV420P格式的H.264码流进行帧间预测，
				根据运动矢量和参考帧获得帧间预测的结果。
				如果直接查找“FUNC(hl_motion_420)()”的定义是无法找到的，
				该函数的定义实际上就是MCFUNC(hl_motion)的定义。
				*/
                FUNC(hl_motion_420)(h, sl, dest_y, dest_cb, dest_cr,
                              h->h264qpel.put_h264_qpel_pixels_tab,
                              h->h264chroma.put_h264_chroma_pixels_tab,
                              h->h264qpel.avg_h264_qpel_pixels_tab,
                              h->h264chroma.avg_h264_chroma_pixels_tab,
                              h->h264dsp.weight_h264_pixels_tab,
                              h->h264dsp.biweight_h264_pixels_tab);
            }
        }
		
        //亮度的IDCT  
        hl_decode_mb_idct_luma(h, sl, mb_type, SIMPLE, transform_bypass,
                               PIXEL_SHIFT, block_offset, linesize, dest_y, 0);

        //色度的IDCT（没有写在一个单独的函数中）  
        if ((SIMPLE || !CONFIG_GRAY || !(h->flags & AV_CODEC_FLAG_GRAY)) &&
            (sl->cbp & 0x30)) {
            uint8_t *dest[2] = { dest_cb, dest_cr };
            //transform_bypass=0，不考虑  
            if (transform_bypass) {
                if (IS_INTRA(mb_type) && h->ps.sps->profile_idc == 244 &&
                    (sl->chroma_pred_mode == VERT_PRED8x8 ||
                     sl->chroma_pred_mode == HOR_PRED8x8)) {
                    h->hpc.pred8x8_add[sl->chroma_pred_mode](dest[0],
                                                            block_offset + 16,
                                                            sl->mb + (16 * 16 * 1 << PIXEL_SHIFT),
                                                            uvlinesize);
                    h->hpc.pred8x8_add[sl->chroma_pred_mode](dest[1],
                                                            block_offset + 32,
                                                            sl->mb + (16 * 16 * 2 << PIXEL_SHIFT),
                                                            uvlinesize);
                } else {
                    idct_add = h->h264dsp.h264_add_pixels4_clear;
                    for (j = 1; j < 3; j++) {
                        for (i = j * 16; i < j * 16 + 4; i++)
                            if (sl->non_zero_count_cache[scan8[i]] ||
                                dctcoef_get(sl->mb, PIXEL_SHIFT, i * 16))
                                idct_add(dest[j - 1] + block_offset[i],
                                         sl->mb + (i * 16 << PIXEL_SHIFT),
                                         uvlinesize);
                        if (chroma422) {
                            for (i = j * 16 + 4; i < j * 16 + 8; i++)
                                if (sl->non_zero_count_cache[scan8[i + 4]] ||
                                    dctcoef_get(sl->mb, PIXEL_SHIFT, i * 16))
                                    idct_add(dest[j - 1] + block_offset[i + 4],
                                             sl->mb + (i * 16 << PIXEL_SHIFT),
                                             uvlinesize);
                        }
                    }
                }
            } else {
                int qp[2];
                if (chroma422) {
                    qp[0] = sl->chroma_qp[0] + 3;
                    qp[1] = sl->chroma_qp[1] + 3;
                } else {
                    qp[0] = sl->chroma_qp[0];
                    qp[1] = sl->chroma_qp[1];
                }
				//色度的IDCT  
				//直流分量的hadamard变换  
                if (sl->non_zero_count_cache[scan8[CHROMA_DC_BLOCK_INDEX + 0]])
                    h->h264dsp.h264_chroma_dc_dequant_idct(sl->mb + (16 * 16 * 1 << PIXEL_SHIFT),
                                                           h->ps.pps->dequant4_coeff[IS_INTRA(mb_type) ? 1 : 4][qp[0]][0]);
                if (sl->non_zero_count_cache[scan8[CHROMA_DC_BLOCK_INDEX + 1]])
                    h->h264dsp.h264_chroma_dc_dequant_idct(sl->mb + (16 * 16 * 2 << PIXEL_SHIFT),
                                                           h->ps.pps->dequant4_coeff[IS_INTRA(mb_type) ? 2 : 5][qp[1]][0]);
				
				//IDCT	
				//最后的“8”代表内部循环处理8次（U,V各4次）  
				h->h264dsp.h264_idct_add8(dest, block_offset,
                                          sl->mb, uvlinesize,
                                          sl->non_zero_count_cache);
            }
        }
    }
}

#if !SIMPLE || BITS == 8

#undef  CHROMA_IDC
#define CHROMA_IDC 3
#include "h264_mc_template.c"

static av_noinline void FUNC(hl_decode_mb_444)(const H264Context *h, H264SliceContext *sl)
{
    const int mb_x    = sl->mb_x;
    const int mb_y    = sl->mb_y;
    const int mb_xy   = sl->mb_xy;
    const int mb_type = h->cur_pic.mb_type[mb_xy];
    uint8_t *dest[3];
    int linesize;
    int i, j, p;
    const int *block_offset = &h->block_offset[0];
    const int transform_bypass = !SIMPLE && (sl->qscale == 0 && h->ps.sps->transform_bypass);
    const int plane_count      = (SIMPLE || !CONFIG_GRAY || !(h->flags & AV_CODEC_FLAG_GRAY)) ? 3 : 1;

    for (p = 0; p < plane_count; p++) {
        dest[p] = h->cur_pic.f->data[p] +
                  ((mb_x << PIXEL_SHIFT) + mb_y * sl->linesize) * 16;
        h->vdsp.prefetch(dest[p] + (sl->mb_x & 3) * 4 * sl->linesize + (64 << PIXEL_SHIFT),
                         sl->linesize, 4);
    }

    h->list_counts[mb_xy] = sl->list_count;

    if (!SIMPLE && MB_FIELD(sl)) {
        linesize     = sl->mb_linesize = sl->mb_uvlinesize = sl->linesize * 2;
        block_offset = &h->block_offset[48];
        if (mb_y & 1) // FIXME move out of this function?
            for (p = 0; p < 3; p++)
                dest[p] -= sl->linesize * 15;
        if (FRAME_MBAFF(h)) {
            int list;
            for (list = 0; list < sl->list_count; list++) {
                if (!USES_LIST(mb_type, list))
                    continue;
                if (IS_16X16(mb_type)) {
                    int8_t *ref = &sl->ref_cache[list][scan8[0]];
                    fill_rectangle(ref, 4, 4, 8, (16 + *ref) ^ (sl->mb_y & 1), 1);
                } else {
                    for (i = 0; i < 16; i += 4) {
                        int ref = sl->ref_cache[list][scan8[i]];
                        if (ref >= 0)
                            fill_rectangle(&sl->ref_cache[list][scan8[i]], 2, 2,
                                           8, (16 + ref) ^ (sl->mb_y & 1), 1);
                    }
                }
            }
        }
    } else {
        linesize = sl->mb_linesize = sl->mb_uvlinesize = sl->linesize;
    }

    if (!SIMPLE && IS_INTRA_PCM(mb_type)) {
        if (PIXEL_SHIFT) {
            const int bit_depth = h->ps.sps->bit_depth_luma;
            GetBitContext gb;
            init_get_bits(&gb, sl->intra_pcm_ptr, 768 * bit_depth);

            for (p = 0; p < plane_count; p++)
                for (i = 0; i < 16; i++) {
                    uint16_t *tmp = (uint16_t *)(dest[p] + i * linesize);
                    for (j = 0; j < 16; j++)
                        tmp[j] = get_bits(&gb, bit_depth);
                }
        } else {
            for (p = 0; p < plane_count; p++)
                for (i = 0; i < 16; i++)
                    memcpy(dest[p] + i * linesize,
                           sl->intra_pcm_ptr + p * 256 + i * 16, 16);
        }
    } else {
        if (IS_INTRA(mb_type)) {
            if (sl->deblocking_filter)
                xchg_mb_border(h, sl, dest[0], dest[1], dest[2], linesize,
                               linesize, 1, 1, SIMPLE, PIXEL_SHIFT);

            for (p = 0; p < plane_count; p++)
                hl_decode_mb_predict_luma(h, sl, mb_type, SIMPLE,
                                          transform_bypass, PIXEL_SHIFT,
                                          block_offset, linesize, dest[p], p);

            if (sl->deblocking_filter)
                xchg_mb_border(h, sl, dest[0], dest[1], dest[2], linesize,
                               linesize, 0, 1, SIMPLE, PIXEL_SHIFT);
        } else {
            FUNC(hl_motion_444)(h, sl, dest[0], dest[1], dest[2],
                      h->h264qpel.put_h264_qpel_pixels_tab,
                      h->h264chroma.put_h264_chroma_pixels_tab,
                      h->h264qpel.avg_h264_qpel_pixels_tab,
                      h->h264chroma.avg_h264_chroma_pixels_tab,
                      h->h264dsp.weight_h264_pixels_tab,
                      h->h264dsp.biweight_h264_pixels_tab);
        }

        for (p = 0; p < plane_count; p++)
            hl_decode_mb_idct_luma(h, sl, mb_type, SIMPLE, transform_bypass,
                                   PIXEL_SHIFT, block_offset, linesize,
                                   dest[p], p);
    }
}

#endif
