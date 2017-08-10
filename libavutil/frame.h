/*
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
 * @ingroup lavu_frame
 * reference-counted frame API
 */

#ifndef AVUTIL_FRAME_H
#define AVUTIL_FRAME_H

#include <stdint.h>

#include "avutil.h"
#include "buffer.h"
#include "dict.h"
#include "rational.h"
#include "samplefmt.h"
#include "pixfmt.h"
#include "version.h"


/**
 * @defgroup lavu_frame AVFrame
 * @ingroup lavu_data
 *
 * @{
 * AVFrame is an abstraction for reference-counted raw multimedia data.
 */

enum AVFrameSideDataType {
    /**
     * The data is the AVPanScan struct defined in libavcodec.
     */
    AV_FRAME_DATA_PANSCAN,
    /**
     * ATSC A53 Part 4 Closed Captions.
     * A53 CC bitstream is stored as uint8_t in AVFrameSideData.data.
     * The number of bytes of CC data is AVFrameSideData.size.
     */
    AV_FRAME_DATA_A53_CC,
    /**
     * Stereoscopic 3d metadata.
     * The data is the AVStereo3D struct defined in libavutil/stereo3d.h.
     */
    AV_FRAME_DATA_STEREO3D,
    /**
     * The data is the AVMatrixEncoding enum defined in libavutil/channel_layout.h.
     */
    AV_FRAME_DATA_MATRIXENCODING,
    /**
     * Metadata relevant to a downmix procedure.
     * The data is the AVDownmixInfo struct defined in libavutil/downmix_info.h.
     */
    AV_FRAME_DATA_DOWNMIX_INFO,
    /**
     * ReplayGain information in the form of the AVReplayGain struct.
     */
    AV_FRAME_DATA_REPLAYGAIN,
    /**
     * This side data contains a 3x3 transformation matrix describing an affine
     * transformation that needs to be applied to the frame for correct
     * presentation.
     *
     * See libavutil/display.h for a detailed description of the data.
     */
    AV_FRAME_DATA_DISPLAYMATRIX,
    /**
     * Active Format Description data consisting of a single byte as specified
     * in ETSI TS 101 154 using AVActiveFormatDescription enum.
     */
    AV_FRAME_DATA_AFD,
    /**
     * Motion vectors exported by some codecs (on demand through the export_mvs
     * flag set in the libavcodec AVCodecContext flags2 option).
     * The data is the AVMotionVector struct defined in
     * libavutil/motion_vector.h.
     */
    AV_FRAME_DATA_MOTION_VECTORS,
    /**
     * Recommmends skipping the specified number of samples. This is exported
     * only if the "skip_manual" AVOption is set in libavcodec.
     * This has the same format as AV_PKT_DATA_SKIP_SAMPLES.
     * @code
     * u32le number of samples to skip from start of this packet
     * u32le number of samples to skip from end of this packet
     * u8    reason for start skip
     * u8    reason for end   skip (0=padding silence, 1=convergence)
     * @endcode
     */
    AV_FRAME_DATA_SKIP_SAMPLES,
    /**
     * This side data must be associated with an audio frame and corresponds to
     * enum AVAudioServiceType defined in avcodec.h.
     */
    AV_FRAME_DATA_AUDIO_SERVICE_TYPE,
    /**
     * Mastering display metadata associated with a video frame. The payload is
     * an AVMasteringDisplayMetadata type and contains information about the
     * mastering display color volume.
     */
    AV_FRAME_DATA_MASTERING_DISPLAY_METADATA,
    /**
     * The GOP timecode in 25 bit timecode format. Data format is 64-bit integer.
     * This is set on the first frame of a GOP that has a temporal reference of 0.
     */
    AV_FRAME_DATA_GOP_TIMECODE,

    /**
     * The data represents the AVSphericalMapping structure defined in
     * libavutil/spherical.h.
     */
    AV_FRAME_DATA_SPHERICAL,

    /**
     * Content light level (based on CTA-861.3). This payload contains data in
     * the form of the AVContentLightMetadata struct.
     */
    AV_FRAME_DATA_CONTENT_LIGHT_LEVEL,
};

