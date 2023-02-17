#ifndef FFMPEG_H
#define FFMPEG_H

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

#if defined(__MINGW64__)
// 需要对应修改config.h的HAVE_PTHREADS变量
#include <pthread.h>
#endif

#include <string>
#include <atomic>
#include "msgqueue.hpp"
#include "msgdef.h"
#include "thread_wrapper.h"

extern "C"{
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <signal.h>

#include "cmdutils.h"

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavfilter/avfilter.h"

#include "libavutil/avutil.h"
#include "libavutil/dict.h"
#include "libavutil/eval.h"
#include "libavutil/fifo.h"
#include "libavutil/hwcontext.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
//#include "libavutil/thread.h"
#include "libavutil/threadmessage.h"

#include "libswresample/swresample.h"
}


#define VSYNC_AUTO       -1
#define VSYNC_PASSTHROUGH 0         // 透传?
#define VSYNC_CFR         1         // 固定帧率(Const Frame-Rate).
#define VSYNC_VFR         2         // 可变帧率(Variable Frame-Rate)
#define VSYNC_VSCFR       0xfe      // video stream cfr. 1)固定帧率 且 只有一个输入视频流 && 输入偏移地址是0; 2)固定帧率且copy_ts=1时会使用?
#define VSYNC_DROP        0xff      // 音视频同步时，允许drop帧？

#define MAX_STREAMS 1024    /* arbitrary sanity check value */

#define mydebug av_log
//#define TYYCODE_TIMESTAMP_DEMUXER   // 用于debug解复用的时间戳
//#define TYYCODE_TIMESTAMP_DECODE
//#define TYYCODE_TIMESTAMP_ENCODER
//#define TYYCODE_TIMESTAMP_MUXER

//ffmpeg_opt.c
/**
 * @brief 这里看到，MATCH_PER_STREAM_OPT的作用是，遍历用户所有的编解码器名字所属的音视频类型是否都与该流st的st->codecpar->codec_type一样，
 *              如果存在一个不一样，那么程序直接退出；否则符合，当用户输入多个编解码器名字时，只会取最后一个编解码器名字作为返回.
 * 例如用户传入：-vcodec libx265 -vcodec libx264，此时假设st是视频流，虽然h265,h264都是属于视频流类型的编解码器，但是只会返回用户最后一个
 * 编解码器名字，即outvar="libx264"作为传出参数
 */
#define MATCH_PER_STREAM_OPT(name, type, outvar, fmtctx, st)\
{\
    int i, ret;\
    for (i = 0; i < o->nb_ ## name; i++) {\
        char *spec = o->name[i].specifier;\
        if ((ret = check_stream_specifier(fmtctx, st, spec)) > 0)\
            outvar = o->name[i].u.type;\
        else if (ret < 0)\
            exit_program(1);\
    }\
}
#define MATCH_PER_STREAM_OPT_EX(name, type, outvar, outvartype, fmtctx, st)\
{\
    int i, ret;\
    for (i = 0; i < o->nb_ ## name; i++) {\
        char *spec = o->name[i].specifier;\
        if ((ret = check_stream_specifier(fmtctx, st, spec)) > 0)\
            outvar = (outvartype)o->name[i].u.type;\
        else if (ret < 0)\
            exit_program(1);\
    }\
}


#define MATCH_PER_TYPE_OPT(name, type, outvar, fmtctx, mediatype)\
{\
    int i;\
    for (i = 0; i < o->nb_ ## name; i++) {\
        char *spec = o->name[i].specifier;\
        if (!strcmp(spec, mediatype))\
            outvar = o->name[i].u.type;\
    }\
}
#define MATCH_PER_TYPE_OPT_EX(name, type, outvar, outvartype, fmtctx, mediatype)\
{\
    int i;\
    for (i = 0; i < o->nb_ ## name; i++) {\
        char *spec = o->name[i].specifier;\
        if (!strcmp(spec, mediatype))\
            outvar = (outvartype)o->name[i].u.type;\
    }\
}

enum HWAccelID {
    HWACCEL_NONE = 0,
    HWACCEL_AUTO,
    HWACCEL_GENERIC,
    HWACCEL_VIDEOTOOLBOX,
    HWACCEL_QSV,
    HWACCEL_CUVID,
};

typedef struct HWAccel {
    const char *name;
    int (*init)(AVCodecContext *s);
    enum HWAccelID id;
    enum AVPixelFormat pix_fmt;
} HWAccel;

typedef struct HWDevice {
    const char *name;
    enum AVHWDeviceType type;
    AVBufferRef *device_ref;    /*硬件设备引用*/
} HWDevice;

/* select an input stream for an output stream */
typedef struct StreamMap {
    int disabled;           /* 1 is this mapping is disabled by a negative map */
    int file_index;
    int stream_index;
    int sync_file_index;
    int sync_stream_index;
    char *linklabel;       /* name of an output link, for mapping lavfi outputs */
} StreamMap;

typedef struct {
    int  file_idx,  stream_idx,  channel_idx; // input
    int ofile_idx, ostream_idx;               // output
} AudioChannelMap;

