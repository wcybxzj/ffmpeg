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

//�����￼�ǽ��������Գ�Ա���з�����
//uint8_t *data[AV_NUM_DATA_POINTERS]�������ԭʼ���ݣ�����Ƶ��˵��YUV��RGB������Ƶ��˵��PCM��
//int linesize[AV_NUM_DATA_POINTERS]��data�С�һ�С����ݵĴ�С��ע�⣺δ�ص���ͼ��Ŀ���һ�����ͼ��Ŀ���
//int width, height����Ƶ֡���͸ߣ�1920x1080,1280x720...��
//int nb_samples����Ƶ��һ��AVFrame�п��ܰ��������Ƶ֡���ڴ˱�ǰ����˼���
//int format�������ԭʼ�������ͣ�YUV420��YUV422��RGB24...��
//int key_frame���Ƿ��ǹؼ�֡
//enum AVPictureType pict_type��֡���ͣ�I,B,P...��
//AVRational sample_aspect_ratio�����߱ȣ�16:9��4:3...��
//int64_t pts����ʾʱ���
//int coded_picture_number������֡���
//int display_picture_number����ʾ֡���
//int8_t *qscale_table��QP��
//uint8_t *mbskip_table����������
//int16_t (*motion_val[2])[2]���˶�ʸ����
//uint32_t *mb_type��������ͱ�
//short *dct_coeff��DCTϵ�������û����ȡ��
//int8_t *ref_index[2]���˶����Ʋο�֡�б���ò��H.264���ֱȽ��µı�׼�Ż��漰����ο�֡��
//int interlaced_frame���Ƿ��Ǹ���ɨ��
//uint8_t motion_subsample_log2��һ������е��˶�ʸ������������ȡlog��

