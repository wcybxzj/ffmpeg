/*
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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
#include "avformat.h"
#include "avio_internal.h"
#include "internal.h"

#include "libavutil/internal.h"
#include "libavutil/opt.h"

/**
 * @file
 * Options definition for AVFormatContext.
 */

FF_DISABLE_DEPRECATION_WARNINGS
#include "options_table.h"
FF_ENABLE_DEPRECATION_WARNINGS
/*
从函数的定义可以看出，
如果AVFormatContext结构体中的AVInputFormat结构体不为空，则返回AVInputFormat的name，
然后尝试返回AVOutputFormat的name，
如果AVOutputFormat也为空，则返回“NULL”。
*/

static const char* format_to_name(void* ptr)
{
    AVFormatContext* fc = (AVFormatContext*) ptr;
    if(fc->iformat) return fc->iformat->name;
    else if(fc->oformat) return fc->oformat->name;
    else return "NULL";
}

static void *format_child_next(void *obj, void *prev)
{
    AVFormatContext *s = obj;
    if (!prev && s->priv_data &&
        ((s->iformat && s->iformat->priv_class) ||
          s->oformat && s->oformat->priv_class))
        return s->priv_data;
    if (s->pb && s->pb->av_class && prev != s->pb)
        return s->pb;
    return NULL;
}

static const AVClass *format_child_class_next(const AVClass *prev)
{
    AVInputFormat  *ifmt = NULL;
    AVOutputFormat *ofmt = NULL;

    if (!prev)
        return &ff_avio_class;

    while ((ifmt = av_iformat_next(ifmt)))
        if (ifmt->priv_class == prev)
            break;

    if (!ifmt)
        while ((ofmt = av_oformat_next(ofmt)))
            if (ofmt->priv_class == prev)
                break;
    if (!ofmt)
        while (ifmt = av_iformat_next(ifmt))
            if (ifmt->priv_class)
                return ifmt->priv_class;

    while (ofmt = av_oformat_next(ofmt))
        if (ofmt->priv_class)
            return ofmt->priv_class;

    return NULL;
}

static AVClassCategory get_category(void *ptr)
{
    AVFormatContext* s = ptr;
    if(s->iformat) return AV_CLASS_CATEGORY_DEMUXER;
    else           return AV_CLASS_CATEGORY_MUXER;
}
/*
option
option字段则指向一个元素个数很多的静态数组avformat_options。
该数组单独定义于libavformat\options_table.h中。
其中包含了AVFormatContext支持的所有的AVOption，如下所示。

avformat_open_input()->avformat_alloc_context()->avformat_get_context_defaults()
*/

static const AVClass av_format_context_class = {
    .class_name     = "AVFormatContext",
    .item_name      = format_to_name,
    .option         = avformat_options,
    .version        = LIBAVUTIL_VERSION_INT,
    .child_next     = format_child_next,
    .child_class_next = format_child_class_next,
    .category       = AV_CLASS_CATEGORY_MUXER,
    .get_category   = get_category,
};

static int io_open_default(AVFormatContext *s, AVIOContext **pb,
                           const char *url, int flags, AVDictionary **options)
{
#if FF_API_OLD_OPEN_CALLBACKS
FF_DISABLE_DEPRECATION_WARNINGS
    if (s->open_cb)
        return s->open_cb(s, pb, url, flags, &s->interrupt_callback, options);
FF_ENABLE_DEPRECATION_WARNINGS
#endif

    return ffio_open_whitelist(pb, url, flags, &s->interrupt_callback, options, s->protocol_whitelist, s->protocol_blacklist);
}

static void io_close_default(AVFormatContext *s, AVIOContext *pb)
{
    avio_close(pb);
}

/*
该函数用于设置AVFormatContext的字段的默认值
从代码中可以看出，avformat_alloc_context()首先调用memset()将AVFormatContext的内存置零；
然后指定它的AVClass（指定了AVClass之后，该结构体就支持和AVOption相关的功能）；
最后调用av_opt_set_defaults()给AVFormatContext的成员变量设置默认值
（av_opt_set_defaults()就是和AVOption有关的一个函数，专门用于给指定的结构体设定默认值，此处暂不分析）。
*/
static void avformat_get_context_defaults(AVFormatContext *s)
{
    memset(s, 0, sizeof(AVFormatContext));

    s->av_class = &av_format_context_class;

    s->io_open  = io_open_default;
    s->io_close = io_close_default;

    av_opt_set_defaults(s);
}

// 分配一个AVFormatContext结构
// 引入头文件：#include "libavformat/avformat.h"
// 实现在:\ffmpeg\libavformat\options.c
// 其中负责申请一个AVFormatContext结构的内存,并进行简单初始化
// avformat_free_context()可以用来释放该结构里的所有东西以及该结构本身
// 也是就说使用 avformat_alloc_context()分配的结构,
// 需要使用avformat_free_context()来释放
// 有些版本中函数名可能为: av_alloc_format_context();
/*
从代码中可以看出，avformat_alloc_context()首先调用av_malloc()为AVFormatContext分配一块内存。
然后调用了一个函数avformat_get_context_defaults()用于给AVFormatContext设置默认值。
avformat_get_context_defaults()的定义如下。
*/
AVFormatContext *avformat_alloc_context(void)
{
    AVFormatContext *ic;
    ic = av_malloc(sizeof(AVFormatContext));
    if (!ic) return ic;
    avformat_get_context_defaults(ic);

    ic->internal = av_mallocz(sizeof(*ic->internal));
    if (!ic->internal) {
        avformat_free_context(ic);
        return NULL;
    }
    ic->internal->offset = AV_NOPTS_VALUE;
    ic->internal->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;
    ic->internal->shortest_end = AV_NOPTS_VALUE;

    return ic;
}

enum AVDurationEstimationMethod av_fmt_ctx_get_duration_estimation_method(const AVFormatContext* ctx)
{
    return ctx->duration_estimation_method;
}

const AVClass *avformat_get_class(void)
{
    return &av_format_context_class;
}