#if 1
typedef struct OptionsContext {
    OptionGroup *g;

    /* input/output options */
    int64_t start_time;                         // -ss选项
    int64_t start_time_eof;
    int seek_timestamp;
    const char *format;                         // -f选项.注：多个-f选项时，只会取最后一个，例如ffmpeg xxx -f s16le -f flv xxx，那么format就是flv

    SpecifierOpt *codec_names;                  //offset:40,-c,-codec,-vcodec
    int        nb_codec_names;
    SpecifierOpt *audio_channels;
    int        nb_audio_channels;
    SpecifierOpt *audio_sample_rate;
    int        nb_audio_sample_rate;
    SpecifierOpt *frame_rates;                  //offset:88, -r选项
    int        nb_frame_rates;
    SpecifierOpt *frame_sizes;                  // -s选项
    int        nb_frame_sizes;
    SpecifierOpt *frame_pix_fmts;
    int        nb_frame_pix_fmts;

    /* input options */
    int64_t input_ts_offset;                    //offset:136. -itsoffset选项,设置输入时间戳偏移
    int loop;                                   //循环次数.例如无限次,-stream_loop -1
    int rate_emu;                               //offset:148 -re
    int accurate_seek;                          //-accurate_seek选项,是否开启精确查找,与-ss有关
    int thread_queue_size;

    SpecifierOpt *ts_scale;                     //-itsscale选项,默认是1.0,设置输入ts的刻度
    int        nb_ts_scale;
    SpecifierOpt *dump_attachment;
    int        nb_dump_attachment;
    SpecifierOpt *hwaccels;
    int        nb_hwaccels;
    SpecifierOpt *hwaccel_devices;
    int        nb_hwaccel_devices;
    SpecifierOpt *hwaccel_output_formats;
    int        nb_hwaccel_output_formats;
    SpecifierOpt *autorotate;                       // -autorotate选项
    int        nb_autorotate;

    /* output options */
    StreamMap *stream_maps;
    int     nb_stream_maps;
    AudioChannelMap *audio_channel_maps; /* one info entry per -map_channel */
    int           nb_audio_channel_maps; /* number of (valid) -map_channel settings */
    int metadata_global_manual;
    int metadata_streams_manual;
    int metadata_chapters_manual;
    const char **attachments;
    int       nb_attachments;

    int chapters_input_file;

    int64_t recording_time;     // 对应输入输出文件的-t选项.注意区分输入输出文件.
                                // 例如输入没设-t,而输出设了10s,输入的recording_time是初始化时的值,而输出的则为10s.实际上这个结构体其它字段都是这样.
    int64_t stop_time;
    uint64_t limit_filesize;    // -fs选项，设置文件大小限制
    float mux_preload;
    float mux_max_delay;        // -muxdelay选项，在init_options()看到默认0.7
    int shortest;               // -shortest选项
    int bitexact;

    int video_disable;          // -vn选项
    int audio_disable;          // -an选项
    int subtitle_disable;
    int data_disable;

    /* indexed by output file stream index */
    int   *streamid_map;
    int nb_streamid_map;

    SpecifierOpt *metadata;     // -metadata选项
    int        nb_metadata;
    SpecifierOpt *max_frames;
    int        nb_max_frames;
    SpecifierOpt *bitstream_filters;
    int        nb_bitstream_filters;
    SpecifierOpt *codec_tags;
    int        nb_codec_tags;
    SpecifierOpt *sample_fmts;  // -sample_fmt选项
    int        nb_sample_fmts;
    SpecifierOpt *qscale;
    int        nb_qscale;
    SpecifierOpt *forced_key_frames;
    int        nb_forced_key_frames;
    SpecifierOpt *force_fps;
    int        nb_force_fps;
    SpecifierOpt *frame_aspect_ratios;      // -aspect选项
    int        nb_frame_aspect_ratios;
    SpecifierOpt *rc_overrides;
    int        nb_rc_overrides;
    SpecifierOpt *intra_matrices;
    int        nb_intra_matrices;
    SpecifierOpt *inter_matrices;
    int        nb_inter_matrices;
    SpecifierOpt *chroma_intra_matrices;
    int        nb_chroma_intra_matrices;
    SpecifierOpt *top_field_first;
    int        nb_top_field_first;
    SpecifierOpt *metadata_map;             // -map_metadata选项
    int        nb_metadata_map;
    SpecifierOpt *presets;
    int        nb_presets;
    SpecifierOpt *copy_initial_nonkeyframes;// -copyinkf选项
    int        nb_copy_initial_nonkeyframes;
    SpecifierOpt *copy_prior_start;         // -copypriorss选项,在开始时间之前复制或丢弃帧
    int        nb_copy_prior_start;
    SpecifierOpt *filters;
    int        nb_filters;
    SpecifierOpt *filter_scripts;
    int        nb_filter_scripts;
    SpecifierOpt *reinit_filters;           // -reinit_filter选项
    int        nb_reinit_filters;
    SpecifierOpt *fix_sub_duration;
    int        nb_fix_sub_duration;
    SpecifierOpt *canvas_sizes;
    int        nb_canvas_sizes;
    SpecifierOpt *pass;
    int        nb_pass;
    SpecifierOpt *passlogfiles;
    int        nb_passlogfiles;
    SpecifierOpt *max_muxing_queue_size;
    int        nb_max_muxing_queue_size;
    SpecifierOpt *guess_layout_max;
    int        nb_guess_layout_max;
    SpecifierOpt *apad;
    int        nb_apad;
    SpecifierOpt *discard;
    int        nb_discard;
    SpecifierOpt *disposition;
    int        nb_disposition;
    SpecifierOpt *program;
    int        nb_program;
    SpecifierOpt *time_bases;
    int        nb_time_bases;
    SpecifierOpt *enc_time_bases;
    int        nb_enc_time_bases;
} OptionsContext;
#endif


//自定义封装输入过滤器结构体
typedef struct InputFilter {
    AVFilterContext    *filter;         // 视频时:指向buffer.音频时:指向abuffer
    struct InputStream *ist;
    struct FilterGraph *graph;
    uint8_t            *name;
    enum AVMediaType    type;           // AVMEDIA_TYPE_SUBTITLE for sub2video

    /*
    typedef struct AVFifoBuffer {
        uint8_t *buffer;//开辟后的内存起始地址
        uint8_t *rptr, *wptr, *end;//rptr是指向可读地址，wptr指向可写地址，end指向开辟地址的末尾.初始化后rptr=wptr=buffer；
        uint32_t rndx, wndx;//初始化后默认都是0,rndx代表此次已经读取的字节数,wndx代表已经写入的字节数(看源码)
    } AVFifoBuffer;
    下面两个结论画图理解即可：
    所以: wndx - rndx就是代表还剩余可读取的字节数大小,即av_fifo_size函数的实现.
    f->end - f->buffer - av_fifo_size(f)代表fifo的剩余空间,f->end - f->buffer代表fifo队列的大小,即av_fifo_space函数的实现.
    */
    AVFifoBuffer *frame_queue;          // 输入过滤器帧队列的大小；初始化时是8帧，av_fifo_alloc(8 * sizeof(AVFrame*))。

    // parameters configured for this input(为此输入配置的参数)
    int format;                                 // 视频时:输入视频的像素格式

    int width, height;                          // 输入视频的宽高
    AVRational sample_aspect_ratio;             // 视频时:宽高比率,一般都是{0,1}

    int sample_rate;
    int channels;                               // 音频通道数
    uint64_t channel_layout;                    // 通道布局

    AVBufferRef *hw_frames_ctx;

    int eof;                                    // 解码完成遇到eof时,可能会通过send_filter_eof()置为1?(可看process_input_packet,configure_filtergraph)
} InputFilter;

typedef struct OutputFilter {
    AVFilterContext     *filter;                // 输出过滤器ctx, 视频时是:buffersink, 音频时是:abuffersink
    struct OutputStream *ost;                   // 输出流
    struct FilterGraph  *graph;                 // 指向FilterGraph封装的系统过滤器
    uint8_t             *name;

    /* temporary storage until stream maps are processed */
    AVFilterInOut       *out_tmp;
    enum AVMediaType     type;

    /* desired output stream properties */
    int width, height;                          // 分辨率，仅视频有效(see /* set the filter output constraints */)
    AVRational frame_rate;                      // 帧率(see /* set the filter output constraints */)
    int format;                                 // 视频时：保存着该编码器上下文支持的视频像素格式；
                                                // 音频时：保存着该编码器上下文支持的音频像素格式；初始化时为-1.(see /* set the filter output constraints */)

    int sample_rate;                            // 采样率，仅音频有效
    uint64_t channel_layout;                    // 通道布局，仅音频有效

    // those are only set if no format is specified and the encoder gives us multiple options
    // 只有在没有指定格式并且编码器提供多个选项的情况下才会设置这些选项
    int *formats;                               // 与format实际是一样的，不过有两个不同点：1.这里的内容是从编码器中获取；2.保存的是数组.
    uint64_t *channel_layouts;                  // 通道布局数组，仅音频有效
    int *sample_rates;                          // 采样率数组，仅音频有效
} OutputFilter;

typedef struct FilterGraph {
    int            index;                       // 过滤器下标.see init_simple_filtergraph()
    const char    *graph_desc;                  // 图形描述.为空表示是简单过滤器,不为空则不是. see filtergraph_is_simple()

    AVFilterGraph *graph;                       // 系统过滤器
    int reconfiguration;                        // =1标记配置了AVFilterGraph

    InputFilter   **inputs;                     // 输入文件过滤器描述，数组
    int          nb_inputs;                     // inputs数组的个数
    OutputFilter **outputs;                     // 输出文件过滤器描述，数组
    int         nb_outputs;                     // outputs数组的个数
} FilterGraph;