enum AVActiveFormatDescription {
    AV_AFD_SAME         = 8,
    AV_AFD_4_3          = 9,
    AV_AFD_16_9         = 10,
    AV_AFD_14_9         = 11,
    AV_AFD_4_3_SP_14_9  = 13,
    AV_AFD_16_9_SP_14_9 = 14,
    AV_AFD_SP_4_3       = 15,
};


/**
 * Structure to hold side data for an AVFrame.
 *
 * sizeof(AVFrameSideData) is not a part of the public ABI, so new fields may be added
 * to the end with a minor bump.
 */
typedef struct AVFrameSideData {
    enum AVFrameSideDataType type;
    uint8_t *data;
    int      size;
    AVDictionary *metadata;
    AVBufferRef *buf;
} AVFrameSideData;

/**
 * This structure describes decoded (raw) audio or video data.
 *
 * AVFrame must be allocated using av_frame_alloc(). Note that this only
 * allocates the AVFrame itself, the buffers for the data must be managed
 * through other means (see below).
 * AVFrame must be freed with av_frame_free().
 *
 * AVFrame is typically allocated once and then reused multiple times to hold
 * different data (e.g. a single AVFrame to hold frames received from a
 * decoder). In such a case, av_frame_unref() will free any references held by
 * the frame and reset it to its original clean state before it
 * is reused again.
 *
 * The data described by an AVFrame is usually reference counted through the
 * AVBuffer API. The underlying buffer references are stored in AVFrame.buf /
 * AVFrame.extended_buf. An AVFrame is considered to be reference counted if at
 * least one reference is set, i.e. if AVFrame.buf[0] != NULL. In such a case,
 * every single data plane must be contained in one of the buffers in
 * AVFrame.buf or AVFrame.extended_buf.
 * There may be a single buffer for all the data, or one separate buffer for
 * each plane, or anything in between.
 *
 * sizeof(AVFrame) is not a part of the public ABI, so new fields may be added
 * to the end with a minor bump.
 *
 * Fields can be accessed through AVOptions, the name string used, matches the
 * C structure field name for fields accessible through AVOptions. The AVClass
 * for AVFrame can be obtained from avcodec_get_frame_class()
 */

//在这里考虑解码的情况对成员进行分析：
//uint8_t *data[AV_NUM_DATA_POINTERS]：解码后原始数据（对视频来说是YUV，RGB，对音频来说是PCM）
//int linesize[AV_NUM_DATA_POINTERS]：data中“一行”数据的大小。注意：未必等于图像的宽，一般大于图像的宽。
//int width, height：视频帧宽和高（1920x1080,1280x720...）
//int nb_samples：音频的一个AVFrame中可能包含多个音频帧，在此标记包含了几个
//int format：解码后原始数据类型（YUV420，YUV422，RGB24...）
//int key_frame：是否是关键帧
//enum AVPictureType pict_type：帧类型（I,B,P...）
//AVRational sample_aspect_ratio：宽高比（16:9，4:3...）
//int64_t pts：显示时间戳
//int coded_picture_number：编码帧序号
//int display_picture_number：显示帧序号
//int8_t *qscale_table：QP表
//uint8_t *mbskip_table：跳过宏块表
//int16_t (*motion_val[2])[2]：运动矢量表
//uint32_t *mb_type：宏块类型表
//short *dct_coeff：DCT系数，这个没有提取过
//int8_t *ref_index[2]：运动估计参考帧列表（貌似H.264这种比较新的标准才会涉及到多参考帧）
//int interlaced_frame：是否是隔行扫描
//uint8_t motion_subsample_log2：一个宏块中的运动矢量采样个数，取log的