/*
AVFrame �ṹ��һ�����ڴ洢ԭʼ���ݣ�����ѹ�����ݣ��������Ƶ��˵�� YUV��RGB������Ƶ��˵�� PCM�� ��
���⻹������һЩ��ص���Ϣ������˵�������ʱ��洢�˺�����ͱ���QP �����˶�ʸ���������ݡ������ʱ��
Ҳ�洢����ص����ݡ������ʹ�� FFMPEG ��������������ʱ��AVFrame ��һ������Ҫ�Ľṹ�塣
=========================================================================================
���濴������Ҫ���������ã������￼�ǽ��������� ��
uint8_t *data[AV_NUM_DATA_POINTERS]�������ԭʼ���ݣ�����Ƶ��˵�� YUV��RGB������Ƶ��˵�� PCM��
int linesize[AV_NUM_DATA_POINTERS]��data �Ĵ�С
int width, height����Ƶ֡���͸ߣ�1920x1080,1280x720...��
int nb_samples����Ƶ��һ�� AVFrame �п��ܰ��������Ƶ֡���ڴ˱�ǰ����˼���
int format�������ԭʼ�������ͣ�YUV420��YUV422��RGB24...��
int key_frame���Ƿ��ǹؼ�֡
enum AVPictureType pict_type��֡���ͣ�I,B,P...��
AVRational sample_aspect_ratio�����߱ȣ�16:9��4:3...��
int64_t pts����ʾʱ���
int coded_picture_number������֡���
int display_picture_number����ʾ֡���
int8_t *qscale_table��QP ��
uint8_t *mbskip_table����������
int16_t (*motion_val[2])[2]���˶�ʸ����
uint32_t *mb_type��������ͱ�
short *dct_coeff��DCT ϵ�������û����ȡ��
int8_t *ref_index[2]���˶����Ʋο�֡�б���ò�� H.264 ���ֱȽ��µı�׼�Ż��漰����ο�֡��
int interlaced_frame���Ƿ��Ǹ���ɨ��
uint8_t motion_subsample_log2��һ������е��˶�ʸ������������ȡ log ��
=========================================================================================
1.data[]
���� packed ��ʽ�����ݣ����� RGB24�� ����浽 data[0]���档
���� planar ��ʽ�����ݣ����� YUV420P�� �����ֿ��� data[0]��data[1]��data[2]...��YUV420P �� data[0]�� Y��data[1]
�� U��data[2]�� V��
����μ���FFMPEG ʵ�� YUV��RGB ����ͼ��ԭʼ����֮���ת����swscale��

4.qscale_table
QP ��ָ��һ���ڴ棬����洢����ÿ������ QP ֵ�����ı���Ǵ������ң�һ��һ�е����ġ�
ÿ������Ӧ 1�� QP��
qscale_table[0]���ǵ� 1 �е� 1 �к��� QP ֵ��
qscale_table[1]���ǵ� 1 �е� 2 �к��� QP ֵ��
qscale_table[2]���ǵ� 1 �е� 3 �к��� QP ֵ���Դ�����...
���ĸ�������ʽ���㣺
ע������С�� 16x16 �ġ�
ÿ�к������
int mb_stride = pCodecCtx->width/16+1
����������
int mb_sum = ((pCodecCtx->height+15)>>4)*(pCodecCtx->width/16+1)

5.motion_subsample_log2
1 ���˶�ʸ�����ܴ����Ļ����С���ÿ����߸߱�ʾ����λ�����أ� ��ע�⣬����ȡ�� log2��
����ע���и����������ݣ�
4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2
�� 1 ���˶�ʸ������ 16x16 �Ļ����ʱ�򣬸�ֵȡ 4��1 ���˶�ʸ������ 8x8 �Ļ����ʱ�򣬸�ֵȡ 3...�Դ�����
6.motion_val
�˶�ʸ�����洢��һ֡��Ƶ�е������˶�ʸ����
��ֵ�Ĵ洢��ʽ�Ƚ��ر�
int16_t (*motion_val[2])[2];
int16_t (*motion_val[x])[y];
����һ��ָ�����飨��������x��Ԫ�أ���x��ָ�룩��
�������Ԫ��Ϊ�� ָ�� int16_t [y] �����ָ�룬����ν������ָ��
Ϊ��Ū�����ֵ��������ô��ģ������Һ�һ���ӹ���...
ע���и���һ�δ��룺
int mv_sample_log2= 4 - motion_subsample_log2;
int mb_width= (width+15)>>4;
int mv_stride= (mb_width << mv_sample_log2) + 1;
motion_val[direction][x + y*mv_stride][0->mv_x, 1->mv_y];
���֪���˸����ݵĽṹ��
1.���ȷ�Ϊ�����б� L0 �� L1
2.ÿ���б���L0 �� L1���洢��һϵ�е� MV��ÿ�� MV ��Ӧһ�����棬��С�� motion_subsample_log2 ������
3.ÿ�� MV ��Ϊ������������꣨x,y��
ע�⣬�� FFMPEG �� MV �� MB �ڴ洢�Ľṹ����û��ʲô�����ģ��� 1 �� MV ����Ļ�����Ͻǻ���� MV����
��Ĵ�Сȡ���� motion_subsample_log2�� ���� 2 �� MV ����Ļ�ϵ� 1 �е� 2 �еĻ���� MV���Դ����ơ������һ
����飨16x16�����˶�ʸ�����п�������ͼ��ʾ��line ����һ���˶�ʸ���ĸ����� 
//���� 8x8 ���ֵ��˶�ʸ������Ĺ�ϵ��
//-------------------------
//| 			| 			|
//|mv[x] 	|mv[x+1] 	|
//-------------------------
//| 			| 			|
//|mv[x+line]|mv[x+line+1]|
//-------------------------

7.mb_type
������ͱ��洢��һ֡��Ƶ�е����к������͡���洢��ʽ�� QP ����ࡣֻ�������� uint32 ���͵ģ��� QP
���� uint8 ���͵ġ�ÿ������Ӧһ��������ͱ�����
����������¶�����ʾ��
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
һ���������������������е�һ�ֻ��������ͣ������Ӧ�ĺ������Ķ�Ӧλ�ᱻ�� 1��
ע��һ�������԰����ü������ͣ�������Щ�����ǲ����ظ������ģ�����˵һ����鲻���ܼ��� 16x16 ���� 8x8��
8.ref_index
�˶����Ʋο�֡�б��洢��һ֡��Ƶ�����к��Ĳο�֡����������б���ʵ�ڱȽ����ѹ�������׼����û��ʲ
ô�õġ�ֻ���� H.264 �����ı����׼���ж�ο�֡�ĸ����������ֶ�Ŀǰ�һ�û���о�͸��ֻ��֪��ÿ����
������� 4 ����ֵ����ֵ��ӳ���ǲο�֡���������Ժ��л����ٽ���ϸ�о��ɡ�
*/



typedef struct AVFrame {
#define AV_NUM_DATA_POINTERS 8
    /**ͼ������ 
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


     /**�Ƿ��ǹؼ�֡ 
     * 1 -> keyframe, 0-> not 
     * - encoding: Set by libavcodec. 
     * - decoding: Set by libavcodec. 
     */  
    int key_frame;

    /**֡���ͣ�I,B,P�� 
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

    /**����Ƶ�������� 
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
    /**QP�� 
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