typedef struct InputStream {
    int file_index;             // 输入文件的下标.例如-i 1.mp4 -i 2.mp4,两个输入文件下标依次是0,1
    AVStream *st;
    int discard;                /* true if stream data should be discarded */
                                // =1,该流读到的包都会被丢弃。输入流有效时,在new_output_stream()初始化为0
    int user_set_discard;
    int decoding_needed;        /* non zero if the packets must be decoded in 'raw_fifo', see DECODING_FOR_* */
                                // 非0表示输入流需要进行转码操作
#define DECODING_FOR_OST    1
#define DECODING_FOR_FILTER 2

    AVCodecContext *dec_ctx;    // 解码器上下文
    AVCodec *dec;               // choose_decoder找到的解码器
    AVFrame *decoded_frame;     // 存放解码后的一帧.视频在decode_video()开辟,音频在decode_audio()
    AVFrame *filter_frame; /* a ref of decoded_frame, to be sent to filters */ ///与decoded_frame类似

    int64_t       start;     /* time when read started */ // 指定-re选项时,在transcode_init会保存转码的开始时间,单位微秒
    /* predicted dts of the next packet read for this stream or (when there are
     * several frames in a packet) of the next frame in current packet (in AV_TIME_BASE units) */
    int64_t       next_dts;  ///单位微秒
    int64_t       dts;       ///< dts of the last packet read for this stream (in AV_TIME_BASE units)
                             ///(该流读取的最后一个包的dts,单位微秒)

    int64_t       next_pts;  ///< synthetic pts for the next decode frame (in AV_TIME_BASE units)(下一个解码帧的合成PTS)
    int64_t       pts;       ///< current pts of the decoded frame  (in AV_TIME_BASE units)
    int           wrap_correction_done;     // 启用的流更正开始时间是否完成, =1表示完成, 主要处理ts流, see process_input()

    int64_t filter_in_rescale_delta_last;   // 仅音频:统计样本数?debug看到该值一直会以样本数递增,例如0-1024-2048-3072...

    int64_t min_pts; /* pts with the smallest value in a current stream */
    int64_t max_pts; /* pts with the higher value in a current stream */

    // when forcing constant input framerate through -r,
    // this contains the pts that will be given to the next decoded frame
    // 当通过-r强制恒定的输入帧率时，这包含了将被赋予下一个解码帧的PTS
    int64_t cfr_next_pts;

    int64_t nb_samples; /* number of samples in the last decoded audio frame before looping.循环前最后解码音频帧中的采样数 */

    double ts_scale;                    // -itsscale选项,默认是1.0,设置输入ts的刻度
    int saw_first_ts;                   // 是否是第一次进来的时间戳,用于处理输入流InputStream的dts/pts(see process_input_packet).
    AVDictionary *decoder_opts;         // 用户输入的解码器选项,是从o中复制的
    AVRational framerate;               /* framerate forced with -r */
    int top_field_first;                // 是否优先显示顶部字段.
                                        // 该值视频时,会在decode_video给AVFrame->top_field_first赋值(如果内容是交错的，则首先显示顶部字段)
    int guess_layout_max;

    int autorotate;                     // -autorotate选项,与旋转角度有关

    int fix_sub_duration;
    struct { /* previous decoded subtitle and related variables */
        int got_output;
        int ret;
        AVSubtitle subtitle;
    } prev_sub;

    struct sub2video {
        int64_t last_pts;
        int64_t end_pts;
        AVFifoBuffer *sub_queue;    ///< queue of AVSubtitle* before filter init
        AVFrame *frame;
        int w, h;
    } sub2video;

    int dr1;

    /* decoded data from this stream goes into all those filters
     * currently video and audio only */
    InputFilter **filters;                      // 对比输出流OutputStream可以看到，输入流可以有多个输入过滤器,
                                                // 因为输出流的filters是一级指针，而这里输入流是二级指针
    int        nb_filters;                      // filters数组元素个数

    int reinit_filters;                         // -reinit_filter选项,ifilter参数改变是否重新初始化filtergraph,
                                                // =0时参数改变不会重新初始化,非0时会.默认值为-1,所以默认会重新初始化

    /* hwaccel options */
    enum HWAccelID hwaccel_id;                  // 硬件解码器id
    enum AVHWDeviceType hwaccel_device_type;    // 硬件设备类型
    char  *hwaccel_device;
    enum AVPixelFormat hwaccel_output_format;

    /* hwaccel context */
    void  *hwaccel_ctx;
    void (*hwaccel_uninit)(AVCodecContext *s);
    int  (*hwaccel_get_buffer)(AVCodecContext *s, AVFrame *frame, int flags);
    int  (*hwaccel_retrieve_data)(AVCodecContext *s, AVFrame *frame);//硬件检索数据回调
    enum AVPixelFormat hwaccel_pix_fmt;
    enum AVPixelFormat hwaccel_retrieved_pix_fmt;//硬件检索像素格式
    AVBufferRef *hw_frames_ctx;

    /* stats */
    // combined size of all the packets read(读取的所有包的大小之和)
    uint64_t data_size;             // 已经读取的包的总字节大小
    /* number of packets successfully read for this stream */
    uint64_t nb_packets;            // 已经读取的包的总数
    // number of frames/samples retrieved from the decoder(从解码器检索到的帧/样本数)
    uint64_t frames_decoded;        // 解码器已经解码的帧数
    uint64_t samples_decoded;       // 已经解码的样本数,仅音频有效

    int64_t *dts_buffer;            // eof时,统计有几个dts被保存到该数组.see decode_video()
    int nb_dts_buffer;              // dts_buffer的大小

    int got_output;                 // =1:标记该输入流至少已经成功解码一个pkt. see process_input_packet()
} InputStream;

#ifndef TYY_POINT_THREAD
#else
typedef pthread_t pthread_t;
#endif
// 封装输入文件相关信息的结构体
typedef struct InputFile {
    AVFormatContext *ctx; // 输入文件的ctx
    int eof_reached;      /* true if eof reached */// =1: 输入文件遇到eof
    int eagain;           /* true if last read attempt returned EAGAIN(如果上次读取尝试返回EAGAIN则为真) */
                          //=1表示上一次读pkt时,返回了eagain

    int ist_index;        /* index of first stream in input_streams *///输入文件中第一个流的下标,一般为0
    int loop;             /* set number of times input stream should be looped *///循环次数.例如无限次,-stream_loop -1
    int64_t duration;     /* actual duration of the longest stream in a file
                             at the moment when looping happens(循环发生时文件中最长流的实际持续时间,即stream_loop选项) */
                          // 该输入文件中时长为最长的流的duration
    AVRational time_base; /* time base of the duration */ // 上面duration字段的时基
    int64_t input_ts_offset;  // -itsoffset选项,设置输入时间戳偏移

    int64_t ts_offset;    // 时间戳偏移地址,目前理解意思为与0相差的偏移地址,所以一般是负数值.
                          // 与input_ts_offset、copy_ts、start_at_zero、-ss选项有关,都没加默认是0.
    int64_t last_ts;      // 上一个ptk的dts
    int64_t start_time;   /* user-specified start time in AV_TIME_BASE or AV_NOPTS_VALUE */
    int seek_timestamp;
    int64_t recording_time;
    int nb_streams;       /* number of stream that ffmpeg is aware of; may be different
                             from ctx.nb_streams if new streams appear during av_read_frame() */
    int nb_streams_warn;  /* number of streams that the user was warned of(警告用户的流的数量) */
    int rate_emu;               // -re选项。从OptionsContext.rate_emu得到，用户输入-re选项，rate_emu的值为1
    int accurate_seek;

#if HAVE_THREADS
    AVThreadMessageQueue *in_thread_queue;      // 该输入文件的线程消息队列.
                                                // 多个输入文件时,会开启多个线程,每个线程通过av_read_frame读到的pkt会存放到该队列
#ifndef TYY_POINT_THREAD
    pthread_t thread;           /* thread reading from this file */
#else
    pthread_t *thread;          // tyycode
#endif
    int non_blocking;           /* reading packets from the thread should not block(从线程读取数据包不应该被阻塞) */
                                // 存在多个输入文件即多线程读取时,av_read_frame是否阻塞,0=阻塞,1=非阻塞

    int joined;                 /* the thread has been joined */ // 0=未被回收,1=已经回收该线程thread
    int thread_queue_size;      /* maximum number of queued packets */
#endif
} InputFile;

enum forced_keyframes_const {
    FKF_N,
    FKF_N_FORCED,
    FKF_PREV_FORCED_N,
    FKF_PREV_FORCED_T,
    FKF_T,
    FKF_NB
};

#define ABORT_ON_FLAG_EMPTY_OUTPUT (1 <<  0)

extern const char *const forced_keyframes_const_names[];