/*
AVFrame 结构体一般用于存储原始数据（即非压缩数据，例如对视频来说是 YUV，RGB，对音频来说是 PCM） ，
此外还包含了一些相关的信息。比如说，解码的时候存储了宏块类型表，QP 表，运动矢量表等数据。编码的时候
也存储了相关的数据。因此在使用 FFMPEG 进行码流分析的时候，AVFrame 是一个很重要的结构体。
=========================================================================================
下面看几个主要变量的作用（在这里考虑解码的情况） ：
uint8_t *data[AV_NUM_DATA_POINTERS]：解码后原始数据（对视频来说是 YUV，RGB，对音频来说是 PCM）
int linesize[AV_NUM_DATA_POINTERS]：data 的大小
int width, height：视频帧宽和高（1920x1080,1280x720...）
int nb_samples：音频的一个 AVFrame 中可能包含多个音频帧，在此标记包含了几个
int format：解码后原始数据类型（YUV420，YUV422，RGB24...）
int key_frame：是否是关键帧
enum AVPictureType pict_type：帧类型（I,B,P...）
AVRational sample_aspect_ratio：宽高比（16:9，4:3...）
int64_t pts：显示时间戳
int coded_picture_number：编码帧序号
int display_picture_number：显示帧序号
int8_t *qscale_table：QP 表
uint8_t *mbskip_table：跳过宏块表
int16_t (*motion_val[2])[2]：运动矢量表
uint32_t *mb_type：宏块类型表
short *dct_coeff：DCT 系数，这个没有提取过
int8_t *ref_index[2]：运动估计参考帧列表（貌似 H.264 这种比较新的标准才会涉及到多参考帧）
int interlaced_frame：是否是隔行扫描
uint8_t motion_subsample_log2：一个宏块中的运动矢量采样个数，取 log 的
=========================================================================================
1.data[]
对于 packed 格式的数据（例如 RGB24） ，会存到 data[0]里面。
对于 planar 格式的数据（例如 YUV420P） ，则会分开成 data[0]，data[1]，data[2]...（YUV420P 中 data[0]存 Y，data[1]
存 U，data[2]存 V）
具体参见：FFMPEG 实现 YUV，RGB 各种图像原始数据之间的转换（swscale）

4.qscale_table
QP 表指向一块内存，里面存储的是每个宏块的 QP 值。宏块的标号是从左往右，一行一行的来的。
每个宏块对应 1个 QP。
qscale_table[0]就是第 1 行第 1 列宏块的 QP 值；
qscale_table[1]就是第 1 行第 2 列宏块的 QP 值；
qscale_table[2]就是第 1 行第 3 列宏块的 QP 值。以此类推...
宏块的个数用下式计算：
注：宏块大小是 16x16 的。
每行宏块数：
int mb_stride = pCodecCtx->width/16+1
宏块的总数：
int mb_sum = ((pCodecCtx->height+15)>>4)*(pCodecCtx->width/16+1)

5.motion_subsample_log2
1 个运动矢量所能代表的画面大小（用宽或者高表示，单位是像素） ，注意，这里取了 log2。
代码注释中给出以下数据：
4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2
即 1 个运动矢量代表 16x16 的画面的时候，该值取 4；1 个运动矢量代表 8x8 的画面的时候，该值取 3...以此类推
6.motion_val
运动矢量表存储了一帧视频中的所有运动矢量。
该值的存储方式比较特别：
int16_t (*motion_val[2])[2];
int16_t (*motion_val[x])[y];
定义一个指针数组（该数组有x个元素，即x个指针），
该数组的元素为： 指向 int16_t [y] 数组的指针，即所谓的数组指针
为了弄清楚该值究竟是怎么存的，花了我好一阵子功夫...
注释中给了一段代码：
int mv_sample_log2= 4 - motion_subsample_log2;
int mb_width= (width+15)>>4;
int mv_stride= (mb_width << mv_sample_log2) + 1;
motion_val[direction][x + y*mv_stride][0->mv_x, 1->mv_y];
大概知道了该数据的结构：
1.首先分为两个列表 L0 和 L1
2.每个列表（L0 或 L1）存储了一系列的 MV（每个 MV 对应一个画面，大小由 motion_subsample_log2 决定）
3.每个 MV 分为横坐标和纵坐标（x,y）
注意，在 FFMPEG 中 MV 和 MB 在存储的结构上是没有什么关联的，第 1 个 MV 是屏幕上左上角画面的 MV（画
面的大小取决于 motion_subsample_log2） ，第 2 个 MV 是屏幕上第 1 行第 2 列的画面的 MV，以此类推。因此在一
个宏块（16x16）的运动矢量很有可能如下图所示（line 代表一行运动矢量的个数） 
//例如 8x8 划分的运动矢量与宏块的关系：
//-------------------------
//| 			| 			|
//|mv[x] 	|mv[x+1] 	|
//-------------------------
//| 			| 			|
//|mv[x+line]|mv[x+line+1]|
//-------------------------

7.mb_type
宏块类型表存储了一帧视频中的所有宏块的类型。其存储方式和 QP 表差不多。只不过其是 uint32 类型的，而 QP
表是 uint8 类型的。每个宏块对应一个宏块类型变量。
宏块类型如下定义所示：
//The following defines may change, don't expect compatibility if you use them.
#define MB_TYPE_INTRA4x4 0x0001
#define MB_TYPE_INTRA16x16 0x0002 //FIXME H.264-specific
#define MB_TYPE_INTRA_PCM 0x0004 //FIXME H.264-specific
#define MB_TYPE_16x16 0x0008
#define MB_TYPE_16x8 0x0010
#define MB_TYPE_8x16 0x0020
#define MB_TYPE_8x8 0x0040
.............................
//Note bits 24-31 are reserved for codec specific use (h264 ref0, mpeg1 0mv, ...)
一个宏块如果包含上述定义中的一种或两种类型，则其对应的宏块变量的对应位会被置 1。
注：一个宏块可以包含好几种类型，但是有些类型是不能重复包含的，比如说一个宏块不可能既是 16x16 又是 8x8。
8.ref_index
运动估计参考帧列表存储了一帧视频中所有宏块的参考帧索引。这个列表其实在比较早的压缩编码标准中是没有什
么用的。只有像 H.264 这样的编码标准才有多参考帧的概念。但是这个字段目前我还没有研究透。只是知道每个宏
块包含有 4 个该值，该值反映的是参考帧的索引。以后有机会再进行细研究吧。
*/



