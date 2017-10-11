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

#include "h264dec.h"

#undef MCFUNC

#if   CHROMA_IDC == 1
#   define MCFUNC(n) FUNC(n ## _420)
#elif CHROMA_IDC == 2
#   define MCFUNC(n) FUNC(n ## _422)
#elif CHROMA_IDC == 3
#   define MCFUNC(n) FUNC(n ## _444)
#endif

#undef  mc_part
#define mc_part MCFUNC(mc_part)
//mc_part()用于判断已经分块后的子宏块是否使用了加权预测。
/*
从源代码可以看出，mc_part()逻辑非常简单，基本上原封不动的把函数参数传递给了它调用的函数：
判断H.264码流是否使用了加权预测，如果使用了的话，就调用加权预测的函数mc_part_weighted()，
否则就使用标准的函数mc_part_std()
*/
static void mc_part(const H264Context *h, H264SliceContext *sl,
                    int n, int square,
                    int height, int delta,
                    uint8_t *dest_y, uint8_t *dest_cb,
                    uint8_t *dest_cr,
                    int x_offset, int y_offset,
                    const qpel_mc_func *qpix_put,
                    h264_chroma_mc_func chroma_put,
                    const qpel_mc_func *qpix_avg,
                    h264_chroma_mc_func chroma_avg,
                    const h264_weight_func *weight_op,
                    const h264_biweight_func *weight_avg,
                    int list0, int list1)
{
    //是否使用加权预测？  
    if ((sl->pwt.use_weight == 2 && list0 && list1 &&
         (sl->pwt.implicit_weight[sl->ref_cache[0][scan8[n]]][sl->ref_cache[1][scan8[n]]][sl->mb_y & 1] != 32)) ||
        sl->pwt.use_weight == 1)
        mc_part_weighted(h, sl, n, square, height, delta, dest_y, dest_cb, dest_cr,
                         x_offset, y_offset, qpix_put, chroma_put,
                         weight_op[0], weight_op[1], weight_avg[0],
                         weight_avg[1], list0, list1, PIXEL_SHIFT, CHROMA_IDC);
    else
        mc_part_std(h, sl, n, square, height, delta, dest_y, dest_cb, dest_cr,
                    x_offset, y_offset, qpix_put, chroma_put, qpix_avg,
                    chroma_avg, list0, list1, PIXEL_SHIFT, CHROMA_IDC);
}

//运动补偿  
//“*_put”处理单向预测，“*_avg”处理双向预测，“weight”处理加权预测

/*
从源代码可以看出，MCFUNC(hl_motion)根据子宏块的划分类型的不同，传递不同的参数调用mc_part()函数。
（1）如果子宏块划分为16x16（等同于没有划分），直接调用mc_part()并且传递如下参数：
	a)单向预测汇编函数集：qpix_put[0] （qpix_put[0]中的函数进行16x16块的四分之一像素运动补偿）。
	b)双向预测汇编函数集：qpix_avg[0]。
	c)square设置为1，delta设置为0。
	d)x_offset和y_offset都设置为0。
（2）如果子宏块划分为16x8，分两次调用mc_part()并且传递如下参数：
	a)单向预测汇编函数集：qpix_put[1] （qpix_put[1]中的函数进行8x8块的四分之一像素运动补偿）。
	b)双向预测汇编函数集：qpix_avg[1]。
	c)square设置为0，delta设置为8。
	其中第1次调用mc_part()的时候x_offset和y_offset都设置为0，第2次调用mc_part()的时候x_offset设置为0，y_offset设置为4。
（3）如果子宏块划分为8x16，分两次调用mc_part()并且传递如下参数：
	a)单向预测汇编函数集：qpix_put[1] （qpix_put[1]中的函数进行8x8块的四分之一像素运动补偿）。
	b)双向预测汇编函数集：qpix_avg[1]。
	c)square设置为0，delta设置为8 * h->mb_linesize。
	其中第1次调用mc_part()的时候x_offset和y_offset都设置为0，第2次调用mc_part()的时候x_offset设置为4，y_offset设置为0。
（4）如果子宏块划分为8x8，说明此时每个8x8子宏块还可以继续划分为8x8，8x8，4x8，4x4几种类型，
此时根据上述的规则，分成4次分别对这些小块做类似的处理。
*/