typedef enum {
    ENCODER_FINISHED = 1,
    MUXER_FINISHED = 2,
} OSTFinished ;

typedef struct OutputStream {
    int file_index;          /* file index */
    int index;               /* stream index in the output file */// 由oc->nb_streams - 1得到,输出流的下标
    int source_index;        /* InputStream index *///对应输入流的下标
                             /*这里说明一下，index与source_index是不一样的，前者是输出流的下标，一般是按顺序递增的。例如
                              由于在open_output_file的if (!o->nb_stream_maps)流程，输出视频流总是优先new，
                              所以输出视频流的index可以说是0，而此时若输入文件的视频流假设是1，那么source_index就是1.
                              这就是两者的区别*/

    AVStream *st;            /* stream in the output file */
    int encoding_needed;     /* true if encoding needed for this stream *///是否需要编码；0=不需要 1=需要.一般由!stream_copy得到
    int frame_number;        // 已经写帧的数量. 控制台就是打印这个值
                             // 视频:在do_video_out()统计写帧的个数;音频:在write_packet()统计写帧的个数;

    /* input pts and corresponding output pts
       for A/V sync */
    struct InputStream *sync_ist; /* input stream to sync against */
    int64_t sync_opts;       /* output frame counter, could be changed to some true timestamp */ // FIXME look at frame_number
                             /* 输出帧计数器，可以更改为一些真实的时间戳. FIXME：查看frame_number */
                             // 视频时: 代表下一帧的pts(see do_video_out()),不过是以自增的方式存储,除以帧率(保存在编码器时基)即得到对应的pts.
                             // 音频时:代表下一帧的pts(see do_audio_out()),不过是以采样点的形式存储,除以采样率即得到对应的pts.

    /* pts of the first frame encoded for this stream, used for limiting
     * recording time */
    int64_t first_pts;
    /* dts of the last packet sent to the muxer */
    int64_t last_mux_dts;       // 发送到muxer的最后一个包的DTS，即最近有效的dts,单位ost->st->time_base
    // the timebase of the packets sent to the muxer
    AVRational mux_timebase;    // 发送到muxer的数据包的时间基准
    AVRational enc_timebase;    // 由OptionsContext.enc_time_bases参数解析得到

    int                    nb_bitstream_filters;    // bsf_ctx数组大小.see init_output_bsfs()
    AVBSFContext            **bsf_ctx;              // 位流数组

    AVCodecContext *enc_ctx;    // 通过enc创建的编码器上下文
    AVCodecParameters *ref_par; /* associated input codec parameters with encoders options applied.
                                 (将输入编解码器参数与应用的编码器选项关联起来)*/

    AVCodec *enc;               // 通过choose_encoder得到的编码器
    int64_t max_frames;         // 通过OptionsContext.max_frames得到
    AVFrame *filtered_frame;    // 用于存储编码后的帧,在reap_filters时会给其开辟内存
    AVFrame *last_frame;
    int last_dropped;
    int last_nb0_frames[3];

    void  *hwaccel_ctx;

    /* video only */
    AVRational frame_rate;              // 帧率，由OptionsContext.frame_rates即-r选项得到
    int is_cfr;                         // 当format_video_sync 等于 VSYNC_CFR或者VSYNC_VSCFR时，为1.
    int force_fps;                      // -force_fps强制帧率选项.设置后不会再自动考虑编码器最好的帧率
    int top_field_first;                // ?
    int rotate_overridden;
    double rotate_override_value;

    AVRational frame_aspect_ratio;      // 对应OptionsContext.frame_aspect_ratios

    /* forced key frames */
    int64_t forced_kf_ref_pts;
    int64_t *forced_kf_pts;             // 强制关键帧pts数组.暂未深入研究
    int forced_kf_count;                // forced_kf_pts数组大小
    int forced_kf_index;                // 当前关键帧下标？
    char *forced_keyframes;
    AVExpr *forced_keyframes_pexpr;
    double forced_keyframes_expr_const_values[FKF_NB];

    /* audio only */
    int *audio_channels_map;             /* list of the channels id to pick from the source stream */
                                         // 要从源流中选择的通道id列表
    int audio_channels_mapped;           /* number of channels in audio_channels_map */

    char *logfile_prefix;
    FILE *logfile;                      // 日志文件句柄

    OutputFilter *filter;               // 指向输出过滤器
    char *avfilter;                     // 最终保存filters或者filters_script中过滤器描述的内容
    char *filters;         ///< filtergraph associated to the -filter option//与-filter选项相关联的Filtergraph
    char *filters_script;  ///< filtergraph script associated to the -filter_script option//与-filter_script选项相关联的Filtergraph脚本

    AVDictionary *encoder_opts;         // 保存着用户指定输出的编码器选项
    AVDictionary *sws_dict;             // 视频转码参数选项，一般用于avfilter滤镜相关.(最终被应用到AVFilterGraph的scale_sws_opts成员)
    AVDictionary *swr_opts;             // 音频相关配置选项.(最终会使用av_opt_set应用到AVFilterGraph的aresample_swr_opts成员)
    AVDictionary *resample_opts;        // 重采样选项.
    char *apad;
    OSTFinished finished;               /* no more packets should be written for this stream(输出流完成，则不应该再为该流写入任何信息包) */
                                        // 主要通过close_all_output_streams()/close_output_stream()/finish_output_stream()标记完成

    int unavailable;                    /* true if the steram is unavailable (possibly temporarily) */
                                        // 流是否可用,0-可用,1-不可用,循环时开始会重置它为0

    int stream_copy;                    // 是否不转码输出，例如-vcodec copy选项.0=转码 1=不转码.只要置为0，音视频都会进入转码的流程。see choose_encoder()

    // init_output_stream() has been called for this stream
    // The encoder and the bitstream filters have been initialized and the stream
    // parameters are set in the AVStream.
    int initialized;                    // =1表示init_output_stream()调用完成.see init_output_stream()

    int inputs_done;                    // =1表示输入流全部处理完成,主要用于选择输入流进行解码,see transcode_step()

    const char *attachment_filename;
    int copy_initial_nonkeyframes;      // 对应OptionsContext.copy_initial_nonkeyframes.复制最初的非关键帧
    int copy_prior_start;               // 对应OptionsContext.copy_prior_start.在开始时间之前复制或丢弃帧
    char *disposition;                  // 对应OptionsContext.disposition，即-disposition选项

    int keep_pix_fmt;                   // 当指定pix_fmt选项 且 *frame_pix_fmt == '+' 时,keep_pix_fmt=1

    /* stats */
    // combined size of all the packets written
    uint64_t data_size;                 // 写入的所有数据包的组合大小
    // number of packets send to the muxer
    uint64_t packets_written;
    // number of frames/samples sent to the encoder
    //(发送到编码器的帧/样本数)
    uint64_t frames_encoded;
    uint64_t samples_encoded;

    /* packet quality factor */
    int quality;                        // 编码质量，等价于ffmpeg命令行打印的q.在write_packet()得到

    int max_muxing_queue_size;          // 默认最大复用队列的大小为128，new_output_stream时指定

    /* the packets are buffered here until the muxer is ready to be initialized */
    AVFifoBuffer *muxing_queue;         // new_output_stream时开辟内存.

    /* packet picture type */
    int pict_type;                      // 帧类型

    /* frame encode sum of squared error values */
    int64_t error[4];                   // 用于存储编码时的错误
} OutputStream;

typedef struct OutputFile {
    AVFormatContext *ctx;   // 输出文件的解复用上下文
    AVDictionary *opts;     // 解复用选项，由o->g->format_opts拷贝得到.see open_output_file()
    int ost_index;          /* index of the first stream in output_streams */
    int64_t recording_time;  ///< desired length of the resulting file in microseconds == AV_TIME_BASE units
                            //结果文件的期望长度(以微秒为单位)== AV_TIME_BASE单位.(录像时长)

    int64_t start_time;      ///< start time in microseconds == AV_TIME_BASE units
    uint64_t limit_filesize; /* filesize limit expressed in bytes(文件大小限制，以字节为单位),-fs选项.*/

    int shortest;           // -shortest选项得到的值,=1:最短时长的流完成输出时,其它流也要关闭.

    int header_written;     // =1表示调用avformat_write_header()成功.
} OutputFile;