typedef struct AVFrame {
#define AV_NUM_DATA_POINTERS 8
    /**图像数据 
     * pointer to the picture/channel planes.
     * This might be different from the first allocated byte
     *
     * Some decoders access areas outside 0,0 - width,height, please
     * see avcodec_align_dimensions2(). Some filters and swscale can read
     * up to 16 bytes beyond the planes, if these filters are to be used,
     * then 16 extra bytes must be allocated.
     *
     * NOTE: Except for hwaccel formats, pointers not needed by the format
     * MUST be set to NULL.
     * - encoding: Set by user 
     * - decoding: set by AVCodecContext.get_buffer() 
     */
     
    uint8_t *data[AV_NUM_DATA_POINTERS];

    /**
     * For video, size in bytes of each picture line.
     * For audio, size in bytes of each plane.
     *
     * For audio, only linesize[0] may be set. For planar audio, each channel
     * plane must be the same size.
     *
     * For video the linesizes should be multiples of the CPUs alignment
     * preference, this is 16 or 32 for modern desktop CPUs.
     * Some code requires such alignment other code can be slower without
     * correct alignment, for yet other it makes no difference.
     *
     * @note The linesize may be larger than the size of usable data -- there
     * may be extra padding present for performance reasons.
     * - encoding: Set by user 
     * - decoding: set by AVCodecContext.get_buffer() 
     */
    int linesize[AV_NUM_DATA_POINTERS];

    /**
     * pointers to the data planes/channels.
     *
     * For video, this should simply point to data[].
     *
     * For planar audio, each channel has a separate data pointer, and
     * linesize[0] contains the size of each channel buffer.
     * For packed audio, there is just one data pointer, and linesize[0]
     * contains the total size of the buffer for all channels.
     *
     * Note: Both data and extended_data should always be set in a valid frame,
     * but for planar audio with more channels that can fit in data,
     * extended_data must be used in order to access all channels.
     * encoding: unused 
     * decoding: set by AVCodecContext.get_buffer() 
     */
    uint8_t **extended_data;

    /**
     * width and height of the video frame
     * - encoding: unused 
     * - decoding: Read by user. 
     */
    int width, height;

    /**
     * number of audio samples (per channel) described by this frame
     * - encoding: Set by user 
     * - decoding: Set by libavcodec 
     */
    int nb_samples;

    /**
     * format of the frame, -1 if unknown or unset
     * Values correspond to enum AVPixelFormat for video frames,
     * enum AVSampleFormat for audio)
     * - encoding: unused 
     * - decoding: Read by user. 
     */
    int format;


     /**是否是关键帧 
     * 1 -> keyframe, 0-> not 
     * - encoding: Set by libavcodec. 
     * - decoding: Set by libavcodec. 
     */  
    int key_frame;

    /**帧类型（I,B,P） 
     * Picture type of the frame, see ?_TYPE below. 
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input). 
     * - decoding: Set by libavcodec. 
     */       
    enum AVPictureType pict_type;

    /**
     * Sample aspect ratio for the video frame, 0/1 if unknown/unspecified.
     * - encoding: unused 
     * - decoding: Read by user. 
     */
    AVRational sample_aspect_ratio;

    /**
     * Presentation timestamp in time_base units (time when frame should be shown to user).
     * - encoding: MUST be set by user. 
     * - decoding: Set by libavcodec.     
     */
    int64_t pts;

#if FF_API_PKT_PTS
    /**
     * PTS copied from the AVPacket that was decoded to produce this frame.
     * @deprecated use the pts field instead
     * - encoding: unused 
     * - decoding: Read by user. 
     */
    attribute_deprecated
    int64_t pkt_pts;
#endif

    /**
     * DTS copied from the AVPacket that triggered returning this frame. (if frame threading isn't used)
     * This is also the Presentation time of this AVFrame calculated from
     * only AVPacket.dts values without pts values.
     */
    int64_t pkt_dts;

    /**
     * picture number in bitstream order
     */
    int coded_picture_number;
    /** 
     * picture number in display order 
     * - encoding: set by 
     * - decoding: Set by libavcodec. 
     */      
    int display_picture_number;

   /* quality (between 1 (good) and FF_LAMBDA_MAX (bad)) 
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input). 
     * - decoding: Set by libavcodec. 
     */      
    int quality;

    /** 
     * for some private data of the user 
     * - encoding: unused 
     * - decoding: Set by user. 
     */
    void *opaque;

#if FF_API_ERROR_FRAME
    /**
     * @deprecated unused
     */
    /** 
     * error 
     * - encoding: Set by libavcodec. if flags&CODEC_FLAG_PSNR. 
     * - decoding: unused 
     */ 
    attribute_deprecated
    uint64_t error[AV_NUM_DATA_POINTERS];
#endif

    /** 
     * When decoding, this signals how much the picture must be delayed. 
     * extra_delay = repeat_pict / (2*fps) 
     * - encoding: unused 
     * - decoding: Set by libavcodec. 
     */ 
    int repeat_pict;

    /** 
     * The content of the picture is interlaced. 
     * - encoding: Set by user. 
     * - decoding: Set by libavcodec. (default 0) 
     */ 
    int interlaced_frame;

    /** 
     * If the content is interlaced, is top field displayed first. 
     * - encoding: Set by user. 
     * - decoding: Set by libavcodec. 
     */ 
    int top_field_first;

    /** 
     * Tell user application that palette has changed from previous frame. 
     * - encoding: ??? (no palette-enabled encoder yet) 
     * - decoding: Set by libavcodec. (default 0). 
     */  
    int palette_has_changed;

    /** 
     * reordered opaque 64bit (generally an integer or a double precision float 
     * PTS but can be anything). 
     * The user sets AVCodecContext.reordered_opaque to represent the input at 
     * that time, 
     * the decoder reorders values as needed and sets AVFrame.reordered_opaque 
     * to exactly one of the values provided by the user through AVCodecContext.reordered_opaque 
     * @deprecated in favor of pkt_pts 
     * - encoding: unused 
     * - decoding: Read by user. 
     */
    int64_t reordered_opaque;

    /**（音频）采样率 
     * Sample rate of the audio data. 
     * 
     * - encoding: unused 
     * - decoding: read by user 
     */ 
    int sample_rate;

    /** 
     * Channel layout of the audio data. 
     * 
     * - encoding: unused 
     * - decoding: read by user. 
     */ 
    uint64_t channel_layout;

    /**
     * AVBuffer references backing the data for this frame. If all elements of
     * this array are NULL, then this frame is not reference counted. This array
     * must be filled contiguously -- if buf[i] is non-NULL then buf[j] must
     * also be non-NULL for all j < i.
     *
     * There may be at most one AVBuffer per data plane, so for video this array
     * always contains all the references. For planar audio with more than
     * AV_NUM_DATA_POINTERS channels, there may be more buffers than can fit in
     * this array. Then the extra AVBufferRef pointers are stored in the
     * extended_buf array.
     */
    AVBufferRef *buf[AV_NUM_DATA_POINTERS];

    /**
     * For planar audio which requires more than AV_NUM_DATA_POINTERS
     * AVBufferRef pointers, this array will hold all the references which
     * cannot fit into AVFrame.buf.
     *
     * Note that this is different from AVFrame.extended_data, which always
     * contains all the pointers. This array only contains the extra pointers,
     * which cannot fit into AVFrame.buf.
     *
     * This array is always allocated using av_malloc() by whoever constructs
     * the frame. It is freed in av_frame_unref().
     */
    AVBufferRef **extended_buf;
    /**
     * Number of elements in extended_buf.
     */
    int        nb_extended_buf;

    AVFrameSideData **side_data;
    int            nb_side_data;

/**
 * @defgroup lavu_frame_flags AV_FRAME_FLAGS
 * @ingroup lavu_frame
 * Flags describing additional frame properties.
 *
 * @{
 */

/**
 * The frame data may be corrupted, e.g. due to decoding errors.
 */
#define AV_FRAME_FLAG_CORRUPT       (1 << 0)
/**
 * A flag to mark the frames which need to be decoded, but shouldn't be output.
 */
#define AV_FRAME_FLAG_DISCARD   (1 << 2)
/**
 * @}
 */

    /**
     * Frame flags, a combination of @ref lavu_frame_flags
     */
    int flags;

    /**
     * MPEG vs JPEG YUV range.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum AVColorRange color_range;

    enum AVColorPrimaries color_primaries;

    enum AVColorTransferCharacteristic color_trc;

    /**
     * YUV colorspace type.
     * - encoding: Set by user
     * - decoding: Set by libavcodec
     */
    enum AVColorSpace colorspace;

    enum AVChromaLocation chroma_location;

    /** 
     * frame timestamp estimated using various heuristics, in stream time base 
     * Code outside libavcodec should access this field using: 
     * av_frame_get_best_effort_timestamp(frame) 
     * - encoding: unused 
     * - decoding: set by libavcodec, read by user. 
     */  
    int64_t best_effort_timestamp;

    /** 
     * reordered pos from the last AVPacket that has been input into the decoder 
     * Code outside libavcodec should access this field using: 
     * av_frame_get_pkt_pos(frame) 
     * - encoding: unused 
     * - decoding: Read by user. 
     */  
    int64_t pkt_pos;

    /** 
     * duration of the corresponding packet, expressed in 
     * AVStream->time_base units, 0 if unknown. 
     * Code outside libavcodec should access this field using: 
     * av_frame_get_pkt_duration(frame) 
     * - encoding: unused 
     * - decoding: Read by user. 
     */
    int64_t pkt_duration;

    /** 
     * metadata. 
     * Code outside libavcodec should access this field using: 
     * av_frame_get_metadata(frame) 
     * - encoding: Set by user. 
     * - decoding: Set by libavcodec. 
     */  
    AVDictionary *metadata;

    /** 
     * decode error flags of the frame, set to a combination of 
     * FF_DECODE_ERROR_xxx flags if the decoder produced a frame, but there 
     * were errors during the decoding. 
     * Code outside libavcodec should access this field using: 
     * av_frame_get_decode_error_flags(frame) 
     * - encoding: unused 
     * - decoding: set by libavcodec, read by user. 
     */  
    int decode_error_flags;
#define FF_DECODE_ERROR_INVALID_BITSTREAM   1
#define FF_DECODE_ERROR_MISSING_REFERENCE   2

    /**
     * number of audio channels, only used for audio.
     * - encoding: unused
     * - decoding: Read by user.
     */
    int channels;

    /**
     * size of the corresponding packet containing the compressed
     * frame.
     * It is set to a negative value if unknown.
     * - encoding: unused
     * - decoding: set by libavcodec, read by user.
     */
    int pkt_size;

#if FF_API_FRAME_QP
    /**QP表 
     * QP table 
     * - encoding: unused 
     * - decoding: Set by libavcodec. 
     */ 
    attribute_deprecated
    int8_t *qscale_table;
    /** 
     * QP store stride 
     * - encoding: unused 
     * - decoding: Set by libavcodec. 
     */  
    attribute_deprecated
    int qstride;

    attribute_deprecated
    int qscale_type;

    AVBufferRef *qp_table_buf;
#endif
    /**
     * For hwaccel-format frames, this should be a reference to the
     * AVHWFramesContext describing the frame.
     */
    AVBufferRef *hw_frames_ctx;

    /**
     * AVBufferRef for free use by the API user. FFmpeg will never check the
     * contents of the buffer ref. FFmpeg calls av_buffer_unref() on it when
     * the frame is unreferenced. av_frame_copy_props() calls create a new
     * reference with av_buffer_ref() for the target frame's opaque_ref field.
     *
     * This is unrelated to the opaque field, although it serves a similar
     * purpose.
     */
    AVBufferRef *opaque_ref;
} AVFrame;