static void MCFUNC(hl_motion)(const H264Context *h, H264SliceContext *sl,
                              uint8_t *dest_y,
                              uint8_t *dest_cb, uint8_t *dest_cr,
                              const qpel_mc_func(*qpix_put)[16],
                              const h264_chroma_mc_func(*chroma_put),
                              const qpel_mc_func(*qpix_avg)[16],
                              const h264_chroma_mc_func(*chroma_avg),
                              const h264_weight_func *weight_op,
                              const h264_biweight_func *weight_avg)
{
    const int mb_xy   = sl->mb_xy;
    const int mb_type = h->cur_pic.mb_type[mb_xy];

    av_assert2(IS_INTER(mb_type));

    if (HAVE_THREADS && (h->avctx->active_thread_type & FF_THREAD_FRAME))
        await_references(h, sl);
    prefetch_motion(h, sl, 0, PIXEL_SHIFT, CHROMA_IDC);

    if (IS_16X16(mb_type)) {
		/* 
         * 16x16 宏块 
         * 
         * +--------+--------+ 
         * |                 | 
         * |                 | 
         * |                 | 
         * +        +        + 
         * |                 | 
         * |                 | 
         * |                 | 
         * +--------+--------+ 
         * 
         */  
        //第3个参数square标志了该块是否为方形  
        //第5个参数delta用于配合square，运动补偿必须以“方形”为单位处理。  
        //当宏块不是“方形”的时候，需要进行2次运动补偿，这时候需要知道第二个方形与起始点dest_y之间的偏移值  
        //几种运动补偿函数：适用于不同大小的方块：  
        //qpix_put[0],qpix_avg[0]一次处理16x16个像素  
        //qpix_put[1],qpix_avg[1]一次处理8x8个像素  
        //qpix_put[2],qpix_avg[2]一次处理4x4个像素  
        //16x16块使用qpix_put[0],qpix_avg[0]  
        //  
        //IS_DIR()通过宏块类型判断本宏块是否使用list0和list1（使用list1的话需要进行双向预测）  
        //  
        mc_part(h, sl, 0, 1, 16, 0, dest_y, dest_cb, dest_cr, 0, 0,
                qpix_put[0], chroma_put[0], qpix_avg[0], chroma_avg[0],
                weight_op, weight_avg,
                IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1));
    } else if (IS_16X8(mb_type)) {
    	/* 
         * 16x8 宏块划分 
         * 
         * +--------+--------+ 
         * |        |        | 
         * |        |        | 
         * |        |        | 
         * +--------+--------+ 
         * 
         */  
        //第2个参数n用于h->mv_cache[list][scan8[n]]中的“n”，该值决定了运动补偿过程中使用哪一个MV  
        /* 
         * mv_cache如下所示 
         * 图中数字为scan8[n]中的n 
         *   | 
         * --+-------------------- 
         *   | x x x x  x  x  x  x 
         *   | x x x x  0  1  4  5 
         *   | x x x x  2  3  6  7 
         *   | x x x x  8  9 12 13 
         *   | x x x x 10 11 14 15 
         */  
        //  
        //dest_cr后面第1个参数x_offset代表了子宏块x偏移值  
        //dest_cr后面第2个参数y_offset代表了子宏块y偏移值（为什么是4而不是8？以YUV420P中的色度为基本单位？）  
  
        //总而言之，x_offset，y_offset决定了子宏块的位置（左上角像素点位置）  
        //而square，delta，和qpix_put[x]中的“x”决定的子宏块的大小（相当于确定了子宏块右下角像素的位置）  
        //上面几个值联合决定了子宏块位置和大小信息  
  
        //上16x8  
        //已经分割为子宏块的运动补偿  
        mc_part(h, sl, 0, 0, 8, 8 << PIXEL_SHIFT, dest_y, dest_cb, dest_cr, 0, 0,
                qpix_put[1], chroma_put[0], qpix_avg[1], chroma_avg[0],
                weight_op, weight_avg,
                IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1));
		//下16x8  
		//已经分割为子宏块的运动补偿
        mc_part(h, sl, 8, 0, 8, 8 << PIXEL_SHIFT, dest_y, dest_cb, dest_cr, 0, 4,
                qpix_put[1], chroma_put[0], qpix_avg[1], chroma_avg[0],
                weight_op, weight_avg,
                IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1));
    } else if (IS_8X16(mb_type)) {
		/* 
         * 8x16 宏块划分 
         * 
         * +--------+ 
         * |        | 
         * |        | 
         * |        | 
         * +--------+ 
         * |        | 
         * |        | 
         * |        | 
         * +--------+ 
         * 
         */  
        //左8x16  
        mc_part(h, sl, 0, 0, 16, 8 * sl->mb_linesize, dest_y, dest_cb, dest_cr, 0, 0,
                qpix_put[1], chroma_put[1], qpix_avg[1], chroma_avg[1],
                &weight_op[1], &weight_avg[1],
                IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1));
		
        //右8x16  
        mc_part(h, sl, 4, 0, 16, 8 * sl->mb_linesize, dest_y, dest_cb, dest_cr, 4, 0,
                qpix_put[1], chroma_put[1], qpix_avg[1], chroma_avg[1],
                &weight_op[1], &weight_avg[1],
                IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1));
    } else {
        /* 
         * 16x16 宏块被划分为4个8x8子块 
         * 
         * +--------+--------+ 
         * |        |        | 
         * |   0    |   1    | 
         * |        |        | 
         * +--------+--------+ 
         * |        |        | 
         * |   2    |   3    | 
         * |        |        | 
         * +--------+--------+ 
         * 
         */ 
        int i;

        av_assert2(IS_8X8(mb_type));
        //循环处理4个8x8宏块  
        for (i = 0; i < 4; i++) {
            const int sub_mb_type = sl->sub_mb_type[i];
            const int n  = 4 * i;
            int x_offset = (i & 1) << 2;
            int y_offset = (i & 2) << 1;
            //每个8x8的块可以再次划分为：8x8，8x4，4x8，4x4  
            if (IS_SUB_8X8(sub_mb_type)) {
			   /* 
                 * 8x8（等同于没划分） 
                 * +----+----+ 
                 * |         | 
                 * +    +    + 
                 * |         | 
                 * +----+----+ 
                 * 
                 */  
                //“qpix_put[1]”说明运动补偿的时候一次处理8x8个像素  
                mc_part(h, sl, n, 1, 8, 0, dest_y, dest_cb, dest_cr,
                        x_offset, y_offset,
                        qpix_put[1], chroma_put[1], qpix_avg[1], chroma_avg[1],
                        &weight_op[1], &weight_avg[1],
                        IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1));
            } else if (IS_SUB_8X4(sub_mb_type)) {
                /* 
                 * 8x4 
                 * +----+----+ 
                 * |         | 
                 * +----+----+ 
                 * |         | 
                 * +----+----+ 
                 * 
                 */  
                //“qpix_put[2]”说明运动补偿的时候一次处理4x4个像素  
                mc_part(h, sl, n, 0, 4, 4 << PIXEL_SHIFT, dest_y, dest_cb, dest_cr,
                        x_offset, y_offset,
                        qpix_put[2], chroma_put[1], qpix_avg[2], chroma_avg[1],
                        &weight_op[1], &weight_avg[1],
                        IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1));
                mc_part(h, sl, n + 2, 0, 4, 4 << PIXEL_SHIFT,
                        dest_y, dest_cb, dest_cr, x_offset, y_offset + 2,
                        qpix_put[2], chroma_put[1], qpix_avg[2], chroma_avg[1],
                        &weight_op[1], &weight_avg[1],
                        IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1));
            } else if (IS_SUB_4X8(sub_mb_type)) {
               /* 
                 * 4x8 
                 * +----+----+ 
                 * |    |    | 
                 * +    +    + 
                 * |    |    | 
                 * +----+----+ 
                 * 
                 */  
                mc_part(h, sl, n, 0, 8, 4 * sl->mb_linesize,
                        dest_y, dest_cb, dest_cr, x_offset, y_offset,
                        qpix_put[2], chroma_put[2], qpix_avg[2], chroma_avg[2],
                        &weight_op[2], &weight_avg[2],
                        IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1));
                mc_part(h, sl, n + 1, 0, 8, 4 * sl->mb_linesize,
                        dest_y, dest_cb, dest_cr, x_offset + 2, y_offset,
                        qpix_put[2], chroma_put[2], qpix_avg[2], chroma_avg[2],
                        &weight_op[2], &weight_avg[2],
                        IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1));
            } else {
                /* 
                 * 4x4 
                 * +----+----+ 
                 * |    |    | 
                 * +----+----+ 
                 * |    |    | 
                 * +----+----+ 
                 * 
                 */ 

				int j;
                av_assert2(IS_SUB_4X4(sub_mb_type));
                for (j = 0; j < 4; j++) {
                    int sub_x_offset = x_offset + 2 * (j & 1);
                    int sub_y_offset = y_offset + (j & 2);
                    mc_part(h, sl, n + j, 1, 4, 0,
                            dest_y, dest_cb, dest_cr, sub_x_offset, sub_y_offset,
                            qpix_put[2], chroma_put[2], qpix_avg[2], chroma_avg[2],
                            &weight_op[2], &weight_avg[2],
                            IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1));
                }
            }
        }
    }

    if (USES_LIST(mb_type, 1))
        prefetch_motion(h, sl, 1, PIXEL_SHIFT, CHROMA_IDC);
}