typedef struct BenchmarkTimeStamps {
    int64_t real_usec;          // transcode()调用前的实时时间，see av_gettime_relative().单位微秒
    int64_t user_usec;          // 进程在用户层消耗的时间.单位微秒
    int64_t sys_usec;           // 进程在内核层消耗的时间.单位微秒
} BenchmarkTimeStamps;

enum OptGroup {
    GROUP_OUTFILE,
    GROUP_INFILE,
};

static const OptionGroupDef groups[] = {
    [GROUP_OUTFILE] = { "output url",  NULL, OPT_OUTPUT },
    [GROUP_INFILE]  = { "input url",   "i",  OPT_INPUT },
};


#if 1

//typedef struct FFmpegMedia{
class FFmpegMedia{
public:
    FFmpegMedia();
    virtual ~FFmpegMedia();

private:
    void init_dynload();

public:
    BenchmarkTimeStamps get_benchmark_time_stamps(void);
    void exit_program(int ret);

public:
    // cmdutil.h func
    void init_opts();
    void finish_group(OptionParseContext *octx, int group_idx,
                             const char *arg);
    void *grow_array(void *array, int elem_size, int *size, int new_size);
    int match_group_separator(const OptionGroupDef *groups, int nb_groups,
                                     const char *opt);
    const OptionDef *find_option(const OptionDef *po, const char *name);
    void add_opt(OptionParseContext *octx, const OptionDef *opt,
                        const char *key, const char *val);
    int opt_default(void *optctx, const char *opt, const char *arg);
    const AVOption *opt_find(void *obj, const char *name, const char *unit,
                                int opt_flags, int search_flags);
    int write_option(void *optctx, const OptionDef *po, const char *opt,
                            const char *arg);
    int parse_optgroup(void *optctx, OptionGroup *g);
    double parse_number_or_die(const char *context, const char *numstr, int type,
                               double min, double max);
    int64_t parse_time_or_die(const char *context, const char *timestr,
                              int is_duration);
    int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);
    void print_error(const char *filename, int err);
    AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                               AVDictionary *codec_opts);
    AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                    AVFormatContext *s, AVStream *st, AVCodec *codec);
    int read_yesno(void);// 后续删掉
    void uninit_opts(void);
    void uninit_parse_context(OptionParseContext *octx);

    int parse_option(void *optctx, const char *opt, const char *arg,
                     const OptionDef *options);
    FILE *get_preset_file(char *filename, size_t filename_size,
                          const char *preset_name, int is_path,
                          const char *codec_name);


    // ffmpeg_opt.c
    void init_options(OptionsContext *o);
    void uninit_options(OptionsContext *o);
    int open_input_file(OptionsContext *o, const char *filename);
    static int open_input_file_ex(void* arg, OptionsContext *o, const char *filename);
//    int open_files(OptionGroupList *l, const char *inout,
//                          int (*open_file)(OptionsContext*, const char*));

    AVDictionary *strip_specifiers(AVDictionary *dict);
    AVCodec *choose_decoder(OptionsContext *o, AVFormatContext *s, AVStream *st);
    void dump_attachment(AVStream *st, const char *filename);
    void assert_file_overwrite(const char *filename);
    int open_files(OptionGroupList *l, const char *inout,
                          int (*open_file)(void*, OptionsContext*, const char*));
    void  add_input_streams(OptionsContext *o, AVFormatContext *ic);

    int open_output_file(OptionsContext *o, const char *filename);
    static int open_output_file_ex(void* arg, OptionsContext *o, const char *filename);
    void init_output_filter(OutputFilter *ofilter, OptionsContext *o,
                                   AVFormatContext *oc);
    OutputStream *new_video_stream(OptionsContext *o, AVFormatContext *oc, int source_index);
    OutputStream *new_output_stream(OptionsContext *o, AVFormatContext *oc, enum AVMediaType type, int source_index);
    int choose_encoder(OptionsContext *o, AVFormatContext *s, OutputStream *ost);
    int get_preset_file_2(const char *preset_name, const char *codec_name, AVIOContext **s);
    uint8_t *get_line(AVIOContext *s);
    void parse_matrix_coeffs(uint16_t *dest, const char *str);
    uint8_t *read_file(const char *filename);
    char *get_ost_filters(OptionsContext *o, AVFormatContext *oc,
                                 OutputStream *ost);
    void check_streamcopy_filters(OptionsContext *o, AVFormatContext *oc,
                                         const OutputStream *ost, enum AVMediaType type);

    OutputStream *new_audio_stream(OptionsContext *o, AVFormatContext *oc, int source_index);
    OutputStream *new_subtitle_stream(OptionsContext *o, AVFormatContext *oc, int source_index);
    OutputStream *new_data_stream(OptionsContext *o, AVFormatContext *oc, int source_index);
    OutputStream *new_attachment_stream(OptionsContext *o, AVFormatContext *oc, int source_index);
    OutputStream *new_unknown_stream(OptionsContext *o, AVFormatContext *oc, int source_index);

    void parse_meta_type(char *arg, char *type, int *index, const char **stream_spec);
    int copy_metadata(char *outspec, char *inspec, AVFormatContext *oc, AVFormatContext *ic, OptionsContext *o);
    int copy_chapters(InputFile *ifile, OutputFile *ofile, int copy_metadata);
    void tyy_print_AVDirnary(AVDictionary *d);

    // 选项参数设置函数
    int opt_map(void *optctx, const char *opt, const char *arg);
    int opt_map_channel(void *optctx, const char *opt, const char *arg);
    int opt_recording_timestamp(void *optctx, const char *opt, const char *arg);
    int opt_data_frames(void *optctx, const char *opt, const char *arg);
    int opt_progress(void *optctx, const char *opt, const char *arg);
    int opt_target(void *optctx, const char *opt, const char *arg);
    int opt_audio_codec(void *optctx, const char *opt, const char *arg);
    int opt_video_channel(void *optctx, const char *opt, const char *arg);
    int opt_video_standard(void *optctx, const char *opt, const char *arg);
    int opt_video_codec(void *optctx, const char *opt, const char *arg);
    int opt_subtitle_codec(void *optctx, const char *opt, const char *arg);
    int opt_data_codec(void *optctx, const char *opt, const char *arg);
    int opt_vsync(void *optctx, const char *opt, const char *arg);
    int opt_abort_on(void *optctx, const char *opt, const char *arg);
    int opt_qscale(void *optctx, const char *opt, const char *arg);
    int opt_profile(void *optctx, const char *opt, const char *arg);
    int opt_video_filters(void *optctx, const char *opt, const char *arg);
    int opt_audio_filters(void *optctx, const char *opt, const char *arg);
    int opt_filter_complex(void *optctx, const char *opt, const char *arg);
    int opt_filter_complex_script(void *optctx, const char *opt, const char *arg);
    int opt_attach(void *optctx, const char *opt, const char *arg);
    int opt_video_frames(void *optctx, const char *opt, const char *arg);
    int opt_audio_frames(void *optctx, const char *opt, const char *arg);
    //int opt_data_frames(void *optctx, const char *opt, const char *arg);
    int opt_sameq(void *optctx, const char *opt, const char *arg);
    int opt_timecode(void *optctx, const char *opt, const char *arg);
    int opt_vstats_file(void *optctx, const char *opt, const char *arg);
    int opt_vstats(void *optctx, const char *opt, const char *arg);
    int opt_old2new(void *optctx, const char *opt, const char *arg);
    int opt_bitrate(void *optctx, const char *opt, const char *arg);
    int opt_streamid(void *optctx, const char *opt, const char *arg);
    int show_hwaccels(void *optctx, const char *opt, const char *arg);
    int opt_audio_qscale(void *optctx, const char *opt, const char *arg);
    int opt_default_new(OptionsContext *o, const char *opt, const char *arg);
    int opt_channel_layout(void *optctx, const char *opt, const char *arg);
    int opt_sdp_file(void *optctx, const char *opt, const char *arg);
    int opt_preset(void *optctx, const char *opt, const char *arg);
    int opt_init_hw_device(void *optctx, const char *opt, const char *arg);
    int opt_filter_hw_device(void *optctx, const char *opt, const char *arg);
    int opt_timelimit(void *optctx, const char *opt, const char *arg);

    // tyy wrapper code
    static int opt_map_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_map_channel_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_recording_timestamp_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_data_frames_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_progress_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_target_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_audio_codec_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_video_channel_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_video_standard_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_video_codec_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_subtitle_codec_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_data_codec_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_vsync_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_abort_on_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_qscale_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_profile_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_video_filters_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_audio_filters_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_filter_complex_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_filter_complex_script_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_attach_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_video_frames_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_audio_frames_ex(void * t, void *optctx, const char *opt, const char *arg);
    //int opt_data_frames(void *optctx, const char *opt, const char *arg);
    static int opt_sameq_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_timecode_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_vstats_file_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_vstats_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_old2new_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_bitrate_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_streamid_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int show_hwaccels_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_audio_qscale_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_default_new_ex(void * t, OptionsContext *o, const char *opt, const char *arg);
    static int opt_channel_layout_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_sdp_file_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_preset_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_init_hw_device_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_filter_hw_device_ex(void * t, void *optctx, const char *opt, const char *arg);
    static int opt_timelimit_ex(void * t, void *optctx, const char *opt, const char *arg);


    // ffmpeg_filters.c
    int init_simple_filtergraph(InputStream *ist, OutputStream *ost);
    void check_filter_outputs(void);
    int filtergraph_is_simple(FilterGraph *fg);

    // ffmpeg.c
    AVCodec *find_codec_or_die(const char *name, enum AVMediaType type, int encoder);
    void remove_avoptions(AVDictionary **a, AVDictionary *b);
    void assert_avoptions(AVDictionary *m);
    int guess_input_channel_layout(InputStream *ist);
    void abort_codec_experimental(AVCodec *c, int encoder);
    InputStream *get_input_stream(OutputStream *ost);
    void set_encoder_id(OutputFile *of, OutputStream *ost);
    void init_encoder_time_base(OutputStream *ost, AVRational default_time_base);
    void parse_forced_key_frames(char *kf, OutputStream *ost,
                                        AVCodecContext *avctx);
    static int compare_int64(const void *a, const void *b);
    int init_output_stream_encode(OutputStream *ost);
    int init_output_stream_streamcopy(OutputStream *ost);
    int init_output_bsfs(OutputStream *ost);
    void print_sdp(void);
    int check_init_output_file(OutputFile *of, int file_index);
    int init_output_stream(OutputStream *ost, char *error, int error_len);


    //ffmpeg_hw.c
    char *hw_device_default_name(enum AVHWDeviceType type);
    HWDevice *hw_device_add(void);
    int hw_device_init_from_type(enum AVHWDeviceType type,
                                        const char *device,
                                        HWDevice **dev_out);
    HWDevice *hw_device_get_by_type(enum AVHWDeviceType type);
    HWDevice *hw_device_match_by_codec(const AVCodec *codec);
    int hw_device_setup_for_decode(InputStream *ist);
    int hw_device_setup_for_encode(OutputStream *ost);
    HWDevice *hw_device_get_by_name(const char *name);
    void hw_device_free_all(void);
    int hw_device_init_from_string(const char *arg, HWDevice **dev_out);


    // tyy func
    void init_g_options();
    void memset_zero_fm();