/**
 * Accessors for some AVFrame fields. These used to be provided for ABI
 * compatibility, and do not need to be used anymore.
 */
int64_t av_frame_get_best_effort_timestamp(const AVFrame *frame);
void    av_frame_set_best_effort_timestamp(AVFrame *frame, int64_t val);
int64_t av_frame_get_pkt_duration         (const AVFrame *frame);
void    av_frame_set_pkt_duration         (AVFrame *frame, int64_t val);
int64_t av_frame_get_pkt_pos              (const AVFrame *frame);
void    av_frame_set_pkt_pos              (AVFrame *frame, int64_t val);
int64_t av_frame_get_channel_layout       (const AVFrame *frame);
void    av_frame_set_channel_layout       (AVFrame *frame, int64_t val);
int     av_frame_get_channels             (const AVFrame *frame);
void    av_frame_set_channels             (AVFrame *frame, int     val);
int     av_frame_get_sample_rate          (const AVFrame *frame);
void    av_frame_set_sample_rate          (AVFrame *frame, int     val);
AVDictionary *av_frame_get_metadata       (const AVFrame *frame);
void          av_frame_set_metadata       (AVFrame *frame, AVDictionary *val);
int     av_frame_get_decode_error_flags   (const AVFrame *frame);
void    av_frame_set_decode_error_flags   (AVFrame *frame, int     val);
int     av_frame_get_pkt_size(const AVFrame *frame);
void    av_frame_set_pkt_size(AVFrame *frame, int val);
AVDictionary **avpriv_frame_get_metadatap(AVFrame *frame);
#if FF_API_FRAME_QP
int8_t *av_frame_get_qp_table(AVFrame *f, int *stride, int *type);
int av_frame_set_qp_table(AVFrame *f, AVBufferRef *buf, int stride, int type);
#endif
enum AVColorSpace av_frame_get_colorspace(const AVFrame *frame);
void    av_frame_set_colorspace(AVFrame *frame, enum AVColorSpace val);
enum AVColorRange av_frame_get_color_range(const AVFrame *frame);
void    av_frame_set_color_range(AVFrame *frame, enum AVColorRange val);