public:
    // ffmpeg.c
    int start();
    void prepare_app_arguments(int *argc_ptr, char ***argv_ptr);

    void init_parse_context(OptionParseContext *octx,
                                   const OptionGroupDef *groups, int nb_groups);
    int split_commandline(OptionParseContext *octx, int argc, char *argv[],
                          const OptionDef *options,
                          const OptionGroupDef *groups, int nb_groups);
    int ffmpeg_parse_options(int argc, char **argv);

    // 主要与转码有关
    static int hwaccel_retrieve_data(AVCodecContext *avctx, AVFrame *input);
    static int hwaccel_decode_init(AVCodecContext *avctx);
    static enum AVPixelFormat get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts);
    static int get_buffer(AVCodecContext *s, AVFrame *frame, int flags);
    int init_input_stream(int ist_index, char *error, int error_len);


    int transcode_init(void);
#ifdef HAVE_THREADS
//#ifdef false
    static void *input_thread(void *arg);// pthread_create的线程回调函数
    void free_input_thread(int i);
    void free_input_threads(void);
    int init_input_thread(int i);
    int init_input_threads(void);
    int get_input_packet_mt(InputFile *f, AVPacket *pkt);
#endif

    int read_key(void);
    void set_tty_echo(int on);
    int check_keyboard_interaction(int64_t cur_time);
    void close_output_stream(OutputStream *ost);
    int need_output(void);

    OutputStream *choose_output(void);
    int got_eagain(void);
    void reset_eagain(void);
    int ifilter_has_all_input_formats(FilterGraph *fg);
    void cleanup_filtergraph(FilterGraph *fg);
    int sub2video_prepare(InputStream *ist, InputFilter *ifilter);
    double get_rotation(AVStream *st);
    int configure_input_video_filter(FilterGraph *fg, InputFilter *ifilter,
                                            AVFilterInOut *in);
    int insert_filter(AVFilterContext **last_filter, int *pad_idx,
                             const char *filter_name, const char *args);
    int insert_trim(int64_t start_time, int64_t duration,
                           AVFilterContext **last_filter, int *pad_idx,
                           const char *filter_name);
    int configure_input_audio_filter(FilterGraph *fg, InputFilter *ifilter,
                                            AVFilterInOut *in);
    int configure_input_filter(FilterGraph *fg, InputFilter *ifilter,
                                      AVFilterInOut *in);
    const enum AVPixelFormat *get_compliance_unofficial_pix_fmts(enum AVCodecID codec_id, const enum AVPixelFormat default_formats[]);
    enum AVPixelFormat choose_pixel_fmt(AVStream *st, AVCodecContext *enc_ctx, AVCodec *codec, enum AVPixelFormat target);
    char *choose_pix_fmts(OutputFilter *ofilter);
    int configure_output_video_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out);
    char *choose_sample_fmts_ex (OutputFilter *ofilter);
    char *choose_sample_rates_ex (OutputFilter *ofilter);
    char *choose_channel_layouts_ex (OutputFilter *ofilter);
    int configure_output_audio_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out);
    int configure_output_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out);
    int sub2video_get_blank_frame(InputStream *ist);
    void sub2video_copy_rect(uint8_t *dst, int dst_linesize, int w, int h,
                                    AVSubtitleRect *r);
    void sub2video_push_ref(InputStream *ist, int64_t pts);
    void sub2video_update(InputStream *ist, AVSubtitle *sub);
    int configure_filtergraph(FilterGraph *fg);

    int check_recording_time(OutputStream *ost);
    void update_benchmark(const char *fmt, ...);
    static char* av_err2str_ex(char *buf, int errnum);
    static char* av_ts2str_ex(char *buf, int64_t ts);
    static char* av_ts2timestr_ex(char *buf, int64_t ts, AVRational *tb);
    static char* av_ts2timestr_ex(char *buf, int64_t ts, AVRational tb);
    void close_all_output_streams(OutputStream *ost, OSTFinished this_stream, OSTFinished others);
    void write_packet(OutputFile *of, AVPacket *pkt, OutputStream *ost, int unqueue);
    void output_packet(OutputFile *of, AVPacket *pkt,
                              OutputStream *ost, int eof);
    double psnr(double d);
    void do_video_stats(OutputStream *ost, int frame_size);
    void do_video_out(OutputFile *of,
                             OutputStream *ost,
                             AVFrame *next_picture,
                             double sync_ipts);
    void do_audio_out(OutputFile *of, OutputStream *ost,
                             AVFrame *frame);
    int reap_filters(int flush);
    int transcode_from_filter(FilterGraph *graph, InputStream **best_ist);
    int get_input_packet(InputFile *f, AVPacket *pkt);
    int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt);
    void check_decode_result(InputStream *ist, int *got_output, int ret);
    int ifilter_parameters_from_frame(InputFilter *ifilter, const AVFrame *frame);
    int ifilter_send_frame(InputFilter *ifilter, AVFrame *frame);
    int send_frame_to_filters(InputStream *ist, AVFrame *decoded_frame);
    int decode_audio(InputStream *ist, AVPacket *pkt, int *got_output,
                            int *decode_failed);
    int decode_video(InputStream *ist, AVPacket *pkt, int *got_output, int64_t *duration_pts, int eof,
                            int *decode_failed);
    void sub2video_flush(InputStream *ist);
    int check_output_constraints(InputStream *ist, OutputStream *ost);
    void do_subtitle_out(OutputFile *of,
                                OutputStream *ost,
                                AVSubtitle *sub);
    int transcode_subtitles(InputStream *ist, AVPacket *pkt, int *got_output,
                                   int *decode_failed);
    void ifilter_parameters_from_codecpar(InputFilter *ifilter, AVCodecParameters *par);
    int ifilter_send_eof(InputFilter *ifilter, int64_t pts);
    int send_filter_eof(InputStream *ist);
    void do_streamcopy(InputStream *ist, OutputStream *ost, const AVPacket *pkt);
    int process_input_packet(InputStream *ist, const AVPacket *pkt, int no_eof);
    AVRational duration_max(int64_t tmp, int64_t *duration, AVRational tmp_time_base,
                                   AVRational time_base);
    int seek_to_start(InputFile *ifile, AVFormatContext *is);
    void finish_output_stream(OutputStream *ost);
    void report_new_stream(int input_index, AVPacket *pkt);
    void sub2video_heartbeat(InputStream *ist, int64_t pts);
    int process_input(int file_index);
    int transcode_step(void);

    void print_final_stats(int64_t total_size);
    void print_report(int is_last_report, int64_t timer_start, int64_t cur_time);

    void flush_encoders(void);
    void term_exit_sigsafe(void);
    void term_exit(void);
    int64_t getmaxrss(void);
    void ffmpeg_cleanup(int ret);

    int transcode(void);


public:
    // ffmpeg.h global var
    InputStream **input_streams;         /*二维数组，用于保存每一个InputStream *输入文件里面的各个流，例如保存了视频流+音频流
    那么input_streams[0]、input_streams[1]就是对应音视频流的信息*/
    int        nb_input_streams;         // input_streams二维数组大小

    InputFile   **input_files;           // 用于保存多个输入文件
    int        nb_input_files;           // 输入文件个数

    OutputStream **output_streams;       // 保存各个输出流的数组
    int         nb_output_streams;       // output_streams二维数组大小

    OutputFile   **output_files;         // 用于保存多个输出文件
    int         nb_output_files;         // 输出文件个数

    FilterGraph **filtergraphs;          // 封装好的系统过滤器数组，每个FilterGraph都会包含对应输入流与输出流的的输入输出过滤器。可看init_simple_filtergraph函数
    int        nb_filtergraphs;          // filtergraphs数组的大小

    //char *vstats_filename;               //  由-vstats_file选项管理,设置后可以将视频流的相关信息保存到文件
    //char *sdp_filename;

    //float audio_drift_threshold;         // -adrift_threshold选项,默认0.1
    //float dts_delta_threshold;           // -dts_delta_threshold选项,默认10,时间戳不连续增量阈值
    //float dts_error_threshold;

    //int audio_volume;                    // -vol选项,默认256
    //int audio_sync_method;               // -async选项,音频同步方法.默认0
    //int video_sync_method;               // -vsync选项,视频同步方法,默认-1,表示自动
    //float frame_drop_threshold;
    //int do_benchmark;
    //int do_benchmark_all;                // -benchmark_all选项，默认0
    //int do_deinterlace;                  // -deinterlace选项,默认0.
    //int do_hex_dump;                     // -hex选项,当指定-dump选项dump pkt,也dump payload
    //int do_pkt_dump;                     // -dump选项,默认0
    //int copy_ts;                         // -copyts选项，默认0
    //int start_at_zero;                   // -start_at_zero选项，默认0
    //int copy_tb;
    //int debug_ts;                        // 是否打印相关时间戳.-debug_ts选项
    //int exit_on_error;                   // -xerror选项,默认是0
    //int abort_on_flags;
    //int print_stats;                     // -stats选项,默认-1,在编码期间打印进度报告.
    //int qp_hist;                         // -qphist选项,默认0,显示QP直方图
    //int stdin_interaction;
    //int frame_bits_per_raw_sample;
    AVIOContext *progress_avio;
    //float max_error_rate;
    char *videotoolbox_pixfmt = NULL;

    //int filter_nbthreads;                // -filter_threads选项,默认0,非复杂过滤器线程数.
    //int filter_complex_nbthreads;        // -filter_complex_threads选项,默认0,复杂过滤器线程数.
    //int vstats_version;                  // -vstats_version选项, 默认2, 要使用的vstats格式的版本

    const AVIOInterruptCB int_cb;

    //OptionDef options[178];
    OptionDef *options = NULL;             // 指向ffmpeg的所有选项数组
    //const HWAccel hwaccels[];
    AVBufferRef *hw_device_ctx = NULL;
    #if CONFIG_QSV
    char *qsv_device;
    #endif
    HWDevice *filter_hw_device = NULL;          // -filter_hw_device选项