/**
 * Get the name of a colorspace.
 * @return a static string identifying the colorspace; can be NULL.
 */
const char *av_get_colorspace_name(enum AVColorSpace val);

/**
 * Allocate an AVFrame and set its fields to default values.  The resulting
 * struct must be freed using av_frame_free().
 *
 * @return An AVFrame filled with default values or NULL on failure.
 *
 * @note this only allocates the AVFrame itself, not the data buffers. Those
 * must be allocated through other means, e.g. with av_frame_get_buffer() or
 * manually.
 */
AVFrame *av_frame_alloc(void);

/**
 * Free the frame and any dynamically allocated objects in it,
 * e.g. extended_data. If the frame is reference counted, it will be
 * unreferenced first.
 *
 * @param frame frame to be freed. The pointer will be set to NULL.
 */
void av_frame_free(AVFrame **frame);

/**
 * Set up a new reference to the data described by the source frame.
 *
 * Copy frame properties from src to dst and create a new reference for each
 * AVBufferRef from src.
 *
 * If src is not reference counted, new buffers are allocated and the data is
 * copied.
 *
 * @warning: dst MUST have been either unreferenced with av_frame_unref(dst),
 *           or newly allocated with av_frame_alloc() before calling this
 *           function, or undefined behavior will occur.
 *
 * @return 0 on success, a negative AVERROR on error
 */
int av_frame_ref(AVFrame *dst, const AVFrame *src);

/**
 * Create a new frame that references the same data as src.
 *
 * This is a shortcut for av_frame_alloc()+av_frame_ref().
 *
 * @return newly created AVFrame on success, NULL on error.
 */
AVFrame *av_frame_clone(const AVFrame *src);

/**
 * Unreference all the buffers referenced by frame and reset the frame fields.
 */
void av_frame_unref(AVFrame *frame);

/**
 * Move everything contained in src to dst and reset src.
 *
 * @warning: dst is not unreferenced, but directly overwritten without reading
 *           or deallocating its contents. Call av_frame_unref(dst) manually
 *           before calling this function to ensure that no memory is leaked.
 */
void av_frame_move_ref(AVFrame *dst, AVFrame *src);

/**
 * Allocate new buffer(s) for audio or video data.
 *
 * The following fields must be set on frame before calling this function:
 * - format (pixel format for video, sample format for audio)
 * - width and height for video
 * - nb_samples and channel_layout for audio
 *
 * This function will fill AVFrame.data and AVFrame.buf arrays and, if
 * necessary, allocate and fill AVFrame.extended_data and AVFrame.extended_buf.
 * For planar formats, one buffer will be allocated for each plane.
 *
 * @warning: if frame already has been allocated, calling this function will
 *           leak memory. In addition, undefined behavior can occur in certain
 *           cases.
 *
 * @param frame frame in which to store the new buffers.
 * @param align required buffer size alignment
 *
 * @return 0 on success, a negative AVERROR on error.
 */