public:
    // ffmpeg.c global var
    int want_sdp;
    BenchmarkTimeStamps current_time;

    int64_t decode_error_stat[2];// 记录解码的状态.下标0记录的是成功解码的参数,下标1是失败的次数.
    int main_return_code = 0;
    volatile int received_nb_signals = 0;
    const char program_name[24] = "ffmpeg";

    volatile int received_sigterm = 0;
    //volatile int received_nb_signals = 0;
    //static atomic_int transcode_init_done = ATOMIC_VAR_INIT(0);
    std::atomic<int> transcode_init_done;
    volatile int ffmpeg_exited = 0;
    //int main_return_code = 0;

    int run_as_daemon  = 0;  // 0=前台运行；1=后台运行
    int nb_frames_dup = 0;
    unsigned dup_warning = 1000;
    int nb_frames_drop = 0;
    //int64_t decode_error_stat[2];// 记录解码的状态.下标0记录的是成功解码的参数,下标1是失败的次数.

    // ffmpeg_cmdutil.h global var
    //const char program_name[];
    const int program_birth_year;
    AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
    AVFormatContext *avformat_opts = NULL;
    AVDictionary *sws_dict = NULL;
    AVDictionary *swr_opts = NULL;
    AVDictionary *format_opts = NULL, *codec_opts = NULL, *resample_opts = NULL;
    int hide_banner;
    //atomic_int transcode_init_done = ATOMIC_VAR_INIT(0);
    //std::atomic<int> transcode_init_done;

    FILE *vstats_file = NULL;

    uint8_t *subtitle_out = NULL;

    // ffmpeg_opt.c global var
    //float max_error_rate  = 2.0/3;
    char *vstats_filename = NULL;
    char *sdp_filename = NULL;

    float audio_drift_threshold = 0.1;
    float dts_delta_threshold   = 10;
    float dts_error_threshold   = 3600*30;  // dts错误阈值,默认30小时.帧能被解码的最大阈值大小？

    int audio_volume      = 256;            // -vol选项,默认256
    int audio_sync_method = 0;              // 音频同步方法.默认0
    int video_sync_method = VSYNC_AUTO;
    float frame_drop_threshold = 0;
    int do_deinterlace    = 0;              // -deinterlace选项,默认0.
    int do_benchmark      = 0;
    int do_benchmark_all  = 0;              // -benchmark_all选项，默认0
    int do_hex_dump       = 0;
    int do_pkt_dump       = 0;
    int copy_ts           = 0;              // copy timestamps,默认0
    int start_at_zero     = 0;              // 使用copy_ts时，将输入时间戳移至0开始,默认0
    int copy_tb           = -1;
    int debug_ts          = 0;              // 是否打印相关时间戳.-debug_ts选项
    int exit_on_error     = 0;              // xerror选项,默认是0
    int abort_on_flags    = 0;
    int print_stats       = -1;             // -stats选项,默认-1,在编码期间打印进度报告.
    int qp_hist           = 0;              // -qphist选项,默认0,显示QP直方图
    int stdin_interaction = 1;              // 可认为是否是交互模式.除pipe、以及/dev/stdin值为0，其它例如普通文件、实时流都是1
    int frame_bits_per_raw_sample = 0;
    float max_error_rate  = 2.0/3;
    int filter_nbthreads = 0;               // -filter_threads选项,默认0,非复杂过滤器线程数.
    int filter_complex_nbthreads = 0;       // -filter_complex_threads选项,默认0,复杂过滤器线程数.
    int vstats_version = 2;                 // -vstats_version选项, 默认2, 要使用的vstats格式的版本


    int intra_only         = 0;
    int file_overwrite     = 0;      // -y选项，重写输出文件，即覆盖该输出文件。0=不重写，1=重写，
                                            // 不过为0时且no_file_overwrite=0时会在终端询问用户是否重写
    int no_file_overwrite  = 0;      // -n选项，不重写输出文件。0=不重写，1=重写
    int do_psnr            = 0;
    int input_sync;
    int input_stream_potentially_available = 0;// 标记，=1代表该输入文件可能是可用的
    int ignore_unknown_streams = 0;
    int copy_unknown_streams = 0;
    int find_stream_info = 1;


    // ffmpeg_cmdutil.c global var
    char** win32_argv_utf8 = NULL;
    int win32_argc = 0;

    // ffmpeg_hw.c global var
    int nb_hw_devices;           // 用户电脑支持的硬件设备数
    HWDevice **hw_devices;       // 用户电脑支持的硬件设备数组

    // tyy gloabl var
    int argc = 0;
    char **argv = NULL;

    // tyy code
public:
    void fm_set_input_filename(const char* filename);
    void fm_set_output_filename(const char* filename);

private:
    // 输入相关参数
    std::string _input_filename;

    // 输出相关参数
    std::string _output_filename;
};
//extern const OptionDef options[];
// 硬件的后续完善
extern const HWAccel hwaccels[];

#else
extern InputStream **input_streams;         /*二维数组，用于保存每一个InputStream *输入文件里面的各个流，例如保存了视频流+音频流
那么input_streams[0]、input_streams[1]就是对应音视频流的信息*/
extern int        nb_input_streams;         // input_streams二维数组大小

extern InputFile   **input_files;           // 用于保存多个输入文件
extern int        nb_input_files;           // 输入文件个数

extern OutputStream **output_streams;       // 保存各个输出流的数组
extern int         nb_output_streams;       // output_streams二维数组大小

extern OutputFile   **output_files;         // 用于保存多个输出文件
extern int         nb_output_files;         // 输出文件个数

extern FilterGraph **filtergraphs;          // 封装好的系统过滤器数组，每个FilterGraph都会包含对应输入流与输出流的的输入输出过滤器。可看init_simple_filtergraph函数
extern int        nb_filtergraphs;          // filtergraphs数组的大小

extern char *vstats_filename;               //  由-vstats_file选项管理,设置后可以将视频流的相关信息保存到文件
extern char *sdp_filename;

extern float audio_drift_threshold;         // -adrift_threshold选项,默认0.1
extern float dts_delta_threshold;           // -dts_delta_threshold选项,默认10,时间戳不连续增量阈值
extern float dts_error_threshold;

extern int audio_volume;                    // -vol选项,默认256
extern int audio_sync_method;               // -async选项,音频同步方法.默认0
extern int video_sync_method;               // -vsync选项,视频同步方法,默认-1,表示自动
extern float frame_drop_threshold;
extern int do_benchmark;
extern int do_benchmark_all;                // -benchmark_all选项，默认0
extern int do_deinterlace;                  // -deinterlace选项,默认0.
extern int do_hex_dump;                     // -hex选项,当指定-dump选项dump pkt,也dump payload
extern int do_pkt_dump;                     // -dump选项,默认0
extern int copy_ts;                         // -copyts选项，默认0
extern int start_at_zero;                   // -start_at_zero选项，默认0
extern int copy_tb;
extern int debug_ts;                        // 是否打印相关时间戳.-debug_ts选项
extern int exit_on_error;                   // -xerror选项,默认是0
extern int abort_on_flags;
extern int print_stats;                     // -stats选项,默认-1,在编码期间打印进度报告.
extern int qp_hist;                         // -qphist选项,默认0,显示QP直方图
extern int stdin_interaction;
extern int frame_bits_per_raw_sample;
extern AVIOContext *progress_avio;
extern float max_error_rate;
extern char *videotoolbox_pixfmt;

extern int filter_nbthreads;                // -filter_threads选项,默认0,非复杂过滤器线程数.
extern int filter_complex_nbthreads;        // -filter_complex_threads选项,默认0,复杂过滤器线程数.
extern int vstats_version;                  // -vstats_version选项, 默认2, 要使用的vstats格式的版本

extern const AVIOInterruptCB int_cb;

extern const OptionDef options[];
extern const HWAccel hwaccels[];
extern AVBufferRef *hw_device_ctx;
#if CONFIG_QSV
extern char *qsv_device;
#endif
extern HWDevice *filter_hw_device;          // -filter_hw_device选项

#endif

//void term_init(void);
//void term_exit(void);

////void reset_options(OptionsContext *o, int is_input);
//void show_usage(void);

//void opt_output_file(void *optctx, const char *filename);

//void remove_avoptions(AVDictionary **a, AVDictionary *b);
//void assert_avoptions(AVDictionary *m);

//int guess_input_channel_layout(InputStream *ist);

//enum AVPixelFormat choose_pixel_fmt(AVStream *st, AVCodecContext *avctx, AVCodec *codec, enum AVPixelFormat target);
//void choose_sample_fmt(AVStream *st, AVCodec *codec);

//int configure_filtergraph(FilterGraph *fg);
//int configure_output_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out);
//void check_filter_outputs(void);
//int ist_in_filtergraph(FilterGraph *fg, InputStream *ist);
//int filtergraph_is_simple(FilterGraph *fg);
//int init_simple_filtergraph(InputStream *ist, OutputStream *ost);
//int init_complex_filtergraph(FilterGraph *fg);

//void sub2video_update(InputStream *ist, AVSubtitle *sub);

//int ifilter_parameters_from_frame(InputFilter *ifilter, const AVFrame *frame);

//int ffmpeg_parse_options(int argc, char **argv);

//int videotoolbox_init(AVCodecContext *s);
//int qsv_init(AVCodecContext *s);
//int cuvid_init(AVCodecContext *s);

//HWDevice *hw_device_get_by_name(const char *name);
//int hw_device_init_from_string(const char *arg, HWDevice **dev);
//void hw_device_free_all(void);

//int hw_device_setup_for_decode(InputStream *ist);
//int hw_device_setup_for_encode(OutputStream *ost);

//int hwaccel_decode_init(AVCodecContext *avctx);



#endif // FFMPEG_H