int av_frame_get_buffer(AVFrame *frame, int align);

/**
 * Check if the frame data is writable.
 *
 * @return A positive value if the frame data is writable (which is true if and
 * only if each of the underlying buffers has only one reference, namely the one
 * stored in this frame). Return 0 otherwise.
 *
 * If 1 is returned the answer is valid until av_buffer_ref() is called on any
 * of the underlying AVBufferRefs (e.g. through av_frame_ref() or directly).
 *
 * @see av_frame_make_writable(), av_buffer_is_writable()
 */
int av_frame_is_writable(AVFrame *frame);

/**
 * Ensure that the frame data is writable, avoiding data copy if possible.
 *
 * Do nothing if the frame is writable, allocate new buffers and copy the data
 * if it is not.
 *
 * @return 0 on success, a negative AVERROR on error.
 *
 * @see av_frame_is_writable(), av_buffer_is_writable(),
 * av_buffer_make_writable()
 */
int av_frame_make_writable(AVFrame *frame);

/**
 * Copy the frame data from src to dst.
 *
 * This function does not allocate anything, dst must be already initialized and
 * allocated with the same parameters as src.
 *
 * This function only copies the frame data (i.e. the contents of the data /
 * extended data arrays), not any other properties.
 *
 * @return >= 0 on success, a negative AVERROR on error.
 */
int av_frame_copy(AVFrame *dst, const AVFrame *src);

/**
 * Copy only "metadata" fields from src to dst.
 *
 * Metadata for the purpose of this function are those fields that do not affect
 * the data layout in the buffers.  E.g. pts, sample rate (for audio) or sample
 * aspect ratio (for video), but not width/height or channel layout.
 * Side data is also copied.
 */
int av_frame_copy_props(AVFrame *dst, const AVFrame *src);

/**
 * Get the buffer reference a given data plane is stored in.
 *
 * @param plane index of the data plane of interest in frame->extended_data.
 *
 * @return the buffer reference that contains the plane or NULL if the input
 * frame is not valid.
 */
AVBufferRef *av_frame_get_plane_buffer(AVFrame *frame, int plane);

/**
 * Add a new side data to a frame.
 *
 * @param frame a frame to which the side data should be added
 * @param type type of the added side data
 * @param size size of the side data
 *
 * @return newly added side data on success, NULL on error
 */
AVFrameSideData *av_frame_new_side_data(AVFrame *frame,
                                        enum AVFrameSideDataType type,
                                        int size);

/**
 * @return a pointer to the side data of a given type on success, NULL if there
 * is no side data with such type in this frame.
 */
AVFrameSideData *av_frame_get_side_data(const AVFrame *frame,
                                        enum AVFrameSideDataType type);

/**
 * If side data of the supplied type exists in the frame, free it and remove it
 * from the frame.
 */
void av_frame_remove_side_data(AVFrame *frame, enum AVFrameSideDataType type);

/**
 * @return a string identifying the side data type
 */
const char *av_frame_side_data_name(enum AVFrameSideDataType type);

/**
 * @}
 */

#endif /* AVUTIL_FRAME_H */
