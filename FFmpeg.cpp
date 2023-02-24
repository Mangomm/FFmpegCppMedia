#ifdef __cplusplus

//fixme: 解决PRIx64报错: invalid suffix on literal; c++ xxx的问题
//see https://blog.csdn.net/e891377/article/details/127028284
#define __STDC_FORMAT_MACROS

//fixme: 修复InitializeSRWLock等api没有声明的问题
#define _WIN32_WINNT 0x0600
//#include <synchapi.h>

//ffmpeg.c的头文件
extern "C"{
#include "config.h"
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
//#include <stdatomic.h>
#include <stdint.h>
#include <inttypes.h>

#include <libavutil/eval.h>

#if HAVE_IO_H
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/fifo.h"
#include "libavutil/hwcontext.h"
#include "libavutil/internal.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/dict.h"
#include "libavutil/display.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/avstring.h"
#include "libavutil/libm.h"
#include "libavutil/imgutils.h"
#include "libavutil/timestamp.h"
#include "libavutil/bprint.h"
#include "libavutil/time.h"
#include "libavutil/thread.h"
#include "libavutil/threadmessage.h"
#include <libavutil/avutil.h>
#include "libavcodec/mathops.h"
#include "libavformat/os_support.h"

# include "libavfilter/avfilter.h"
# include "libavfilter/buffersrc.h"
# include "libavfilter/buffersink.h"

#if HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#elif HAVE_GETPROCESSTIMES
#include <windows.h>
#endif
#if HAVE_GETPROCESSMEMORYINFO
#include <windows.h>
#include <psapi.h>
#endif
#if HAVE_SETCONSOLECTRLHANDLER
#include <windows.h>
#endif


#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_TERMIOS_H
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#elif HAVE_KBHIT
#include <conio.h>
#endif

#include <time.h>

//#include "ffmpeg.h"
#include "cmdutils.h"

#include "libavutil/avassert.h"
}

#include "ffmpeg.h"// 不要放进extern，因为ffmpeg.h有C++的模板头文件,C编译器编不过去
#endif

using namespace HCMFFmpegMedia;

// ffmpeg.c
const char *const HCMFFmpegMedia::forced_keyframes_const_names[] = {
    "n",
    "n_forced",
    "prev_forced_n",
    "prev_forced_t",
    "t",
    NULL
};


// ffmpeg_opt.c
#define DEFAULT_PASS_LOGFILENAME_PREFIX "ffmpeg2pass"

const HWAccel HCMFFmpegMedia::hwaccels[] = {
#if CONFIG_VIDEOTOOLBOX
    { "videotoolbox", videotoolbox_init, HWACCEL_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX },
#endif
#if CONFIG_LIBMFX
    { "qsv",   qsv_init,   HWACCEL_QSV,   AV_PIX_FMT_QSV },
#endif
#if CONFIG_CUVID
    { "cuvid", cuvid_init, HWACCEL_CUVID, AV_PIX_FMT_CUDA },
#endif
    { 0 },
};


//int FFmpegMedia::_want_sdp = 1;
int decode_interrupt_cb(void *ctx)
{
    //return received_nb_signals > atomic_load(&transcode_init_done);

    FFmpegMedia *fm = (FFmpegMedia*)ctx;
    if(!fm){
        return -1;
    }
    return fm->received_nb_signals > fm->transcode_init_done;
}
//const AVIOInterruptCB int_cb = { decode_interrupt_cb, NULL };


FFmpegMedia::FFmpegMedia() :
    //int_cb({ decode_interrupt_cb, NULL }),
    int_cb({ decode_interrupt_cb, this }),
    program_birth_year(2000)
{
    memset_zero_fm();
    init_g_options();
    transcode_init_done = 0;
}

FFmpegMedia::FFmpegMedia(Queue<FMMessage*> *msgqueue) :
    int_cb({ decode_interrupt_cb, this }),
    program_birth_year(2000)
{
    memset_zero_fm();
    init_g_options();
    transcode_init_done = 0;
    _msg_queue = msgqueue;
}

FFmpegMedia::~FFmpegMedia(){
    if(options){
        delete options;
        options = NULL;
    }

    //_msg_queue->clear();// 不能在这里处理,因为还有消息但未处理,会影响其它正在使用该队列的对象.
}

void FFmpegMedia::init_dynload(){
#ifdef _WIN32
    /* Calling SetDllDirectory with the empty string (but not NULL) removes the
     * current working directory from the DLL search path as a security pre-caution. */
    SetDllDirectory((LPCWSTR)"");
#endif
}

BenchmarkTimeStamps FFmpegMedia::get_benchmark_time_stamps(void){
    // 1. 获取实时时间
    BenchmarkTimeStamps time_stamps = { av_gettime_relative() };
#if HAVE_GETRUSAGE
    struct rusage rusage;

    getrusage(RUSAGE_SELF, &rusage);
    time_stamps.user_usec =
        (rusage.ru_utime.tv_sec * 1000000LL) + rusage.ru_utime.tv_usec;
    time_stamps.sys_usec =
        (rusage.ru_stime.tv_sec * 1000000LL) + rusage.ru_stime.tv_usec;
#elif HAVE_GETPROCESSTIMES
    HANDLE proc;
    FILETIME c, e, k, u;
    /*
     * 获取当前进程的句柄，它是一个伪句柄，不需要回收，因为是伪句柄，不是真正的句柄
     * GetCurrentProcess() see: https://blog.csdn.net/zhangjinqing1234/article/details/6449844
    */
    proc = GetCurrentProcess();
    /*
     * GetProcessTimes() : 获取进程的创建时间、结束时间、内核时间、用户时间,
     * see: http://www.vbgood.com/api-getprocesstimes.html
     * FILETIME结构 :FILETIME 结构表示自 1601 年 1 月 1 日以来的 100 纳秒为间隔数(单位为100纳秒),
     *                  结构包含组合在一起形成一个 64 位值的两个 32 位值。
     * see: https://blog.csdn.net/vincen1989/article/details/7868879
     *
     * 除以10是因为要将100ns(纳秒)转成μs(微秒).具体转换：
     * 1s=10^6μs;
     * 1s=10^9ns;
     * 那么，10^6μs=10^9ns;  得出，1μs=1000ns.
     * 设100ns的单位是hs，那么有：
     * 1μs=10hs;
     * 故，从获取到的时间即(int64_t)u.dwHighDateTime << 32 | u.dwLowDateTime)设为x，单位100ns即hs，
     * 转化后的结果是y，单位是微秒，那么由它们的比是一样的得到表达式：
     * 1μs=10hs;yμs=xhs; ==> 1/y=10/x; ==> x=10y; ==> y=x/10
     * 这就是下面除以10的原因.
    */
    GetProcessTimes(proc, &c, &e, &k, &u);
    time_stamps.user_usec =
        ((int64_t)u.dwHighDateTime << 32 | u.dwLowDateTime) / 10;
    time_stamps.sys_usec =
        ((int64_t)k.dwHighDateTime << 32 | k.dwLowDateTime) / 10;
#else
    time_stamps.user_usec = time_stamps.sys_usec = 0;
#endif
    return time_stamps;
}

int64_t FFmpegMedia::getmaxrss(void)
{
#if HAVE_GETRUSAGE && HAVE_STRUCT_RUSAGE_RU_MAXRSS
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    return (int64_t)rusage.ru_maxrss * 1024;
#elif HAVE_GETPROCESSMEMORYINFO
    HANDLE proc;
    PROCESS_MEMORY_COUNTERS memcounters;
    proc = GetCurrentProcess();
    memcounters.cb = sizeof(memcounters);
    GetProcessMemoryInfo(proc, &memcounters, sizeof(memcounters));
    return memcounters.PeakPagefileUsage;
#else
    return 0;
#endif
}

void FFmpegMedia::ffmpeg_cleanup(int ret)
{
    int i, j;

    if (do_benchmark) {
        int maxrss = getmaxrss() / 1024;
        av_log(NULL, AV_LOG_INFO, "bench: maxrss=%ikB\n", maxrss);
    }

    // 1. 释放fg数组相关内容
    for (i = 0; i < nb_filtergraphs; i++) {
        FilterGraph *fg = filtergraphs[i];
        // 1.1 释放滤波图
        avfilter_graph_free(&fg->graph);

        // 1.2 释放InputFilter数组
        for (j = 0; j < fg->nb_inputs; j++) {
            while (av_fifo_size(fg->inputs[j]->frame_queue)) {//释放帧队列剩余的帧
                AVFrame *frame;
                av_fifo_generic_read(fg->inputs[j]->frame_queue, &frame,
                                     sizeof(frame), NULL);
                av_frame_free(&frame);
            }
            av_fifo_freep(&fg->inputs[j]->frame_queue);
            if (fg->inputs[j]->ist->sub2video.sub_queue) {
                while (av_fifo_size(fg->inputs[j]->ist->sub2video.sub_queue)) {
                    AVSubtitle sub;
                    av_fifo_generic_read(fg->inputs[j]->ist->sub2video.sub_queue,
                                         &sub, sizeof(sub), NULL);
                    avsubtitle_free(&sub);
                }
                av_fifo_freep(&fg->inputs[j]->ist->sub2video.sub_queue);
            }
            av_buffer_unref(&fg->inputs[j]->hw_frames_ctx);
            av_freep(&fg->inputs[j]->name);
            av_freep(&fg->inputs[j]);
        }
        av_freep(&fg->inputs);

        // 1.3 释放OutputFilter数组
        for (j = 0; j < fg->nb_outputs; j++) {
            av_freep(&fg->outputs[j]->name);
            av_freep(&fg->outputs[j]->formats);
            av_freep(&fg->outputs[j]->channel_layouts);
            av_freep(&fg->outputs[j]->sample_rates);
            av_freep(&fg->outputs[j]);
        }
        av_freep(&fg->outputs);
        av_freep(&fg->graph_desc);

        // 1.4 释放fg元素
        av_freep(&filtergraphs[i]);
    }
    // 1.5 释放fg数组本身
    av_freep(&filtergraphs);

    av_freep(&subtitle_out);

    // 2. 关闭输出文件
    /* close files */
    for (i = 0; i < nb_output_files; i++) {
        OutputFile *of = output_files[i];
        AVFormatContext *s;
        if (!of)
            continue;
        s = of->ctx;
        if (s && s->oformat && !(s->oformat->flags & AVFMT_NOFILE))
            avio_closep(&s->pb);
        avformat_free_context(s);
        av_dict_free(&of->opts);

        av_freep(&output_files[i]);
    }

    // 3. 关闭输出流
    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost = output_streams[i];

        if (!ost)
            continue;

        for (j = 0; j < ost->nb_bitstream_filters; j++)
            av_bsf_free(&ost->bsf_ctx[j]);
        av_freep(&ost->bsf_ctx);

        av_frame_free(&ost->filtered_frame);
        av_frame_free(&ost->last_frame);
        av_dict_free(&ost->encoder_opts);

        av_freep(&ost->forced_keyframes);
        av_expr_free(ost->forced_keyframes_pexpr);
        av_freep(&ost->avfilter);
        av_freep(&ost->logfile_prefix);

        av_freep(&ost->audio_channels_map);
        ost->audio_channels_mapped = 0;

        av_dict_free(&ost->sws_dict);
        av_dict_free(&ost->swr_opts);

        avcodec_free_context(&ost->enc_ctx);// 释放编码器上下文
        avcodec_parameters_free(&ost->ref_par);

        if (ost->muxing_queue) {
            while (av_fifo_size(ost->muxing_queue)) {
                AVPacket pkt;
                av_fifo_generic_read(ost->muxing_queue, &pkt, sizeof(pkt), NULL);
                av_packet_unref(&pkt);
            }
            av_fifo_freep(&ost->muxing_queue);
        }

        av_freep(&output_streams[i]);
    }

    // 4. 回收输入线程
#if HAVE_THREADS
    free_input_threads();
#endif

    // 5. 关闭输入文件
    for (i = 0; i < nb_input_files; i++) {
        avformat_close_input(&input_files[i]->ctx);
        av_freep(&input_files[i]);
    }

    // 6. 关闭输入流
    for (i = 0; i < nb_input_streams; i++) {
        InputStream *ist = input_streams[i];

        av_frame_free(&ist->decoded_frame);
        av_frame_free(&ist->filter_frame);
        av_dict_free(&ist->decoder_opts);
        avsubtitle_free(&ist->prev_sub.subtitle);
        av_frame_free(&ist->sub2video.frame);
        av_freep(&ist->filters);
        av_freep(&ist->hwaccel_device);
        av_freep(&ist->dts_buffer);

        avcodec_free_context(&ist->dec_ctx);// 关闭解码器上下文

        av_freep(&input_streams[i]);
    }

    if (vstats_file) {
        if (fclose(vstats_file)){
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_ERROR,
                   "Error closing vstats file, loss of information possible: %s\n",
                   av_err2str_ex(str, AVERROR(errno)));
        }

    }
    av_freep(&vstats_filename);

    // 7. 关闭输入输出文件、流数组本身.
    av_freep(&input_streams);
    av_freep(&input_files);
    av_freep(&output_streams);
    av_freep(&output_files);

    uninit_opts();

    avformat_network_deinit();

    if (received_sigterm) {
        av_log(NULL, AV_LOG_INFO, "Exiting normally, received signal %d.\n",
               (int) received_sigterm);
    } else if (ret && atomic_load(&transcode_init_done)) {
        av_log(NULL, AV_LOG_INFO, "Conversion failed!\n");
    }
    term_exit();
    ffmpeg_exited = 1;
}

void FFmpegMedia::exit_program(int ret)
{
    // 1. 回收相关内容
//    if (program_exit)
//        program_exit(ret);
    ffmpeg_cleanup(ret);

    // 2. 退出进程
    av_log(NULL, AV_LOG_FATAL, "exit_program ret: %d.\n", ret);
    //print_error("test", ret);
    exit(ret);
}

int FFmpegMedia::hwaccel_retrieve_data(AVCodecContext *avctx, AVFrame *input)
{
    InputStream *ist = (InputStream *)avctx->opaque;
    AVFrame *output = NULL;
    enum AVPixelFormat output_format = ist->hwaccel_output_format;
    int err;

    if (input->format == output_format) {
        // Nothing to do.
        return 0;
    }

    output = av_frame_alloc();
    if (!output)
        return AVERROR(ENOMEM);

    output->format = output_format;

    err = av_hwframe_transfer_data(output, input, 0);
    if (err < 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to transfer data to "
               "output frame: %d.\n", err);
        goto fail;
    }

    err = av_frame_copy_props(output, input);
    if (err < 0) {
        av_frame_unref(output);
        goto fail;
    }

    av_frame_unref(input);
    av_frame_move_ref(input, output);
    av_frame_free(&output);

    return 0;

fail:
    av_frame_free(&output);
    return err;
}

int FFmpegMedia::hwaccel_decode_init(AVCodecContext *avctx)
{
    InputStream *ist = (InputStream *)avctx->opaque;

    ist->hwaccel_retrieve_data = &hwaccel_retrieve_data;

    return 0;
}

enum AVPixelFormat FFmpegMedia::get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts)
{
    InputStream *ist = (InputStream *)s->opaque;
    const enum AVPixelFormat *p;
    int ret;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(*p);
        const AVCodecHWConfig  *config = NULL;
        int i;

        if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
            break;

        if (ist->hwaccel_id == HWACCEL_GENERIC ||
            ist->hwaccel_id == HWACCEL_AUTO) {
            for (i = 0;; i++) {
                config = avcodec_get_hw_config(s->codec, i);
                if (!config)
                    break;
                if (!(config->methods &
                      AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
                    continue;
                if (config->pix_fmt == *p)
                    break;
            }
        }
        if (config) {
            if (config->device_type != ist->hwaccel_device_type) {
                // Different hwaccel offered, ignore.
                continue;
            }

            ret = hwaccel_decode_init(s);
            if (ret < 0) {
                if (ist->hwaccel_id == HWACCEL_GENERIC) {
                    av_log(NULL, AV_LOG_FATAL,
                           "%s hwaccel requested for input stream #%d:%d, "
                           "but cannot be initialized.\n",
                           av_hwdevice_get_type_name(config->device_type),
                           ist->file_index, ist->st->index);
                    return AV_PIX_FMT_NONE;
                }
                continue;
            }
        } else {
            const HWAccel *hwaccel = NULL;
            int i;
            for (i = 0; hwaccels[i].name; i++) {
                if (hwaccels[i].pix_fmt == *p) {
                    hwaccel = &hwaccels[i];
                    break;
                }
            }
            if (!hwaccel) {
                // No hwaccel supporting this pixfmt.
                continue;
            }
            if (hwaccel->id != ist->hwaccel_id) {
                // Does not match requested hwaccel.
                continue;
            }

            ret = hwaccel->init(s);
            if (ret < 0) {
                av_log(NULL, AV_LOG_FATAL,
                       "%s hwaccel requested for input stream #%d:%d, "
                       "but cannot be initialized.\n", hwaccel->name,
                       ist->file_index, ist->st->index);
                return AV_PIX_FMT_NONE;
            }
        }

        if (ist->hw_frames_ctx) {
            s->hw_frames_ctx = av_buffer_ref(ist->hw_frames_ctx);
            if (!s->hw_frames_ctx)
                return AV_PIX_FMT_NONE;
        }

        ist->hwaccel_pix_fmt = *p;
        break;
    }

    return *p;
}

int FFmpegMedia::get_buffer(AVCodecContext *s, AVFrame *frame, int flags)
{
    InputStream *ist = (InputStream *)s->opaque;

    if (ist->hwaccel_get_buffer && frame->format == ist->hwaccel_pix_fmt)
        return ist->hwaccel_get_buffer(s, frame, flags);

    return avcodec_default_get_buffer2(s, frame, flags);
}

//与hw_device_get_by_type同理
HWDevice *FFmpegMedia::hw_device_get_by_name(const char *name)
{
    int i;
    for (i = 0; i < nb_hw_devices; i++) {
        if (!strcmp(hw_devices[i]->name, name))
            return hw_devices[i];
    }
    return NULL;
}

int FFmpegMedia::hw_device_init_from_string(const char *arg, HWDevice **dev_out)
{
    // "type=name:device,key=value,key2=value2"
    // "type:device,key=value,key2=value2"
    // -> av_hwdevice_ctx_create()
    // "type=name@name"
    // "type@name"
    // -> av_hwdevice_ctx_create_derived()

    AVDictionary *options = NULL;
    const char *type_name = NULL, *name = NULL, *device = NULL;
    enum AVHWDeviceType type;
    HWDevice *dev, *src;
    AVBufferRef *device_ref = NULL;
    int err;
    const char *errmsg, *p, *q;
    size_t k;

    k = strcspn(arg, ":=@");
    p = arg + k;

    type_name = av_strndup(arg, k);
    if (!type_name) {
        err = AVERROR(ENOMEM);
        goto fail;
    }
    type = av_hwdevice_find_type_by_name(type_name);
    if (type == AV_HWDEVICE_TYPE_NONE) {
        errmsg = "unknown device type";
        goto invalid;
    }

    if (*p == '=') {
        k = strcspn(p + 1, ":@");

        name = av_strndup(p + 1, k);
        if (!name) {
            err = AVERROR(ENOMEM);
            goto fail;
        }
        if (hw_device_get_by_name(name)) {
            errmsg = "named device already exists";
            goto invalid;
        }

        p += 1 + k;
    } else {
        name = hw_device_default_name(type);
        if (!name) {
            err = AVERROR(ENOMEM);
            goto fail;
        }
    }

    if (!*p) {
        // New device with no parameters.
        err = av_hwdevice_ctx_create(&device_ref, type,
                                     NULL, NULL, 0);
        if (err < 0)
            goto fail;

    } else if (*p == ':') {
        // New device with some parameters.
        ++p;
        q = strchr(p, ',');
        if (q) {
            if (q - p > 0) {
                device = av_strndup(p, q - p);
                if (!device) {
                    err = AVERROR(ENOMEM);
                    goto fail;
                }
            }
            err = av_dict_parse_string(&options, q + 1, "=", ",", 0);
            if (err < 0) {
                errmsg = "failed to parse options";
                goto invalid;
            }
        }

        err = av_hwdevice_ctx_create(&device_ref, type,
                                     q ? device : p[0] ? p : NULL,
                                     options, 0);
        if (err < 0)
            goto fail;

    } else if (*p == '@') {
        // Derive from existing device.

        src = hw_device_get_by_name(p + 1);
        if (!src) {
            errmsg = "invalid source device name";
            goto invalid;
        }

        err = av_hwdevice_ctx_create_derived(&device_ref, type,
                                             src->device_ref, 0);
        if (err < 0)
            goto fail;
    } else {
        errmsg = "parse error";
        goto invalid;
    }

    dev = hw_device_add();
    if (!dev) {
        err = AVERROR(ENOMEM);
        goto fail;
    }

    dev->name = name;
    dev->type = type;
    dev->device_ref = device_ref;

    if (dev_out)
        *dev_out = dev;

    name = NULL;
    err = 0;
done:
    av_freep(&type_name);
    av_freep(&name);
    av_freep(&device);
    av_dict_free(&options);
    return err;
invalid:
    av_log(NULL, AV_LOG_ERROR,
           "Invalid device specification \"%s\": %s\n", arg, errmsg);
    err = AVERROR(EINVAL);
    goto done;
fail:
    av_log(NULL, AV_LOG_ERROR,
           "Device creation failed: %d.\n", err);
    av_buffer_unref(&device_ref);
    goto done;
}

void FFmpegMedia::hw_device_free_all(void)
{
    int i;
    for (i = 0; i < nb_hw_devices; i++) {
        av_freep(&hw_devices[i]->name);
        av_buffer_unref(&hw_devices[i]->device_ref);
        av_freep(&hw_devices[i]);
    }
    av_freep(&hw_devices);
    nb_hw_devices = 0;
}

char *FFmpegMedia::hw_device_default_name(enum AVHWDeviceType type)
{
    // Make an automatic name of the form "type%d".  We arbitrarily
    // limit at 1000 anonymous devices of the same type - there is
    // probably something else very wrong if you get to this limit.
    const char *type_name = av_hwdevice_get_type_name(type);
    char *name;
    size_t index_pos;
    int index, index_limit = 1000;
    index_pos = strlen(type_name);
    name = (char *)av_malloc(index_pos + 4);
    if (!name)
        return NULL;
    for (index = 0; index < index_limit; index++) {
        snprintf(name, index_pos + 4, "%s%d", type_name, index);
        if (!hw_device_get_by_name(name))
            break;
    }
    if (index >= index_limit) {
        av_freep(&name);
        return NULL;
    }
    return name;
}

HWDevice *FFmpegMedia::hw_device_add(void)
{
    int err;
    err = av_reallocp_array(&hw_devices, nb_hw_devices + 1,
                            sizeof(*hw_devices));
    if (err) {
        nb_hw_devices = 0;
        return NULL;
    }
    hw_devices[nb_hw_devices] = (HWDevice *)av_mallocz(sizeof(HWDevice));
    if (!hw_devices[nb_hw_devices])
        return NULL;
    return hw_devices[nb_hw_devices++];
}

int FFmpegMedia::hw_device_init_from_type(enum AVHWDeviceType type,
                                    const char *device,
                                    HWDevice **dev_out)
{
    AVBufferRef *device_ref = NULL;
    HWDevice *dev;
    char *name;
    int err;

    name = hw_device_default_name(type);
    if (!name) {
        err = AVERROR(ENOMEM);
        goto fail;
    }

    err = av_hwdevice_ctx_create(&device_ref, type, device, NULL, 0);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Device creation failed: %d.\n", err);
        goto fail;
    }

    dev = hw_device_add();
    if (!dev) {
        err = AVERROR(ENOMEM);
        goto fail;
    }

    dev->name = name;
    dev->type = type;
    dev->device_ref = device_ref;

    if (dev_out)
        *dev_out = dev;

    return 0;

fail:
    av_freep(&name);
    av_buffer_unref(&device_ref);
    return err;
}

///该函数的注解可能不太准确，不过意思是这样，因为笔者还没详细debug硬件相关的代码
/**
 * @brief 从用户支持的硬件设备列表中，获取指定的硬件设备。
 * @param type 指定硬件设备的类型
 * @return 找到该类型的设备，则返回硬件设备；否则返回NULL。
 * @note nb_hw_devices是用户支持的硬件设备数，为0说明不支持，直接返回NULL
*/
HWDevice *FFmpegMedia::hw_device_get_by_type(enum AVHWDeviceType type)
{
    HWDevice *found = NULL;
    int i;
    for (i = 0; i < nb_hw_devices; i++) {
        if (hw_devices[i]->type == type) {
            if (found)
                return NULL;
            found = hw_devices[i];
        }
    }
    return found;
}

/**
 * @brief 通过编解码器匹配硬件设备。
 * @param codec 编解码器
 * @return 找到返回对应的硬件设备；找不到返回NULL
*/
HWDevice *FFmpegMedia::hw_device_match_by_codec(const AVCodec *codec)
{
    const AVCodecHWConfig *config;
    HWDevice *dev;
    int i;
    /*
     * avcodec_get_hw_config(): 检索编解码器支持的硬件配置。
     * 索引值从0到某个最大值返回索引配置描述符;所有其他值返回NULL。
     * 如果编解码器不支持任何硬件配置，那么它将总是返回NULL。
     * avcodec_get_hw_config()的源码不难,主要是(以解码为例)：
     * 1)hw_configs的指向，我们通过编解码器的name字段看到，解码器是"h264"，
     * 所以在libavcodec/h264dec.c找到ff_h264_decoder，这样就知道hw_configs二维数组的指向了。
     * 只要理解hw_configs的指向，那么看这个函数就很简单了.
    */
    for (i = 0;; i++) {
        config = avcodec_get_hw_config(codec, i);
        if (!config)
            return NULL;
        if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
            continue;

        //通过类型获取硬件设备.注意，该函数就是源码函数，在本文件中可以找到.
        dev = hw_device_get_by_type(config->device_type);
        if (dev)
            return dev;
    }
}

/**
 * @brief 后续再详细分析
*/
int FFmpegMedia::hw_device_setup_for_decode(InputStream *ist)
{
    const AVCodecHWConfig *config;
    enum AVHWDeviceType type;
    HWDevice *dev = NULL;
    int err, auto_device = 0;

    if (ist->hwaccel_device) {
        dev = hw_device_get_by_name(ist->hwaccel_device);
        if (!dev) {
            if (ist->hwaccel_id == HWACCEL_AUTO) {
                auto_device = 1;
            } else if (ist->hwaccel_id == HWACCEL_GENERIC) {
                type = ist->hwaccel_device_type;
                err = hw_device_init_from_type(type, ist->hwaccel_device,
                                               &dev);
            } else {
                // This will be dealt with by API-specific initialisation
                // (using hwaccel_device), so nothing further needed here.
                return 0;
            }
        } else {
            if (ist->hwaccel_id == HWACCEL_AUTO) {
                ist->hwaccel_device_type = dev->type;
            } else if (ist->hwaccel_device_type != dev->type) {
                av_log(ist->dec_ctx, AV_LOG_ERROR, "Invalid hwaccel device "
                       "specified for decoder: device %s of type %s is not "
                       "usable with hwaccel %s.\n", dev->name,
                       av_hwdevice_get_type_name(dev->type),
                       av_hwdevice_get_type_name(ist->hwaccel_device_type));
                return AVERROR(EINVAL);
            }
        }
    } else {
        if (ist->hwaccel_id == HWACCEL_AUTO) {
            auto_device = 1;
        } else if (ist->hwaccel_id == HWACCEL_GENERIC) {
            type = ist->hwaccel_device_type;
            dev = hw_device_get_by_type(type);
            if (!dev)
                err = hw_device_init_from_type(type, NULL, &dev);
        } else {
            /*推流一般走这里，例如1.mkv的推流命令dev返回是空*/
            dev = hw_device_match_by_codec(ist->dec);
            if (!dev) {
                // No device for this codec, but not using generic hwaccel
                // and therefore may well not need one - ignore.
                // (这个编解码器没有设备，但没有使用通用的hwaccel，因此可能不需要一个 - 忽略)
                return 0;
            }
        }
    }

    if (auto_device) {
        int i;
        if (!avcodec_get_hw_config(ist->dec, 0)) {
            // Decoder does not support any hardware devices.
            return 0;
        }
        for (i = 0; !dev; i++) {
            config = avcodec_get_hw_config(ist->dec, i);
            if (!config)
                break;
            type = config->device_type;
            dev = hw_device_get_by_type(type);
            if (dev) {
                av_log(ist->dec_ctx, AV_LOG_INFO, "Using auto "
                       "hwaccel type %s with existing device %s.\n",
                       av_hwdevice_get_type_name(type), dev->name);
            }
        }
        for (i = 0; !dev; i++) {
            config = avcodec_get_hw_config(ist->dec, i);
            if (!config)
                break;
            type = config->device_type;
            // Try to make a new device of this type.
            err = hw_device_init_from_type(type, ist->hwaccel_device,
                                           &dev);
            if (err < 0) {
                // Can't make a device of this type.
                continue;
            }
            if (ist->hwaccel_device) {
                av_log(ist->dec_ctx, AV_LOG_INFO, "Using auto "
                       "hwaccel type %s with new device created "
                       "from %s.\n", av_hwdevice_get_type_name(type),
                       ist->hwaccel_device);
            } else {
                av_log(ist->dec_ctx, AV_LOG_INFO, "Using auto "
                       "hwaccel type %s with new default device.\n",
                       av_hwdevice_get_type_name(type));
            }
        }
        if (dev) {
            ist->hwaccel_device_type = type;
        } else {
            av_log(ist->dec_ctx, AV_LOG_INFO, "Auto hwaccel "
                   "disabled: no device found.\n");
            ist->hwaccel_id = HWACCEL_NONE;
            return 0;
        }
    }

    if (!dev) {
        av_log(ist->dec_ctx, AV_LOG_ERROR, "No device available "
               "for decoder: device type %s needed for codec %s.\n",
               av_hwdevice_get_type_name(type), ist->dec->name);
        return err;
    }

    ist->dec_ctx->hw_device_ctx = av_buffer_ref(dev->device_ref);
    if (!ist->dec_ctx->hw_device_ctx)
        return AVERROR(ENOMEM);

    return 0;
}

void FFmpegMedia::abort_codec_experimental(AVCodec *c, int encoder)
{
    exit_program(1);
}

/**
 * @brief 两个逻辑：
 * 1）输入流需要转码：对输入流ist做相关赋值，并调用avcodec_open2()
 * 2）输入流不需要转码：给pts、next_pts赋AV_NOPTS_VALUE()，这一步转码逻辑同样会执行
 *
 * @param ist_index 流下标
 * @param error 错误描述
 * @param error_len 错误描述长度
 *
 * @return 成功=0； 失败=负数
*/
int FFmpegMedia::init_input_stream(int ist_index, char *error, int error_len)
{
    int ret;
    InputStream *ist = input_streams[ist_index];

    /* 1.输入流需要转码的流程 */
    if (ist->decoding_needed) {
        /* 1.1获取输入流的编解码器.
         * 解码器在add_input_streams时已经赋值 */
        AVCodec *codec = ist->dec;
        if (!codec) {
            snprintf(error, error_len, "Decoder (codec %s) not found for input stream #%d:%d",
                    avcodec_get_name(ist->dec_ctx->codec_id), ist->file_index, ist->st->index);
            return AVERROR(EINVAL);
        }

        /* 1.2在add_input_streams对ist赋值的基础上，再对ist相关内容赋值 */
        ist->dec_ctx->opaque                = ist;
        ist->dec_ctx->get_format            = get_format;// 自定义获取像素格式,主要与硬件加速相关
        ist->dec_ctx->get_buffer2           = get_buffer;// 自定义获取buffer,主要与硬件加速相关
        ist->dec_ctx->thread_safe_callbacks = 1;         // 当用户自定义get_buffer,那么需要设置该值

        /*
         * 在AVCodecContext结构体中对refcounted_frames成员的解释：如果非零，则从avcodec_decode_video2()和avcodec_decode_audio4()返回的解码音
         * 频和视频帧是引用计数的，并且无限期有效。当它们不再需要时，调用者必须使用av_frame_unref()释放它们。否则，解码的帧一定不能被调用方释放，
         * 并且只有在下一次解码调用之前才有效。如果使用了avcodec_receive_frame()，这将总是自动启用。
         * -编码:未使用。
         * -解码:由调用者在avcodec_open2()之前设置。
        */
        av_opt_set_int(ist->dec_ctx, "refcounted_frames", 1, 0);

        if (ist->dec_ctx->codec_id == AV_CODEC_ID_DVB_SUBTITLE && /* dvb字幕类型 */
           (ist->decoding_needed & DECODING_FOR_OST)) {/* 该输入流需要重新解码,open_output_file有：ist->decoding_needed |= DECODING_FOR_OST */
            /* 为dvb字幕设置compute_edt选项，该选项搜compute_edt源码看到，它作用是: 使用pts或timeout计算时间结束.
            AV_DICT_DONT_OVERWRITE：若存在该key，则不覆盖已有的value */
            av_dict_set(&ist->decoder_opts, "compute_edt", "1", AV_DICT_DONT_OVERWRITE);
            if (ist->decoding_needed & DECODING_FOR_FILTER)
                av_log(NULL, AV_LOG_WARNING, "Warning using DVB subtitles for filtering and output at the same time is not fully supported, also see -compute_edt [0|1]\n");
        }

        /* 默认输入文件的字幕类型为ass? */
        av_dict_set(&ist->decoder_opts, "sub_text_format", "ass", AV_DICT_DONT_OVERWRITE);

        /* Useful for subtitles retiming by lavf (FIXME), skipping samples in
         * audio, and video decoders such as cuvid or mediacodec.
         * (适用于由lavf (FIXME)重定时的字幕，跳过音频和视频解码器，如cuvid或mediacodec)
         * 例如1.mkv，ist->st->time_base={1,1000}
        */
        ist->dec_ctx->pkt_timebase = ist->st->time_base;

        /* 若用户没指定threads选项，则默认"auto" */
        if (!av_dict_get(ist->decoder_opts, "threads", NULL, 0))
            av_dict_set(&ist->decoder_opts, "threads", "auto", 0);
        /* Attached pics are sparse, therefore we would not want to delay their decoding till EOF.
         * (附件中的图片是稀少的，因此我们不想将它们的解码延迟到EOF)
         * 附属图片，则threads重设为1.
        */
        if (ist->st->disposition & AV_DISPOSITION_ATTACHED_PIC)
            av_dict_set(&ist->decoder_opts, "threads", "1", 0);

        /* 1.3对输入流的硬件相关处理.推流没用到,后续再详细研究. */
        ret = hw_device_setup_for_decode(ist);
        if (ret < 0) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            snprintf(error, error_len, "Device setup failed for "
                     "decoder on input stream #%d:%d : %s",
                     ist->file_index, ist->st->index, av_err2str_ex(str, ret));
            return ret;
        }

        /* 1.4编解码器上下文与编解码器关联.
         * 这里看到，编解码器的相关选项是在这里被应用的 */
        if ((ret = avcodec_open2(ist->dec_ctx, codec, &ist->decoder_opts)) < 0) {
            if (ret == AVERROR_EXPERIMENTAL){
                //abort_codec_experimental(codec, 0);// 等价于exit_program(1);
                av_log(NULL, AV_LOG_FATAL, "avcodec_open2 AVERROR_EXPERIMENTAL(0).\n");
                return -1;
            }

            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            snprintf(error, error_len,
                     "Error while opening decoder for input stream "
                     "#%d:%d : %s",
                     ist->file_index, ist->st->index, av_err2str_ex(str, ret));
            return ret;
        }

        /* 1.5检测是否含有多余的选项.
         * 当avcodec_open2成功打开后，若decoder_opts包含未知选项，会返回在decoder_opts中，
         * 所以此时decoder_opts只要有一个选项，就说明是非法的 */
        assert_avoptions(ist->decoder_opts);
    }

    /* 2.输入流不需要转码的流程 */
    ist->next_pts = AV_NOPTS_VALUE;
    ist->next_dts = AV_NOPTS_VALUE;

    return 0;
}

/**
 * @brief 根据输出流ost保存对应的输入流下标，获取输入流结构体.
 * @param ost 输出流
 * @return 成功=返回输出流对应的输入流; 失败=NULL
*/
InputStream *FFmpegMedia::get_input_stream(OutputStream *ost)
{
    if (ost->source_index >= 0)
        return input_streams[ost->source_index];
    return NULL;
}

/**
 * @brief 这里会给每一个流增加一个encoder元数据选项。
 * @param of 输出文件封装结构体
 * @param ost 输出流
 */
int FFmpegMedia::set_encoder_id(OutputFile *of, OutputStream *ost)
{
    AVDictionaryEntry *e;

    uint8_t *encoder_string;
    int encoder_string_len;
    int format_flags = 0;
    int codec_flags = ost->enc_ctx->flags;

    /* 1.若ost->st->metadata已经存在encoder选项，则返回,不做处理.
     * 因为在open_output_file()时会将每一个流的元数据的encoder置为NULL,所以不会直接返回. */
    if (av_dict_get(ost->st->metadata, "encoder",  NULL, 0))
        return 0;


//    {
//        //tyycode
//        /* 测试av_opt_eval_flags():可以看到，从of-ctx找到fflags后，就是libavformat/options_table.h文件里的变量
//         * static const AVOption avformat_options[]的fflags选项. */
//        const AVOption *o = av_opt_find(of->ctx, "fflags", NULL, 0, 0);
//        if (!o)
//            return;
//        /* 假设解复用选项of->opts的fflags的value值被设为"32"，那么通过av_opt_eval_flags()后，将里面的数字会设置到format_flags中，
//         * 所以看到，format_flags的值 从0变成32.注，因为fflags是int64(具体类型需要看源码)，
//         * 所以必须是该类型是字符串，例下面"32"写成"32.2"是无法读到数字的 */
//        av_opt_eval_flags(of->ctx, o, "32", &format_flags);
//    }

    /* 2.若用户设置了解复用的fflags选项，则进行获取，然后保存在临时变量format_flags中 */
    /* 关于AVOption模块，可参考: https://blog.csdn.net/ericbar/article/details/79872779 */
    e = av_dict_get(of->opts, "fflags", NULL, 0);
    if (e) {
        // 若解复用不支持fflags选项，则返回
        const AVOption *o = av_opt_find(of->ctx, "fflags", NULL, 0, 0);
        if (!o)
            return -1;
        /* opt_eval_funcs: 计算选项字符串。这组函数可用于计算选项字符串并从中获取数字。它们所做的事情与av_opt_set()相同，只是结果被写入调用方提供的指针 */
        av_opt_eval_flags(of->ctx, o, e->value, &format_flags);
    }

    /* 3.同理，若用户设置了编码器的flags选项，则进行获取，然后保存在临时变量codec_flags中 */
    e = av_dict_get(ost->encoder_opts, "flags", NULL, 0);
    if (e) {
        const AVOption *o = av_opt_find(ost->enc_ctx, "flags", NULL, 0, 0);
        if (!o)
            return -1;
        av_opt_eval_flags(ost->enc_ctx, o, e->value, &codec_flags);
    }

    /* 4.获取编码器名字encoder选项的长度.LIBAVCODEC_IDENT宏里面的 ## 是拼接前后两个宏参数的意思. */
    encoder_string_len = sizeof(LIBAVCODEC_IDENT) + strlen(ost->enc->name) + 2;
    encoder_string     = (uint8_t *)av_mallocz(encoder_string_len);
    if (!encoder_string){
        //exit_program(1);
        return -1;
    }

    /* 5.若format_flags和codec_flags都不包含宏AVFMT_FLAG_BITEXACT，则使用LIBAVCODEC_IDENT字符串,
        否则直接使用"Lavc ". */
    if (!(format_flags & AVFMT_FLAG_BITEXACT) && !(codec_flags & AV_CODEC_FLAG_BITEXACT)){
        // 推流走这里
        char *t1 = AV_STRINGIFY(AV_VERSION_DOT(58,100,54));//将宏转成字符串
        char *tyycode1 = LIBAVCODEC_IDENT;
        char *tyycode2 = LIBAVCODEC_IDENT " ";//这里只是在末尾加个空格
        av_strlcpy((char *)encoder_string, LIBAVCODEC_IDENT " ", encoder_string_len);
    }
    else{
        av_strlcpy((char *)encoder_string, "Lavc ", encoder_string_len);
    }
    av_strlcat((char *)encoder_string, ost->enc->name, encoder_string_len);//字符串拼接
    av_dict_set(&ost->st->metadata, "encoder",  (const char *)encoder_string,
                AV_DICT_DONT_STRDUP_VAL | AV_DICT_DONT_OVERWRITE);//最终设置到流的元数据

    {
        //tyycode
        AVDictionaryEntry *t = NULL;
        while((t = av_dict_get(ost->st->metadata, "", t, AV_DICT_IGNORE_SUFFIX))){
            printf("tyy_print_AVDirnary, t->key: %s, t->value: %s\n", t->key, t->value);
        }
    }

    return 0;
}

/**
 * @brief 初始化编码器时间基,优先参考用户指定的时基,再参考输入容器的时基,最后才参考参数的默认时基。
 * @param ost 输出流
 * @param default_time_base 编码的时间基
 *
 * @note 转时基只涉及输入容器与输出容器的时间基。解复用以及复用的时间基(即输入输出容器的时间基，对应字段是AVFormatCtx->stream->time_base)。
 * 而这里的编码时间基，可认为是中间的时间基，用于运算。
 * 可参考这三篇(不一定百分百准确)：
 * https://blog.csdn.net/weixin_44517656/article/details/110452852
 * https://blog.csdn.net/weixin_44517656/article/details/110494609
 * https://blog.csdn.net/weixin_44517656/article/details/110559611
 * @return void
*/
void FFmpegMedia::init_encoder_time_base(OutputStream *ost, AVRational default_time_base)
{
    InputStream *ist = get_input_stream(ost);
    AVCodecContext *enc_ctx = ost->enc_ctx;
    AVFormatContext *oc;

    // 1.用户指定且有效，则使用用户的参数作为编码时间基
    if (ost->enc_timebase.num > 0) {
        enc_ctx->time_base = ost->enc_timebase;
        return;
    }

    // 2.用户指定且无效，那么判断输入流的流时间基(解码时间基)，若有效则使用输入容器的时间基，否则使用参数传进的默认时间基
    if (ost->enc_timebase.num < 0) {
        if (ist) {
            enc_ctx->time_base = ist->st->time_base;
            return;
        }

        // 用户传入的编码时间基非法，使用参数传进的默认时间基(看下面第三点).
        oc = output_files[ost->file_index]->ctx;
        av_log(oc, AV_LOG_WARNING, "Input stream data not available, using default time base\n");
    }

    // 3.用户没指定，则使用默认时基
    enc_ctx->time_base = default_time_base;
}

int FFmpegMedia::compare_int64(const void *a, const void *b)
{
    return FFDIFFSIGN(*(const int64_t *)a, *(const int64_t *)b);
}

/**
 * @brief 后续遇到再分析，因为比较少用
*/
int FFmpegMedia::parse_forced_key_frames(char *kf, OutputStream *ost,
                                    AVCodecContext *avctx)
{
    char *p;
    int n = 1, i, size, index = 0;
    int64_t t, *pts;

    for (p = kf; *p; p++)
        if (*p == ',')
            n++;
    size = n;
    pts = (int64_t *)av_malloc_array(size, sizeof(*pts));
    if (!pts) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate forced key frames array.\n");
        //exit_program(1);
        return -1;
    }

    p = kf;
    for (i = 0; i < n; i++) {
        char *next = strchr(p, ',');

        if (next)
            *next++ = 0;

        if (!memcmp(p, "chapters", 8)) {

            AVFormatContext *avf = output_files[ost->file_index]->ctx;
            int j;

            if (avf->nb_chapters > INT_MAX - size ||
                !(pts = (int64_t *)av_realloc_f(pts, size += avf->nb_chapters - 1,
                                     sizeof(*pts)))) {
                av_log(NULL, AV_LOG_FATAL,
                       "Could not allocate forced key frames array.\n");
                //exit_program(1);
                return -1;
            }
            t = p[8] ? parse_time_or_die("force_key_frames", p + 8, 1) : 0;
            t = av_rescale_q(t, AV_TIME_BASE_Q, avctx->time_base);

            for (j = 0; j < avf->nb_chapters; j++) {
                AVChapter *c = avf->chapters[j];
                av_assert1(index < size);
                pts[index++] = av_rescale_q(c->start, c->time_base,
                                            avctx->time_base) + t;
            }

        } else {

            t = parse_time_or_die("force_key_frames", p, 1);
            av_assert1(index < size);
            pts[index++] = av_rescale_q(t, AV_TIME_BASE_Q, avctx->time_base);

        }

        p = next;
    }

    av_assert0(index == size);
    qsort(pts, size, sizeof(*pts), compare_int64);
    ost->forced_kf_count = size;
    ost->forced_kf_pts   = pts;

    return 0;
}

/**
 * @brief 对封装的输出流结构体OutputStream进行初始化。
 *  主要是OutputStream内部相关成员以及OutputStream->enc_ctx的初始化。
 * @param ost 输出流
 * @return =0表示成功，否则为一个与AVERROR代码对应的负值 或者 程序退出
*/
int FFmpegMedia::init_output_stream_encode(OutputStream *ost)
{
    InputStream *ist = get_input_stream(ost);
    AVCodecContext *enc_ctx = ost->enc_ctx;
    AVCodecContext *dec_ctx = NULL;
    AVFormatContext *oc = output_files[ost->file_index]->ctx;// 获取输出流对应输出文件的oc
    int j, ret;

    /* 1.给输出流的元数据添加encoder选项 */
    ret = set_encoder_id(output_files[ost->file_index], ost);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "set_encoder_id failed.\n");
        return -1;
    }

    // Muxers use AV_PKT_DATA_DISPLAYMATRIX to signal rotation. On the other
    // hand, the legacy API makes demuxers set "rotate" metadata entries,
    // which have to be filtered out to prevent leaking them to output files.
    /*(muxer使用AV_PKT_DATA_DISPLAYMATRIX来表示旋转。另一方面，遗留API使解复用器设置“旋转”元数据条目，
     * 这些条目必须被过滤掉，以防止泄漏到输出文件中。)*/
    /* 2.若输出流的元数据存在rotate选项，则将其删除 */
    av_dict_set(&ost->st->metadata, "rotate", NULL, 0);

    /* 3.主要初始化ost的disposition */
    if (ist) {
        // 推流转码往这走
        ost->st->disposition          = ist->st->disposition;/* 这个流中的disposition成员好像与附属图片相关 */

        dec_ctx = ist->dec_ctx; // 获取解码器

        enc_ctx->chroma_sample_location = dec_ctx->chroma_sample_location;/* 这定义了色度样本的位置 */
    } else {
        /* 可后续研究 */
        for (j = 0; j < oc->nb_streams; j++) {
            AVStream *st = oc->streams[j];
            /* 这个if条件当某个媒体类型存在多个时，才会满足.例如ost是视频流，且输出视频流有两个，那么就会满足 */
            if (st != ost->st && st->codecpar->codec_type == ost->st->codecpar->codec_type)
                break;
        }
        if (j == oc->nb_streams)
            if (ost->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ||
                ost->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                ost->st->disposition = AV_DISPOSITION_DEFAULT;
    }

    /* 4.获取视频帧率 */
    if (enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        /* 4.1 若用户没有指定输出帧率，优先从输出过滤器中获取，否则考虑输入流中的帧率. if条件越在上面，优先级越高 */
        if (!ost->frame_rate.num)
            ost->frame_rate = av_buffersink_get_frame_rate(ost->filter->filter);
        if (ist && !ost->frame_rate.num)
            ost->frame_rate = ist->framerate;
        if (ist && !ost->frame_rate.num)
            ost->frame_rate = ist->st->r_frame_rate;
        if (ist && !ost->frame_rate.num) {
            ost->frame_rate = (AVRational){25, 1};
            av_log(NULL, AV_LOG_WARNING,
                   "No information "
                   "about the input framerate is available. Falling "
                   "back to a default value of 25fps for output stream #%d:%d. Use the -r option "
                   "if you want a different framerate.\n",
                   ost->file_index, ost->index);
        }

        /* 4.2 若编码器有支持的帧率数组，且没有强制帧率，那么会根据上面存储的帧率，在ffmpeg的帧率数组中找到一个最近的帧率 */
        if (ost->enc->supported_framerates && !ost->force_fps) {
            int idx = av_find_nearest_q_idx(ost->frame_rate, ost->enc->supported_framerates);
            ost->frame_rate = ost->enc->supported_framerates[idx];
        }

        // reduce frame rate for mpeg4 to be within the spec limits
        // (降低mpeg4的帧率，使其在规格限制内)
        // 4.3 mp4时的特殊处理帧率
        // 怎么降低，有兴趣的自行看源码，内部会涉及一些算法,这里不深入研究
        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG4) {
            av_reduce(&ost->frame_rate.num, &ost->frame_rate.den,
                      ost->frame_rate.num, ost->frame_rate.den, 65535);
        }
    }

    /* 5.设置不同媒体类型的编码器中的相关参数 */
    switch (enc_ctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        /* 通过输出过滤器获取音频相关参数.
         * 转码时,该函数被调用前已经配置完输出过滤器,所以从输出过滤器中获取. */
        enc_ctx->sample_fmt     = (AVSampleFormat)av_buffersink_get_format(ost->filter->filter);
        if (dec_ctx)
            enc_ctx->bits_per_raw_sample = FFMIN(dec_ctx->bits_per_raw_sample,
                                                 av_get_bytes_per_sample(enc_ctx->sample_fmt) << 3);
        enc_ctx->sample_rate    = av_buffersink_get_sample_rate(ost->filter->filter);
        enc_ctx->channel_layout = av_buffersink_get_channel_layout(ost->filter->filter);
        enc_ctx->channels       = av_buffersink_get_channels(ost->filter->filter);

        init_encoder_time_base(ost, av_make_q(1, enc_ctx->sample_rate));// 通过采样率设置音频编码器的时间基
        break;

    case AVMEDIA_TYPE_VIDEO:
        // 通过帧率设置视频编码器的时间基.注意av_inv_q是分子分母调转然后返回.例如帧率={25,1}，那么返回时间基就是{1,25}
        // 视频编码器的时间基一般都是设为帧率的倒数,see https://blog.csdn.net/weixin_44517656/article/details/110355462
        init_encoder_time_base(ost, av_inv_q(ost->frame_rate));

        // 若编码器的时间基存在一个为0，则从过滤器中获取时间基
        if (!(enc_ctx->time_base.num && enc_ctx->time_base.den))
            enc_ctx->time_base = av_buffersink_get_time_base(ost->filter->filter);

        /* 检测帧率是否很大，因为上面讲过，编码器的时基就是帧率的倒数，若时基很小，说明帧率很大。
         * 若帧率很大，会判断视频同步方法：若不是PASSTHROUGH，则会优先判断是否是auto，然后再判断是否是vscfr、cfr.
         * 理解这里需要知道逻辑运算符的优先级，或运算是比与运算低的，具体看open_output_file()中-map=0的字幕流逻辑. */
        if (   av_q2d(enc_ctx->time_base) < 0.001 && video_sync_method != VSYNC_PASSTHROUGH /* 1)时间基小于千分之一且video_sync_method!=0 */
           && (video_sync_method == VSYNC_CFR /* 4)视频同步方法是cfr */
               || video_sync_method == VSYNC_VSCFR || /* 3)视频同步方法是vscfr */
               (video_sync_method == VSYNC_AUTO && !(oc->oformat->flags & AVFMT_VARIABLE_FPS))))/* 2)视频同步方法是自动且不包含该宏 */
        {
            av_log(oc, AV_LOG_WARNING, "Frame rate very high for a muxer not efficiently supporting it.\n"
                                       "Please consider specifying a lower framerate, a different muxer or -vsync 2\n");
        }

        /* 将forced_kf_pts由单位AV_TIME_BASE_Q转成enc_ctx->time_base的时基单位.暂未深入研究
         * av_rescale_q()的原理：假设要转的pts=a，时基1{x1,y1}，时基2{x2,y2};
         * 因为转化前后的比是一样的，设转换后的pts=b，那么有a*x1/y1=b*x2/y2; 化简：
         * b=(a*x1*y2)/(y1*x2).
         * 对比注释，a * bq / cq这句话实际上看不出来，我们看源码验证上面：
         * 在av_rescale_q_rnd内部会将两个时基换算，然后调用av_rescale_rnd()，参数最终变成：
         * av_rescale_rnd(a, x1*y2, y1*x2, AV_ROUND_NEAR_INF);
         * 在av_rescale_rnd()内部，正常逻辑走：return (a * b + r) / c;
         * 那么去掉加上r，就验证了我们上面的原理.
         * av_rescale_q() see https://blog.csdn.net/weixin_44517656/article/details/110559611 */
        for (j = 0; j < ost->forced_kf_count; j++)
            ost->forced_kf_pts[j] = av_rescale_q(ost->forced_kf_pts[j],
                                                 AV_TIME_BASE_Q,
                                                 enc_ctx->time_base);

        /* 获取样品宽高比。获取方法：用户指定，则会使用av_mul_q获取；没指定，则从过滤器获取宽高比.
         * av_mul_q:两个有理数相乘，结果通过参数b传出,内部调用了av_reduce().
        */
        enc_ctx->width  = av_buffersink_get_w(ost->filter->filter);
        enc_ctx->height = av_buffersink_get_h(ost->filter->filter);
        enc_ctx->sample_aspect_ratio = ost->st->sample_aspect_ratio =
            ost->frame_aspect_ratio.num ? // overridden by the -aspect cli option(被-aspect命令行选项覆盖)
            av_mul_q(ost->frame_aspect_ratio, (AVRational){ enc_ctx->height, enc_ctx->width }) :
            av_buffersink_get_sample_aspect_ratio(ost->filter->filter);

        /* 从过滤器获取像素格式 */
        enc_ctx->pix_fmt = (AVPixelFormat)av_buffersink_get_format(ost->filter->filter);
        /* 内部libavcodec像素/样本格式的每个样本/像素位 */
        if (dec_ctx)
            enc_ctx->bits_per_raw_sample = FFMIN(dec_ctx->bits_per_raw_sample,
                                                 av_pix_fmt_desc_get(enc_ctx->pix_fmt)->comp[0].depth);

        /* 设置帧率到编码器上下文 */
        enc_ctx->framerate = ost->frame_rate;
        /* 设置帧率到输出流的平均帧率 */
        ost->st->avg_frame_rate = ost->frame_rate;

        if (!dec_ctx ||
            enc_ctx->width   != dec_ctx->width  ||
            enc_ctx->height  != dec_ctx->height ||
            enc_ctx->pix_fmt != dec_ctx->pix_fmt) {
            enc_ctx->bits_per_raw_sample = frame_bits_per_raw_sample;
        }

        if (ost->top_field_first == 0) {
            enc_ctx->field_order = AV_FIELD_BB;
        } else if (ost->top_field_first == 1) {
            enc_ctx->field_order = AV_FIELD_TT;
        }

        /*
         * 强制关键帧表达式解析.
         * 两种逻辑解析：
         * 1）forced_keyframes前5个字符面是"expr:"；
         * 2）forced_keyframes前6个字符面是"source"；
         * 暂不深入研究，比较少遇到，二次封装时可注释掉.
        */
        if (ost->forced_keyframes) {
            /*
             * av_expr_parse(): 解析表达式。
             * @param expr 是一个指针，在成功解析的情况下放置一个包含解析值的AVExpr，否则为NULL。
             *              当用户不再需要AVExpr时，必须使用av_expr_free()释放指向AVExpr的对象。
             * @param s 表达式作为一个以0结尾的字符串，例如“1+2^3+5*5+sin(2/3)”。就理解成是一个字符串即可.
             *
             * @param const_names NULL终止数组，零终止字符串的常量标识符，例如{"PI"， "E"， 0}
             * @param func1_names NULL终止数组，包含0终止的funcs1标识符字符串
             * @param funcs1 NULL结束的函数指针数组，用于带有1个参数的函数
             * @param func2_names NULL终止数组，包含0终止的funcs2标识符字符串
             * @param funcs2带两个参数的函数指针数组，以NULL结尾
             *
             * @param log_ctx父日志上下文
             * @return >= 0表示成功，否则为一个与AVERROR代码对应的负值
            */
            if (!strncmp(ost->forced_keyframes, "expr:", 5)) {
                ret = av_expr_parse(&ost->forced_keyframes_pexpr, ost->forced_keyframes+5,
                                    forced_keyframes_const_names, NULL, NULL, NULL, NULL, 0, NULL);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR,
                           "Invalid force_key_frames expression '%s'\n", ost->forced_keyframes+5);
                    return ret;
                }
                /*forced_keyframes_expr_const_values[]数组初始化*/
                ost->forced_keyframes_expr_const_values[FKF_N] = 0;
                ost->forced_keyframes_expr_const_values[FKF_N_FORCED] = 0;
                ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_N] = NAN;
                ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_T] = NAN;

                // Don't parse the 'forced_keyframes' in case of 'keep-source-keyframes',
                // parse it only for static kf timings
                //(不要在'keep-source-keyframes'的情况下解析' force_keyframes '，只在静态kf计时时解析它)
            } else if(strncmp(ost->forced_keyframes, "source", 6)) {
                ret = parse_forced_key_frames(ost->forced_keyframes, ost, ost->enc_ctx);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR,
                           "parse_forced_key_frames failed.\n");
                    return ret;
                }
            }
        }
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        enc_ctx->time_base = AV_TIME_BASE_Q;//字幕编码上下文的时基单位是微秒.
        //没有分辨率，默认使用输入流中保存的分辨率
        if (!enc_ctx->width) {
            enc_ctx->width     = input_streams[ost->source_index]->st->codecpar->width;
            enc_ctx->height    = input_streams[ost->source_index]->st->codecpar->height;
        }
        break;
    case AVMEDIA_TYPE_DATA:
        break;
    default:
        //abort();
        return -1;
        break;
    }

    /* 6.以编码器上下文的时基作为复用的时基 */
    ost->mux_timebase = enc_ctx->time_base;

    return 0;
}

/**
 * @brief 设置AVCodecContext.hw_device_ctx
 * @param ost 输出流
 * @return 成功=0； 失败=负数
*/
int FFmpegMedia::hw_device_setup_for_encode(OutputStream *ost)
{
    HWDevice *dev;

    /*1.找到编码器支持且用户硬件设备也支持的硬件设备*/
    dev = hw_device_match_by_codec(ost->enc);
    if (dev) {
        /*1.1对dev->device_ref引用计数加1，
        ost->enc_ctx->hw_device_ctx与dev->device_ref指向是相同的*/
        /*ost->enc_ctx->hw_device_ctx的注释(对比ost->enc_ctx->hw_frames_ctx)：
         AVHWDeviceContext的引用，描述将被硬件编码器/解码器使用的设备。引用由调用者设置，然后由libavcodec拥有(并释放)。
         如果编解码器设备不需要硬件帧，或者使用的任何硬件帧都是由libavcodec内部分配的，那么应该使用这个选项。
         如果用户希望提供任何用于编码器输入或解码器输出的帧，那么应该使用hw_frames_ctx来代替。当在get_format()中为解码器设置hw_frames_ctx时，
         在解码相关的流段时，该字段将被忽略，但可以在一个接一个的get_format()调用中再次使用。
         对于编码器和解码器，这个字段都应该在调用avcodec_open2()之前设置，并且之后不能写入。
         请注意，一些解码器可能需要在最初设置该字段以支持hw_frames_ctx -在这种情况下，所有使用的帧上下文必须在相同的设备上创建。*/
        ost->enc_ctx->hw_device_ctx = av_buffer_ref(dev->device_ref);
        if (!ost->enc_ctx->hw_device_ctx)
            return AVERROR(ENOMEM);
        return 0;
    } else {
        // No device required, or no device available.
        return 0;
    }
}

/**
 * @brief 不转码时，给OutputStream类型初始化的流程，基本与转码的流程差不多.
 * @param ost 输出流
 * @return 成功=0； 失败=返回负数或者程序退出.
 */
int FFmpegMedia::init_output_stream_streamcopy(OutputStream *ost)
{
    OutputFile *of = output_files[ost->file_index];
    InputStream *ist = get_input_stream(ost);
    AVCodecParameters *par_dst = ost->st->codecpar;
    AVCodecParameters *par_src = ost->ref_par;
    AVRational sar;
    int i, ret;
    uint32_t codec_tag = par_dst->codec_tag;

    av_assert0(ist && !ost->filter);

    /*1.从输入流中拷贝参数到输出流的编码器上下文.
     该函数源码很简单，里面是深拷贝实现.*/
    ret = avcodec_parameters_to_context(ost->enc_ctx, ist->st->codecpar);
    if (ret >= 0)
        ret = av_opt_set_dict(ost->enc_ctx, &ost->encoder_opts);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL,
               "Error setting up codec context options.\n");
        return ret;
    }

    /*2.从输出流的编码器拷贝参数到ost->ref_par.
     该函数源码很简单，里面是深拷贝实现.
    avcodec_parameters_to_context()/avcodec_parameters_from_context()基本一样的，只不过赋值的对象调换了*/
    ret = avcodec_parameters_from_context(par_src, ost->enc_ctx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL,
               "Error getting reference codec parameters.\n");
        return ret;
    }

    /*3.后续分析这个tag*/
    if (!codec_tag) {//ost->st->codecpar->codec_tag
        unsigned int codec_tag_tmp;
        if (!of->ctx->oformat->codec_tag ||
            av_codec_get_id (of->ctx->oformat->codec_tag, par_src->codec_tag) == par_src->codec_id ||
            !av_codec_get_tag2(of->ctx->oformat->codec_tag, par_src->codec_id, &codec_tag_tmp))
            codec_tag = par_src->codec_tag;
    }

    /*4. 从par_src拷贝编解码器信息到par_dst。
     * 看回上面，par_src是从输入流的参数得到的。参数传递的流程：
     * ist->st->codecpar ==> ost->enc_ctx ==> par_src(ost->ref_par) ==> par_dst(ost->st->codecpar)*/
    ret = avcodec_parameters_copy(par_dst, par_src);
    if (ret < 0)
        return ret;

    par_dst->codec_tag = codec_tag;

    if (!ost->frame_rate.num)
        ost->frame_rate = ist->framerate;
    ost->st->avg_frame_rate = ost->frame_rate;

    /*5.将内部时间信息从一个流传输到另一个流。*/
    /* avformat_transfer_internal_stream_timing_info():
     * @param ofmt ost的目标输出格式
     * @param ost 需要定时复制和调整的输出流
     * @param ist 引用输入流以从中复制计时
     * @param copy_tb 定义需要从何处导入流编解码器时基
     * 看该函数源码，大概就是对时基和帧率做处理.
    */
    ret = avformat_transfer_internal_stream_timing_info(of->ctx->oformat, ost->st, ist->st, (AVTimebaseSource)copy_tb);
    if (ret < 0)
        return ret;

    // copy timebase while removing common factors(在删除公共因子的同时复制时基)
    /*6.初始化 流的时基, 将编码器的时基拷贝给流的时基.*/
    if (ost->st->time_base.num <= 0 || ost->st->time_base.den <= 0)
        ost->st->time_base = av_add_q(av_stream_get_codec_timebase(ost->st), (AVRational){0, 1});

    // copy estimated duration as a hint to the muxer((复制估计持续时间作为对muxer的提示))
    /*7.若输出流的duration不存在，则将输入流的duration转换单位后，赋值给输出流的duration*/
    if (ost->st->duration <= 0 && ist->st->duration > 0)
        ost->st->duration = av_rescale_q(ist->st->duration, ist->st->time_base, ost->st->time_base);

    // copy disposition
    ost->st->disposition = ist->st->disposition;

    /*8.若输入流的side_data存在，则使用输入流的size_data给输出流赋值.不懂可参考转码的注释.*/
    if (ist->st->nb_side_data) {
        for (i = 0; i < ist->st->nb_side_data; i++) {
            const AVPacketSideData *sd_src = &ist->st->side_data[i];
            uint8_t *dst_data;

            dst_data = av_stream_new_side_data(ost->st, sd_src->type, sd_src->size);
            if (!dst_data)
                return AVERROR(ENOMEM);
            memcpy(dst_data, sd_src->data, sd_src->size);
        }
    }

    /*是否对side_data数据类型为AV_PKT_DATA_DISPLAYMATRIX的内容重写.*/
    if (ost->rotate_overridden) {
        uint8_t *sd = av_stream_new_side_data(ost->st, AV_PKT_DATA_DISPLAYMATRIX,
                                              sizeof(int32_t) * 9);
        if (sd)
            av_display_rotation_set((int32_t *)sd, -ost->rotate_override_value);//转码时，这里旋转重写的值(rotate_override_value)是0.
    }

    /*9.根据不同媒体类型给成员赋值*/
    switch (par_dst->codec_type) {//par_dst = ost->st->codecpar;
    case AVMEDIA_TYPE_AUDIO:
        /*-acodec copy和-vol不能同时使用.因为我们看到audio_volume默认是256,
         当不是256说明用户添加-vol，就需要解码，而用户又指定了copy，所以ffmpeg会报错.*/
        if (audio_volume != 256) {
            av_log(NULL, AV_LOG_FATAL, "-acodec copy and -vol are incompatible (frames are not decoded)\n");
            //exit_program(1);
            return -1;
        }
        /* block_align: 仅音频。某些格式所需的每个编码音频帧的字节数。
         * 对应于WAVEFORMATEX中的nBlockAlign。*/
        if((par_dst->block_align == 1 || par_dst->block_align == 1152 || par_dst->block_align == 576) && par_dst->codec_id == AV_CODEC_ID_MP3)
            par_dst->block_align= 0;
        if(par_dst->codec_id == AV_CODEC_ID_AC3)
            par_dst->block_align= 0;
        break;
    case AVMEDIA_TYPE_VIDEO:
        //获取宽高比，优先获取用户输入的frame_aspect_ratio，再到ist->st->sample_aspect_ratio、par_src->sample_aspect_ratio
        if (ost->frame_aspect_ratio.num) { // overridden by the -aspect cli option(被-aspect命令行选项覆盖)
            sar =
                av_mul_q(ost->frame_aspect_ratio,
                         (AVRational){ par_dst->height, par_dst->width });
            av_log(NULL, AV_LOG_WARNING, "Overriding aspect ratio "
                   "with stream copy may produce invalid files\n");//(用流复制重写纵横比可能会产生无效文件)
            }
        else if (ist->st->sample_aspect_ratio.num)
            sar = ist->st->sample_aspect_ratio;
        else
            sar = par_src->sample_aspect_ratio;
        ost->st->sample_aspect_ratio = par_dst->sample_aspect_ratio = sar;
        ost->st->avg_frame_rate = ist->st->avg_frame_rate;
        ost->st->r_frame_rate = ist->st->r_frame_rate;
        break;
    }

    ost->mux_timebase = ist->st->time_base;

    return 0;
}

/**
 * @brief 具体作用暂不分析.代码逻辑很简单.
 */
int FFmpegMedia::init_output_bsfs(OutputStream *ost)
{
    AVBSFContext *ctx;
    int i, ret;

    // 如果nb_bitstream_filters为0，是不会覆盖输出流已有的参数ost->st->codecpar
    // 没设置会直接返回0
    if (!ost->nb_bitstream_filters)
        return 0;

    /*1.遍历ost->bsf_ctx数组，给其赋值*/
    for (i = 0; i < ost->nb_bitstream_filters; i++) {
        ctx = ost->bsf_ctx[i];

        /*这里的赋值流程很简单，就是像链表一样赋值。
         * 1）当i=0时，使用ost->st->codecpar给ctx->par_in赋值；
         * 然后av_bsf_init()将ctx->par_in的值赋给ctx->par_out；
         * 2）当i!=0时，使用上一个元素的par_out给当前ctx->par_in赋值；
         * 然后av_bsf_init()继续将ctx->par_in的值赋给ctx->par_out；
         * 以此类推。画成赋值顺序的链表可以这样表示(注是赋值顺序，不是指针的指向)：
         * ost->st->codecpar ==> [0]ctx->par_in ==> [0]ctx->par_out
         * ==> [1]ctx->par_in ==> [1]ctx->par_out
         * ==> [2]ctx->par_in ==> [2]ctx->par_out 以此类推...
        */
        ret = avcodec_parameters_copy(ctx->par_in,
                                      i ? ost->bsf_ctx[i - 1]->par_out : ost->st->codecpar);
        if (ret < 0)
            return ret;

        //同理
        ctx->time_base_in = i ? ost->bsf_ctx[i - 1]->time_base_out : ost->st->time_base;

        /* av_bsf_init(): 初始化AVBSFContext.
         * 注，内部会调用avcodec_parameters_copy(ctx->par_out, ctx->par_in);
         * 给ctx->par_out赋值*/
        ret = av_bsf_init(ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error initializing bitstream filter: %s\n",
                   ost->bsf_ctx[i]->filter->name);
            return ret;
        }
    }

    /*2.将ost->bsf_ctx尾元素的par_out，给输出流的参数ost->st->codecpar赋值.
     * 但注意，ctx->par_out实际上是由ost->st->codecpar赋值得到的，这里再使用
     * ctx->par_out给ost->st->codecpar赋值，有什么区别吗？
     * 笔者认为，唯一区别就是ctx->par_out经过了av_bsf_init的处理，与原来的参数
     * 比应该就是多了初始化的部分。
    .*/
    ctx = ost->bsf_ctx[ost->nb_bitstream_filters - 1];
    ret = avcodec_parameters_copy(ost->st->codecpar, ctx->par_out);
    if (ret < 0)
        return ret;

    ost->st->time_base = ctx->time_base_out;

    return 0;
}

void FFmpegMedia::print_sdp(void)
{
    char sdp[16384];
    int i;
    int j;
    AVIOContext *sdp_pb;
    AVFormatContext **avc;

    for (i = 0; i < nb_output_files; i++) {
        if (!output_files[i]->header_written)
            return;
    }

    avc = (AVFormatContext **)av_malloc_array(nb_output_files, sizeof(*avc));
    if (!avc)
        exit_program(1);
    for (i = 0, j = 0; i < nb_output_files; i++) {
        if (!strcmp(output_files[i]->ctx->oformat->name, "rtp")) {
            avc[j] = output_files[i]->ctx;
            j++;
        }
    }

    if (!j)
        goto fail;

    av_sdp_create(avc, j, sdp, sizeof(sdp));

    if (!sdp_filename) {
        printf("SDP:\n%s\n", sdp);
        fflush(stdout);
    } else {
        if (avio_open2(&sdp_pb, sdp_filename, AVIO_FLAG_WRITE, &int_cb, NULL) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to open sdp file '%s'\n", sdp_filename);
        } else {
            avio_printf(sdp_pb, "SDP:\n%s", sdp);
            avio_closep(&sdp_pb);
            av_freep(&sdp_filename);
        }
    }

fail:
    av_freep(&avc);
}

/* open the muxer when all the streams are initialized */
/**
 * @brief 若输出文件的所有输出流已经初始化完成。则给该输出文件进行写头，并且会清空每个输出流的复用队列。
 * 清空复用队列的操作：若队列没有包则不作处理；否则会将包读出来，然后调用write_packet()进行写帧操作，
 * 具体写帧流程看write_packet()，不难.
 *
 * @param of 输出文件
 * @param file_index 输出文件下标
*/
int FFmpegMedia::check_init_output_file(OutputFile *of, int file_index)
{
    int ret, i;

    /* 1.检查输出文件的每个输出流是否都已经被初始化.
     * 注，只有所有输出流完成初始化才会写头. */
    for (i = 0; i < of->ctx->nb_streams; i++) {
        // of->ost_index基本是0,暂时还没遇到过第一个流不是0的情况
        OutputStream *ost = output_streams[of->ost_index + i];
        if (!ost->initialized)
            return 0;
    }

    of->ctx->interrupt_callback = int_cb;

    /* 2.写头.
     * 这里看到，输出流的解复用选项在这里被使用。
     * 输入流的解复用选项在avformat_open_input()被使用.
    */
    ret = avformat_write_header(of->ctx, &of->opts);
    if (ret < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_log(NULL, AV_LOG_ERROR,
               "Could not write header for output file #%d "
               "(incorrect codec parameters ?): %s\n",
               file_index, av_err2str_ex(str, ret));
        return ret;
    }
    //assert_avoptions(of->opts);
    of->header_written = 1;//标记写头成功

    av_dump_format(of->ctx, file_index, of->ctx->url, 1);

    if (sdp_filename || want_sdp)
        print_sdp();

    // 3. flush复用队列的pkt.
    /* ost->muxing_queue的内容从哪里写入?
     * 答:当输出文件的所有输出流还没初始化完成,若有部分输出流编码完成,
     * 会在write_packet()时会先缓存到复用队列.然后这里把这部分输出流缓存的pkt优先写帧.
     */
    /* flush the muxing queues */
    for (i = 0; i < of->ctx->nb_streams; i++) {
        OutputStream *ost = output_streams[of->ost_index + i];

        /* try to improve muxing time_base (only possible if nothing has been written yet) */
        /* 尝试改进muxing time_base(只有在还没有写入任何内容的情况下才可能)*/
        /* av_fifo_size()：返回AVFifoBuffer中以字节为单位的数据量，也就是你可以从它读取的数据量。
         * 源码很简单：return (uint32_t)(f->wndx - f->rndx);
         *
         * 而该fifo队列没使用时，f->wndx=f->rndx=0;
         * 详细看AVFifoBuffer *frame_queue的注释.
        */
        if (!av_fifo_size(ost->muxing_queue))
            ost->mux_timebase = ost->st->time_base;//只有fifo队列没使用才会进来

        /*
         * av_fifo_generic_read():将数据从AVFifoBuffer提供 给 用户提供的回调.
         * 大概看了一下源码，数据会读完后，AVFifoBuffer的rptr会指向这片内存的起始地址.
         * 所以下面就是，将fifo的pkt出来，然后调用write_packet写包，最终while会把复用队列清空。
         * 若想更好邻居，需要理解av_fifo_generic_read()，而理解它需要理解rptr、wptr、rndx、wndx的含义。
         * 而理解这几个成员，则需要把libavutil/fifo.c里面的api看完，源码不多，该fifo队列设计类似ffplay的帧队列.
         * 不想花时间的，知道av_fifo_generic_read的作用：
         * 1）从fifo读出来出来，保存在&pkt；
         * 2）然后会把sizeof(pkt)大小的字节从fifo删掉即可。
        */
        while (av_fifo_size(ost->muxing_queue)) {
            AVPacket pkt;
            av_fifo_generic_read(ost->muxing_queue, &pkt, sizeof(pkt), NULL);
            ret = write_packet(of, &pkt, ost, 1);
            if(ret < 0){
                return -1;
            }
        }
    }

    return 0;
}

/**
 * @brief 初始化输出流，流程分为初始化分转码和不转码。
 * 1.转码的流程：
 * 1）打开avcodec_open2前对ost、ost->enc_ctx初始化；
 * 2）然后调用avcodec_open2；
 * 3）将ost->enc_ctx的参数拷贝到ost->st->codecpar、ost->st->codec;
 * 4）调用init_output_bsfs()处理bsfs。
 * 5）调用check_init_output_file()，判断是否全部初始化完成，完成则写头avformat_write_header()。
 * 2.不转码的流程：
 * 1）调用init_output_stream_streamcopy初始化。
 *      注意，该函数内部并未调用avcodec_open2，也就说不转码时不需要调用该函数.
 * 2）调用init_output_bsfs()处理bsfs。
 * 3）调用check_init_output_file()，判断是否全部初始化完成，完成则写头avformat_write_header()。
 *
 * @param ost 输出流
 * @param error 指针，出错时将错误字符串进行传出
 * @param error_len error的长度
 */
int FFmpegMedia::init_output_stream(OutputStream *ost, char *error, int error_len)
{
    int ret = 0;

    /* 1.输出流需要编码的流程 */
    if (ost->encoding_needed) {
        AVCodec      *codec = ost->enc;// 在new_xxx_stream()时初始化了
        AVCodecContext *dec = NULL;
        InputStream *ist;

        /* 1.1给输出流结构体OutputStream相关成员赋值 */
        ret = init_output_stream_encode(ost);
        if (ret < 0)
            return ret;

        /* 1.2若输入文件存在subtitle_header(相当于存在字幕流)，输出文件也应该在输出流拷贝一份subtitle_header.
         *
         * dec->subtitle_header解释:
         * 包含文本字幕样式信息的头。
         * 对于SUBTITLE_ASS副标题类型，它应该包含整个ASS[脚本信息]和[V4+样式]部分，
         * 加上[事件]行和下面的格式行。它不应该包括任何对白(对话)线。
         *
         * dec->subtitle_header应该指的是字幕？而不是指字幕头部相关的内容？笔者对字幕不是太熟悉，有兴趣可自行研究.
        */
        if ((ist = get_input_stream(ost)))
            dec = ist->dec_ctx;
        if (dec && dec->subtitle_header) {
            /* ASS code assumes this buffer is null terminated so add extra byte.
            (ASS代码假设这个缓冲区是空的，因此添加额外的字节)*/
            ost->enc_ctx->subtitle_header = (uint8_t *)av_mallocz(dec->subtitle_header_size + 1);
            if (!ost->enc_ctx->subtitle_header)
                return AVERROR(ENOMEM);
            memcpy(ost->enc_ctx->subtitle_header, dec->subtitle_header, dec->subtitle_header_size);
            ost->enc_ctx->subtitle_header_size = dec->subtitle_header_size;
        }
        /* 1.3编码时线程数没设置，将设为自动 */
        if (!av_dict_get(ost->encoder_opts, "threads", NULL, 0))
            av_dict_set(&ost->encoder_opts, "threads", "auto", 0);
        /* 1.4音频时，若没设置音频码率，默认是128k */
        if (ost->enc->type == AVMEDIA_TYPE_AUDIO &&
            !codec->defaults &&
            !av_dict_get(ost->encoder_opts, "b", NULL, 0) &&
            !av_dict_get(ost->encoder_opts, "ab", NULL, 0))
            av_dict_set(&ost->encoder_opts, "b", "128000", 0);

        /* 1.5若AVHWFramesContext中的像素格式与av_buffersink_get_format获取的像素格式一样，
         * 则对av_buffersink_get_hw_frames_ctx得到的AVHWFramesContext引用数加1,
         * 否则调用hw_device_setup_for_encode，对dev->device_ref引用数加1 */
        //简单来说，这里就是设置enc_ctx->hw_frames_ctx或者enc_ctx->hw_device_ctx
        if (ost->filter && av_buffersink_get_hw_frames_ctx(ost->filter->filter) &&
            ((AVHWFramesContext*)av_buffersink_get_hw_frames_ctx(ost->filter->filter)->data)->format ==
            av_buffersink_get_format(ost->filter->filter)) {
            /*
             * ost->enc_ctx->hw_frames_ctx的注释：
             * 对AVHWFramesContext的引用，描述了输入(编码)或输出(解码)帧。引用由调用者设置，
             * 然后由libavcodec拥有(和释放)——设置后调用者永远不应该读取它。
             *
             * - decoding:该字段应该由调用者从get_format()回调中设置。之前的引用(如果有的话)在get_format()调用之前总是会被libavcodec取消。
             * 如果默认的get_buffer2()是与hwaccel像素格式一起使用的，那么这个AVHWFramesContext将用于分配帧缓冲区。
             * - encoding:对于配置为hwaccel像素格式的硬件编码器，该字段应该由调用者设置为描述输入帧的AVHWFramesContext的引用。
             * AVHWFramesContext.format必须等于AVCodecContext.pix_fmt。
             * 这个字段应该在调用avcodec_open2()之前设置。
            */
            /*看av_buffer_ref()源码：就是开辟一个AVBufferRef结构体后，浅拷贝指向传入参数，即就是引用数+1*/
            ost->enc_ctx->hw_frames_ctx = av_buffer_ref(av_buffersink_get_hw_frames_ctx(ost->filter->filter));
            if (!ost->enc_ctx->hw_frames_ctx)
                return AVERROR(ENOMEM);
        } else {
            ret = hw_device_setup_for_encode(ost);
            if (ret < 0) {
                char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                snprintf(error, error_len, "Device setup failed for "
                         "encoder on output stream #%d:%d : %s",
                     ost->file_index, ost->index, av_err2str_ex(str, ret));
                return ret;
            }
        }

        /* 1.6判断输入输出的字幕类型是否是非法转换.
         * 这里与open_output_file的new字幕流的相关判断类似 */
        if (ist && ist->dec->type == AVMEDIA_TYPE_SUBTITLE && ost->enc->type == AVMEDIA_TYPE_SUBTITLE) {
            int input_props = 0, output_props = 0;
            AVCodecDescriptor const *input_descriptor =
                avcodec_descriptor_get(dec->codec_id);
            AVCodecDescriptor const *output_descriptor =
                avcodec_descriptor_get(ost->enc_ctx->codec_id);
            // 例如输入描述符的字幕类型是ass(name=ass)，input_descriptor->props=131072，这个值就是AV_CODEC_PROP_TEXT_SUB的值
            // 以推流命令的-scodec text，那么通过enc_ctx->codec_id得到的output_descriptor->props也是131072，那么认为合法转换
            if (input_descriptor)
                input_props = input_descriptor->props & (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);
            if (output_descriptor)
                output_props = output_descriptor->props & (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);
            if (input_props && output_props && input_props != output_props) {
                snprintf(error, error_len,
                         "Subtitle encoding currently only possible from text to text "
                         "or bitmap to bitmap");//(字幕编码目前只能从文本到文本 或 位图到位图)
                return AVERROR_INVALIDDATA;
            }
        }

        /* 1.7编解码器上下文与编解码器关联.
         * 这里看到，编解码器的相关选项是在这里被应用的 */
        if ((ret = avcodec_open2(ost->enc_ctx, codec, &ost->encoder_opts)) < 0) {
            if (ret == AVERROR_EXPERIMENTAL){
                //abort_codec_experimental(codec, 1);
                av_log(NULL, AV_LOG_FATAL, "avcodec_open2 AVERROR_EXPERIMENTAL(1).\n");
                return -1;
            }

            snprintf(error, error_len,
                     "Error while opening encoder for output stream #%d:%d - "
                     "maybe incorrect parameters such as bit_rate, rate, width or height",
                    ost->file_index, ost->index);
            return ret;
        }
        /* 1.8 设置音频的样本大小.
         * 若是音频且音频编码器不支持在每次调用中接收不同数量的样本，那么设置样本大小 */
        if (ost->enc->type == AVMEDIA_TYPE_AUDIO &&
            !(ost->enc->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
            av_buffersink_set_frame_size(ost->filter->filter,
                                            ost->enc_ctx->frame_size);

        /* 1.9检测是否有未知的编码器选项，因为avcodec_open2应用后，未知的选项会返回ost->encoder_opts本身 */
        assert_avoptions(ost->encoder_opts);

        //比特率设置过低。处理步骤它接受bits/s作为参数，而不是kbits/s
        if (ost->enc_ctx->bit_rate && ost->enc_ctx->bit_rate < 1000 &&
            ost->enc_ctx->codec_id != AV_CODEC_ID_CODEC2 /* don't complain about 700 bit/s modes */)
            av_log(NULL, AV_LOG_WARNING, "The bitrate parameter is set too low."
                                         " It takes bits/s as argument, not kbits/s\n");

        /* 1.10从编码器上下文拷贝参数到流中.
         * 因为上面我们对编码器部分成员做了赋值 */
        ret = avcodec_parameters_from_context(ost->st->codecpar, ost->enc_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL,
                   "Error initializing the output stream codec context.\n");
            //exit_program(1);
            return -1;
        }

        /* 1.11将ost->enc_ctx拷贝到ost->st->codec，这一步可以不需要？
         * 这一步st->codec里的参数被新版本的st->codecpar代替了，
         * ffmpeg注释说st->codec后面可能废弃，这里笔者感觉可以不写，不过写一下问题也不大. */
        /*
         * FIXME: ost->st->codec should't be needed here anymore.
         * (修复:ost->st->codec在这里不需要了)
         */
        /*
         * avcodec_copy_context()注释：
         * 将源AVCodecContext的设置复制到目标AVCodecContext中。
         * 最终的目的地编解码器上下文将是未打开的，即你需要调用avcodec_open2()，然
         * 后才能使用这个AVCodecContext解码/编码, 视频/音频数据。
         * 弃用:
         * 此函数的语义定义不明确，不应使用。如果需要将流参数从一个编解码器上下文传输到另一个，
         * 请使用中间的AVCodecParameters实例和avcodec_parameters_from_context()/avcodec_parameters_to_context()功能。
        */
        ret = avcodec_copy_context(ost->st->codec, ost->enc_ctx);
        if (ret < 0)
            return ret;

        /* 1.12将ost->enc_ctx->coded_side_data[i]的AVPacketSideData数据拷贝到st->side_data[type]，
         * 而这里的st是输出流的st. */
        // 与整个编码流相关联的附加数据
        if (ost->enc_ctx->nb_coded_side_data) {
            int i;

            for (i = 0; i < ost->enc_ctx->nb_coded_side_data; i++) {
                const AVPacketSideData *sd_src = &ost->enc_ctx->coded_side_data[i];
                uint8_t *dst_data;

                /* av_stream_new_side_data()注释:
                 * 从流分配新信息。
                 * 返回值：指针指向新分配的数据，否则为NULL.
                 *
                 * 但是注意，av_stream_new_side_data()内部是没有拷贝数据的，只是单纯的开辟内存，
                 * 源码很简单：
                 * 1）若传入的sd_src->type在st->side_data数组找到，则st->side_data[type]指向新new的data，
                 * 并返回data；
                 * 2）若传入的sd_src->type在st->side_data数组找不到，则st->side_data数组元素加1，然后该元素同样指向新new的data，
                 * 并返回data；
                 * 注：st指第一个形参stream.
                */
                dst_data = av_stream_new_side_data(ost->st, sd_src->type, sd_src->size);
                if (!dst_data)
                    return AVERROR(ENOMEM);

                /* 拷贝数据.
                 * 看源码知道dst_data和st->side_data[type]指向一样，
                 * 所以此时相当于将ost->enc_ctx->coded_side_data[i]的数据拷贝到st->side_data[type] */
                memcpy(dst_data, sd_src->data, sd_src->size);
            }
        }

        /*
         * Add global input side data. For now this is naive, and copies it
         * from the input stream's global side data. All side data should
         * really be funneled over AVFrame and libavfilter, then added back to
         * packet side data, and then potentially using the first packet for
         * global side data.
         * (添加全局输入side data。现在，这是幼稚的，从输入流的全局side data复制它。
         * 所有side data都应该通过AVFrame和libavfilter过滤，然后添加回包side data，然后可能使用第一个包用于全局side data)
         */
        /* 1.13将ist->st->side_data[i]的AVPacketSideData数据拷贝到st->side_data[type]
         * 这里看到，上面依赖coded_side_data填了输出流的side data，是可能被ist->st->side_data重写的. */
        if (ist) {
            int i;
            for (i = 0; i < ist->st->nb_side_data; i++) {
                AVPacketSideData *sd = &ist->st->side_data[i];
                uint8_t *dst = av_stream_new_side_data(ost->st, sd->type, sd->size);
                if (!dst)
                    return AVERROR(ENOMEM);
                memcpy(dst, sd->data, sd->size);

                /* AV_PKT_DATA_DISPLAYMATRIX: 这个side data包含一个3x3的变换矩阵，
                 * 它描述了一个仿射变换，需要应用到解码的视频帧中以获得正确的表示。*/

                /* av_display_rotation_set():
                 * 初始化一个描述按指定角度(以度为单位)纯逆时针旋转的变换矩阵。
                 * @param matri 分配的转换矩阵(将被此函数完全覆盖).
                 * @param angle rotation angle in degrees(单位为度).
                 * 源码就不深入研究了.
                */
                if (ist->autorotate && sd->type == AV_PKT_DATA_DISPLAYMATRIX)
                    //av_display_rotation_set((uint32_t *)dst, 0);
                    av_display_rotation_set((int32_t *)dst, 0);
            }
        }

        // copy timebase while removing common factors(在删除公共因子的同时复制时基)
        /* 1.14 将编码器的时基拷贝给流的时基.
         * 传{0,1}在两个有理数中加上相当于加上0，故等价于拷贝.
         * (注上面init_output_stream_encode()是给编码器的enc_ctx->time_base赋值) */
        // 例如enc_ctx->time_base={1,1000000},cp后，st->time_base={1,1000000}
        if (ost->st->time_base.num <= 0 || ost->st->time_base.den <= 0)
            ost->st->time_base = av_add_q(ost->enc_ctx->time_base, (AVRational){0, 1});

        // copy estimated duration as a hint to the muxer(复制估计持续时间作为对muxer的提示)
        /* 1.15 若输出流的duration不存在，则将输入流的duration转换单位后，赋值给输出流的duration.
         * 单位从ist->st->time_base转成ost->st->time_base */
        if (ost->st->duration <= 0 && ist && ist->st->duration > 0)
            ost->st->duration = av_rescale_q(ist->st->duration, ist->st->time_base, ost->st->time_base);

        ost->st->codec->codec= ost->enc_ctx->codec;// 浅拷贝
    } else if (ost->stream_copy) {/* 2.输出流不需要编码的流程 */
        ret = init_output_stream_streamcopy(ost);
        if (ret < 0)
            return ret;
    }

    // parse user provided disposition, and update stream values(解析用户提供的处理，并更新流值)
    /* 3.若用户设置disposition，则将其设置到ost->st->disposition。推流没用到 */
    if (ost->disposition) {
        /*关于AVOption模块，可参考: https://blog.csdn.net/ericbar/article/details/79872779*/
        static const AVOption opts[] = {
            { "disposition"         , NULL, 0, AV_OPT_TYPE_FLAGS, { .i64 = 0 }, (double)INT64_MIN, (double)INT64_MAX, 0, .unit = "flags" },
            { "default"             , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_DEFAULT           }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "dub"                 , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_DUB               }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "original"            , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_ORIGINAL          }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "comment"             , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_COMMENT           }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "lyrics"              , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_LYRICS            }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "karaoke"             , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_KARAOKE           }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "forced"              , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_FORCED            }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "hearing_impaired"    , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_HEARING_IMPAIRED  }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "visual_impaired"     , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_VISUAL_IMPAIRED   }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "clean_effects"       , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_CLEAN_EFFECTS     }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "attached_pic"        , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_ATTACHED_PIC      }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "captions"            , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_CAPTIONS          }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "descriptions"        , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_DESCRIPTIONS      }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "dependent"           , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_DEPENDENT         }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { "metadata"            , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = AV_DISPOSITION_METADATA          }, (double)INT64_MIN, (double)INT64_MAX, 0,  .unit = "flags" },
            { NULL },
        };
        static const AVClass avclass = {
            .class_name = "",
            .item_name  = av_default_item_name,
            .option     = opts,
            .version    = LIBAVUTIL_VERSION_INT,
        };
        const AVClass *pclass = &avclass;

        /* av_opt_eval_flags(): 利用参2给参4设置值，值为参3(可参考set_encoder_id()里的注释).
         * 传参2的作用是利用该AVOption里面的一些属性，
         * 例如disposition的类型是AV_OPT_TYPE_FLAGS，共用体default_val取的是i64 */
        ret = av_opt_eval_flags(&pclass, &opts[0], ost->disposition, &ost->st->disposition);
        if (ret < 0)
            return ret;
    }

    /* initialize bitstream filters for the output stream
     * needs to be done here, because the codec id for streamcopy is not
     * known until now
     * (输出流的初始化位流过滤器需要在这里完成，因为流复制的编解码器id到现在还不知道)*/
    /* 4.根据是否设置bitstream_filters，来给ost->bsf_ctx数组赋值。
     * 内部会给ost->st->codecpar重新拷贝处理. */
    ret = init_output_bsfs(ost);
    if (ret < 0)
        return ret;

    ost->initialized = 1;// 标记该输出流初始化完成.

    /* 5.判断输出文件的所有输出流是否初始化完成，若完成，则会对该输出文件进行写头 */
    ret = check_init_output_file(output_files[ost->file_index], ost->file_index);
    if (ret < 0)
        return ret;

    return ret;
}

/**
 * @brief 判断FilterGraph的graph_desc描述是否为空.
 * @param fg ffmpeg封装的系统过滤器
 * @return =1 graph_desc为空；=0 graph_desc不为空
 */
int FFmpegMedia::filtergraph_is_simple(FilterGraph *fg)
{
    return !fg->graph_desc;
}

/**
 * @brief 打开输入流的解码器, 输出流满足一定条件也会打开编码器,
 *          但音视频一般不会在transcode_init打开.
 *
 * @return 0-成功 负数-失败
 */
int FFmpegMedia::transcode_init(void)
{
    int ret = 0, i, j, k;
    AVFormatContext *oc;
    OutputStream *ost;
    InputStream *ist;
    char error[1024] = {0};

    /* 1.暂时没看懂,推流命令没用到 */
    for (i = 0; i < nb_filtergraphs; i++) {
        FilterGraph *fg = filtergraphs[i];
        for (j = 0; j < fg->nb_outputs; j++) {
            OutputFilter *ofilter = fg->outputs[j];
            if (!ofilter->ost || ofilter->ost->source_index >= 0)//笔者没看懂这里if条件代表的含义
                continue;//推流命令走这里
            if (fg->nb_inputs != 1)
                continue;
            for (k = nb_input_streams-1; k >= 0 ; k--)
                if (fg->inputs[0]->ist == input_streams[k])
                    break;
            ofilter->ost->source_index = k;
        }
    }

    /* init framerate emulation(init帧速率仿真) */
    /* 2.用户指定-re选项，则给每个输入文件的每个输入流保存一个开始时间 */
    for (i = 0; i < nb_input_files; i++) {
        InputFile *ifile = input_files[i];
        if (ifile->rate_emu){
            for (j = 0; j < ifile->nb_streams; j++){
                //这里看到ist_index的作用，估计是为了防止一些输入文件中, 第一个流下标不是0的情况
                input_streams[j + ifile->ist_index]->start = av_gettime_relative();
            }
        }
    }

    /* init input streams */
    /* 3.进一步初始化输入流(add_input_streams初始化了一部分),内部会对硬件相关处理.
     * 转码时实际上主要是打开解码器(avcodec_open2()),不转码则只处理一些参数 */
    for (i = 0; i < nb_input_streams; i++)
        if ((ret = init_input_stream(i, error, sizeof(error))) < 0) {
            /* 只要有一个输入流初始化失败，则将所有输出流编解码器上下文全部关闭 */
            for (i = 0; i < nb_output_streams; i++) {
                ost = output_streams[i];
                /* 这个函数释放enc_ctx内部相关内容, 但不会释放enc_ctx本身，需要avcodec_free_context()去做.
                 * 参考ffmpeg/tests/api/api-h264-test.c/video_decode_example(),
                 * 实际上ffmpeg注释也说了，avcodec_alloc_context3()开辟的上下文应该使用avcodec_free_context()释放 */
                avcodec_close(ost->enc_ctx);
            }
            goto dump_format;
        }

    /* open each encoder */
    /* 4.进一步初始化输出流(new_video(audio,subtitle,data)_stream初始化了一部分),内部会对硬件相关处理.
     * 转码时实际上主要是打开编码器(avcodec_open2()),不转码则只处理一些参数. */
    for (i = 0; i < nb_output_streams; i++) {
        // skip streams fed from filtergraphs until we have a frame for them(跳过过滤器的数据流，直到我们找到合适的帧)
        /* 因为我们在open_output_file()给需要转码的流创建了filter，所以音视频流会执行continue */
        if (output_streams[i]->filter)
            continue;

        /* 字幕一般会走这里，因为在open_output_files()创建filter时，字幕不会创建 */
        ret = init_output_stream(output_streams[i], error, sizeof(error));
        if (ret < 0)
            goto dump_format;
    }

    /* discard unused programs */
    /* 5. 暂不研究，推流没用到 */
    for (i = 0; i < nb_input_files; i++) {
        InputFile *ifile = input_files[i];
        for (j = 0; j < ifile->ctx->nb_programs; j++) {
            AVProgram *p = ifile->ctx->programs[j];
            int discard  = AVDISCARD_ALL;

            for (k = 0; k < p->nb_stream_indexes; k++)
                if (!input_streams[ifile->ist_index + p->stream_index[k]]->discard) {
                    discard = AVDISCARD_DEFAULT;
                    break;
                }
            p->discard = (AVDiscard)discard;
        }
    }

    /* write headers for files with no streams */
    /* 写没有流的文件头.输出文件是/dev/null这种的处理？
     * 6. 暂不研究，推流没用到
     */
    for (i = 0; i < nb_output_files; i++) {
        oc = output_files[i]->ctx;
        if (oc->oformat->flags & AVFMT_NOSTREAMS && oc->nb_streams == 0) {
            ret = check_init_output_file(output_files[i], i);
            if (ret < 0)
                goto dump_format;
        }
    }

    // 下面都是dump打印到控制台的相关内容,可以不看.
 dump_format:
    /* dump the stream mapping */
    /*dump输入输出流的信息，比较简单，可以不看*/
    av_log(NULL, AV_LOG_INFO, "Stream mapping:\n");

    /* 7. 遍历每个输入流的每个输入过滤器的graph_desc是否为空，不为空则打印相关信息；为空则不打印. */
    for (i = 0; i < nb_input_streams; i++) {
        ist = input_streams[i];

        //遍历输入流的每个输入过滤器
        for (j = 0; j < ist->nb_filters; j++) {
            if (!filtergraph_is_simple(ist->filters[j]->graph)) {//推流命令不会进来
                av_log(NULL, AV_LOG_INFO, "  Stream #%d:%d (%s) -> %s",
                       ist->file_index, ist->st->index, ist->dec ? ist->dec->name : "?",
                       ist->filters[j]->name);
                if (nb_filtergraphs > 1)
                    av_log(NULL, AV_LOG_INFO, " (graph %d)", ist->filters[j]->graph->index);
                av_log(NULL, AV_LOG_INFO, "\n");
            }
        }
    }

    /* 8. 遍历输出流 */
    for (i = 0; i < nb_output_streams; i++) {
        ost = output_streams[i];

        /*附属图片的处理*/
        if (ost->attachment_filename) {
            /* an attached file */
            av_log(NULL, AV_LOG_INFO, "  File %s -> Stream #%d:%d\n",
                   ost->attachment_filename, ost->file_index, ost->index);
            continue;
        }

        /* 判断输出流的输出过滤器的graph_desc是否为空，
         * 不为空则打印，然后continue；为空则不打印.
         * 这里一般是用到过滤器的处理.*/
        if (ost->filter && !filtergraph_is_simple(ost->filter->graph)) {
            /* output from a complex graph */
            av_log(NULL, AV_LOG_INFO, "  %s", ost->filter->name);
            if (nb_filtergraphs > 1)
                av_log(NULL, AV_LOG_INFO, " (graph %d)", ost->filter->graph->index);

            av_log(NULL, AV_LOG_INFO, " -> Stream #%d:%d (%s)\n", ost->file_index,
                   ost->index, ost->enc ? ost->enc->name : "?");
            continue;
        }

        /* 下面是正常流程 */
        av_log(NULL, AV_LOG_INFO, "  Stream #%d:%d -> #%d:%d",
               input_streams[ost->source_index]->file_index,
               input_streams[ost->source_index]->st->index,
               ost->file_index,
               ost->index);//打印第一个输出流的map信息
        if (ost->sync_ist != input_streams[ost->source_index])
            av_log(NULL, AV_LOG_INFO, " [sync #%d:%d]",
                   ost->sync_ist->file_index,
                   ost->sync_ist->st->index);

        if (ost->stream_copy)
            av_log(NULL, AV_LOG_INFO, " (copy)");
        else {
            const AVCodec *in_codec    = input_streams[ost->source_index]->dec;//获取输入流的解码器
            const AVCodec *out_codec   = ost->enc;//获取输出流的编码器
            const char *decoder_name   = "?";
            const char *in_codec_name  = "?";
            const char *encoder_name   = "?";
            const char *out_codec_name = "?";
            const AVCodecDescriptor *desc;

            // 判断解码器是属于本地(native)还是属于第三方库
            if (in_codec) {
                decoder_name  = in_codec->name;//获取第三方库解码器名字
                desc = avcodec_descriptor_get(in_codec->id);//获取本地ffmepg的描述
                if (desc)
                    in_codec_name = desc->name;//描述存在，则从描述获取ffmpeg的解码器名字
                if (!strcmp(decoder_name, in_codec_name))//若第三方库解码器名字和ffmpeg的解码器名字相等，则认为是本地的解码器
                    decoder_name = "native";
            }

            // 判断编码器是属于本地(native)还是属于第三方库.
            if (out_codec) {
                encoder_name   = out_codec->name;//第三方库编码器名字。例如libx264
                desc = avcodec_descriptor_get(out_codec->id);//
                if (desc)
                    out_codec_name = desc->name;//ffmpeg的编码器名字，例如h264
                if (!strcmp(encoder_name, out_codec_name))//例如上面例子，因为不相等，所以encoder_name="libx264"
                    encoder_name = "native";
            }

            av_log(NULL, AV_LOG_INFO, " (%s (%s) -> %s (%s))",
                   in_codec_name, decoder_name,
                   out_codec_name, encoder_name);
        }
        av_log(NULL, AV_LOG_INFO, "\n");
    }//<== for (i = 0; i < nb_output_streams; i++) end ==>

    if (ret) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", error);
        return ret;
    }

    /* 9. 标记transcode_init()完成.
     atomic_store是一个原子操作，理解成C++的std::atomic<>模板即可*/
    //atomic_store(&transcode_init_done, 1);
    transcode_init_done = 1;

    return 0;
}

/**
 * @brief 读取包线程.多个输入文件时,一个输入文件对应一个输入线程.
 *          该函数的处理还是比较简单的.
 * @param arg
 */
#if HAVE_THREADS
//#if 0
void *FFmpegMedia::input_thread(void *arg)
{
    InputFile *f = (InputFile *)arg;
    unsigned flags = f->non_blocking ? AV_THREAD_MESSAGE_NONBLOCK : 0;
    int ret = 0;

    while (1) {
        AVPacket pkt;
        // 1. 读pkt
        ret = av_read_frame(f->ctx, &pkt);

        if (ret == AVERROR(EAGAIN)) {
            av_usleep(10000);
            continue;
        }
        if (ret < 0) {
            av_thread_message_queue_set_err_recv(f->in_thread_queue, ret);
            break;
        }

        // 2. 发送pkt到消息队列
        ret = av_thread_message_queue_send(f->in_thread_queue, &pkt, flags);
        if (flags && ret == AVERROR(EAGAIN)) {//非阻塞且eagain时
            flags = 0;
            ret = av_thread_message_queue_send(f->in_thread_queue, &pkt, flags);
            av_log(f->ctx, AV_LOG_WARNING,
                   "Thread message queue blocking; consider raising the "
                   "thread_queue_size option (current value: %d)\n",
                   f->thread_queue_size);//队列大小不够
        }
        // 3. 真正发送错误(一般是调用者设置了停止发送导致该错误),会把该pkt内部引用释放掉,并设置错误接收.
        // 看源码知道,设置错误接收是为了:当有调用者阻塞在接收包函数时, 让其返回.
        if (ret < 0) {
            if (ret != AVERROR_EOF){
                char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_log(f->ctx, AV_LOG_ERROR,
                       "Unable to send packet to main thread: %s\n",
                       FFmpegMedia::av_err2str_ex(str, ret));
            }

            av_packet_unref(&pkt);
            av_thread_message_queue_set_err_recv(f->in_thread_queue, ret);
            break;
        }
    }

    return NULL;
}

/**
 * @brief 回收一个输入文件线程.
 * @param i 输入文件下标
 */
void FFmpegMedia::free_input_thread(int i)
{
    InputFile *f = input_files[i];
    AVPacket pkt;

    if (!f || !f->in_thread_queue)
        return;

    // 1. 往线程队列发送eof. 这样做能保证生产者无法再往该队列send pkt.
    av_thread_message_queue_set_err_send(f->in_thread_queue, AVERROR_EOF);

    // 2. 将线程队列剩余的pkt释放引用
    while (av_thread_message_queue_recv(f->in_thread_queue, &pkt, 0) >= 0)
        av_packet_unref(&pkt);

    // 3. 回收该线程.
#ifndef TYY_POINT_THREAD
    pthread_join(f->thread, NULL);
#else
    pthread_join(*f->thread, NULL);
    if(f->thread){//tyycode
        av_free(f->thread);
        f->thread = nullptr;
    }
#endif

    f->joined = 1;

    // 4. 回收队列
    av_thread_message_queue_free(&f->in_thread_queue);
}

/**
 * @brief 回收所有输入文件线程.
 */
void FFmpegMedia::free_input_threads(void)
{
    int i;

    for (i = 0; i < nb_input_files; i++)
        free_input_thread(i);
}


/**
 * @brief 为输入文件创建线程，当存在多个输入文件时，一个输入文件对应一个线程。
 * @param i 输入文件的下标.
 * @return 0=成功； 负数=失败
*/
int FFmpegMedia::init_input_thread(int i)
{
    int ret;
    InputFile *f = input_files[i];

    /* 1.若输入文件数为1时，不需要开辟线程 */
    if (nb_input_files == 1)
        return 0;

    //lavfi指虚拟设备？新版本不支持该选项,可看https://www.jianshu.com/p/7f675764704b
    /*2.是否设置为非阻塞，设置条件：
     1）pb存在，则判断seekable是否为0，为0则说明不可以seek，则设置为非阻塞，否则为非0时，不会设置；
     2）pb不存在，则判断输入格式是否是lavfi，是则不设置非阻塞，否则设置为非阻塞*/
    if (f->ctx->pb ? !f->ctx->pb->seekable :
        strcmp(f->ctx->iformat->name, "lavfi"))
        f->non_blocking = 1;

    /* 3.开辟消息队列。
     内部消息队列最终是依赖AVFifoBuffer、pthread_mutex_t、pthread_cond_t去实现的. */
    ret = av_thread_message_queue_alloc(&f->in_thread_queue,
                                        f->thread_queue_size, sizeof(AVPacket));
    if (ret < 0)
        return ret;

    /* 4.创建线程 */
#ifndef TYY_POINT_THREAD
    if ((ret = pthread_create(&f->thread, NULL, input_thread, f))) {
#else
    if ((ret = pthread_create((pthread_t*)f->thread, NULL, input_thread, f))) {
#endif
        av_log(NULL, AV_LOG_ERROR, "pthread_create failed: %s. Try to increase `ulimit -v` or decrease `ulimit -s`.\n", strerror(ret));
        av_thread_message_queue_free(&f->in_thread_queue);
        return AVERROR(ret);
    }

    return 0;
}

/**
 * @brief 为每个输入文件创建线程.
 * @return 0=成功； 负数=失败
*/
int FFmpegMedia::init_input_threads(void)
{
    int i, ret;

    for (i = 0; i < nb_input_files; i++) {
        ret = init_input_thread(i);
        if (ret < 0)
            return ret;
    }
    return 0;
}

/**
 * @brief 多线程读取时,会从消息队列中读取pkt.
 * av_thread_message_queue_recv的实现不难.
 * @return 成功=0 失败返回负数(EAGAIN或者err_recv)
*/
int FFmpegMedia::get_input_packet_mt(InputFile *f, AVPacket *pkt)
{
    return av_thread_message_queue_recv(f->in_thread_queue, pkt,
                                        f->non_blocking ?
                                        AV_THREAD_MESSAGE_NONBLOCK : 0);
}
#endif


/* read a key without blocking */
int FFmpegMedia::read_key(void)
{
    unsigned char ch;
#if HAVE_TERMIOS_H
    int n = 1;
    struct timeval tv;
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    n = select(1, &rfds, NULL, NULL, &tv);
    if (n > 0) {
        n = read(0, &ch, 1);
        if (n == 1)
            return ch;

        return n;
    }
#elif HAVE_KBHIT
#    if HAVE_PEEKNAMEDPIPE
    static int is_pipe;
    static HANDLE input_handle;
    DWORD dw, nchars;
    // 1. 获取输入句柄，并且获取控制台模式(不过ffmpeg没有对控制台模式进一步处理).
    if(!input_handle){
        //获取输入设备的句柄.GetStdHandle see https://blog.csdn.net/dark_cy/article/details/89103875
        input_handle = GetStdHandle(STD_INPUT_HANDLE);
        /*GetConsoleMode() see https://docs.microsoft.com/en-us/windows/console/getconsolemode.
         返回0表示错误；非0成功.*/
        is_pipe = !GetConsoleMode(input_handle, &dw);//获取控制台模式 see https://blog.csdn.net/zanglengyu/article/details/125855938
    }

    // 2. 获取控制台模式失败的处理
    if (is_pipe) {
        /* When running under a GUI, you will end here.(当在GUI下运行时，您将在这里结束) */
        /* PeekNamedPipe()：从句柄的缓冲区读取数据，若不想读取，参2传NULL即可.管道不存在会报错返回0。
         * msdn：https://docs.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-peeknamedpipe
         * see http://www.manongjc.com/detail/19-xpsioohwzlyyvpp.html的例子*/
        if (!PeekNamedPipe(input_handle, NULL, 0, NULL, &nchars, NULL)) {//nchars是读到的字节数
            // input pipe may have been closed by the program that ran ffmpeg
            //(输入管道可能已被运行ffmpeg的程序关闭)
            return -1;
        }
        //Read it
        if(nchars != 0) {
            read(0, &ch, 1);
            return ch;
        }else{
            return -1;
        }
    }
#    endif

    if(kbhit())
        return(getch());
#endif
    return -1;
}

void FFmpegMedia::set_tty_echo(int on)
{
#if HAVE_TERMIOS_H
    struct termios tty;
    if (tcgetattr(0, &tty) == 0) {
        if (on) tty.c_lflag |= ECHO;
        else    tty.c_lflag &= ~ECHO;
        tcsetattr(0, TCSANOW, &tty);
    }
#endif
}

/**
 * @brief 处理一些键盘事件.不详细研究了.
*/
int FFmpegMedia::check_keyboard_interaction(int64_t cur_time)
{
    int i, ret, key;
    static int64_t last_time;
    if (received_nb_signals){
        printf("tyycode, received_nb_signals: %d\n", received_nb_signals);
        return AVERROR_EXIT;
    }

    /* read_key() returns 0 on EOF */
    /*1.本次距离上一次时间有0.1s及以上，且是前台运行，则从键盘读取一个字符*/
    if(cur_time - last_time >= 100000 && !run_as_daemon){
        key =  read_key();
        last_time = cur_time;
    }else
        key = -1;
    if (key == 'q')
        return AVERROR_EXIT;
    if (key == '+') av_log_set_level(av_log_get_level()+10);
    if (key == '-') av_log_set_level(av_log_get_level()-10);
    if (key == 's') qp_hist     ^= 1;
    if (key == 'h'){
        if (do_hex_dump){
            do_hex_dump = do_pkt_dump = 0;
        } else if(do_pkt_dump){
            do_hex_dump = 1;
        } else
            do_pkt_dump = 1;
        av_log_set_level(AV_LOG_DEBUG);
    }
    if (key == 'c' || key == 'C'){
        char buf[4096], target[64], command[256], arg[256] = {0};
        double time;
        int k, n = 0;
        fprintf(stderr, "\nEnter command: <target>|all <time>|-1 <command>[ <argument>]\n");
        i = 0;
        set_tty_echo(1);
        while ((k = read_key()) != '\n' && k != '\r' && i < sizeof(buf)-1)
            if (k > 0)
                buf[i++] = k;
        buf[i] = 0;
        set_tty_echo(0);
        fprintf(stderr, "\n");
        if (k > 0 &&
            (n = sscanf(buf, "%63[^ ] %lf %255[^ ] %255[^\n]", target, &time, command, arg)) >= 3) {
            av_log(NULL, AV_LOG_DEBUG, "Processing command target:%s time:%f command:%s arg:%s",
                   target, time, command, arg);
            for (i = 0; i < nb_filtergraphs; i++) {
                FilterGraph *fg = filtergraphs[i];
                if (fg->graph) {
                    if (time < 0) {
                        ret = avfilter_graph_send_command(fg->graph, target, command, arg, buf, sizeof(buf),
                                                          key == 'c' ? AVFILTER_CMD_FLAG_ONE : 0);
                        fprintf(stderr, "Command reply for stream %d: ret:%d res:\n%s", i, ret, buf);
                    } else if (key == 'c') {
                        fprintf(stderr, "Queuing commands only on filters supporting the specific command is unsupported\n");
                        ret = AVERROR_PATCHWELCOME;
                    } else {
                        ret = avfilter_graph_queue_command(fg->graph, target, command, arg, 0, time);
                        if (ret < 0){
                            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                            fprintf(stderr, "Queuing command failed with error %s\n", av_err2str_ex(str, ret));
                        }

                    }
                }
            }
        } else {
            av_log(NULL, AV_LOG_ERROR,
                   "Parse error, at least 3 arguments were expected, "
                   "only %d given in string '%s'\n", n, buf);
        }
    }
    if (key == 'd' || key == 'D'){
        int debug=0;
        if(key == 'D') {
            debug = input_streams[0]->st->codec->debug<<1;
            if(!debug) debug = 1;
            while(debug & (FF_DEBUG_DCT_COEFF
#if FF_API_DEBUG_MV
                                             |FF_DEBUG_VIS_QP|FF_DEBUG_VIS_MB_TYPE
#endif
                                                                                  )) //unsupported, would just crash
                debug += debug;
        }else{
            char buf[32];
            int k = 0;
            i = 0;
            set_tty_echo(1);
            while ((k = read_key()) != '\n' && k != '\r' && i < sizeof(buf)-1)
                if (k > 0)
                    buf[i++] = k;
            buf[i] = 0;
            set_tty_echo(0);
            fprintf(stderr, "\n");
            if (k <= 0 || sscanf(buf, "%d", &debug)!=1)
                fprintf(stderr,"error parsing debug value\n");
        }
        for(i=0;i<nb_input_streams;i++) {
            input_streams[i]->st->codec->debug = debug;
        }
        for(i=0;i<nb_output_streams;i++) {
            OutputStream *ost = output_streams[i];
            ost->enc_ctx->debug = debug;
        }
        if(debug) av_log_set_level(AV_LOG_DEBUG);
        fprintf(stderr,"debug=%d\n", debug);
    }
    if (key == '?'){
        fprintf(stderr, "key    function\n"
                        "?      show this help\n"
                        "+      increase verbosity\n"
                        "-      decrease verbosity\n"
                        "c      Send command to first matching filter supporting it\n"
                        "C      Send/Queue command to all matching filters\n"
                        "D      cycle through available debug modes\n"
                        "h      dump packets/hex press to cycle through the 3 states\n"
                        "q      quit\n"
                        "s      Show QP histogram\n"
        );
    }
    return 0;
}

/**
 * @brief 给该输出流标记ENCODER_FINISHED.
 * @param ost 输出流
*/
void FFmpegMedia::close_output_stream(OutputStream *ost)
{
    OutputFile *of = output_files[ost->file_index];

    //ost->finished |= ENCODER_FINISHED;
    ost->finished = (OSTFinished)(ost->finished | ENCODER_FINISHED);
    if (of->shortest) {//重新设置录像时长.
        int64_t end = av_rescale_q(ost->sync_opts - ost->first_pts, ost->enc_ctx->time_base, AV_TIME_BASE_Q);
        of->recording_time = FFMIN(of->recording_time, end);
    }
}

/* Return 1 if there remain streams where more output is wanted, 0 otherwise.(如果还有需要更多输出的流，则返回1，否则返回0) */
/**
 * @brief 如果还有需要更多输出的流，则返回1，否则返回0
*/
int FFmpegMedia::need_output(void)
{
    int i;

    /* 1.遍历所有输出流：
     * 1）若某个流已经完成 或者 该流>=用户指定的输出文件大小限制，那么不处理该流；
     * 2）否则，则去判断 该输出流的帧数 是否>= 用户指定的最大帧数，若>=，则会把 包含该输出流的文件的所有流 都标记为完成.*/
    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost    = output_streams[i];
        OutputFile *of       = output_files[ost->file_index];
        AVFormatContext *os  = output_files[ost->file_index]->ctx;

        if (ost->finished ||    /* 该输出流已经完成 */
            (os->pb && avio_tell(os->pb) >= of->limit_filesize))/* 当前输出io的字节大小 >= 用户指定的输出文件大小限制 */
            continue;

        if (ost->frame_number >= ost->max_frames) {//该输出流的帧数 >= 用户指定的最大帧数
            int j;
            for (j = 0; j < of->ctx->nb_streams; j++)
                close_output_stream(output_streams[of->ost_index + j]);
            continue;//不会往下执行返回1，所以当进入该if，就会返回0.
        }

        return 1;
    }

    return 0;
}

/**
 * Select the output stream to process.
 *
 * @return  selected output stream, or NULL if none available
 */
/**
 * @brief 选择一个输出流去处理。选择原理：
 * 1）若输出流没有初始化 且 输入没完成，那么优先返回该输出流;
 * 2）若已完成，则依赖输出流的st->cur_dts去选，遍历所有输出流，每次只
 *      选ost->st->cur_dts值最小的输出流进行返回。
 *
 * @return 成功=返回选择的输出流； 失败=返回NULL，表示流不可用
 */
OutputStream *FFmpegMedia::choose_output(void)
{
    int i;
    int64_t opts_min = INT64_MAX;// 记录所有输出流中dts最小的值，首次会设为最大值，是为了满足第一次if条件
    OutputStream *ost_min = NULL;// 对应dts为最小时的流

    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost = output_streams[i];
        int64_t opts = ost->st->cur_dts == AV_NOPTS_VALUE ? INT64_MIN :
                       av_rescale_q(ost->st->cur_dts, ost->st->time_base,
                                    AV_TIME_BASE_Q);// 为空则设为INT64_MIN，否则将其转成微秒单位

        if (ost->st->cur_dts == AV_NOPTS_VALUE)
            av_log(NULL, AV_LOG_DEBUG,
                "cur_dts is invalid st:%d (%d) [init:%d i_done:%d finish:%d] (this is harmless if it occurs once at the start per stream)\n",
                ost->st->index, ost->st->id, ost->initialized, ost->inputs_done, ost->finished);//(如果在每个流开始时只发生一次，那么这是无害的)

        // 1.若输出流没有初始化 且 输入没完成，那么优先返回该输出流
        if (!ost->initialized && !ost->inputs_done)
            return ost;

        /* 2.依赖输出流的st->cur_dts(opts)去选，遍历所有输出流，每次只
         * 选ost->st->cur_dts值最小的输出流进行返回 */
        if (!ost->finished && opts < opts_min) {
            opts_min = opts;
            ost_min  = ost->unavailable ? NULL : ost;
        }
    }
    return ost_min;
}

/**
 * @brief 获取一个不可用的流，若没有则返回0.
 * @return 1=该流不可用；0=可用
*/
int FFmpegMedia::got_eagain(void)
{
    int i;
    for (i = 0; i < nb_output_streams; i++)
        if (output_streams[i]->unavailable)
            return 1;
    return 0;
}

/**
 * @brief 将所有输入文件的eagain置为0，再将所有输出流的unavailable置为0
*/
void FFmpegMedia::reset_eagain(void)
{
    int i;
    for (i = 0; i < nb_input_files; i++)
        input_files[i]->eagain = 0;
    for (i = 0; i < nb_output_streams; i++)
        output_streams[i]->unavailable = 0;
}

// Filters can be configured only if the formats of all inputs are known.
//(只有当所有输入的格式都已知时，才能配置过滤器)
/**
 * @brief 遍历所有输入过滤器是否已经配置format.
 * @param fg 封装的系统过滤器
 *
 * @return 都已经配置format，返回1； 否则返回0
 */
int FFmpegMedia::ifilter_has_all_input_formats(FilterGraph *fg)
{
    int i;
    //遍历输入过滤器数组，若存在format没配置的元素，则返回0，只对视频、音频检测。
    for (i = 0; i < fg->nb_inputs; i++) {
        if (fg->inputs[i]->format < 0 && (fg->inputs[i]->type == AVMEDIA_TYPE_AUDIO ||
                                          fg->inputs[i]->type == AVMEDIA_TYPE_VIDEO))
            return 0;
    }
    return 1;
}

/**
 * @brief 回收fg,实际上就是单纯回收了AVFilterGraph.
 * @param fg
 */
void FFmpegMedia::cleanup_filtergraph(FilterGraph *fg)
{
    int i;
    //思考一下fg->outputs[i]->filter是在哪里开辟的.
    //答:在配置InputFilter,OutputFilter时使用avfilter_graph_create_filter创建的.
    //可以看到,avfilter_graph_create_filter创建的AVFilterContext不需要我们回收,ffmpeg只是将其直接置空
    for (i = 0; i < fg->nb_outputs; i++)
        fg->outputs[i]->filter = (AVFilterContext *)NULL;
    for (i = 0; i < fg->nb_inputs; i++)
        fg->inputs[i]->filter = (AVFilterContext *)NULL;
    avfilter_graph_free(&fg->graph);
}

int FFmpegMedia::sub2video_prepare(InputStream *ist, InputFilter *ifilter)
{
    AVFormatContext *avf = input_files[ist->file_index]->ctx;
    int i, w, h;

    /* Compute the size of the canvas for the subtitles stream.
       If the subtitles codecpar has set a size, use it. Otherwise use the
       maximum dimensions of the video streams in the same file. */
    w = ifilter->width;
    h = ifilter->height;
    if (!(w && h)) {
        for (i = 0; i < avf->nb_streams; i++) {
            if (avf->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                w = FFMAX(w, avf->streams[i]->codecpar->width);
                h = FFMAX(h, avf->streams[i]->codecpar->height);
            }
        }
        if (!(w && h)) {
            w = FFMAX(w, 720);
            h = FFMAX(h, 576);
        }
        av_log(avf, AV_LOG_INFO, "sub2video: using %dx%d canvas\n", w, h);
    }
    ist->sub2video.w = ifilter->width  = w;
    ist->sub2video.h = ifilter->height = h;

    ifilter->width  = ist->dec_ctx->width  ? ist->dec_ctx->width  : ist->sub2video.w;
    ifilter->height = ist->dec_ctx->height ? ist->dec_ctx->height : ist->sub2video.h;

    /* rectangles are AV_PIX_FMT_PAL8, but we have no guarantee that the
       palettes for all rectangles are identical or compatible */
    ifilter->format = AV_PIX_FMT_RGB32;

    ist->sub2video.frame = av_frame_alloc();
    if (!ist->sub2video.frame)
        return AVERROR(ENOMEM);
    ist->sub2video.last_pts = INT64_MIN;
    ist->sub2video.end_pts  = INT64_MIN;
    return 0;
}

/**
 * @brief 新建AVFilterContext, 并进行filter之间进行连接.
 * 与ffplay的INSERT_FILT宏类似,不过INSERT_FILT宏是倒转过来连接的,
 * INSERT_FILT宏last_filter是向前连接,这里last_filter是向后连接.
 *
 * @param last_filter       上一个AVFilterContext.
 * @param pad_idx           输入AVFilterContext的 pad 下标
 * @param filter_name       AVFilterContext name.
 * @param args              新建AVFilterContext的字符串描述.
 *
 * @return 成功-0 失败负数
*/
int FFmpegMedia::insert_filter(AVFilterContext **last_filter, int *pad_idx,
                         const char *filter_name, const char *args)
{
    AVFilterGraph *graph = (*last_filter)->graph;
    AVFilterContext *ctx;
    int ret;

    // 1. 创建一个滤波器实例AVFilterContext，并添加到AVFilterGraph中
    ret = avfilter_graph_create_filter(&ctx,
                                       avfilter_get_by_name(filter_name),
                                       filter_name, args, NULL, graph);
    if (ret < 0)
        return ret;

    // 2. filter之间进行连接.将 last_filter 连接到 ctx.
    // srcpad,dstpad下标一般都是填0.
    ret = avfilter_link(*last_filter, *pad_idx, ctx, 0);
    if (ret < 0)
        return ret;

    // 3. 更新last_filter
    *last_filter = ctx;
    *pad_idx     = 0;
    return 0;
}

/**
 * @brief 插入trim filter.
 *
 * @param start_time trim的首帧开始时间.
 * @param duration  trim的最大输出时间.
 * @param last_filter AVFilterContext链表,指向链表尾部.
 * @param pad_idx 使用AVFilterContext的输入pad的下标,一般都是0.
 *          注,AVFilterContext可以有多个输入输出AVFilterPad,代表数据的流向,一般输入输出接口都是0.
 * @param filter_name trim filter的名字.
 *
 * @return 成功-0,注start_time,duration为无效值同样返回成功. 失败-负数
*/
int FFmpegMedia::insert_trim(int64_t start_time, int64_t duration,
                       AVFilterContext **last_filter, int *pad_idx,
                       const char *filter_name)
{
    AVFilterGraph *graph = (*last_filter)->graph;
    AVFilterContext *ctx;
    const AVFilter *trim;
    // 1. 获取AVFilterContext下标为pad_idx输出口的媒体类型.
    // 注,AVFilterContext可以有多个输入输出口(即AVFilterPad).
    enum AVMediaType type = avfilter_pad_get_type((*last_filter)->output_pads, *pad_idx);
    const char *name = (type == AVMEDIA_TYPE_VIDEO) ? "trim" : "atrim";
    int ret = 0;

    // 开始时间、时长两个值必须要有一个有效以上才会插入trim filter
    if (duration == INT64_MAX && start_time == AV_NOPTS_VALUE)
        return 0;

    // 2. 通过名字获取trim filter(AVFilter类型).类似"buffer","scale","buffersink"这些滤镜
    trim = avfilter_get_by_name(name);
    if (!trim) {
        av_log(NULL, AV_LOG_ERROR, "%s filter not present, cannot limit "
               "recording time.\n", name);
        return AVERROR_FILTER_NOT_FOUND;
    }

    // 3. 创建fliter_ctx，参1为graph,参2为avfilter_get_by_name返回的filter实例,参3为给这个实例自定义的名称，不然可能默认是null.
    //类似函数：ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
    //avfilter_graph_create_filter内部会调用avfilter_graph_alloc_filter,并且会进行初始化,看源码不难.
    ctx = avfilter_graph_alloc_filter(graph, trim, filter_name);
    if (!ctx)
        return AVERROR(ENOMEM);

    //设置输出最大时长, Maximum duration of the output(see libavfilter/trim.c)
    if (duration != INT64_MAX) {
        ret = av_opt_set_int(ctx, "durationi", duration,
                                AV_OPT_SEARCH_CHILDREN);
    }
    //设置首帧开始时间, Timestamp of the first frame that(see libavfilter/trim.c)
    if (ret >= 0 && start_time != AV_NOPTS_VALUE) {
        ret = av_opt_set_int(ctx, "starti", start_time,
                                AV_OPT_SEARCH_CHILDREN);
    }
    if (ret < 0) {
        av_log(ctx, AV_LOG_ERROR, "Error configuring the %s filter", name);
        return ret;
    }

    // 初始化AVFilterContext. avfilter_graph_alloc_filter+avfilter_init_str 等价于 avfilter_graph_create_filter.
    ret = avfilter_init_str(ctx, NULL);//这里的字符串描述是NULL,因为也可以通过av_opt_set_int设置,
                                       //看了一下源码,传字符串描述最终也是通过av_opt_set设置的(大概看了一下).
    if (ret < 0)
        return ret;

    // 4. 连接AVFilterContext.
    ret = avfilter_link(*last_filter, *pad_idx, ctx, 0);
    if (ret < 0)
        return ret;

    *last_filter = ctx;
    *pad_idx     = 0;
    return 0;
}

//获取旋转角度
double FFmpegMedia::get_rotation(AVStream *st)
{
    /*
     * av_stream_get_side_data()：从信息流中获取边信息。
     * 参1：流；参2：所需的边信息类型；参3：用于存储边信息大小的指针(可选)；
     * 存在返回数据指针，，否则为NULL。
     *
     * AV_PKT_DATA_DISPLAYMATRIX：这个边数据包含一个描述仿射的3x3变换矩阵转换，
     * 需要应用到解码的视频帧正确的显示。数据的详细描述请参见libavutil/display.h
    */
    uint8_t* displaymatrix = av_stream_get_side_data(st,
                                                     AV_PKT_DATA_DISPLAYMATRIX, NULL);

    /*
     * av_display_rotation_get()：提取变换矩阵的旋转分量。
     * 参1：转换矩阵。
     * 返回转换旋转帧的角度(以度为单位)逆时针方向。角度将在[-180.0,180.0]范围内。如果矩阵是奇异的则返回NaN。
     * @note：浮点数本质上是不精确的，所以调用者是建议在使用前将返回值舍入到最接近的整数。
    */
    double theta = 0;
    if (displaymatrix)
        theta = -av_display_rotation_get((int32_t*) displaymatrix);

    theta -= 360*floor(theta/360 + 0.9/360);

    if (fabs(theta - 90*round(theta/90)) > 2)
        av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
               "If you want to help, upload a sample "
               "of this file to ftp://upload.ffmpeg.org/incoming/ "
               "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");

    return theta;
}

/**
 * @brief 配置输入视频过滤器.该函数执行完,filter链表大概是这样的:
 * buffer->insert_filter函数的滤镜(transpose,hflip,vflip,rotate)->yadif->trim->in->filter_ctx.
 * @param fg fg
 * @param ifilter InputFilter
 * @param in AVFilterInOut. 例如是avfilter_graph_parse2解析过滤器字符串时的传出参数inputs.
 *
 * @return 成功-0 失败-负数
*/
int FFmpegMedia::configure_input_video_filter(FilterGraph *fg, InputFilter *ifilter,
                                        AVFilterInOut *in)
{
    AVFilterContext *last_filter;
    // 1. 获取输入源filter(AVFilter类型)--->buffer
    const AVFilter *buffer_filt = avfilter_get_by_name("buffer");
    InputStream *ist = ifilter->ist;
    InputFile     *f = input_files[ist->file_index];
    AVRational tb = ist->framerate.num ? av_inv_q(ist->framerate) :
                                         ist->st->time_base;//av_inv_q是分子分母调转然后返回.帧率存在,使用帧率倒数作为时基,否则使用ist->st->time_base
    AVRational fr = ist->framerate;
    AVRational sar;
    AVBPrint args;//一个string buffer,当成C++的string理解
    char name[255];
    int ret, pad_idx = 0;
    int64_t tsoffset = 0;
    AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();

    if (!par)
        return AVERROR(ENOMEM);
    memset(par, 0, sizeof(*par));
    par->format = AV_PIX_FMT_NONE;

    // 2. 如果输入流是音频,返回.因为本函数是配置视频的filter
    if (ist->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        av_log(NULL, AV_LOG_ERROR, "Cannot connect video filter to audio input\n");
        ret = AVERROR(EINVAL);
        goto fail;
    }

    if (!fr.num)//若ist->framerate为空,会通过容器和编解码器猜出对应的帧率
        fr = av_guess_frame_rate(input_files[ist->file_index]->ctx, ist->st, NULL);

    // 3. 字幕相关,暂不研究,推流命令没用到.
    if (ist->dec_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
        ret = sub2video_prepare(ist, ifilter);
        if (ret < 0)
            goto fail;
    }

    sar = ifilter->sample_aspect_ratio;// 宽高比率,一般都是{0,1}
    if(!sar.den)
        sar = (AVRational){0,1};

    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
    //例如执行av_bprintf后:args.str="video_size=848x480:pix_fmt=0:time_base=1/1000:pixel_aspect=0/1:sws_param=flags=2"
    av_bprintf(&args,
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:"
             "pixel_aspect=%d/%d:sws_param=flags=%d",
             ifilter->width, ifilter->height, ifilter->format,
             tb.num, tb.den, sar.num, sar.den,
             SWS_BILINEAR + ((ist->dec_ctx->flags&AV_CODEC_FLAG_BITEXACT) ? SWS_BITEXACT:0));
    //追加帧率,例如: args.str="video_size=848x480:pix_fmt=0:time_base=1/1000:pixel_aspect=0/1:sws_param=flags=2:frame_rate=25/1"
    if (fr.num && fr.den)
        av_bprintf(&args, ":frame_rate=%d/%d", fr.num, fr.den);

    // 4 创建一个滤波器实例AVFilterContext，并添加到AVFilterGraph中.(这个是输入buffer filter)
    snprintf(name, sizeof(name), "graph %d input from stream %d:%d", fg->index,
             ist->file_index, ist->st->index);//要给创建的filter实例的实例名,可任意,不要与其它重复即可.
    if ((ret = avfilter_graph_create_filter(&ifilter->filter, buffer_filt, name,
                                            args.str, NULL, fg->graph)) < 0)
        goto fail;

    // 5. 将处理过的par参数传递给AVFilterContext.
    // 具体是传递给成员priv,priv是一个BufferSourceContext类型成员.
    /* av_buffersrc_parameters_set()(源码很简单):
     * 使用提供的参数初始化 buffersrc 或 abuffersrc filter。这个函数可以被调用多次，之后的调用会覆盖之前的调用。
     * 一些参数也可以通过AVOptions设置，然后最后使用的方法优先.
     * 参1: an instance of the buffersrc or abuffersrc filter.
     * 参2: 流参数。随后传递给这个过滤器的帧必须符合这些参数。所有在param中分配的字段仍然属于调用者，libavfilter将在必要时进行内部复制或引用.
     * return 0 on success, a negative AVERROR code on failure. */
    par->hw_frames_ctx = ifilter->hw_frames_ctx;
    ret = av_buffersrc_parameters_set(ifilter->filter, par);
    if (ret < 0)
        goto fail;
    av_freep(&par);//av_buffersrc_parameters_alloc开辟的内存仍属于调用者,设置完后需要释放.
    last_filter = ifilter->filter;//保存该AVFilterContext

    // 6. 判断是否有旋转角度，如果有需要使用对应的过滤器进行处理；没有则不会添加.
    /*
     * 一 图片的相关旋转操作命令：
     * 1）垂直翻转：                      ffmpeg -i fan.jpg -vf vflip -y vflip.png
     * 2）水平翻转：                      ffmpeg -i fan.jpg -vf hflip -y hflip.png
     * 3）顺时针旋转60°(PI代表180°)：      ffmpeg -i fan.jpg -vf rotate=PI/3 -y rotate60.png
     * 4）顺时针旋转90°：                 ffmpeg -i fan.jpg -vf rotate=90*PI/180 -y rotate90.png
     * 5）逆时针旋转90°(负号代表逆时针，正号代表顺时针)：ffmpeg -i fan.jpg -vf rotate=-90*PI/180 -y rotate90-.png
     * 6）逆时针旋转90°：                  ffmpeg -i fan.jpg -vf transpose=2 -y transpose2.png
     * rotate、transpose的值具体使用ffmpeg -h filter=filtername去查看。
     * 注意1：上面的图片使用ffprobe去看不会有metadata元数据，所以自然不会有rotate与Side data里面的displaymatrix。只有视频才有。
     * 注意2：使用是rotate带有黑底的，例如上面的rotate60.png。图片的很好理解，都是以原图进行正常的旋转，没有难度。
     *
     *
     * 二 视频文件相关旋转的操作：
     * 1.1 使用rotate选项：
     * 1） ffmpeg -i 2_audio.mp4 -metadata:s:v rotate='90' -codec copy 2_audio_rotate90.mp4
     * 但是这个命令实际效果是：画面变成逆时针的90°操作。使用ffprobe一看：
     *     Metadata:
              rotate          : 270
              handler_name    : VideoHandler
           Side data:
              displaymatrix: rotation of 90.00 degrees
           Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 44100 Hz, stereo, fltp, 184 kb/s (default)
            Metadata:
              handler_name    : 粤语
    * 可以看到rotate是270°，但displaymatrix确实是转了90°。
    *
    * 2）ffmpeg -i 2_audio.mp4 -metadata:s:v rotate='270' -codec copy 2_audio_rotate270.mp4
    * 同样rotate='270'时，画面变成顺时针90°的操作。rotate=90，displaymatrix=rotation of -90.00 degrees。
    *
    * 3）ffmpeg -i 2_audio.mp4 -metadata:s:v rotate='180' -codec copy 2_audio_rotate180.mp4
    * 而180的画面是倒转的，这个可以理解。rotate=180，displaymatrix=rotation of -180.00 degrees。
    *
    * 2.1 使用transpose选项
    * 1）ffmpeg -i 2_audio.mp4  -vf transpose=1 -codec copy 2_audio_transpose90.mp4(顺时针90°)
    * 2）ffmpeg -i 2_audio.mp4  -vf transpose=2 2_audio_transpose-90.mp4(逆时针90°，不能加-codec copy，否则与transpose冲突)
    * 上面命令按预期正常顺时针的旋转了90°和逆时针旋转90°的画面，但是使用ffprobe看不到rotate或者displaymatrix对应的角度。
    *
    * 3.1 使用rotate+transpose选项
    * 1） ffmpeg -i 2_audio.mp4 -vf transpose=1 -metadata:s:v rotate='90' -vcodec libx264 2_audio_rotate90.mp4
    * 2）ffmpeg -i 2_audio.mp4 -vf transpose=1 -metadata:s:v rotate='180' -vcodec libx264 2_audio_rotate180.mp4
    * 3）ffmpeg -i 2_audio.mp4 -vf transpose=1 -metadata:s:v rotate='270' -vcodec libx264 2_audio_rotate270.mp4
    * 只要使用了transpose选项，rotate就会失效。例如运行上面三个命令，实际只顺时针旋转了90°，即transpose=1的效果，并且，只要存在transpose，它和2.1一样，
    *   使用ffprobe看不到rotate或者displaymatrix对应的角度，这种情况是我们不愿意看到的。所以经过分析，我们最终还是得回到只有rotate选项的情况。
    *
    * 目前我们先记着1.1的三种情况的结果就行，后续有空再深入研究旋转，并且实时流一般都会返回theta=0，不会有旋转的操作。
    */
    if (ist->autorotate) {//与ffplay处理旋转角度是一样的.
        double theta = get_rotation(ist->st);

        if (fabs(theta - 90) < 1.0) {
            // 转换过滤器，clock代表，顺时针旋转，等价于命令transpose=1。
            // 可用ffmpeg -h filter=transpose查看。查看所有filter：ffmpeg -filters
            ret = insert_filter(&last_filter, &pad_idx, "transpose", "clock");
        } else if (fabs(theta - 180) < 1.0) {
            ret = insert_filter(&last_filter, &pad_idx, "hflip", NULL);// 镜像左右反转过滤器
            if (ret < 0)
                return ret;
            ret = insert_filter(&last_filter, &pad_idx, "vflip", NULL);// 镜像上下反转过滤器，经过这个过滤器处理后，画面会反转，类似水中倒影。
        } else if (fabs(theta - 270) < 1.0) {
            ret = insert_filter(&last_filter, &pad_idx, "transpose", "cclock");// 逆时针旋转，等价于命令transpose=2.
        } else if (fabs(theta) > 1.0) {
            char rotate_buf[64];
            snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
            ret = insert_filter(&last_filter, &pad_idx, "rotate", rotate_buf);// 旋转角度过滤器
        }
        if (ret < 0)
            return ret;
    }

    // 7. 若指定-deinterlace选项,创建yadif过滤器并添加到graph中.
    if (do_deinterlace) {// -deinterlace选项
        AVFilterContext *yadif;

        snprintf(name, sizeof(name), "deinterlace_in_%d_%d",
                 ist->file_index, ist->st->index);
        if ((ret = avfilter_graph_create_filter(&yadif,
                                                avfilter_get_by_name("yadif"),
                                                name, "", NULL,
                                                fg->graph)) < 0)
            return ret;

        if ((ret = avfilter_link(last_filter, 0, yadif, 0)) < 0)
            return ret;

        last_filter = yadif;
    }

    // 8. 指定了录像相关的选项,会插入trim filter.
    snprintf(name, sizeof(name), "trim_in_%d_%d",
             ist->file_index, ist->st->index);
    if (copy_ts) {
        //若用户指定start_time(-ss选项),则获取,否则为0.
        tsoffset = f->start_time == AV_NOPTS_VALUE ? 0 : f->start_time;
        //若用户指定-copyts选项,但没指定-start_at_zero选项,且f->ctx->start_time有效,
        //加上f->ctx->start_time的值保存在tsoffset.
        if (!start_at_zero && f->ctx->start_time != AV_NOPTS_VALUE)
            tsoffset += f->ctx->start_time;
    }
    // 没传-ss选项 或者 传了-ss但是没启用精确查找-accurate_seek选项,insert_trim的开始时间为AV_NOPTS_VALUE,否则为tsoffset.
    // 也就说只有传了-ss以及-accurate_seek选项才会指定trim的start_time.等价于语句:
    // f->start_time != AV_NOPTS_VALUE && f->accurate_seek ? tsoffset : AV_NOPTS_VALUE
    ret = insert_trim(((f->start_time == AV_NOPTS_VALUE) || !f->accurate_seek) ?
                      AV_NOPTS_VALUE : tsoffset, f->recording_time,
                      &last_filter, &pad_idx, name);
    if (ret < 0)
        return ret;

    // 9. 连接.执行到这里,最终指向输入参数的in->filter_ctx.
    // in参数是由avfilter_graph_parse2解析过滤器字符串时的传出参数inputs.
    if ((ret = avfilter_link(last_filter, 0, in->filter_ctx, in->pad_idx)) < 0)
        return ret;
    return 0;
fail:
    av_freep(&par);

    return ret;
}

/**
 * @brief 配置输入音频过滤器. 该函数执行完,filter链表大概是这样的:
 * abuffer->aresample->volume->trim->in->filter_ctx.
 *
 * @param fg        fg
 * @param ifilter   InputFilter
 * @param in        AVFilterInOut. 例如是avfilter_graph_parse2解析过滤器字符串时的传出参数inputs.
 *
 * @return 成功-0 失败-负数
*/
int FFmpegMedia::configure_input_audio_filter(FilterGraph *fg, InputFilter *ifilter,
                                        AVFilterInOut *in)
{
    AVFilterContext *last_filter;
    // 1. 获取输入源filter(AVFilter类型)--->abuffer.(注意视频的是buffer,这个是固定的)
    const AVFilter *abuffer_filt = avfilter_get_by_name("abuffer");
    InputStream *ist = ifilter->ist;
    InputFile     *f = input_files[ist->file_index];
    AVBPrint args;
    char name[255];
    int ret, pad_idx = 0;
    int64_t tsoffset = 0;

    //该输入过滤器所属的输入流的解码器不是音频,直接返回错误.
    if (ist->dec_ctx->codec_type != AVMEDIA_TYPE_AUDIO) {
        av_log(NULL, AV_LOG_ERROR, "Cannot connect audio filter to non audio input\n");
        return AVERROR(EINVAL);
    }

    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);//初始化字符串buffer
    av_bprintf(&args, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s",
             1, ifilter->sample_rate,
             ifilter->sample_rate,
             av_get_sample_fmt_name((AVSampleFormat)ifilter->format));//添加字符串描述到args.str
    //存在通道布局则在args.str末尾追加,否则追加通道数的字符串描述
    if (ifilter->channel_layout)
        av_bprintf(&args, ":channel_layout=0x%" PRIx64, ifilter->channel_layout);
    else
        av_bprintf(&args, ":channels=%d", ifilter->channels);

    // 2. 创建一个滤波器实例AVFilterContext，并添加到AVFilterGraph中.(这个是输入abuffer filter)
    snprintf(name, sizeof(name), "graph_%d_in_%d_%d", fg->index,
             ist->file_index, ist->st->index);
    if ((ret = avfilter_graph_create_filter(&ifilter->filter, abuffer_filt,
                                            name, args.str, NULL,
                                            fg->graph)) < 0)
        return ret;
    last_filter = ifilter->filter;

    //该宏与insert_filter()函数功能是一样的.
#define AUTO_INSERT_FILTER_INPUT(opt_name, filter_name, arg) do {                 \
    AVFilterContext *filt_ctx;                                              \
                                                                            \
    av_log(NULL, AV_LOG_INFO, opt_name " is forwarded to lavfi "            \
           "similarly to -af " filter_name "=%s.\n", arg);                  \
                                                                            \
    snprintf(name, sizeof(name), "graph_%d_%s_in_%d_%d",      \
                fg->index, filter_name, ist->file_index, ist->st->index);   \
    ret = avfilter_graph_create_filter(&filt_ctx,                           \
                                       avfilter_get_by_name(filter_name),   \
                                       name, arg, NULL, fg->graph);         \
    if (ret < 0)                                                            \
        return ret;                                                         \
                                                                            \
    ret = avfilter_link(last_filter, 0, filt_ctx, 0);                       \
    if (ret < 0)                                                            \
        return ret;                                                         \
                                                                            \
    last_filter = filt_ctx;                                                 \
} while (0)

    // 3. 若设置-async选项,添加aresample滤镜.
    if (audio_sync_method > 0) {
        char args[256] = {0};

        //async选项:简化1个参数音频时间戳匹配,0(禁用),1(填充和微调)，>1(每秒最大采样拉伸/压缩).该选项会被设置到SwrContext.async.
        av_strlcatf(args, sizeof(args), "async=%d", audio_sync_method);

        //若设置-adrift_threshold选项,则改变SwrContext.min_hard_compensation的值.
        if (audio_drift_threshold != 0.1)
            /* min_hard_comp: 设置时间戳和音频数据之间的最小差异(以秒为单位)，以触发填充/修剪数据.
             * 该选项会被设置到SwrContext.min_hard_compensation.
             * min_hard_compensation的解释: SWR最小值，低于此值将不会发生无声注入/样品下降. */
            av_strlcatf(args, sizeof(args), ":min_hard_comp=%f", audio_drift_threshold);

        //first_pts: 假设第一个pts应该是这个值(在样本中). 该选项会被设置到SwrContext.firstpts_in_samples.
        if (!fg->reconfiguration)
            av_strlcatf(args, sizeof(args), ":first_pts=0");

        AUTO_INSERT_FILTER_INPUT("-async", "aresample", args);
    }

//     if (ost->audio_channels_mapped) {
//         int i;
//         AVBPrint pan_buf;
//         av_bprint_init(&pan_buf, 256, 8192);
//         av_bprintf(&pan_buf, "0x%"PRIx64,
//                    av_get_default_channel_layout(ost->audio_channels_mapped));
//         for (i = 0; i < ost->audio_channels_mapped; i++)
//             if (ost->audio_channels_map[i] != -1)
//                 av_bprintf(&pan_buf, ":c%d=c%d", i, ost->audio_channels_map[i]);
//         AUTO_INSERT_FILTER_INPUT("-map_channel", "pan", pan_buf.str);
//         av_bprint_finalize(&pan_buf, NULL);
//     }

    // 4. 若指定-vol选项改变音量,添加volume滤镜.
    if (audio_volume != 256) {
        char args[256];

        av_log(NULL, AV_LOG_WARNING, "-vol has been deprecated. Use the volume "
               "audio filter instead.\n");

        //我们看到设置音量时是没有key的,只有一个value,有兴趣的可以看看最终设置到哪个变量.
        snprintf(args, sizeof(args), "%f", audio_volume / 256.);
        AUTO_INSERT_FILTER_INPUT("-vol", "volume", args);
    }

    // 5. 指定了录像相关的选项,会插入trim filter.这部分代码与视频是一样的.
    snprintf(name, sizeof(name), "trim for input stream %d:%d",
             ist->file_index, ist->st->index);
    if (copy_ts) {
        tsoffset = f->start_time == AV_NOPTS_VALUE ? 0 : f->start_time;
        if (!start_at_zero && f->ctx->start_time != AV_NOPTS_VALUE)
            tsoffset += f->ctx->start_time;
    }
    ret = insert_trim(((f->start_time == AV_NOPTS_VALUE) || !f->accurate_seek) ?
                      AV_NOPTS_VALUE : tsoffset, f->recording_time,
                      &last_filter, &pad_idx, name);
    if (ret < 0)
        return ret;

    // 6. 连接.执行到这里,最终指向输入参数的in->filter_ctx.
    // in参数是由avfilter_graph_parse2解析过滤器字符串时的传出参数inputs.
    if ((ret = avfilter_link(last_filter, 0, in->filter_ctx, in->pad_idx)) < 0)
        return ret;

    return 0;
}

/**
 * @brief 配置输入过滤器.
 * @param fg fg
 * @param ifilter InputFilter
 * @param in AVFilterInOut. 例如是avfilter_graph_parse2解析过滤器字符串时的传出参数inputs.
 *
 * @return 成功-0 失败-负数
 */
int FFmpegMedia::configure_input_filter(FilterGraph *fg, InputFilter *ifilter,
                                  AVFilterInOut *in)
{
    // 1. 输入流的解码器没找到,返回错误.
    if (!ifilter->ist->dec) {
        av_log(NULL, AV_LOG_ERROR,
               "No decoder for stream #%d:%d, filtering impossible\n",
               ifilter->ist->file_index, ifilter->ist->st->index);
        return AVERROR_DECODER_NOT_FOUND;
    }

    // 2. 根据不同的媒体类型配置输入filter.
    switch (avfilter_pad_get_type(in->filter_ctx->input_pads, in->pad_idx)) {
    case AVMEDIA_TYPE_VIDEO: return configure_input_video_filter(fg, ifilter, in);
    case AVMEDIA_TYPE_AUDIO: return configure_input_audio_filter(fg, ifilter, in);
    default: /*av_assert0(0);*/ return -1;
    }
}

/**
 * @brief 根据codec_id获取相关像素格式.  比较简单,了解一下即可.
*/
const enum AVPixelFormat *FFmpegMedia::get_compliance_unofficial_pix_fmts(enum AVCodecID codec_id, const enum AVPixelFormat default_formats[])
{
    //mpeg相关像素格式,主要是yuv相关的包格式
    static const enum AVPixelFormat mjpeg_formats[] =
        { AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_YUVJ444P,
          AV_PIX_FMT_YUV420P,  AV_PIX_FMT_YUV422P,  AV_PIX_FMT_YUV444P,
          AV_PIX_FMT_NONE };
    //jpeg相关像素格式,类似mpeg,比mpeg的种类多一点
    static const enum AVPixelFormat ljpeg_formats[] =
        { AV_PIX_FMT_BGR24   , AV_PIX_FMT_BGRA    , AV_PIX_FMT_BGR0,
          AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ444P, AV_PIX_FMT_YUVJ422P,
          AV_PIX_FMT_YUV420P , AV_PIX_FMT_YUV444P , AV_PIX_FMT_YUV422P,
          AV_PIX_FMT_NONE};

    if (codec_id == AV_CODEC_ID_MJPEG) {
        return mjpeg_formats;
    } else if (codec_id == AV_CODEC_ID_LJPEG) {
        return ljpeg_formats;
    } else {
        return default_formats;
    }
}

/**
 * @brief 该函数选择像素格式思路很简单:
 * 1. 编解码器的像素数组存在的情况：判断target在数组中，则返回target;否则会自动选择,返回对应的best像素格式.
 * 2. 编解码器的像素数组存在的情况：直接返回target.
 *
 * @param st 流
 * @param enc_ctx 编解码器上下文
 * @param codec 编解码器
 * @param target 想要的像素格式
 * @return AVPixelFormat
*/
enum AVPixelFormat FFmpegMedia::choose_pixel_fmt(AVStream *st, AVCodecContext *enc_ctx, AVCodec *codec, enum AVPixelFormat target)
{
    // 1. 编解码器的像素格式数组存在,进入if.
    if (codec && codec->pix_fmts) {
        const enum AVPixelFormat *p = codec->pix_fmts;// 编解码器的默认像素格式数组.
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(target);//通过目标像素格式获取其描述.
        //FIXME: This should check for AV_PIX_FMT_FLAG_ALPHA after PAL8 pixel format without alpha is implemented
        int has_alpha = desc ? desc->nb_components % 2 == 0 : 0;// desc存在: nb_components为偶数,has_alpha=1,为奇数has_alpha=0;
                                                                // desc不存在: has_alpha=0;
        enum AVPixelFormat best= AV_PIX_FMT_NONE;

        // 允许合规的非官方的标准,那么会根据enc_ctx->codec_id改变p数组的值,也有可能不改变.
        if (enc_ctx->strict_std_compliance <= FF_COMPLIANCE_UNOFFICIAL) {
            p = get_compliance_unofficial_pix_fmts(enc_ctx->codec_id, p);
        }

        // 遍历target是否在数组p, 存在则退出,不存在会遍历到数组末尾退出
        for (; *p != AV_PIX_FMT_NONE; p++) {
            /* avcodec_find_best_pix_fmt_of_2():内部单纯调用av_find_best_pix_fmt_of_2().
             * av_find_best_pix_fmt_of_2():
             * 计算当从一种特定像素格式转换到另一种格式时将发生何种损失。
             * 当从一种像素格式转换到另一种像素格式时，可能会发生信息丢失。
             * 例如，从RGB24转换为GRAY时，会丢失颜色信息。类似地，在从某些格式转换到其他格式时也会发生其他损失。
             * 这些损失包括色度的损失，也包括分辨率的损失，颜色深度的损失，由于颜色空间转换的损失，阿尔法位的损失或由于颜色量化的损失.
             * av_get_fix_fmt_loss()告诉您从一种像素格式转换到另一种像素格式时会发生的各种类型的丢失.
             * 参1 目标像素格式1
             * 参2 目标像素格式2
             * 参3 源像素格式
             * 参4 是否使用源像素格式alpha通道
             * 返回值: 返回标志的组合，通知您将发生何种损失(对于无效的dst_pix_fmt表示是最大损失). */
            best= avcodec_find_best_pix_fmt_of_2(best, *p, target, has_alpha, NULL);
            if (*p == target)
                break;
        }

        //这里看到,只有像素格式数组p不支持target,ffmpeg才会返回自动选择的best,支持的话就一定会返回target.
        if (*p == AV_PIX_FMT_NONE) {
            if (target != AV_PIX_FMT_NONE)
                av_log(NULL, AV_LOG_WARNING,
                       "Incompatible pixel format '%s' for codec '%s', auto-selecting format '%s'\n",
                       av_get_pix_fmt_name(target),
                       codec->name,
                       av_get_pix_fmt_name(best));
            return best;
        }
    }

    return target;
}

/**
 * @brief 获取输出像素格式的名字.
 * @param ofilter OutputFilter
 * @return 成功-找到返回对应的像素格式,找不到返回NULL; 失败-程序退出.
*/
char *FFmpegMedia::choose_pix_fmts(OutputFilter *ofilter)
{
    OutputStream *ost = ofilter->ost;

    // 1. -strict选项.
    // 可以通过设置到ost->encoder_opts中来设置-strict选项的值.
    AVDictionaryEntry *strict_dict = av_dict_get(ost->encoder_opts, "strict", NULL, 0);
    if (strict_dict)
        // used by choose_pixel_fmt() and below
        // libavcodec/options_table.h的avcodec_options数组.
        // avcodec_options数组内部有两个"strict"选项,其中一个会设置到变量AVCodecContext.strict_std_compliance.
        // strict大概意思是严格遵守相关标准.
        av_opt_set(ost->enc_ctx, "strict", strict_dict->value, 0);

    // 2. 若keep_pix_fmt不为0,直接从编解码器上下文返回像素格式.
     if (ost->keep_pix_fmt) {
        /* avfilter_graph_set_auto_convert(): 启用或禁用图形内部的自动格式转换(源码很简单)。
         * 请注意，格式转换仍然可以在显式插入的 scale和aresample filters 中发生。
         * @param标记任何AVFILTER_AUTO_CONVERT_*常量. */
        avfilter_graph_set_auto_convert(ofilter->graph->graph,
                                            AVFILTER_AUTO_CONVERT_NONE);
        if (ost->enc_ctx->pix_fmt == AV_PIX_FMT_NONE)
            return NULL;
        return av_strdup(av_get_pix_fmt_name(ost->enc_ctx->pix_fmt));
    }

    // 3. 返回像素名字.
    if (ost->enc_ctx->pix_fmt != AV_PIX_FMT_NONE) { // 3.1 优先从编解码器上下文中返回
        return av_strdup(av_get_pix_fmt_name(choose_pixel_fmt(ost->st, ost->enc_ctx, ost->enc, ost->enc_ctx->pix_fmt)));
    } else if (ost->enc && ost->enc->pix_fmts) {    // 3.2 否则从编解码器中返回
        const enum AVPixelFormat *p;
        AVIOContext *s = NULL;
        uint8_t *ret;
        int len;

        if (avio_open_dyn_buf(&s) < 0)//打开一个只写的内存流
            exit_program(1);

        p = ost->enc->pix_fmts;
        if (ost->enc_ctx->strict_std_compliance <= FF_COMPLIANCE_UNOFFICIAL) {
            p = get_compliance_unofficial_pix_fmts(ost->enc_ctx->codec_id, p);
        }

        //把像素数组里的元素 换成 像素名字拼接,通过ret指针返回.
        for (; *p != AV_PIX_FMT_NONE; p++) {
            const char *name = av_get_pix_fmt_name(*p);
            avio_printf(s, "%s|", name);
        }
        len = avio_close_dyn_buf(s, &ret);
        ret[len - 1] = 0;
        return (char*)ret;
    } else
        return NULL;// 3.3否则返回NULL
}

/**
 * @brief 配置输出视频过滤器.该函数执行完,filter链表大概是这样的:
 * out->filter_ctx->scale->format->fps->trim->buffersink。
 * 与配置输入视频过滤器的顺序可认为是相反的.
 *
 * @param fg fg
 * @param ofilter OutputFilter
 * @param out AVFilterInOut. 例如是avfilter_graph_parse2解析过滤器字符串时的传出参数outputs.
 *
 * @return 成功-0 失败-负数,或程序退出(choose_pix_fmts).
*/
int FFmpegMedia::configure_output_video_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out)
{
    char *pix_fmts;
    OutputStream *ost = ofilter->ost;
    OutputFile    *of = output_files[ost->file_index];
    AVFilterContext *last_filter = out->filter_ctx;
    int pad_idx = out->pad_idx;
    int ret;
    char name[255];

    // 1. 创建一个滤波器实例AVFilterContext，并添加到AVFilterGraph中.(buffersink)
    snprintf(name, sizeof(name), "out_%d_%d", ost->file_index, ost->index);
    ret = avfilter_graph_create_filter(&ofilter->filter,
                                       avfilter_get_by_name("buffersink"),
                                       name, NULL, NULL, fg->graph);

    if (ret < 0)
        return ret;

    // 2. 若输出过滤器存在分辨率,添加scale滤镜
    if (ofilter->width || ofilter->height) {
        // ofilter的width、height等信息是从open_output_file()的第10步拷贝编码器以及用户传进的参数得到的.
        printf("tyycode ofilter->width: %d, ofilter->height: %d\n", ofilter->width, ofilter->height);
        char args[255];
        AVFilterContext *filter;
        AVDictionaryEntry *e = NULL;

        snprintf(args, sizeof(args), "%d:%d",
                 ofilter->width, ofilter->height);//scale的分辨率参数
        printf("tyycode args: %s\n", args);

        while ((e = av_dict_get(ost->sws_dict, "", e,
                                AV_DICT_IGNORE_SUFFIX))) {//用户对滤镜scale指定的参数.
            av_strlcatf(args, sizeof(args), ":%s=%s", e->key, e->value);
        }
        printf("tyycode args: %s\n", args);

        //snprintf会自动清理,所以上次的name不会影响到本次的name.
        snprintf(name, sizeof(name), "scaler_out_%d_%d",
                 ost->file_index, ost->index);
        if ((ret = avfilter_graph_create_filter(&filter, avfilter_get_by_name("scale"),
                                                name, args, NULL, fg->graph)) < 0)
            return ret;
        if ((ret = avfilter_link(last_filter, pad_idx, filter, 0)) < 0)
            return ret;

        last_filter = filter;
        pad_idx = 0;
    }

    // 3. 若输出流(具体是输出流的编码器上下文或者编码器)存在像素格式名字,则添加format滤镜
    if ((pix_fmts = choose_pix_fmts(ofilter))) {
        printf("tyycode pix_fmts: %s\n", pix_fmts);
        AVFilterContext *filter;
        snprintf(name, sizeof(name), "format_out_%d_%d",
                 ost->file_index, ost->index);//该name并没用到,ffmpeg直接用"format"了
        ret = avfilter_graph_create_filter(&filter,
                                           avfilter_get_by_name("format"),
                                           "format", pix_fmts, NULL, fg->graph);
        av_freep(&pix_fmts);
        if (ret < 0)
            return ret;
        if ((ret = avfilter_link(last_filter, pad_idx, filter, 0)) < 0)
            return ret;

        last_filter = filter;
        pad_idx     = 0;
    }

    // 4. 若输出流的帧率存在,则添加fps滤镜.
    // 因为这里if条件固定是0,所以一定不会进来.
    if (ost->frame_rate.num && 0) {
        AVFilterContext *fps;
        char args[255];

        snprintf(args, sizeof(args), "fps=%d/%d", ost->frame_rate.num,
                 ost->frame_rate.den);
        snprintf(name, sizeof(name), "fps_out_%d_%d",
                 ost->file_index, ost->index);
        ret = avfilter_graph_create_filter(&fps, avfilter_get_by_name("fps"),
                                           name, args, NULL, fg->graph);
        if (ret < 0)
            return ret;

        ret = avfilter_link(last_filter, pad_idx, fps, 0);
        if (ret < 0)
            return ret;
        last_filter = fps;
        pad_idx = 0;
    }

    // 5. 指定了录像相关的选项,会插入trim filter.
    snprintf(name, sizeof(name), "trim_out_%d_%d",
             ost->file_index, ost->index);
    ret = insert_trim(of->start_time, of->recording_time,
                      &last_filter, &pad_idx, name);//输入输出文件的start_time,recording_time是否一致,有兴趣的可以自行研究
    if (ret < 0)
        return ret;

    // 6. 连接.执行到这里,最终指向输出的buffersink.
    if ((ret = avfilter_link(last_filter, pad_idx, ofilter->filter, 0)) < 0)
        return ret;

    return 0;
}

/* Define a function for building a string containing a list of
 * allowed formats.(定义一个函数，用于构建包含允许格式列表的字符串) */
//exit_program类似choose_pix_fmts()内的操作
/**
 * @brief 用于定义一些函数,返回该格式对应的名字 或者 返回该格式数组对应的名字(拼接得到).过程:
 * 1. 若ofilter.var成员存在,则直接返回该成员的name;
 * 2. 否则若ofilter.supported_list数组存在,或先通过类型获取name,然后拼接到s中,最终拼接的字符串通过ret返回.
 * 3. 若都不存在,返回NULL.
 *
 * @param suffix choose_后的后缀,用于组成完整的函数名.
 * @param type var变量的类型.例如int
 * @param var ofilter内部的变量名
 * @param supported_list ofilter内部的变量名,一般是数组
 * @param none supported_list数组的结束符
 * @param get_name 定义一些调用语句,用来获取名字
 */
//#define DEF_CHOOSE_FORMAT(suffix, type, var, supported_list, none, get_name)   \
//static char *choose_ ## suffix (OutputFilter *ofilter)                         \
//{                                                                              \
//    if (ofilter->var != none) {                                                \
//        get_name((type)ofilter->var);                                                \
//        return av_strdup(name);                                                \
//    } else if (ofilter->supported_list) {                                      \
//        const type *p;                                                         \
//        AVIOContext *s = NULL;                                                 \
//        uint8_t *ret;                                                          \
//        int len;                                                               \
//                                                                               \
//        if (avio_open_dyn_buf(&s) < 0)                                         \
//            exit_program(1);                                                   \
//                                                                               \
//        for (p = (const type*)ofilter->supported_list; *p != none; p++) {                   \
//            get_name((type)*p);                                                      \
//            avio_printf(s, "%s|", name);                                       \
//        }                                                                      \
//        len = avio_close_dyn_buf(s, &ret);                                     \
//        ret[len - 1] = 0;                                                      \
//        return (char*)ret;                                                            \
//    } else                                                                     \
//        return NULL;                                                           \
//}

////DEF_CHOOSE_FORMAT(pix_fmts, enum AVPixelFormat, format, formats, AV_PIX_FMT_NONE,
////                  GET_PIX_FMT_NAME)
////使用宏DEF_CHOOSE_FORMAT定义3个函数.
//DEF_CHOOSE_FORMAT(sample_fmts, enum AVSampleFormat, format, formats,
//                  AV_SAMPLE_FMT_NONE, GET_SAMPLE_FMT_NAME)

//DEF_CHOOSE_FORMAT(sample_rates, int, sample_rate, sample_rates, 0,
//                  GET_SAMPLE_RATE_NAME)

//DEF_CHOOSE_FORMAT(channel_layouts, uint64_t, channel_layout, channel_layouts, 0,
//                  GET_CH_LAYOUT_NAME)


#define DEF_CHOOSE_FORMAT_EX(suffix, type, var, supported_list, none, get_name)   \
char *FFmpegMedia::choose_ ## suffix (OutputFilter *ofilter, int *ret_val)                   \
{                                                                              \
    *ret_val = 0;                                                                \
    if (ofilter->var != none) {                                                \
        get_name((type)ofilter->var);                                          \
        return av_strdup(name);                                                \
    } else if (ofilter->supported_list) {                                      \
        const type *p;                                                         \
        AVIOContext *s = NULL;                                                 \
        uint8_t *ret;                                                          \
        int len;                                                               \
                                                                               \
        if (avio_open_dyn_buf(&s) < 0) {                                        \
            *ret_val = -1;                                                       \
            return NULL;/* exit_program(1) */                                   \
        }                                                                      \
        for (p = (const type *)ofilter->supported_list; *p != none; p++) {     \
            get_name((type)*p);                                                \
            avio_printf(s, "%s|", name);                                       \
        }                                                                      \
        len = avio_close_dyn_buf(s, &ret);                                     \
        ret[len - 1] = 0;                                                      \
        return (char *)ret;                                                            \
    } else                                                                     \
        return NULL;                                                           \
}

DEF_CHOOSE_FORMAT_EX(sample_fmts_ex, enum AVSampleFormat, format, formats,
                  AV_SAMPLE_FMT_NONE, GET_SAMPLE_FMT_NAME)

DEF_CHOOSE_FORMAT_EX(sample_rates_ex, int, sample_rate, sample_rates, 0,
                  GET_SAMPLE_RATE_NAME)

DEF_CHOOSE_FORMAT_EX(channel_layouts_ex, uint64_t, channel_layout, channel_layouts, 0,
                  GET_CH_LAYOUT_NAME)

/**
 * @brief 配置输出音频过滤器.该函数执行完,filter链表大概是这样的:
 * out->filter_ctx->pan->aformat->volume->apad->trim->abuffersink。
 *
 * @param fg fg
 * @param ofilter OutputFilter
 * @param out AVFilterInOut. 例如是avfilter_graph_parse2解析过滤器字符串时的传出参数outputs.
 *
 * @return 成功-0 失败-负数,或程序退出(例如choose_sample_fmts里面的宏).
 *
 * @note 这个函数和configure_output_video_filter在debug时,无法加载相关变量值,这貌似是与qt+gdb的问题.
 *          不过我们可以通过打印来进行调试.
*/
int FFmpegMedia::configure_output_audio_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out)
{
    OutputStream *ost = ofilter->ost;
    OutputFile    *of = output_files[ost->file_index];
    AVCodecContext *codec  = ost->enc_ctx;
    AVFilterContext *last_filter = out->filter_ctx;
    int pad_idx = out->pad_idx;
    char *sample_fmts, *sample_rates, *channel_layouts;
    char name[255];
    int ret;

    // 1. 创建一个滤波器实例AVFilterContext，并添加到AVFilterGraph中.(abuffersink)
    snprintf(name, sizeof(name), "out_%d_%d", ost->file_index, ost->index);
    ret = avfilter_graph_create_filter(&ofilter->filter,
                                       avfilter_get_by_name("abuffersink"),
                                       name, NULL, NULL, fg->graph);
    if (ret < 0)
        return ret;

    //设置abuffersink接受所有通道数(accept all channel counts),最终设置到BufferSinkContext.all_channel_counts变量
    if ((ret = av_opt_set_int(ofilter->filter, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
        return ret;

#define AUTO_INSERT_FILTER(opt_name, filter_name, arg) do {                 \
    AVFilterContext *filt_ctx;                                              \
                                                                            \
    av_log(NULL, AV_LOG_INFO, opt_name " is forwarded to lavfi "            \
           "similarly to -af " filter_name "=%s.\n", arg);                  \
                                                                            \
    ret = avfilter_graph_create_filter(&filt_ctx,                           \
                                       avfilter_get_by_name(filter_name),   \
                                       filter_name, arg, NULL, fg->graph);  \
    if (ret < 0)                                                            \
        return ret;                                                         \
                                                                            \
    ret = avfilter_link(last_filter, pad_idx, filt_ctx, 0);                 \
    if (ret < 0)                                                            \
        return ret;                                                         \
                                                                            \
    last_filter = filt_ctx;                                                 \
    pad_idx = 0;                                                            \
} while (0)

    // 2. 若音频通道map数组有内容,添加pan滤镜.
    // pan滤镜定义在libavfilter/af_pan.c
    //printf("+++++++++++tyycode ost->audio_channels_mapped: %d\n", ost->audio_channels_mapped);//没加对应选项为0.
    if (ost->audio_channels_mapped) {
        int i;
        AVBPrint pan_buf;
        //开辟一个最大缓存为8192,初始化大小为256字节的buffer.参2代表初始化字节大小;参3代表该缓存最大大小.
        av_bprint_init(&pan_buf, 256, 8192);
        av_bprintf(&pan_buf, "0x%" PRIx64,
                   av_get_default_channel_layout(ost->audio_channels_mapped));//添加字符串描述到pan_buf(一般字符串都是放在pan_buf.str)
        //将map数组里的内容追加到pan_buf
        for (i = 0; i < ost->audio_channels_mapped; i++)
            if (ost->audio_channels_map[i] != -1)
                av_bprintf(&pan_buf, "|c%d=c%d", i, ost->audio_channels_map[i]);

        AUTO_INSERT_FILTER("-map_channel", "pan", pan_buf.str);
        /* av_bprint_finalize():完成打印缓冲区.
         * 打印缓冲区之后将不再被使用，但是len和size字段仍然有效。
         * 参2: 如果不是NULL，用于返回缓冲区内容的永久副本,
         * 如果内存分配失败，则返回NULL;如果为NULL，则丢弃并释放缓冲区*/
        av_bprint_finalize(&pan_buf, NULL);//参2传NULL代表释放pan_buf内开辟过的内存.
    }

    if (codec->channels && !codec->channel_layout)
        codec->channel_layout = av_get_default_channel_layout(codec->channels);

    // 3. 若从输出过滤器OutputFilter获取到采样格式、采样率、通道布局其中一个,那么添加aformat滤镜.
    // (音频三元组一般指采样率，采样大小和通道数)
//    sample_fmts     = choose_sample_fmts(ofilter);
//    sample_rates    = choose_sample_rates(ofilter);
//    channel_layouts = choose_channel_layouts(ofilter);
    sample_fmts     = choose_sample_fmts_ex(ofilter, &ret);
    if(!sample_fmts && ret == -1){
        av_log(NULL, AV_LOG_ERROR, "choose_sample_fmts_ex failed.\n");
        return -1;
    }
    sample_rates    = choose_sample_rates_ex(ofilter, &ret);
    if(!sample_rates && ret == -1){
        av_log(NULL, AV_LOG_ERROR, "choose_sample_rates_ex failed.\n");
        return -1;
    }
    channel_layouts = choose_channel_layouts_ex(ofilter, &ret);
    if(!channel_layouts && ret == -1){
        av_log(NULL, AV_LOG_ERROR, "choose_channel_layouts_ex failed.\n");
        return -1;
    }
    if (sample_fmts || sample_rates || channel_layouts) {
        AVFilterContext *format;
        char args[256];
        args[0] = 0;

        if (sample_fmts)
            av_strlcatf(args, sizeof(args), "sample_fmts=%s:",
                            sample_fmts);
        if (sample_rates)
            av_strlcatf(args, sizeof(args), "sample_rates=%s:",
                            sample_rates);
        if (channel_layouts)
            av_strlcatf(args, sizeof(args), "channel_layouts=%s:",
                            channel_layouts);
        //上面av_strlcatf追加完字符串后,args末尾的":"冒号不用去掉吗?留个疑问.
        //例如打印结果: args="sample_fmts=s16p:sample_rates=48000:channel_layouts=0x3:"
        //看到aformat创建时args末尾可以保留冒号":".
        //printf("+++++++++++tyycode args: %s\n", args);

        av_freep(&sample_fmts);
        av_freep(&sample_rates);
        av_freep(&channel_layouts);

        snprintf(name, sizeof(name), "format_out_%d_%d",
                 ost->file_index, ost->index);
        ret = avfilter_graph_create_filter(&format,
                                           avfilter_get_by_name("aformat"),
                                           name, args, NULL, fg->graph);
        if (ret < 0)
            return ret;

        ret = avfilter_link(last_filter, pad_idx, format, 0);
        if (ret < 0)
            return ret;

        last_filter = format;
        pad_idx = 0;
    }

    // 4. 若指定-vol选项改变音量,添加volume滤镜.
    // 因为if条件与上0,所以肯定不会进来,类似视频的fps滤镜处理.
    // 实际上音量滤镜这步的处理在configure_input_audio_filter()已经处理.
    if (audio_volume != 256 && 0) {
        char args[256];

        snprintf(args, sizeof(args), "%f", audio_volume / 256.);
        AUTO_INSERT_FILTER("-vol", "volume", args);
    }

    // 5. 若apad和shortest都存在,且输出流中存在视频流, 那么添加apad滤镜.
    if (ost->apad && of->shortest) {
        char args[256];
        int i;

        // 判断是否存在视频流
        for (i=0; i<of->ctx->nb_streams; i++)
            if (of->ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                break;

        // 存在视频流,那么i一定小于of->ctx->nb_streams,添加apad滤镜
        if (i<of->ctx->nb_streams) {
            snprintf(args, sizeof(args), "%s", ost->apad);
            AUTO_INSERT_FILTER("-apad", "apad", args);
        }
    }

    // 6. 指定了录像相关的选项,会插入trim filter.
    snprintf(name, sizeof(name), "trim for output stream %d:%d",
             ost->file_index, ost->index);
    ret = insert_trim(of->start_time, of->recording_time,
                      &last_filter, &pad_idx, name);
    if (ret < 0)
        return ret;

    // 7. 连接.执行到这里,最终指向输出的abuffersink.
    if ((ret = avfilter_link(last_filter, pad_idx, ofilter->filter, 0)) < 0)
        return ret;

    return 0;
}

/**
 * @brief 配置输出过滤器.
 *
 * @param fg fg
 * @param ofilter OutputFilter
 * @param out AVFilterInOut. 例如是avfilter_graph_parse2解析过滤器字符串时的传出参数outputs.
 *
 * @return 成功-0 失败-负数,或者程序退出.
*/
int FFmpegMedia::configure_output_filter(FilterGraph *fg, OutputFilter *ofilter, AVFilterInOut *out)
{
    // 1. 要配置的输出过滤器对应的输出流为空,报错.
    if (!ofilter->ost) {
        av_log(NULL, AV_LOG_FATAL, "Filter %s has an unconnected output\n", ofilter->name);
        //exit_program(1);
        return -1;
    }

    // 2. 根据不同的媒体类型配置输出filter.
    switch (avfilter_pad_get_type(out->filter_ctx->output_pads, out->pad_idx)) {
    case AVMEDIA_TYPE_VIDEO: return configure_output_video_filter(fg, ofilter, out);
    case AVMEDIA_TYPE_AUDIO: return configure_output_audio_filter(fg, ofilter, out);
    default: /*av_assert0(0);*/return -1;
    }
}

/* sub2video hack:
   Convert subtitles to video with alpha to insert them in filter graphs.
   This is a temporary solution until libavfilter gets real subtitles support.
 */
int FFmpegMedia::sub2video_get_blank_frame(InputStream *ist)
{
    int ret;
    AVFrame *frame = ist->sub2video.frame;

    av_frame_unref(frame);
    ist->sub2video.frame->width  = ist->dec_ctx->width  ? ist->dec_ctx->width  : ist->sub2video.w;
    ist->sub2video.frame->height = ist->dec_ctx->height ? ist->dec_ctx->height : ist->sub2video.h;
    ist->sub2video.frame->format = AV_PIX_FMT_RGB32;
    if ((ret = av_frame_get_buffer(frame, 32)) < 0)
        return ret;
    memset(frame->data[0], 0, frame->height * frame->linesize[0]);
    return 0;
}

void FFmpegMedia::sub2video_copy_rect(uint8_t *dst, int dst_linesize, int w, int h,
                                AVSubtitleRect *r)
{
    uint32_t *pal, *dst2;
    uint8_t *src, *src2;
    int x, y;

    if (r->type != SUBTITLE_BITMAP) {
        av_log(NULL, AV_LOG_WARNING, "sub2video: non-bitmap subtitle\n");
        return;
    }
    if (r->x < 0 || r->x + r->w > w || r->y < 0 || r->y + r->h > h) {
        av_log(NULL, AV_LOG_WARNING, "sub2video: rectangle (%d %d %d %d) overflowing %d %d\n",
            r->x, r->y, r->w, r->h, w, h
        );
        return;
    }

    dst += r->y * dst_linesize + r->x * 4;
    src = r->data[0];
    pal = (uint32_t *)r->data[1];
    for (y = 0; y < r->h; y++) {
        dst2 = (uint32_t *)dst;
        src2 = src;
        for (x = 0; x < r->w; x++)
            *(dst2++) = pal[*(src2++)];
        dst += dst_linesize;
        src += r->linesize[0];
    }
}

void FFmpegMedia::sub2video_push_ref(InputStream *ist, int64_t pts)
{
    AVFrame *frame = ist->sub2video.frame;
    int i;
    int ret;

    av_assert1(frame->data[0]);
    ist->sub2video.last_pts = frame->pts = pts;
    for (i = 0; i < ist->nb_filters; i++) {
        ret = av_buffersrc_add_frame_flags(ist->filters[i]->filter, frame,
                                           AV_BUFFERSRC_FLAG_KEEP_REF |
                                           AV_BUFFERSRC_FLAG_PUSH);
        if (ret != AVERROR_EOF && ret < 0){
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_WARNING, "Error while add the frame to buffer source(%s).\n",
                   av_err2str_ex(str, ret));
        }
    }
}

void FFmpegMedia::sub2video_update(InputStream *ist, AVSubtitle *sub)
{
    AVFrame *frame = ist->sub2video.frame;
    int8_t *dst;
    int     dst_linesize;
    int num_rects, i;
    int64_t pts, end_pts;

    if (!frame)
        return;
    if (sub) {
        pts       = av_rescale_q(sub->pts + sub->start_display_time * 1000LL,
                                 AV_TIME_BASE_Q, ist->st->time_base);
        end_pts   = av_rescale_q(sub->pts + sub->end_display_time   * 1000LL,
                                 AV_TIME_BASE_Q, ist->st->time_base);
        num_rects = sub->num_rects;
    } else {
        pts       = ist->sub2video.end_pts;
        end_pts   = INT64_MAX;
        num_rects = 0;
    }
    if (sub2video_get_blank_frame(ist) < 0) {
        av_log(ist->dec_ctx, AV_LOG_ERROR,
               "Impossible to get a blank canvas.\n");
        return;
    }
    dst          = (int8_t *)frame->data    [0];
    dst_linesize = frame->linesize[0];
    for (i = 0; i < num_rects; i++)
        sub2video_copy_rect((uint8_t *)dst, dst_linesize, frame->width, frame->height, sub->rects[i]);
    sub2video_push_ref(ist, pts);
    ist->sub2video.end_pts = end_pts;
}

/**
 * @brief 配置每个流的FilterGraph.
 * 视频流配置完后, 可能是这样的:
 * buffer->insert_filter函数的滤镜(transpose,hflip,vflip,rotate)->yadif->trim->(in->filter_ctx);(输入)
 * (out->filter_ctx)->scale->format->fps->trim->buffersink;(输出)
 * 音频流配置完后, 可能是这样的:
 * abuffer->aresample->volume->trim->(in->filter_ctx);
 * (out->filter_ctx)->pan->aformat->volume->apad->trim->abuffersink。
 *
 * 其中音视频都有: in->filter_ctx = out->filter_ctx;
 *
 * @param fg fg
 * @return 成功-0 失败-负数或者程序退出
 */
int FFmpegMedia::configure_filtergraph(FilterGraph *fg)
{
    AVFilterInOut *inputs, *outputs, *cur;
    int ret, i, simple = filtergraph_is_simple(fg);// 按例子的推流命令时,simple=1
    const char *graph_desc = simple ? fg->outputs[0]->ost->avfilter :
                                      fg->graph_desc;// graph_desc一般是"null"或者"anull",fg->graph_desc是空

    // 1. 先清理上一次的FilterGraph,然后再开辟AVFilterGraph.
    cleanup_filtergraph(fg);
    if (!(fg->graph = avfilter_graph_alloc()))
        return AVERROR(ENOMEM);

    // 2. 如果是简单过滤器,需要组成对应的字符串描述设置到对应的变量;复杂则不需要,因为本身就是字符串描述.
    // fg->graph_desc过滤器字符串描述为空表示简单过滤器,不为空表示复杂过滤器.
    if (simple) {
        OutputStream *ost = fg->outputs[0]->ost;
        char args[512];
        AVDictionaryEntry *e = NULL;

        fg->graph->nb_threads = filter_nbthreads;// -filter_threads选项,默认0,非复杂过滤器线程数.

        // 2.1 将用户传进的sws_dict字典参数拼接成字符串描述,格式为"key1=value1:key2=value2"
        args[0] = 0;
        while ((e = av_dict_get(ost->sws_dict, "", e,
                                AV_DICT_IGNORE_SUFFIX))) {
            av_strlcatf(args, sizeof(args), "%s=%s:", e->key, e->value);
        }
        if (strlen(args))
            args[strlen(args)-1] = 0;//末尾添加哨兵字符0,同时可以去掉末尾的":"
        // 2.2 应用该描述
        fg->graph->scale_sws_opts = av_strdup(args);

        // 每次将字符串的首个字符置为0, 相当于清空args数组, 下一次av_strlcatf就会在传进的数组的首字节开始写入.
        /* 注,av_strlcatf每次会在实际写入的字节数的下一个字节补0,很重要,这个实际是vsnprintf函数的作用.
         * 例如实际写入5字节,那么在第6字节会补0. 理解这一点,我们就知道为啥这里都使用args数组,而不会影响下一次拼接.
         *
         * 例如sws_dict处理完后,args="flags=bicubic", 经过args[0]=0后,args="\0lags=bicubic",
         * 假设拼接时swr_opts有内容:"ch=2:", 那么在执行本次av_strlcatf:
         * 1)如果av_strlcatf不在实际写入的字节数的下一个字节补0,猜想得到的应该是: args="ch=2:=bicubic",那么这样就一定会影响到下一个选项的设置(当然这种是不存在的).
         * 2)而实际av_strlcatf是会在实际写入的字节数的下一个字节补0,真正得到的是: args="ch=2:\0bicubic",那么就一定不会影响到下一个选项的设置.
         * . */
        args[0] = 0;
        //av_dict_set(&ost->swr_opts, "ch", "2", 0);//tyy code
        while ((e = av_dict_get(ost->swr_opts, "", e,
                                AV_DICT_IGNORE_SUFFIX))) {
            av_strlcatf(args, sizeof(args), "%s=%s:", e->key, e->value);
        }
        if (strlen(args))
            args[strlen(args)-1] = 0;
        av_opt_set(fg->graph, "aresample_swr_opts", args, 0);//这里通过av_opt_set设置.

        //resample_opts最终应用到哪?这里看到这个字典并未被ffmpeg使用
        args[0] = '\0';
        while ((e = av_dict_get(fg->outputs[0]->ost->resample_opts, "", e,
                                AV_DICT_IGNORE_SUFFIX))) {
            av_strlcatf(args, sizeof(args), "%s=%s:", e->key, e->value);
        }
        if (strlen(args))
            args[strlen(args) - 1] = '\0';

        //我们看libavfilter/avfiltergraph.c, 这个threads最终同样是被设置到上面的fg->graph->nb_threads.
        e = av_dict_get(ost->encoder_opts, "threads", NULL, 0);
        if (e)
            av_opt_set(fg->graph, "threads", e->value, 0);
    } else {
        fg->graph->nb_threads = filter_complex_nbthreads;//复杂过滤器的线程数,同样是设置到fg->graph->nb_threads
    }
    //这里我们知道,简单和复杂过滤器字符串描述的区别是:
    //简单是用户输入key=val的形式,然后ffmpeg再将这些组成字符串描述保存; 而复杂则是用户直接传字符串描述.

    // 3. 解析过滤器字符串.
    // avfilter_graph_parse2的作用：1）解析字符串；2）并且将滤波图的集合放在inputs、outputs中。
    /* avfilter_graph_parse2(): 将字符串描述的图形添加到图形中。
     * 参1: 将解析图上下文链接到其中的过滤器图
     * 参2: 要解析的字符串
     * 参3:[out] 输入一个包含解析图的所有空闲(未链接)输入的链接列表将在这里返回。调用者将使用avfilter_inout_free()释放它。
     * 参4:[out] 输出一个链表，包含解析图的所有空闲(未链接)输出。调用者将使用avfilter_inout_free()释放它。
     * return: zero on success, a negative AVERROR code on error.
     *
     * 注意: 这个函数返回在解析图之后未链接的输入和输出，然后调用者处理它们.
     * 注意: 这个函数不引用graph中已经存在的部分，输入参数在返回时将包含图中新解析部分的输入。类似地，outputs参数将包含新创建的filters的输出。
    */
    // 复杂过滤器字符串一般都是调avfilter_graph_parse2函数直接解析,非常方便;
    // 简单字符串一般都是依赖avfilter_graph_create_filter+avfilter_link来处理.
    // ffmpeg这里都使用到.
    if ((ret = avfilter_graph_parse2(fg->graph, graph_desc, &inputs, &outputs)) < 0)
        goto fail;

    // 4. 硬件相关,暂不分析
    // -filter_hw_device选项和是否有hw_device_ctx
    if (filter_hw_device || hw_device_ctx) {
        AVBufferRef *device = filter_hw_device ? filter_hw_device->device_ref
                                               : hw_device_ctx;
        for (i = 0; i < fg->graph->nb_filters; i++) {
            fg->graph->filters[i]->hw_device_ctx = av_buffer_ref(device);
            if (!fg->graph->filters[i]->hw_device_ctx) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
        }
    }

    // 5. 简单过滤器只能有1 input and 1 output.当不符合时,程序报错.
    // !inputs代表0个输入, inputs->next代表大于1个输入,输出同理.
    if (simple && (!inputs || inputs->next || !outputs || outputs->next)) {
        const char *num_inputs;
        const char *num_outputs;
        // 判断输入和输出过滤器的个数
        if (!outputs) {
            num_outputs = "0";
        } else if (outputs->next) {
            num_outputs = ">1";
        } else {
            num_outputs = "1";
        }
        if (!inputs) {
            num_inputs = "0";
        } else if (inputs->next) {
            num_inputs = ">1";
        } else {
            num_inputs = "1";
        }
        //简单过滤器只能有1 input and 1 output.
        av_log(NULL, AV_LOG_ERROR, "Simple filtergraph '%s' was expected "
               "to have exactly 1 input and 1 output."
               " However, it had %s input(s) and %s output(s)."
               " Please adjust, or use a complex filtergraph (-filter_complex) instead.\n",
               graph_desc, num_inputs, num_outputs);
        ret = AVERROR(EINVAL);
        goto fail;
    }

    // 此时AVFilterContext的链表会被保存在inputs、outputs,那么就开始配置它们.

    // 6. 配置输入输出过滤器
    // 我们看AVFilterInOut的定义以及avfilter_graph_parse2()源码,ffmpeg大概是这样处理的(笔者没详细看,不一定完全准确):
    // 每一个AVFilterInOut保存一个AVFilterContext, 链表由AVFilterInOut *next链接.
    // 不过转码推流命令调用一次avfilter_graph_parse2,一般AVFilterInOut*链表只有一个元素,下一个元素是指向NULL的.
    // 6.1 配置输入过滤器
    // 配置完后,InputFilter->filter保存着输入过滤器链表的头,即指向buffer,abuffer.
    for (cur = inputs, i = 0; cur; cur = cur->next, i++)
        if ((ret = configure_input_filter(fg, fg->inputs[i], cur)) < 0) {
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
            goto fail;
        }
    //注意!!! avfilter_inout_free执行完后,内部的inputs->filter_ctx并不会被释放掉,所以不用担心配置完输入过滤器后,该过滤器被释放掉.
    //这部分我已经测试过,测试很简单,释放前先保存inputs->filter_ctx,释放后看inputs->filter_ctx是否还有值,结果测试是有的.
    //下面的输出过滤器同理.
    avfilter_inout_free(&inputs);

    // 6.2 配置输出过滤器.
    // 配置完后,OutputFilter->filter保存着输出过滤器链表的尾,即指向buffersink,abuffersink.
    for (cur = outputs, i = 0; cur; cur = cur->next, i++){
        //configure_output_filter(fg, fg->outputs[i], cur);
        ret = configure_output_filter(fg, fg->outputs[i], cur);// 注,ffmpeg本身没出来这个函数的返回值
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "configure_output_filter failed.\n");
            avfilter_inout_free(&outputs);// tyycode
            goto fail;
        }
    }

    avfilter_inout_free(&outputs);

    /* 注意!!! 输入和输出过滤器是如何链接起来的呢？
     答:我们观察inputs->filter_ctx与outputs->filter_ctx,这两者的指向是一样的,所以当我们处理完输入过滤器后,
        inputs->filter_ctx是输入过滤器的尾部元素;而在处理输出过滤器时,outputs->filter_ctx是作为开始的,所以这就解释了
        输入输出过滤器是通过inputs->filter_ctx=outputs->filter_ctx来链接的.
        所以下面可以直接提交整个滤波图. */

    // 7. 提交整个滤波图
    if ((ret = avfilter_graph_config(fg->graph, NULL)) < 0)
        goto fail;

    /* limit the lists of allowed formats to the ones selected, to
     * make sure they stay the same if the filtergraph is reconfigured later */
    // (将允许的格式列表限制为所选格式，以确保在以后重新配置filtergraph时它们保持不变)
    // 8. 把输出过滤器buffersink,abuffersink相关的参数保存下来.
    for (i = 0; i < fg->nb_outputs; i++) {
        OutputFilter *ofilter = fg->outputs[i];
        AVFilterContext *sink = ofilter->filter;

        ofilter->format = av_buffersink_get_format(sink);

        ofilter->width  = av_buffersink_get_w(sink);//注意宽高视频才会有值,音频是没有的
        ofilter->height = av_buffersink_get_h(sink);

        ofilter->sample_rate    = av_buffersink_get_sample_rate(sink);//注意采样率,通道布局音频才会有值,视频是没有的
        ofilter->channel_layout = av_buffersink_get_channel_layout(sink);
    }

    fg->reconfiguration = 1;// 标记配置了AVFilterGraph

    // 9. 设置音频的样本大小
    for (i = 0; i < fg->nb_outputs; i++) {
        // 9.1  检测 该输出过滤器 对应的 输出流的编码器 是否已经初始化,因为复杂的过滤器图会在前面初始化.
        OutputStream *ost = fg->outputs[i]->ost;
        if (!ost->enc) {
            /* identical to the same check in ffmpeg.c, needed because
               complex filter graphs are initialized earlier */
            // (与ffmpeg.c中的检查相同，这是必需的，因为复杂的过滤器图会在前面初始化)
            av_log(NULL, AV_LOG_ERROR, "Encoder (codec %s) not found for output stream #%d:%d\n",
                     avcodec_get_name(ost->st->codecpar->codec_id), ost->file_index, ost->index);
            ret = AVERROR(EINVAL);
            goto fail;
        }

        // 若是音频 且 音频编码器不支持在每次调用中接收不同数量的样本，那么设置样本大小(参考init_output_stream的做法).
        if (ost->enc->type == AVMEDIA_TYPE_AUDIO &&
            !(ost->enc->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
            av_buffersink_set_frame_size(ost->filter->filter,
                                         ost->enc_ctx->frame_size);//一般会进来
    }

    /* 10. 将InputFilter的fifo队列缓存的帧发送到过滤器.
     * 这部分缓存的帧是在哪写入的?
     * 答:在ifilter_send_frame()函数.
     * 当ifilter_has_all_input_formats()检测到有没初始化完成的InputFilter时,就会缓存下来. */
    for (i = 0; i < fg->nb_inputs; i++) {
        /*test1:测试sizeof(t1)的大小
        //AVFrame *t1;
        //printf("sizeof(t1): %d, sizeof(AVFrame): %d\n", sizeof(t1), sizeof(AVFrame));// 8 536 */
        /*test2:测试拷贝地址.
        int *a;
        int *v1 = (int*)malloc(sizeof (int));
        *v1 = 1000;
        memcpy(&a, &v1, sizeof(a));
        printf("a: %#X, &a: %#X, v1: %#X, &v1: %#X\n", a, &a, v1, &v1);*/
        while (av_fifo_size(fg->inputs[i]->frame_queue)) {
            AVFrame *tmp;
            //注意,这里每次只会读8字节(64位机器时),因为sizeof(t1)=8
            //为什么只读8字节呢?因为我们在av_fifo_generic_write时,就是写指针的地址的,这样我们就得到指向AVFrame*的数据.
            //这种处理相当于C++的vertor<AVFrame*>,即队列存储的是指向数据的地址(画图理解即可).
            av_fifo_generic_read(fg->inputs[i]->frame_queue, &tmp, sizeof(tmp), NULL);
            ret = av_buffersrc_add_frame(fg->inputs[i]->filter, tmp);
            av_frame_free(&tmp);//这里会把tmp释放掉,也就说,av_buffersrc_add_frame内部会进行相关copy操作.
            if (ret < 0)
                goto fail;
        }
    }

    // 11. InputFilter遇到eof,则往过滤器刷空帧
    // InputFilter数组一般只有一个元素.
    /* send the EOFs for the finished inputs(发送EOF对于完成输入的) */
    for (i = 0; i < fg->nb_inputs; i++) {
        if (fg->inputs[i]->eof) {
            printf("tyy code ++++++++++++++++ fg->inputs[i]->eof: %d\n", fg->inputs[i]->eof);
            ret = av_buffersrc_add_frame(fg->inputs[i]->filter, NULL);//刷空帧.
            if (ret < 0)
                goto fail;
        }
    }

    // 12. 字幕相关,暂不深入研究,推流没用到.
    /* process queued up subtitle packets(处理排队的字幕数据包) */
    for (i = 0; i < fg->nb_inputs; i++) {
        InputStream *ist = fg->inputs[i]->ist;
        if (ist->sub2video.sub_queue && ist->sub2video.frame) {
            while (av_fifo_size(ist->sub2video.sub_queue)) {
                AVSubtitle tmp;
                av_fifo_generic_read(ist->sub2video.sub_queue, &tmp, sizeof(tmp), NULL);
                sub2video_update(ist, &tmp);
                avsubtitle_free(&tmp);
            }
        }
    }

    return 0;

fail:
    cleanup_filtergraph(fg);
    return ret;
}

int FFmpegMedia::check_recording_time(OutputStream *ost)
{
    OutputFile *of = output_files[ost->file_index];

    // 当前的时长已经大于等于用户要录像的时长，则给该输出流标记ENCODER_FINISHED，返回0；否则返回1
    // 例如视频流结束了,of->recording_time就是视频流的时长,音频流会进来判断是否大于这个时长.
    if (of->recording_time != INT64_MAX &&
        av_compare_ts(ost->sync_opts - ost->first_pts, ost->enc_ctx->time_base, of->recording_time,
                      AV_TIME_BASE_Q) >= 0) {
        close_output_stream(ost);
        return 0;
    }
    return 1;
}

av_always_inline char* FFmpegMedia::av_err2str_ex(char *buf, int errnum)
{
    memset(buf, 0, AV_ERROR_MAX_STRING_SIZE);
    return av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, errnum);
}

av_always_inline char* FFmpegMedia::av_ts2str_ex(char *buf, int64_t ts){
    memset(buf, 0, AV_ERROR_MAX_STRING_SIZE);
    return av_ts_make_string(buf, ts);
}

av_always_inline char* FFmpegMedia::av_ts2timestr_ex(char *buf, int64_t ts, AVRational *tb){
    memset(buf, 0, AV_ERROR_MAX_STRING_SIZE);
    return av_ts_make_time_string(buf, ts, tb);
}

av_always_inline char* FFmpegMedia::av_ts2timestr_ex(char *buf, int64_t ts, AVRational tb){
    memset(buf, 0, AV_ERROR_MAX_STRING_SIZE);
    return av_ts_make_time_string(buf, ts, &tb);
}

void FFmpegMedia::update_benchmark(const char *fmt, ...)
{
    if (do_benchmark_all) {
        BenchmarkTimeStamps t = get_benchmark_time_stamps();
        va_list va;
        char buf[1024];

        if (fmt) {
            va_start(va, fmt);
            vsnprintf(buf, sizeof(buf), fmt, va);
            va_end(va);
            av_log(NULL, AV_LOG_INFO,
                   "bench: %8" PRIu64 " user %8" PRIu64 " sys %8" PRIu64 " real %s \n",
                   t.user_usec - current_time.user_usec,
                   t.sys_usec - current_time.sys_usec,
                   t.real_usec - current_time.real_usec, buf);
        }
        current_time = t;
    }
}

/**
 * @brief 给输出流退出时进行标记。
 * @param ost 输出流
 * @param this_stream ost的标记
 * @param others ost以外的其它输出流的标记
 * @note 并不是真正关闭输出流
*/
void FFmpegMedia::close_all_output_streams(OutputStream *ost, OSTFinished this_stream, OSTFinished others)
{
    int i;
    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost2 = output_streams[i];
        //ost2->finished |= ost == ost2 ? this_stream : others;
        ost2->finished = (OSTFinished)(ost2->finished | (ost == ost2 ? this_stream : others));
    }
}


/**
 * @brief 将pkt写到输出文件(可以是文件或者实时流地址)
 * @param of 输出文件
 * @param pkt pkt
 * @param ost 输出流
 * @param unqueue 是否已经计算该pkt在ost->frame_number中,=1表示已经计算。see check_init_output_file()
*/
int FFmpegMedia::write_packet(OutputFile *of, AVPacket *pkt, OutputStream *ost, int unqueue)
{
    AVFormatContext *s = of->ctx;
    AVStream *st = ost->st;
    int ret;

    /*
     * Audio encoders may split the packets --  #frames in != #packets out.
     * But there is no reordering, so we can limit the number of output packets
     * by simply dropping them here.
     * Counting encoded video frames needs to be done separately because of
     * reordering, see do_video_out().
     * Do not count the packet when unqueued because it has been counted when queued.
     */
    /*
     * 音频编码器可能会分割数据包 --  #输入帧!= #输出包。
     * 但是没有重新排序，所以我们可以通过简单地将它们放在这里来限制输出数据包的数量。
     * 由于重新排序，编码的视频帧需要单独计数，参见do_video_out()。
     * 不要在未排队时计数数据包，因为它在排队时已被计数。
     */
    /* 1. 统计已经写帧的数量. */
    // "视频需要编码"以外的类型，且unqueue=0表示该pkt没有计算时，会进行计算.
    if (!(st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && ost->encoding_needed) && !unqueue) {
        if (ost->frame_number >= ost->max_frames) {//帧数大于允许的最大帧数，不作处理，并将该包释放
            av_packet_unref(pkt);
            return 0;// 暂时认为正常
        }

        ost->frame_number++;// 视频流需要编码时，不会在这里计数，会在do_video_out()单独计数

#ifdef TYYCODE_TIMESTAMP_MUXER
        mydebug(NULL, AV_LOG_INFO, "write_packet(), tpye: %s, ost->frame_number: %d\n",
                av_get_media_type_string(st->codecpar->codec_type), ost->frame_number);
#endif
    }

    /* 2.没有写头或者写头失败，先将包缓存在复用队列，然后返回，不会调用到写帧函数 */
    if (!of->header_written) {
        AVPacket tmp_pkt = {0};
        /* the muxer is not initialized yet, buffer the packet(muxer尚未初始化，请缓冲该数据包) */
        // 2.1 fifo没有空间，则给其扩容
        if (!av_fifo_space(ost->muxing_queue)) {// return f->end - f->buffer - av_fifo_size(f);
            /* 新队列大小求法：若在最大队列大小内，那么以当前队列大小的两倍扩容.
             * 例如用户不指定大小，默认是给复用队列先开辟8个空间大小，若此时没空间，说明av_fifo_size()返回的值为8*sizeof(pkt)，
             * 那么乘以2后，new_size值就是16个pkt的大小了. */
            int new_size = FFMIN(2 * av_fifo_size(ost->muxing_queue),
                                 ost->max_muxing_queue_size);
            /* 若输出流的包缓存到达最大复用队列，那么ffmpeg会退出程序 */
            if (new_size <= av_fifo_size(ost->muxing_queue)) {
                av_log(NULL, AV_LOG_ERROR,
                       "Too many packets buffered for output stream %d:%d.\n",
                       ost->file_index, ost->st->index);
                //exit_program(1);
                return -1;
            }
            ret = av_fifo_realloc2(ost->muxing_queue, new_size);
            if (ret < 0){
                //exit_program(1);
                return -1;
            }
        }

        /*
         * av_packet_make_refcounted(): 确保给定数据包所描述的数据被引用计数。
         * @note 此函数不确保引用是可写的。 为此使用av_packet_make_writable。
         * @see av_packet_ref.
         * @see av_packet_make_writable.
         * @param pkt 计算需要参考的PKT数据包.
         * @return 成功为0，错误为负AVERROR。失败时，数据包不变。
         * 源码也不难，工作是：
         * 拷贝packet数据到pkt->buf->data中，再令pkt->data指向它.
         */
        ret = av_packet_make_refcounted(pkt);
        if (ret < 0){
            //exit_program(1);
            return -1;
        }
        av_packet_move_ref(&tmp_pkt, pkt);// 所有权转移，源码很简单
        /*
         * av_fifo_generic_write(): 将数据从用户提供的回调提供给AVFifoBuffer。
         * @param f 要写入的AVFifoBuffer.
         * @param src 数据来源;非const，因为它可以被定义在func中的函数用作可修改的上下文.
         * @param size 要写入的字节数.
         * @param func 一般写函数;第一个参数是src，第二个参数是dest_buf，第三个参数是dest_buf_size.
         * Func必须返回写入dest_buf的字节数，或<= 0表示没有更多可写入的数据。如果func为NULL, src将被解释为源数据的简单字节数组。
         * @return 写入FIFO的字节数.
         * av_fifo_generic_write()的源码不算难.
         */
        av_fifo_generic_write(ost->muxing_queue, &tmp_pkt, sizeof(tmp_pkt), NULL);
        return 0;
    }

    // 3. 将pkt.pts/pkt.dts置为无效值
    if ((st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_sync_method == VSYNC_DROP) ||
        (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_sync_method < 0))
        pkt->pts = pkt->dts = AV_NOPTS_VALUE;

    // 4. 获取该pkt的编码质量,图片类型等信息.
    if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        int i;
        // 关于下面获取q编码质量可参考https://blog.csdn.net/u012117034/article/details/123453863
        /*
         * av_packet_get_side_data():从packet获取side info.
         * @param type 期待side information的类型
         * @param size 用于存储side info大小的指针(可选)
         * @return 如果存在数据，则指向数据的指针，否则为NULL.
         * 源码很简单.
        */
        // 获取编码质量状态的side data info
        uint8_t *sd = av_packet_get_side_data(pkt, AV_PKT_DATA_QUALITY_STATS,
                                              NULL);
        // 从sd中获取前32bit
        /* AV_RL32(sd)的大概调用过程：
         * 1)AV_RL(32, p)
         * 2）而AV_RL的定义：
         * #   define AV_RL(s, p)    AV_RN##s(p)
         * 3）所以此时调用变成(##表示将参数与前后拼接)：
         * AV_RN32(p)
         * 4）而AV_RN32的定义：
         * #   define AV_RN32(p) AV_RN(32, p)
         * 5）而AV_RN的定义：
         * #   define AV_RN(s, p) (*((const __unaligned uint##s##_t*)(p)))
         * 所以最终就是取sd前32bit的内容.(这部分是我使用vscode看源码得出的，qt貌似会跳转的共用体，不太准？)
         *
         * 至于为什么要获取32bit，看AV_PKT_DATA_QUALITY_STATS枚举注释，这个信息会由编码器填充，
         * 被放在side info的前32bit，然后第5个字节存放帧类型，第6个字节存放错误次数，
         * 第7、8两字节为保留位，
         * 后面剩余的u64le[error count]是一个数组,每个元素是8字节.
        */
        ost->quality = sd ? AV_RL32(sd) : -1;
        ost->pict_type = sd ? sd[4] : AV_PICTURE_TYPE_NONE;
#ifdef TYYCODE_TIMESTAMP_MUXER
            mydebug(NULL, AV_LOG_INFO, "write_packet(), video, ost->quality: %d, ost->pict_type: %c\n",
                    ost->quality, av_get_picture_type_char((enum AVPictureType)ost->pict_type));
#endif

        for (i = 0; i<FF_ARRAY_ELEMS(ost->error); i++) {// ost->error[4]数组大小是4，i可能是0-4的值,即最多保存近4次的错误
            if (sd && i < sd[5])// 若错误次数(error count) 大于0次以上，即表示有错误
                ost->error[i] = AV_RL64(sd + 8 + 8*i);// sd+8是跳过quality+pict_type+保留位，8*i表示跳过前面已经获取的错误.
            else
                ost->error[i] = -1;// 没有错误
        }

        // 是否通过帧率对duration重写
        if (ost->frame_rate.num && ost->is_cfr) {
            // (通过帧速率覆盖数据包持续时间，这应该不会发生)
            if (pkt->duration > 0)
                av_log(NULL, AV_LOG_WARNING, "Overriding packet duration by frame rate, this should not happen\n");
            pkt->duration = av_rescale_q(1, av_inv_q(ost->frame_rate),
                                         ost->mux_timebase);
        }
    }

    // 5. 将pkt相关的时间戳转成输出容器的时基
    /* 内部调用av_rescale_q，将pkt里面与时间戳相关的pts、dts、duration、convergence_duration转成以ost->st->time_base为单位 */
    av_packet_rescale_ts(pkt, ost->mux_timebase, ost->st->time_base);
#ifdef TYYCODE_TIMESTAMP_MUXER
            mydebug(NULL, AV_LOG_WARNING, "write_packet(), ost->mux_timebase: %d/%d, "
                    "ost->st->time_base: %d/%d, "
                    "output stream %d:%d\n",
                    ost->mux_timebase.num, ost->mux_timebase.den,
                    ost->st->time_base.num, ost->st->time_base.den,
                    ost->file_index, ost->st->index);
#endif

    /* 6. 输出流需要有时间戳的处理 */
    /* AVFMT_NOTIMESTAMPS: 格式不需要/有任何时间戳.
     * 所以当没有该宏时，就说明需要时间戳 */
    if (!(s->oformat->flags & AVFMT_NOTIMESTAMPS)) {
        // 6.1 解码时间戳比显示时间戳大，重写它们的时间戳
        if (pkt->dts != AV_NOPTS_VALUE &&
            pkt->pts != AV_NOPTS_VALUE &&
            pkt->dts > pkt->pts) {
            av_log(s, AV_LOG_WARNING, "Invalid DTS: %" PRId64" PTS: %" PRId64" in output stream %d:%d, replacing by guess\n",
                   pkt->dts, pkt->pts,
                   ost->file_index, ost->st->index);
            pkt->pts =
            pkt->dts = pkt->pts + pkt->dts + ost->last_mux_dts + 1
                     - FFMIN3(pkt->pts, pkt->dts, ost->last_mux_dts + 1)
                     - FFMAX3(pkt->pts, pkt->dts, ost->last_mux_dts + 1);// 减去最大最小值，最终得到的就是三者之中，处于中间的那个值
#ifdef TYYCODE_TIMESTAMP_MUXER
            mydebug(NULL, AV_LOG_WARNING, "write_packet(), pkt->dts: %" PRId64", pkt->pts: %" PRId64", ost->last_mux_dts + 1: %" PRId64", "
                    "output stream %d:%d\n", pkt->dts, pkt->pts, ost->last_mux_dts + 1, ost->file_index, ost->st->index);
#endif
        }

        /* 6.2 判断当前pkt.dts与上一个pkt.dts比是否单调递增,以此修正pkt的pts/dts */
        if ((st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO || st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO || st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) &&
            pkt->dts != AV_NOPTS_VALUE &&
            !(st->codecpar->codec_id == AV_CODEC_ID_VP9 && ost->stream_copy) && /* VP9且不转码以外的条件 */
            ost->last_mux_dts != AV_NOPTS_VALUE) {
            /* AVFMT_TS_NONSTRICT：格式不需要严格增加时间戳，但它们仍然必须是单调的 */
            // 包含该宏,max是last_mux_dts; 不包含,max是last_mux_dts+1.加1主要是确保单调递增.
            int64_t max = ost->last_mux_dts + !(s->oformat->flags & AVFMT_TS_NONSTRICT);

#ifdef TYYCODE_TIMESTAMP_MUXER
            mydebug(NULL, AV_LOG_INFO, "write_packet(), --, "
                    "pkt->dts: %" PRId64", pkt->pts: %" PRId64", ost->last_mux_dts + 1: %" PRId64", ""max, "
                    "%" PRId64",output stream %d:%d\n",
                    pkt->dts, pkt->pts, ost->last_mux_dts + 1, max,
                    ost->file_index, ost->st->index);
#endif

            if (pkt->dts < max) {
                int loglevel = max - pkt->dts > 2 || st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ? AV_LOG_WARNING : AV_LOG_DEBUG;
                av_log(s, loglevel, "Non-monotonous DTS in output stream "
                       "%d:%d; previous: %" PRId64", current: %" PRId64"; ",
                       ost->file_index, ost->st->index, ost->last_mux_dts, pkt->dts);// dts没有单调递增
                if (exit_on_error) {//-xerror选项，默认是0
                    av_log(NULL, AV_LOG_FATAL, "aborting.\n");
                    //exit_program(1);
                    return -1;
                }
                av_log(s, loglevel, "changing to %" PRId64". This may result "
                       "in incorrect timestamps in the output file.\n",
                       max);// (这可能会导致输出文件中的时间戳不正确)
#ifdef TYYCODE_TIMESTAMP_MUXER
                mydebug(NULL, AV_LOG_INFO, "write_packet(), pkt->dts < max\n");
#endif
                if (pkt->pts >= pkt->dts)
                    pkt->pts = FFMAX(pkt->pts, max);
                pkt->dts = max;
            }

        }
    }
    ost->last_mux_dts = pkt->dts;   // 保存最近一次有效的dts

    ost->data_size += pkt->size;    // 统计所有写入的包的字节大小
    ost->packets_written++;         // 统计所有写入的包的个数

    pkt->stream_index = ost->index;

    if (debug_ts) {// -debug_ts选项
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_log(NULL, AV_LOG_INFO, "muxer <- type:%s "
                "pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s size:%d\n",
                av_get_media_type_string(ost->enc_ctx->codec_type),
                av_ts2str_ex(str, pkt->pts), av_ts2timestr_ex(str, pkt->pts, &ost->st->time_base),
                av_ts2str_ex(str, pkt->dts), av_ts2timestr_ex(str, pkt->dts, &ost->st->time_base),
                pkt->size
              );
    }

    // 7. 写帧
    ret = av_interleaved_write_frame(s, pkt);
    if (ret < 0) {
        print_error("av_interleaved_write_frame()", ret);
        main_return_code = 1;
        close_all_output_streams(ost, OSTFinished(MUXER_FINISHED | ENCODER_FINISHED), ENCODER_FINISHED);
    }
    av_packet_unref(pkt);

    return ret;
}

/*
 * Send a single packet to the output, applying any bitstream filters
 * associated with the output stream.  This may result in any number
 * of packets actually being written, depending on what bitstream
 * filters are applied.  The supplied packet is consumed and will be
 * blank (as if newly-allocated) when this function returns.
 *
 * If eof is set, instead indicate EOF to all bitstream filters and
 * therefore flush any delayed packets to the output.  A blank packet
 * must be supplied in this case.
 */
/*(向输出发送单个包，应用与输出流相关的任何位流过滤器。这可能会导致实际写入任意数量的数据包，具体取决于应用了什么位流过滤器。
 * 当此函数返回时，所提供的包将被消耗，并且为空(就像新分配的一样)。
 * 如果设置了eof，则将eof指示为所有位流过滤器，因此将任何延迟的数据包刷新到输出。
 * 在这种情况下，必须提供一个空白包)*/
/**
 * @brief write_packet.按照有位流和无位流的流程，不难.
 * @param of 输出文件
 * @param pkt 编码后的包
 * @param ost 输出流
 * @param eof eof
*/
int FFmpegMedia::output_packet(OutputFile *of, AVPacket *pkt,
                          OutputStream *ost, int eof)
{
    int ret = 0;

    /* apply the output bitstream filters, if any.(如果有的话，应用输出位流过滤器) */
    // 1. 有位流的写包流程
    if (ost->nb_bitstream_filters) {// 推流没用到,暂未研究
        int idx;

        ret = av_bsf_send_packet(ost->bsf_ctx[0], eof ? NULL : pkt);
        if (ret < 0)
            goto finish;

        eof = 0;
        idx = 1;
        while (idx) {
            /* get a packet from the previous filter up the chain */
            ret = av_bsf_receive_packet(ost->bsf_ctx[idx - 1], pkt);
            if (ret == AVERROR(EAGAIN)) {
                ret = 0;
                idx--;
                continue;
            } else if (ret == AVERROR_EOF) {
                eof = 1;
            } else if (ret < 0)
                goto finish;

            /* send it to the next filter down the chain or to the muxer */
            if (idx < ost->nb_bitstream_filters) {
                ret = av_bsf_send_packet(ost->bsf_ctx[idx], eof ? NULL : pkt);
                if (ret < 0)
                    goto finish;
                idx++;
                eof = 0;
            } else if (eof){
                goto finish;
            } else{
                ret = write_packet(of, pkt, ost, 0);
                if(ret < 0){
                    goto finish;
                }
            }

        }
    } else if (!eof){
        // 2. 没位流的流程, 且eof=0. 没位流 且 eof=1时，该函数不处理任何东西
        ret = write_packet(of, pkt, ost, 0);
        if(ret < 0){
            goto finish;
        }
    }

finish:
    if (ret < 0 && ret != AVERROR_EOF) {
        av_log(NULL, AV_LOG_ERROR, "Error applying bitstream filters to an output "
               "packet for stream #%d:%d.\n", ost->file_index, ost->index);
        if(exit_on_error){
            //exit_program(1);
            return -2;
        }

        return ret;
    }

    /* 注意: ffmpeg本身对于 使用位流发生错误时 是没处理返回值的(因为每次调用output_packet都是没处理返回值),
        而我们在调用output_packet的地方处理返回值后,位流发送错误相当于处理了,这是区别.
        不想让位流失败退出，可以在位流失败处将ret置为0(或者增加变量记录). */

    //return 0;//不能直接返回0,因为来到这里可能是0或者eof
    return ret;
}

double FFmpegMedia::psnr(double d)
{
    return -10.0 * log10(d);
}

/**
 * @brief 往vstats_file文件写入视频流的相关信息
 * @param ost 输出流
 * @param frame_size 帧大小
 */
void FFmpegMedia::do_video_stats(OutputStream *ost, int frame_size)
{
    AVCodecContext *enc;
    int frame_number;
    double ti1, bitrate, avg_bitrate;

    /* this is executed just the first time do_video_stats is called */
    //这只在第一次调用do_video_stats时执行
    if (!vstats_file) {
        vstats_file = fopen(vstats_filename, "w");
        if (!vstats_file) {
            perror("fopen");
            //exit_program(1);
            return;// 这里打开失败暂时不做处理
        }
    }

    enc = ost->enc_ctx;
    if (enc->codec_type == AVMEDIA_TYPE_VIDEO) {
        frame_number = ost->st->nb_frames;//这个流的帧数
        if (vstats_version <= 1) {
            fprintf(vstats_file, "frame= %5d q= %2.1f ", frame_number,
                    ost->quality / (float)FF_QP2LAMBDA);
        } else  {
            fprintf(vstats_file, "out= %2d st= %2d frame= %5d q= %2.1f ", ost->file_index, ost->index, frame_number,
                    ost->quality / (float)FF_QP2LAMBDA);
        }

        if (ost->error[0]>=0 && (enc->flags & AV_CODEC_FLAG_PSNR))
            fprintf(vstats_file, "PSNR= %6.2f ", psnr(ost->error[0] / (enc->width * enc->height * 255.0 * 255.0)));

        fprintf(vstats_file,"f_size= %6d ", frame_size);
        /* compute pts value */
        ti1 = av_stream_get_end_pts(ost->st) * av_q2d(ost->st->time_base);
        if (ti1 < 0.01)
            ti1 = 0.01;

        bitrate     = (frame_size * 8) / av_q2d(enc->time_base) / 1000.0;
        avg_bitrate = (double)(ost->data_size * 8) / ti1 / 1000.0;
        fprintf(vstats_file, "s_size= %8.0fkB time= %0.3f br= %7.1fkbits/s avg_br= %7.1fkbits/s ",
               (double)ost->data_size / 1024, ti1, bitrate, avg_bitrate);
        fprintf(vstats_file, "type= %c\n", av_get_picture_type_char((AVPictureType)ost->pict_type));
    }
}

/**
 * @brief 将next_picture编码成pkt，然后写帧
 * @param of 输出文件
 * @param ost 输出流
 * @param next_picture 要编码的帧
 * @param sync_ipts 要编码的帧的pts.即AVFrame->pts的值,但精度更好
*/
int FFmpegMedia::do_video_out(OutputFile *of,
                         OutputStream *ost,
                         AVFrame *next_picture,
                         double sync_ipts)
{
    int ret, format_video_sync;
    AVPacket pkt;
    AVCodecContext *enc = ost->enc_ctx;
    AVCodecParameters *mux_par = ost->st->codecpar;
    AVRational frame_rate;
    int nb_frames, nb0_frames, i;
    double delta, delta0;
    double duration = 0;
    int frame_size = 0;
    InputStream *ist = NULL;
    AVFilterContext *filter = ost->filter->filter;

    if (ost->source_index >= 0)
        ist = input_streams[ost->source_index];

    /* 1.获取duration.其中获取duration会参考以下3种方法，duration是可能被重写的 */
    /* 1.1通过过滤器里面的帧率得到duration */
    frame_rate = av_buffersink_get_frame_rate(filter);
    if (frame_rate.num > 0 && frame_rate.den > 0)
        /*
         * 我们在init_output_stream_encode()调用init_encoder_time_base()看到，以视频为例，
         * enc->time_base是由帧率的倒数赋值的，所以这里正常帧率求出的duration是1(也可能是其它). */
        duration = 1/(av_q2d(frame_rate) * av_q2d(enc->time_base));

    /* 1.2若用户指定-r帧率选项，则通过用户的-r选项得到duration */
    if(ist && ist->st->start_time != AV_NOPTS_VALUE && ist->st->first_dts != AV_NOPTS_VALUE && ost->frame_rate.num)
        duration = FFMIN(duration, 1/(av_q2d(ost->frame_rate) * av_q2d(enc->time_base)));

    /* 1.3通过pkt中的pkt_duration得到duration */
    if (!ost->filters_script &&                 /* 过滤器脚本为空 */
        !ost->filters &&                        /* 过滤器字符串为空 */
        (nb_filtergraphs == 0 || !filtergraphs[0]->graph_desc) && /* 过滤器数为空或者第一个FilterGraph的graph_desc为空 */
        next_picture && /* 帧已经开辟内存 */
        ist &&      /* 输入流存在 */
        lrintf(next_picture->pkt_duration * av_q2d(ist->st->time_base) / av_q2d(enc->time_base)) > 0) /* 求出的duration合法 */
    {
        /* lrintf()是四舍五入函数.
         * next_picture->pkt_duration：对应packet的持续时间，以AVStream->time_base单位表示，如果未知则为0。
         * 下面是利用两个比相等求出的：d1/t1=d2/t2，假设pkt_duration=40，ist->st->time_base={1,1000},enc->time_base={1,25}
         * 那么next_picture->pkt_duration * av_q2d(ist->st->time_base)得到的就是40/1000;也就是d1/t1，
         * 而除以1/25，那么就变成乘以25，最终得到式子d2=(d1*t2)/t1，即d2=(40*25)/1000。
         */
        duration = lrintf(next_picture->pkt_duration * av_q2d(ist->st->time_base) / av_q2d(enc->time_base));
        //duration = av_rescale_q(next_picture->pkt_duration, ist->st->time_base, enc->time_base);// 等价于lrintf的处理,但传参是int64
    }

    {
        int a = mid_pred(1,3,5);    // 3
        a = mid_pred(3,4,7);        // 4
        a = mid_pred(4,1,11);       // 4
        a = mid_pred(5,5,11);       // 5
        a = mid_pred(100,4,20);     // 20
        int b = 0;
    }

    // 2. 获取要编码的帧数
    /*
     * 2.1 eof时进来.
     * 在reap_filters的av_buffersink_get_frame_flags调用失败时，
     * flush=1且是文件末尾，并是视频类型时，会传next_picture=NULL进来.
     * 可以全局搜一下do_video_out函数，只在两处地方被调用. */
    if (!next_picture) {
        // end, flushing
        /* mid_pred():一段汇编代码，它的作用是取三个数中,值在中间的那个数. */
        nb0_frames = nb_frames = mid_pred(ost->last_nb0_frames[0],
                                          ost->last_nb0_frames[1],
                                          ost->last_nb0_frames[2]);
    } else {
        // 2.2 计算delta0,delta
        /* delta0是输入帧(next_picture)与输出帧之间的“漂移” */
        delta0 = sync_ipts - ost->sync_opts; // delta0 is the "drift" between the input frame (next_picture) and where it would fall in the output.
        delta  = delta0 + duration;// 此次输入帧next_picture应显示的时长, 用于判断sync_ipts与ost->sync_opts之差是否在一帧以内

#ifdef TYYCODE_TIMESTAMP_ENCODER
        // 不要用av_ts2str()宏打印浮点数,会导致精度缺少
        mydebug(NULL, AV_LOG_INFO, "do_video_out(), sync_ipts: %f, ost->sync_opts: %" PRId64", delta0: %f, "
                "duration: %f, delta: %f, "
                "time_base:%d/%d\n",
                sync_ipts, ost->sync_opts, delta0,
                duration, delta,
                ost->enc_ctx->time_base.num, ost->enc_ctx->time_base.den);
#endif

        /* by default, we output a single frame(默认情况下，我们输出单个帧). */
        nb0_frames = 0; // tracks the number of times the PREVIOUS frame should be duplicated, mostly for variable framerate (VFR)
                        // (跟踪前一帧应该被复制的次数，主要是为了可变帧率(VFR))
        nb_frames = 1;  // 默认编码一帧

        /* 2.3 选择视频同步格式 */
        format_video_sync = video_sync_method;// video_sync_method默认是-1(VSYNC_AUTO)
        // 自动选择视频同步格式
        if (format_video_sync == VSYNC_AUTO) {
            if(!strcmp(of->ctx->oformat->name, "avi")) {
                format_video_sync = VSYNC_VFR;// 输出格式是avi，使用可变帧率.
            } else
                /* 1.输出文件格式允许可变帧率：
                    1）输出文件格式也允许没有时间戳，则视频同步格式为VSYNC_PASSTHROUGH。
                    2）输出文件格式不允许没有时间戳，则视频同步格式为VSYNC_VFR。(正常走这里)
                  2.输出文件格式不允许可变帧率，则为VSYNC_CFR。*/
                format_video_sync = (of->ctx->oformat->flags & AVFMT_VARIABLE_FPS) ?
                            ((of->ctx->oformat->flags & AVFMT_NOTIMESTAMPS) ? VSYNC_PASSTHROUGH : VSYNC_VFR) : VSYNC_CFR;

            // 如果是VSYNC_CFR才会往下判断
            if (   ist
                && format_video_sync == VSYNC_CFR
                && input_files[ist->file_index]->ctx->nb_streams == 1   // 输入流(视频流)只有1个
                && input_files[ist->file_index]->input_ts_offset == 0)  // 默认是0
            {
                format_video_sync = VSYNC_VSCFR;
            }
            if (format_video_sync == VSYNC_CFR && copy_ts) {// -copyts选项
                format_video_sync = VSYNC_VSCFR;
            }
        }

        ost->is_cfr = (format_video_sync == VSYNC_CFR || format_video_sync == VSYNC_VSCFR);

        // 2.4 sync_ipts < sync_opts时的控制处理
        if (delta0 < 0 && /* sync_ipts小于ost->sync_opts */
            delta > 0 &&  /* 表示差距在一帧以内. delta0此时为负，而根据上面delta的计算，此时delta0 + duration需要是一个正数 */
            format_video_sync != VSYNC_PASSTHROUGH &&
            format_video_sync != VSYNC_DROP) {
            if (delta0 < -0.6) {
                av_log(NULL, AV_LOG_VERBOSE, "Past duration %f too large\n", -delta0);
            } else
                av_log(NULL, AV_LOG_DEBUG, "Clipping frame in rate conversion by %f\n", -delta0);// 裁剪帧的速率转换为-delta0

            sync_ipts = ost->sync_opts;// 使sync_ipts等于sync_opts,相当于让这一帧慢点显示
            duration += delta0;// 那么此时，时长应该减去追上的那一段，即delta0，因为输入与输出的差就是delta0.
            delta0 = 0;

#ifdef TYYCODE_TIMESTAMP_ENCODER
        // 不要用av_ts2str()宏打印浮点数,会导致精度缺少
        mydebug(NULL, AV_LOG_INFO, "do_video_out() control, sync_ipts: %f, ost->sync_opts: %" PRId64", delta0: %f, "
                "duration: %f, delta: %f, "
                "time_base:%d/%d\n",
                sync_ipts, ost->sync_opts, delta0,
                duration, delta,
                ost->enc_ctx->time_base.num, ost->enc_ctx->time_base.den);
#endif

        }

        // 2.5 先研究VFR,其它后续研究
        switch (format_video_sync) {
        case VSYNC_VSCFR:
            if (ost->frame_number == 0 && delta0 >= 0.5) {
                av_log(NULL, AV_LOG_DEBUG, "Not duplicating %d initial frames\n", (int)lrintf(delta0));
                delta = duration;
                delta0 = 0;
                ost->sync_opts = lrint(sync_ipts);
            }
        case VSYNC_CFR:
            // FIXME set to 0.5 after we fix some dts/pts bugs like in avidec.c
            if (frame_drop_threshold && delta < frame_drop_threshold && ost->frame_number) {
                nb_frames = 0;
            } else if (delta < -1.1)
                nb_frames = 0;
            else if (delta > 1.1) {
                nb_frames = lrintf(delta);
                if (delta0 > 1.1)
                    nb0_frames = lrintf(delta0 - 0.6);
            }
            break;
        case VSYNC_VFR:// 主要研究这里
            if (delta <= -0.6)// delta是个负数，不会进入上面的if控制语句，那么由上面语句代入delta计算得到：sync_opts - sync_ipts - duration >= 0.6
                              // 意思是: 输入比输出 小于 一帧+0.6，标记nb_frames=0,表示该帧不显示
                nb_frames = 0;
            else if (delta > 0.6)// 需要考虑上面的if语句，sync_ipts - sync_opts + duration > 0.6
                ost->sync_opts = lrint(sync_ipts);// 两种情况:1)sync_ipts < sync_opts时,因为delta0<0,一般此时delta大于0,所以上面if会
                                                  //          将ost->sync_opts赋值给sync_ipts,这里相当于赋原来的值,所以这种情况没太大意义.
                                                  //       2)sync_ipts > sync_opts时,不会进入上面的if,这种就是当输入帧的pts比输出的pts大,输出的pts要追上
#ifdef TYYCODE_TIMESTAMP_ENCODER
            else if(delta > -0.6 && delta <= 0.6){
                mydebug(NULL, AV_LOG_INFO, "do_video_out() switch else if, sync_ipts: %f, ost->sync_opts: %" PRId64", delta0: %f, "
                        "duration: %f, delta: %f\n",
                        sync_ipts, ost->sync_opts, delta0,
                        duration, delta);
            }
#endif
#ifdef TYYCODE_TIMESTAMP_ENCODER
        // 不要用av_ts2str()宏打印浮点数,会导致精度缺少
        mydebug(NULL, AV_LOG_INFO, "do_video_out() switch, delta: %f, nb_frames: %d, "
                "sync_ipts: %f, ost->sync_opts: %" PRId64"\n", delta, nb_frames, sync_ipts, ost->sync_opts);
#endif
            break;
        case VSYNC_DROP:
        case VSYNC_PASSTHROUGH:
            ost->sync_opts = lrint(sync_ipts);
            break;
        default:
            /*av_assert0(0);*/return -1;
        }
    }//<== else end ==>

    // 3. 再次判断可进行编码的帧数. 当frame_number>=最大帧数，nb_frames将会是0或者负数,表示不能再编码.
    nb_frames = FFMIN(nb_frames, ost->max_frames - ost->frame_number);
    nb0_frames = FFMIN(nb0_frames, nb_frames);// VFR时这里没太大意义
#ifdef TYYCODE_TIMESTAMP_ENCODER
        mydebug(NULL, AV_LOG_INFO, "do_video_out() , ost->max_frames: %" PRId64", ost->frame_number: %d, "
                "nb_frames: %d, nb0_frames: %d\n",
                ost->max_frames, ost->frame_number, nb_frames, nb0_frames);
#endif


    // 4. 记录nb0_frames
    // 将数组的第一、第二个元素拷贝到第二、第三个元素的位置.
    // 注，因为这里在拷贝第一个元素时，会将第二个元素的内容覆盖，
    // 所以绝对不能使用memcpy，只能使用memmove，因为它能确保拷贝之前将重复的内存先拷贝到目的地址。
    // see https://www.runoob.com/cprogramming/c-function-memmove.html.
    memmove(ost->last_nb0_frames + 1,
            ost->last_nb0_frames,
            sizeof(ost->last_nb0_frames[0]) * (FF_ARRAY_ELEMS(ost->last_nb0_frames) - 1));
    ost->last_nb0_frames[0] = nb0_frames;

    // 5.  转码推流不会进来,后续研究
    if (nb0_frames == 0 && ost->last_dropped) {
        nb_frames_drop++;
        av_log(NULL, AV_LOG_VERBOSE,
               "*** dropping frame %d from stream %d at ts %" PRId64"\n",
               ost->frame_number, ost->st->index, ost->last_frame->pts);
    }
    if (nb_frames > (nb0_frames && ost->last_dropped) + (nb_frames > nb0_frames)) {
        //帧太大会drop掉
        if (nb_frames > dts_error_threshold * 30) {
            av_log(NULL, AV_LOG_ERROR, "%d frame duplication too large, skipping\n", nb_frames - 1);
            nb_frames_drop++;
            return 0;// 认为正常,不让该推流退出
        }
        nb_frames_dup += nb_frames - (nb0_frames && ost->last_dropped) - (nb_frames > nb0_frames);
        av_log(NULL, AV_LOG_VERBOSE, "*** %d dup!\n", nb_frames - 1);
        if (nb_frames_dup > dup_warning) {
            av_log(NULL, AV_LOG_WARNING, "More than %d frames duplicated\n", dup_warning);
            dup_warning *= 10;//复制超过一定数量会提示，并且下一次提示是本次的10倍？
        }
    }
    ost->last_dropped = nb_frames == nb0_frames && next_picture;// 两者相等且帧存在,就认为被drop了

    // 6. 编码
    /* duplicates frame if needed */
    for (i = 0; i < nb_frames; i++) {
        AVFrame *in_picture;
        int forced_keyframe = 0;
        double pts_time;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        // 6.1 获取要编码的帧in_picture
        if (i < nb0_frames && ost->last_frame) {// nb0_frames>i,例如nb0_frames>0，说明有drop帧,那么一直显示上一帧?
            // 这里正常不会进来
            in_picture = ost->last_frame;
        } else
            in_picture = next_picture;

        if (!in_picture)
            return 0;

        // 6.2 更新要编码的帧的pts
        in_picture->pts = ost->sync_opts;

        // 6.3 检查录像时间
        if (!check_recording_time(ost))
            return 0;// 返回0应该也没问题,因为函数内部会标记ost已经完成

        // 6.4 设置in_picture的一些参数
        // 包含两个宏其中之一 并且 top_field_first>=0
        // AV_CODEC_FLAG_INTERLACED_DCT指隔行扫描？AV_CODEC_FLAG_INTERLACED_ME注释是：交错运动估计
        if (enc->flags & (AV_CODEC_FLAG_INTERLACED_DCT | AV_CODEC_FLAG_INTERLACED_ME) &&
            ost->top_field_first >= 0)
            in_picture->top_field_first = !!ost->top_field_first;// 如果内容是交错的，则首先显示顶部字段。

        // 设置field_order。field_order: 交错视频中的场的顺序。
        if (in_picture->interlaced_frame) {// 图片的内容是交错的
            if (enc->codec->id == AV_CODEC_ID_MJPEG)
                mux_par->field_order = in_picture->top_field_first ? AV_FIELD_TT:AV_FIELD_BB;
            else
                mux_par->field_order = in_picture->top_field_first ? AV_FIELD_TB:AV_FIELD_BT;
        } else
            mux_par->field_order = AV_FIELD_PROGRESSIVE;

        in_picture->quality = enc->global_quality;// 编解码器的全局质量，无法按帧更改。这应该与MPEG-1/2/4 qscale成比例。
        in_picture->pict_type = (AVPictureType)0;// Picture type of the frame.

        // 6.5 强制关键帧的处理.暂时不研究.
        // 利用AVFrame的pts给ost->forced_kf_ref_pts赋值
        if (ost->forced_kf_ref_pts == AV_NOPTS_VALUE &&
            in_picture->pts != AV_NOPTS_VALUE)
            ost->forced_kf_ref_pts = in_picture->pts;

        // 单位转成秒.因为此时的in_picture->pts - ost->forced_kf_ref_pts单位都是enc->time_base
        pts_time = in_picture->pts != AV_NOPTS_VALUE ?
            (in_picture->pts - ost->forced_kf_ref_pts) * av_q2d(enc->time_base) : NAN;

        if (ost->forced_kf_index < ost->forced_kf_count &&
            in_picture->pts >= ost->forced_kf_pts[ost->forced_kf_index]) {// 正常流程不会进来
            ost->forced_kf_index++;
            forced_keyframe = 1;
        } else if (ost->forced_keyframes_pexpr) {// 正常流程不会进来
            double res;
            ost->forced_keyframes_expr_const_values[FKF_T] = pts_time;
            res = av_expr_eval(ost->forced_keyframes_pexpr,
                               ost->forced_keyframes_expr_const_values, NULL);
            ff_dlog(NULL, "force_key_frame: n:%f n_forced:%f prev_forced_n:%f t:%f prev_forced_t:%f -> res:%f\n",
                    ost->forced_keyframes_expr_const_values[FKF_N],
                    ost->forced_keyframes_expr_const_values[FKF_N_FORCED],
                    ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_N],
                    ost->forced_keyframes_expr_const_values[FKF_T],
                    ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_T],
                    res);
            if (res) {
                forced_keyframe = 1;
                ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_N] =
                    ost->forced_keyframes_expr_const_values[FKF_N];
                ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_T] =
                    ost->forced_keyframes_expr_const_values[FKF_T];
                ost->forced_keyframes_expr_const_values[FKF_N_FORCED] += 1;
            }

            ost->forced_keyframes_expr_const_values[FKF_N] += 1;
        } else if (   ost->forced_keyframes
                   && !strncmp(ost->forced_keyframes, "source", 6)
                   && in_picture->key_frame==1) {// 正常流程不会进来
            forced_keyframe = 1;
        }

        if (forced_keyframe) {// 正常流程不会进来
            in_picture->pict_type = AV_PICTURE_TYPE_I;
            av_log(NULL, AV_LOG_DEBUG, "Forced keyframe at time %f\n", pts_time);
        }

        update_benchmark(NULL);//没有指定选项-benchmark_all，可忽略
        if (debug_ts) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_INFO, "encoder <- type:video "
                   "frame_pts:%s frame_pts_time:%s time_base:%d/%d\n",
                   av_ts2str_ex(str, in_picture->pts), av_ts2timestr_ex(str, in_picture->pts, &enc->time_base),
                   enc->time_base.num, enc->time_base.den);
        }

        ost->frames_encoded++;// 统计发送到编码器的帧个数

        // 6.6 将帧发送到编码器
        ret = avcodec_send_frame(enc, in_picture);
        if (ret < 0)
            goto error;

        // Make sure Closed Captions will not be duplicated(确保不会重复关闭字幕)
        // av_frame_remove_side_data(): 如果frame中存在所提供类型的边数据，请将其释放并从frame中删除
        av_frame_remove_side_data(in_picture, AV_FRAME_DATA_A53_CC);

        // 6.7 循环从编码器中读取编码后的pkt
        while (1) {
            // 6.7.1 从编码器获取pkt
            ret = avcodec_receive_packet(enc, &pkt);
            update_benchmark("encode_video %d.%d", ost->file_index, ost->index);
            if (ret == AVERROR(EAGAIN))
                break;//一般每次读完一个pkt会从这里退出while
            if (ret < 0)
                goto error;

            if (debug_ts) {
                char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_log(NULL, AV_LOG_INFO, "encoder -> type:video "
                       "pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s\n",
                       av_ts2str_ex(str, pkt.pts), av_ts2timestr_ex(str, pkt.pts, &enc->time_base),
                       av_ts2str_ex(str, pkt.dts), av_ts2timestr_ex(str, pkt.dts, &enc->time_base));
            }

            // 6.7.2 编码后的pkt.pts为空 且 编码器能力集不包含AV_CODEC_CAP_DELAY
            /* AV_CODEC_CAP_DELAY:
             * 编码器或解码器需要在末尾使用NULL输入进行刷新，以便提供完整和正确的输出。
             * NOTE: 如果没有设置此标志，则保证编解码器永远不会输入NULL数据。用户仍然可以向公共编码或解码函数发送NULL数据，
             * 但libavcodec不会将其传递给编解码器，除非设置了此标志。
             *
             * Decoders: 解码器有一个非零延迟，需要在最后用avpkt->data=NULL, avpkt->size=0来获得延迟的数据，直到解码器不再返回帧。
             * Encoders: 编码器需要在编码结束时提供NULL数据，直到编码器不再返回数据。
             *
             * NOTE: 对于实现AVCodec.encode2()函数的编码器，设置此标志还意味着编码器必须设置每个输出包的pts和持续时间。
             * 如果未设置此标志，则pts和持续时间将由libavcodec从输入帧中确定。
            */
            if (pkt.pts == AV_NOPTS_VALUE && !(enc->codec->capabilities & AV_CODEC_CAP_DELAY))
                pkt.pts = ost->sync_opts;

            // 6.7.3 将编码后的时基转成复用时基. 这里需要留意
            // ffmpeg实际上这样转回丢失部分精度,可参考编码前后pkt.pts的值.
            av_packet_rescale_ts(&pkt, enc->time_base, ost->mux_timebase);

            if (debug_ts) {
                char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_log(NULL, AV_LOG_INFO, "encoder -> type:video "
                    "pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s\n",
                    av_ts2str_ex(str, pkt.pts), av_ts2timestr_ex(str, pkt.pts, &ost->mux_timebase),
                    av_ts2str_ex(str, pkt.dts), av_ts2timestr_ex(str, pkt.dts, &ost->mux_timebase));
            }

            frame_size = pkt.size;// 记录当前帧大小

            // 6.7.4 写帧
            ret = output_packet(of, &pkt, ost, 0);
            if(ret < 0){
                av_log(NULL, AV_LOG_ERROR, "output_packet failed, ret: %d\n", ret);
                goto error;
            }

            /* if two pass, output log */
            if (ost->logfile && enc->stats_out) {// 忽略，推流没用到
                fprintf(ost->logfile, "%s", enc->stats_out);
            }
        }//<== while (1) end ==>

        // 6.8 预测下一输出帧的pts
        ost->sync_opts++;

        // 6.9 记录视频写帧的个数
        /*
         * For video, number of frames in == number of packets out.
         * But there may be reordering, so we can't throw away frames on encoder
         * flush, we need to limit them here, before they go into encoder.
         */
        /* 对于视频，输入的帧数=输出的包数。但是可能会有重新排序，所以我们不能在编码器flush时丢弃帧，
         * 我们需要在它们进入编码器之前，在这里限制它们. */
        ost->frame_number++;
#ifdef TYYCODE_TIMESTAMP_ENCODER
        mydebug(NULL, AV_LOG_INFO, "do_video_out(), ost->frame_number: %d\n", ost->frame_number);
#endif

        //打印视频相关信息，vstats_filename默认是NULL，默认不会进来
        if (vstats_filename && frame_size)
            do_video_stats(ost, frame_size);
    }//<== for (i = 0; i < nb_frames; i++) end ==>

    // 7. 将当前帧引用给last_frame
    if (!ost->last_frame)
        ost->last_frame = av_frame_alloc();
    av_frame_unref(ost->last_frame);
    if (next_picture && ost->last_frame)
        av_frame_ref(ost->last_frame, next_picture);// 不管上面的逻辑如何执行,每次都会保存上一帧
    else
        av_frame_free(&ost->last_frame);// next_picture为空会进入这里，因为last_frame在上面是av_frame_alloc的

    return 0;

error:
    av_log(NULL, AV_LOG_FATAL, "Video encoding failed\n");
    //exit_program(1);
    return ret;
}

/**
 * @brief 对frame进行编码，并进行写帧操作.
 * @param of 输出文件
 * @param ost 输出流
 * @param frame 要编码的帧
 * @return void.
 * @note 成功或者输出流完成编码不返回任何内容, 失败程序退出.
*/
int FFmpegMedia::do_audio_out(OutputFile *of, OutputStream *ost,
                         AVFrame *frame)
{
    AVCodecContext *enc = ost->enc_ctx;
    AVPacket pkt;
    int ret;

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    // 1. 检查该输出流是否完成编码.
    if (!check_recording_time(ost))
        return 0;

    // 2. 输入帧为空，或者 audio_sync_method小于0，pts会参考ost->sync_opts.
    // audio_sync_method默认0
    if (frame->pts == AV_NOPTS_VALUE || audio_sync_method < 0)
        frame->pts = ost->sync_opts;

    // 3. 根据当前帧的pts和nb_samples预估下一帧的pts。
    ost->sync_opts = frame->pts + frame->nb_samples;// 音频时,frame->pts的单位是采样点个数,例如采样点数是1024,采样频率是44.1k，那么就是0.23s.
                                                    // 可参考ffplay的decoder_decode_frame()注释

    // 4. 统计已经编码的采样点个数和帧数.
    ost->samples_encoded += frame->nb_samples;
    ost->frames_encoded++;

    av_assert0(pkt.size || !pkt.data);
    update_benchmark(NULL);
    if (debug_ts) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_log(NULL, AV_LOG_INFO, "encoder <- type:audio "
               "frame_pts:%s frame_pts_time:%s time_base:%d/%d\n",
               av_ts2str_ex(str, frame->pts), av_ts2timestr_ex(str, frame->pts, &enc->time_base),
               enc->time_base.num, enc->time_base.den);
    }

    // 5. 发送输入帧到编码器进行编码
    ret = avcodec_send_frame(enc, frame);
    if (ret < 0)
        goto error;

#ifdef TYYCODE_TIMESTAMP_ENCODER
    // 统计输入到音频编码器的帧数.根据这里可以指定音频输入帧数!=输出包数.
    // 例如输入帧是1299,而输出包是1300
    static int inputPktNum = 0;
    inputPktNum++;
    mydebug(NULL, AV_LOG_INFO, "do_audio_out(), inputPktNum: %d\n", inputPktNum);
#endif

    // 6. 循环获取编码后的pkt,并进行写帧
    while (1) {
        // 6.1 获取编码后的pkt
        ret = avcodec_receive_packet(enc, &pkt);
        if (ret == AVERROR(EAGAIN))
            break;
        if (ret < 0)
            goto error;

        update_benchmark("encode_audio %d.%d", ost->file_index, ost->index);

        // 6.2 每次写帧前都会将pts转成复用的时基
        av_packet_rescale_ts(&pkt, enc->time_base, ost->mux_timebase);

        if (debug_ts) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_INFO, "encoder -> type:audio "
                   "pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s\n",
                   av_ts2str_ex(str, pkt.pts), av_ts2timestr_ex(str, pkt.pts, &enc->time_base),
                   av_ts2str_ex(str, pkt.dts), av_ts2timestr_ex(str, pkt.dts, &enc->time_base));
        }

        // 6.3 写帧
        ret = output_packet(of, &pkt, ost, 0);
        if(ret < 0){
            goto error;
        }
    }

    return 0;
error:
    av_log(NULL, AV_LOG_FATAL, "Audio encoding failed\n");
    //exit_program(1);
    return ret;
}

/**
 * Get and encode new output from any of the filtergraphs, without causing
 * activity.(在不引起活动的情况下，从任何过滤图获取并编码新的输出)
 *
 * @return  0 for success, <0 for severe errors
 */
/**
 * @brief 遍历每个输出流,从输出过滤器中获取一帧送去编码,然后进行写帧.
 * @param flush =1时,会调用do_video_out().
 * @return 成功=0, 失败返回负数或者程序退出.
*/
int FFmpegMedia::reap_filters(int flush)
{
    AVFrame *filtered_frame = NULL;
    int i;

    /* Reap all buffers present in the buffer sinks */
    /* 一 获取在buffer sinks中存在的所有缓冲区 */
    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost = output_streams[i];
        OutputFile    *of = output_files[ost->file_index];
        AVFilterContext *filter;
        AVCodecContext *enc = ost->enc_ctx;
        int ret = 0;

        /* 1. 若ost->filter为空或者ost->filter->graph->graph为空则跳过(具体意义是没有解码帧则不编码)。
         *
         * ost->filter在init_simple_filtergraph()时被创建。
         * AVFilterGraph是什么时候？在configure_filtergraph().
         * configure_filtergraph()什么时候被调用?在该输入流的pkt被解码第一帧的时候.
         */
        if (!ost->filter || !ost->filter->graph->graph)
            continue;
        filter = ost->filter->filter;// 输出过滤器AVFilterContext

        /* 2. 对还没调用init_output_stream()初始化的输出流，则调用。
         * 一般视频、音频都是在这里初始化.内部主要是打开编解码器和写头. */
        if (!ost->initialized) {
            char error[1024] = "";
            ret = init_output_stream(ost, error, sizeof(error));
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error initializing output stream %d:%d -- %s\n",
                       ost->file_index, ost->index, error);
                //exit_program(1);
                return ret;
            }
        }

        // 若ost->filtered_frame为空则给其开辟内存.
        if (!ost->filtered_frame && !(ost->filtered_frame = av_frame_alloc())) {
            return AVERROR(ENOMEM);
        }
        filtered_frame = ost->filtered_frame;

        // 3. 循环从过滤器中获取处理后的帧,然后进行编码.
        while (1) {
            double float_pts = AV_NOPTS_VALUE; // this is identical to filtered_frame.pts but with higher precision
                                               // 这与filtered_frame.pts相同，但精度更高
            // 3.1 从输出过滤器读取一帧。
            // while一般从这里的 av_buffersink_get_frame_flags 退出，第二次再读时，因为输出过滤器没有帧可读会返回AVERROR(EAGAIN)。
            ret = av_buffersink_get_frame_flags(filter, filtered_frame,
                                               AV_BUFFERSINK_FLAG_NO_REQUEST);
            if (ret < 0) {
                /* 不是eagain也不是文件末尾 */
                if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_log(NULL, AV_LOG_WARNING,
                           "Error in av_buffersink_get_frame_flags(): %s\n", av_err2str_ex(str, ret));
                } else if (flush && ret == AVERROR_EOF) {/* flush=1且是文件末尾 */
                    if (av_buffersink_get_type(filter) == AVMEDIA_TYPE_VIDEO){
                        ret = do_video_out(of, ost, NULL, AV_NOPTS_VALUE);// 转码结束时从transcode_from_filter()会来到这个逻辑
                        if(ret < 0){
                            av_log(NULL, AV_LOG_ERROR, "do_video_out flush null frame failed.\n");
                            return -1;
                        }
                    }
                }
                break;
            }

            if (ost->finished) {// 正常不会进来
                av_frame_unref(filtered_frame);
                continue;
            }

            // 3.2 计算float_pts、filtered_frame->pts的值.
            /* 思路也不难:
             * 1)若用户指定start_time，则解码后的帧的显示时间戳需要减去start_time，并且增加精度。
             * 例如本来3s显示该帧，假设用户start_time=1s，那么就应该在3-1=2s就显示该帧.
             * 2)若没指定，start_time=0，只会增加精度.
             * 这两者求法是一样的，区别是float_pts用的tb.den被修改过，
             * 而filtered_frame->pts使用原来的enc->time_base去求，看ffmpeg的float_pts注释，区别是精度不一样. */
            if (filtered_frame->pts != AV_NOPTS_VALUE) {
                int64_t start_time = (of->start_time == AV_NOPTS_VALUE) ? 0 : of->start_time;// 用户是否指定起始时间
                AVRational filter_tb = av_buffersink_get_time_base(filter);
                AVRational tb = enc->time_base;
                int tyycode = av_log2(tb.den);
                // 3.2.1 扩大分母
                // 1）av_clip(): 用来限制参1最终落在[0,16]的范围.
                // 2）av_log2()(参考ffplay音频相关): 应该类似C++的log2()函数返回数字的以2为底的对数。
                // 例如表达式是：av_log2(48000)=15;
                // 大概估计是2^x=48000, 2^15(32768)<48000<2^16(65536). x=15
                // av_log2(25)=4;
                // 2^x=25, 2^4<25<2^5. 所以x=4.   貌似返回的是小的次方值
                int extra_bits = av_clip(29 - av_log2(tb.den), 0, 16);
                //int extra_bits = av_clip(32 - av_log2(tb.den), 0, 16);// tyycode,改成32测试好像也没太大问题

                tb.den <<= extra_bits;// 扩大分母,为了下面转换float_pts时,得到精度更大的值.例如从25变成1638400

                // 3.2.2 转换精度
                float_pts =
                    av_rescale_q(filtered_frame->pts, filter_tb, tb) -
                    av_rescale_q(start_time, AV_TIME_BASE_Q, tb);

                // 3.2.3 转换单位为enc->time_base
                float_pts /= 1 << extra_bits;

                // avoid exact midoints to reduce the chance of rounding differences, this can be removed in case the fps code is changed to work with integers
                // (避免精确的中点以减少舍入差异的机会，这可以在FPS代码更改为使用整数时删除)
                float_pts += FFSIGN(float_pts) * 1.0 / (1<<17);// FFSIGN宏很简单:就是取正负,但除以1<<17是啥意思?留个疑问

                // 3.2.4 计算filtered_frame->pts的值。主要是转单位为enc->time_base
                filtered_frame->pts =
                    av_rescale_q(filtered_frame->pts, filter_tb, enc->time_base) -
                    av_rescale_q(start_time, AV_TIME_BASE_Q, enc->time_base);
            }

            // 到这里,float_pts与filtered_frame->pts的单位是enc->time_base.

            // 3.3 将读到的帧进行编码
            switch (av_buffersink_get_type(filter)) {
            case AVMEDIA_TYPE_VIDEO:
                // 用户没指定输出流的宽高比，会使用帧的宽高比给编码器的宽高比赋值
                if (!ost->frame_aspect_ratio.num)
                    enc->sample_aspect_ratio = filtered_frame->sample_aspect_ratio;

                if (debug_ts) {
                    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_log(NULL, AV_LOG_INFO, "filter -> pts:%s pts_time:%s exact:%f time_base:%d/%d\n",
                            av_ts2str_ex(str, filtered_frame->pts), av_ts2timestr_ex(str, filtered_frame->pts, &enc->time_base),
                            float_pts,
                            enc->time_base.num, enc->time_base.den);
                }

                ret = do_video_out(of, ost, filtered_frame, float_pts);
                if(ret < 0){
                    av_log(NULL, AV_LOG_ERROR, "do_video_out failed.\n");
                    return -1;
                }
                break;
            case AVMEDIA_TYPE_AUDIO:
                // AV_CODEC_CAP_PARAM_CHANGE: 编解码器支持在任何点更改参数。
                if (!(enc->codec->capabilities & AV_CODEC_CAP_PARAM_CHANGE) &&
                    enc->channels != filtered_frame->channels) {//编码器不支持更改参数且编码器与要编码的帧不一样，那么不编码该帧.
                    av_log(NULL, AV_LOG_ERROR,
                           "Audio filter graph output is not normalized and encoder does not support parameter changes\n");
                    break;
                }

                ret = do_audio_out(of, ost, filtered_frame);
                if(ret < 0){
                    av_log(NULL, AV_LOG_ERROR, "do_audio_out failed.\n");
                    return -1;
                }
                break;
            default:
                // TODO support subtitle filters
                //av_assert0(0);
                return -1;
            }//<== switch (av_buffersink_get_type(filter)) end ==>


            av_frame_unref(filtered_frame);
        }//<== while (1) end ==>

    }//<== for (i = 0; i < nb_output_streams; i++) end ==>

    return 0;
}

/**
 * Perform a step of transcoding for the specified filter graph.(执行指定滤波图的转码步骤)
 *
 * @param[in]  graph     filter graph to consider(要考虑的滤波图)
 * @param[out] best_ist  input stream where a frame would allow to continue(输入流中的帧允许继续，传出参数)
 * @return  0 for success, <0 for error
 */
/**
 * @brief 从buffersink读取帧:
 * 1)如果有帧,会调reap_filters进行编码写帧;
 * 2)如果没有帧可读,eof或者读帧失败会直接返回;否则会进行eagain的判断:
 *      2.1)会选择输入文件中读帧失败次数最多的输入流,作为best_ist进行传出.
 * @param gragh 滤波图
 * @param best_ist eagain时,失败次数最多的输入流.
 * @return 成功=0,注意best_ist=NULL时也会返回0,认为转码结束; 失败返回负数或者程序退出.
*/
int FFmpegMedia::transcode_from_filter(FilterGraph *graph, InputStream **best_ist)
{
    int i, ret;
    int nb_requests, nb_requests_max = 0;
    InputFilter *ifilter;
    InputStream *ist;

    *best_ist = NULL;

//    {
//        // 从ffmpeg源码提取的avfilter_graph_request_oldest的部分代码,
//        // 用于测试avfilter_graph_request_oldest的作用.不测试时必须注释掉.
//        int r = -1;
//        AVFilterLink *oldest = graph->graph->sink_links[0];
//        oldest = graph->graph->sink_links[0];
//        if (oldest->dst->filter->activate) {
//            /* For now, buffersink is the only filter implementing activate. */
//            r = av_buffersink_get_frame_flags(oldest->dst, NULL,
//                                              AV_BUFFERSINK_FLAG_PEEK);
//            if (r != AVERROR_EOF)
//                return r;
//        } else {
//            //r = ff_request_frame(oldest);
//        }
//    }

    /*
     * avfilter_graph_request_oldest(): 在最老的sink link中请求帧。
     * 如果请求返回AVERROR_EOF，则尝试下一个。
     * 请注意，这个函数并不是过滤器图的唯一调度机制，它只是一个方便的函数，在正常情况下以平衡的方式帮助排出过滤器图。
     * 还要注意，AVERROR_EOF并不意味着帧在进程期间没有到达某些sinks。
     * 当有多个sink links时，如果请求的link返回一个EOF，这可能会导致过滤器刷新发送到另一个sink link的挂起的帧，尽管这些帧没有被请求。
     * @return ff_request_frame()的返回值，或者如果所有的links都返回AVERROR_EOF，则返回AVERROR_EOF。
     * ff_request_frame()的返回值：
     * >=0： 成功；
     * AVERROR_EOF：文件结尾；
     * AVERROR(EAGAIN)：如果之前的过滤器当前不能输出一帧，也不能保证EOF已经到达。
    */
    /* 1.获取graph有多少frame 可以从buffersink读取.正常都是返回eagain(-11) */
    ret = avfilter_graph_request_oldest(graph->graph);
    if (ret >= 0)
        return reap_filters(0);

    if (ret == AVERROR_EOF) {
        ret = reap_filters(1);
        // 为每个OutputFilter标记编码结束
        for (i = 0; i < graph->nb_outputs; i++)
            close_output_stream(graph->outputs[i]->ost);// 正常时会 在这里标记结束.
        return ret;
    }
    if (ret != AVERROR(EAGAIN))
        return ret;

    // 处理EAGAIN的情况
    /* 2. 判断输入文件是否处理完成. */
    for (i = 0; i < graph->nb_inputs; i++) {
        ifilter = graph->inputs[i];
        // 2.1 输入文件遇到eagain或者eof(上面的是输出流的EAGAIN,eof处理),执行continue
        ist = ifilter->ist;
        if (input_files[ist->file_index]->eagain ||
            input_files[ist->file_index]->eof_reached)
            continue;

        /* av_buffersrc_get_nb_failed_requests():
         * 获取失败请求的数量。当调用request_frame方法而缓冲区中没有帧时，请求失败。
         * 当添加一个帧时，该数字被重置*/
        // 2.2 记录遇到失败次数最多的输入流.
        // 注意是从输入filter请求.
        nb_requests = av_buffersrc_get_nb_failed_requests(ifilter->filter);
        if (nb_requests > nb_requests_max) {
            nb_requests_max = nb_requests;
            *best_ist = ist;
        }
    }

    // 3. 找不到best_ist,说明输入文件遇到eagain或者eof,或者没有失败次数.那么会标记对应的输出流为不可用
    if (!*best_ist)
        for (i = 0; i < graph->nb_outputs; i++)
            graph->outputs[i]->ost->unavailable = 1;

    // 4. 找不到best_ist(即*best_ist=NULL),也会返回0.
    return 0;
}

/**
 * @brief 从输入文件读取一个pkt.
 * @return 成功=0 失败返回负数
*/
int FFmpegMedia::get_input_packet(InputFile *f, AVPacket *pkt)
{
    // 1. 判断是否指定-re选项稳定读取pkt.
    // 实现稳定读取的思路很简单,就是对比当前读输入流当前的dts与该输入流转码经过的时间的大小.
    if (f->rate_emu) {
        int i;
        for (i = 0; i < f->nb_streams; i++) {
            InputStream *ist = input_streams[f->ist_index + i];
            // 很简单,就是:dts*1000000/1000000.内部会进行四舍五入的操作
            int64_t pts = av_rescale(ist->dts, 1000000, AV_TIME_BASE);
            // 该输入流转码经过的时间
            int64_t now = av_gettime_relative() - ist->start;
#ifdef TYYCODE_TIMESTAMP_DEMUXER
            mydebug(NULL, AV_LOG_INFO, "get_input_packet(), ist->dts: %" PRId64", pts(AV_TIME_BASE): %" PRId64", "
                   "ist->start: %" PRId64", now: %" PRId64"\n", ist->dts, pts, ist->start, now);
#endif
            // 输入流的读取速度比实际时间差还要大,说明太快了,需要暂停一下,那么返回EAGAIN.
            if (pts > now)
                return AVERROR(EAGAIN);
        }
    }

    // 2. 从输入文件读取一个pkt.
#if HAVE_THREADS
    //多个输入文件的流程
    if (nb_input_files > 1)
        return get_input_packet_mt(f, pkt);
#endif
    //单个文件的正常流程
    return av_read_frame(f->ctx, pkt);
}

// This does not quite work like avcodec_decode_audio4/avcodec_decode_video2.
// There is the following difference: if you got a frame, you must call
// it again with pkt=NULL. pkt==NULL is treated differently from pkt->size==0
// (pkt==NULL means get more output, pkt->size==0 is a flush/drain packet)
/* 这并不像avcodec_decode_audio4/avcodec_decode_video2那样工作.
 * 有以下区别：如果您有一个frame，则必须使用pkt=NULL再次调用它。
 * pkt==NULL与pkt->size==0的处理方式不同（pkt==NULL意味着获得更多输出，pkt->size==0是一个刷新/漏包）
*/
/**
 * @brief 解码pkt.
 * @param avctx 解码器上下文
 * @param frame 用于存储解码后的一帧
 * @param got_frame =1:解码一帧成功; =0:解码一帧失败,可能发包或者接收帧错误,可能遇到eagain,使用返回值判断即可.
 * @param pkt 待解码的pkt
 *
 * @return 成功0,失败返回负数. 注意,当返回0成功时,是否成功解码一帧需要配合got_frame的值才能判断,因为此时接收包可能遇到eagain.
 *
 * @note avcodec_decode_video2是旧版本的解码方式,avcodec_send_packet + avcodec_receive_frame是新版本ffmpeg的解码方式.
*/
int FFmpegMedia::decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
    int ret;

    *got_frame = 0;

    // 1. 发送包去解码.
    if (pkt) {
        ret = avcodec_send_packet(avctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
        // (特别是，我们不期望AVERROR(EAGAIN)，因为我们使用avcodec_receive_frame()读取所有已解码的帧，直到完成)
        if (ret < 0 && ret != AVERROR_EOF)// 除了eof,发包错误都会直接返回.
            return ret;
    }

    // 2. 获取解码后的一帧
    ret = avcodec_receive_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR(EAGAIN))//除了eagain,错误都会直接返回.
        return ret;

    // 3. 不是eagain,got_frame标记为1.
    if (ret >= 0)
        *got_frame = 1;

    //接收包遇到eagain时,返回值是0,got_frame=0
    return 0;
}

/**
 * @brief 对decode()的结果进行检测.与-xerror选项有关.
 * @param ist 输入流
 * @param got_output decode的传出参数.
 * @param ret 解码后的返回值
*/
int FFmpegMedia::check_decode_result(InputStream *ist, int *got_output, int ret)
{
    // 1. 保存成功解码一帧的次数,以及解码失败的次数.
    /* 分析got_output、ret的情况:
     * 1)当got_output=1时:
     *  1.1)ret=0(大于0也一样),那么表示decode()成功解码一帧;次数保存在decode_error_stat[0];
     *  1.2)ret=负数,不存在.因为decode(),got_output=1时返回值必定是0.所以不考虑.
     * 2)当got_output=0时:
     *  2.1)ret=0(大于0也一样),不会进入该if,所以不考虑.
     *  2.2)ret=负数,那么表示解码失败.次数保存在decode_error_stat[1];
    */
    if (*got_output || ret<0)
        decode_error_stat[ret<0] ++;

    // 2. 若解码失败 且 用户指定-xerror选项,那么程序退出.
    if (ret < 0 && exit_on_error){
        //exit_program(1);
        return -1;
    }

    // 3. 解码帧成功,但是该帧存在错误.用户指定-xerror选项,那么程序退出.没指定只会报警告.
    if (*got_output && ist) {
        //decode_error_flags:解码帧的错误标志，如果解码器产生帧，但在解码期间存在错误，则设置为FF_DECODE_ERROR_xxx标志的组合.
        //ist->decoded_frame->flags:帧标志，@ref lavu_frame_flags的组合.
        //AV_FRAME_FLAG_CORRUPT:帧数据可能被损坏，例如由于解码错误.
        if (ist->decoded_frame->decode_error_flags || (ist->decoded_frame->flags & AV_FRAME_FLAG_CORRUPT)) {
            av_log(NULL, exit_on_error ? AV_LOG_FATAL : AV_LOG_WARNING,
                   "%s: corrupt decoded frame in stream %d\n", input_files[ist->file_index]->ctx->url, ist->st->index);
            if (exit_on_error){
                //exit_program(1);
                return -1;
            }
        }
    }

    return 0;
}

/**
 * @brief 从解码后的一帧获取相关参数保存到InputFilter,如果涉及到硬件,还会给其添加相关引用.
 * @param ifilter 封装的输入过滤器.
 * @param frame 解码后的一帧.
 * @return 成功-0 失败-负数.
*/
int FFmpegMedia::ifilter_parameters_from_frame(InputFilter *ifilter, const AVFrame *frame)
{
    // 1. 若ifilter->hw_frames_ctx引用不为空,则先释放.
    /* av_buffer_unref(): 释放一个给定的引用，如果没有更多的引用，则自动释放缓冲区。
    @param buf被释放的引用。返回时将指针设置为NULL */
    av_buffer_unref(&ifilter->hw_frames_ctx);

    // 2. 从解码帧拷贝相关参数到InputFilter中.
    ifilter->format = frame->format;

    ifilter->width               = frame->width;
    ifilter->height              = frame->height;
    ifilter->sample_aspect_ratio = frame->sample_aspect_ratio;

    ifilter->sample_rate         = frame->sample_rate;
    ifilter->channels            = frame->channels;
    ifilter->channel_layout      = frame->channel_layout;

    // 3. 给ifilter->hw_frames_ctx添加引用.(硬件相关)
    if (frame->hw_frames_ctx) {
        ifilter->hw_frames_ctx = av_buffer_ref(frame->hw_frames_ctx);
        if (!ifilter->hw_frames_ctx)
            return AVERROR(ENOMEM);
    }

    return 0;
}

/**
 * @brief 主要工作是 配置FilterGraph 和 将解码后的一帧送去过滤器. 主要流程:
 * 1)判断InputFilter的相关参数是否被改变,若和解码后的帧参数不一致的话,need_reinit=1;
 *      特殊地,用户指定-reinit_filter=0时,即使参数不一样,need_reinit会重置为0;
 * 2)如果需要重新初始化 或者 fg->graph没有初始化的话, 则进入配置流程.
 * 3)发送该解码帧到fg->graph的中buffersrc.
 *
 * @param ifilter 输入过滤器
 * @param frame 解码帧
 * @return 成功=0, 失败返回负数或者程序退出.
 */
int FFmpegMedia::ifilter_send_frame(InputFilter *ifilter, AVFrame *frame)
{
    FilterGraph *fg = ifilter->graph;//获取该输入过滤器对应的fg, see init_simple_filtergraph
    int need_reinit, ret, i;

    /* determine if the parameters for this input changed(确定此输入的参数是否已更改) */
    need_reinit = ifilter->format != frame->format;//判断输入流与解码后的帧格式是否不一致

    // 1. 判断输入流的输入过滤器是否需要重新初始化.
    // 判断依据:输入流中的输入过滤器保存的参数 是否与 解码帧的参数 存在不一样.
    switch (ifilter->ist->st->codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        need_reinit |= ifilter->sample_rate    != frame->sample_rate ||
                       ifilter->channels       != frame->channels ||
                       ifilter->channel_layout != frame->channel_layout;
        break;
    case AVMEDIA_TYPE_VIDEO:
        need_reinit |= ifilter->width  != frame->width ||
                       ifilter->height != frame->height;
        break;
    }

    // 2. 如果没指定-reinit_filter 且 fg->graph已经初始化,那么need_reinit会置为0.
    // 即输入流参数改变不重新初始化FilterGraph
    if (!ifilter->ist->reinit_filters && fg->graph)
        need_reinit = 0;

    // 3. 硬件输入过滤器的hw_frames_ctx 与 帧的hw_frames_ctx不相等 或者 其内部的data不相等(硬件相关,暂不深入研究)
    if (!!ifilter->hw_frames_ctx != !!frame->hw_frames_ctx ||
        (ifilter->hw_frames_ctx && ifilter->hw_frames_ctx->data != frame->hw_frames_ctx->data))
        need_reinit = 1;

    // 4. 如果需要重新初始化InputFilter,则从解码帧拷贝相关参数.以便更新输入过滤器.
    if (need_reinit) {
        ret = ifilter_parameters_from_frame(ifilter, frame);
        if (ret < 0)
            return ret;
    }

    /* (re)init the graph if possible, otherwise buffer the frame and return */
    // ( (重新)初始化图形如果可能，否则缓冲帧并返回 )
    // 5. 如果需要重新初始化 或者 fg->graph没有初始化的话,往下执行.
    if (need_reinit || !fg->graph) {
        // 5.1 遍历fg的每个InputFilter的format是否已经初始化完成.
        // 正常推流命令,一般一个输入流对应一个fg,一个fg包含一个InputFilter(nb_inputs=1),一个OutputFilter(nb_outputs=1)
        for (i = 0; i < fg->nb_inputs; i++) {

            // 5.1.1 若fg中InputFilter数组,存在有未初始化的InputFilter->format,那么会把该解码帧先放到ifilter的帧队列.
            // 上面看到ifilter_parameters_from_frame()时会初始化format.
            if (!ifilter_has_all_input_formats(fg)) {
                /* av_frame_clone(): 创建一个引用与src相同数据的frame,源码很简单.
                 * 这是av_frame_alloc()+av_frame_ref()的快捷方式.
                 * 成功返回新创建的AVFrame，错误返回NULL. */
                AVFrame *tmp = av_frame_clone(frame);
                if (!tmp)
                    return AVERROR(ENOMEM);
                av_frame_unref(frame);

                //ifilter的帧队列空间不足,按两倍大小扩容.
                if (!av_fifo_space(ifilter->frame_queue)) {
                    ret = av_fifo_realloc2(ifilter->frame_queue, 2 * av_fifo_size(ifilter->frame_queue));
                    if (ret < 0) {
                        av_frame_free(&tmp);
                        return ret;
                    }
                }

                //av_fifo_generic_write(): 将数据从用户提供的回调提供给AVFifoBuffer。详细看write_packet()的注释.
                //这里会将数据先保存到ifilter的帧队列
                av_fifo_generic_write(ifilter->frame_queue, &tmp, sizeof(tmp), NULL);
                return 0;
            }
        }

        // 5.2 编码、写帧操作.
        // 一般不是这里调用去编码的.
        // 这里调用reap_filters(1)是为了处理need_reinit=1,fg->graph!=NULL时,清空旧过滤器中的旧解码帧到输出url.
        ret = reap_filters(1);
        if (ret < 0 && ret != AVERROR_EOF) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_ERROR, "Error while filtering: %s\n", av_err2str_ex(str, ret));
            return ret;
        }

        // 5.3 配置FilterGraph(音视频流都是在这里调用去配置fg的).
        ret = configure_filtergraph(fg);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error reinitializing filters!\n");
            return ret;
        }
    }

    // 6. 将该解码帧送去滤波器,以便得到想要的输出帧格式.(最终会在reap_filters获取该输出帧格式和进行编码)
    ret = av_buffersrc_add_frame_flags(ifilter->filter, frame, AV_BUFFERSRC_FLAG_PUSH);
    if (ret < 0) {
        if (ret != AVERROR_EOF){
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_ERROR, "Error while filtering: %s\n", av_err2str_ex(str, ret));
        }

        return ret;
    }

    return 0;
}

/**
 * @brief 将解码帧 送到 InputFilter数组的各个元素(ist->filters[i])处理, 具体看ifilter_send_frame().
 * @param ist 输入流
 * @param decoded_frame 解码帧
 * @return 成功=0, 失败返回负数或者程序退出.
 */
int FFmpegMedia::send_frame_to_filters(InputStream *ist, AVFrame *decoded_frame)
{
    int i, ret;
    AVFrame *f;

    av_assert1(ist->nb_filters > 0); /* ensure ret is initialized */
    // 1. 遍历输入流的每个过滤器(正常一个输入流只有一个InputFilter,即nb_filters=1).
    for (i = 0; i < ist->nb_filters; i++) {
        // 1.1 获取f的值.
        if (i < ist->nb_filters - 1) {//减1意义是: 只有当nb_filters>1时, 才会走这个if
            f = ist->filter_frame;
            ret = av_frame_ref(f, decoded_frame);//引用解码帧到过滤器帧
            if (ret < 0)
                break;
        } else
            f = decoded_frame;//推流正常走这里

        // 1.2 主要工作是 配置FilterGraph 和 将解码后的一帧送去过滤器.
        ret = ifilter_send_frame(ist->filters[i], f);
        if (ret == AVERROR_EOF)
            ret = 0; /* ignore */
        if (ret < 0) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_ERROR,
                   "Failed to inject frame into filter network: %s\n", av_err2str_ex(str, ret));
            break;
        }
    }
    return ret;
}

/**
 * @brief 解码pkt,得到一帧解码帧,然后送去过滤器过滤.
 * @param ist 输入流
 * @param pkt 将要解码的pkt
 * @param got_output =1:解码一帧成功; =0:解码一帧失败,也可能遇到eagain,使用返回值判断即可.
 * @param decode_failed 标记decode()调用是否失败,=1表示解码失败.
 * @return 成功-0 失败-负数
 */
int FFmpegMedia::decode_audio(InputStream *ist, AVPacket *pkt, int *got_output,
                        int *decode_failed)
{
    AVFrame *decoded_frame;
    AVCodecContext *avctx = ist->dec_ctx;
    int ret, err = 0;
    AVRational decoded_frame_tb;

    // 1. 给解码帧开辟内存
    if (!ist->decoded_frame && !(ist->decoded_frame = av_frame_alloc()))
        return AVERROR(ENOMEM);
    if (!ist->filter_frame && !(ist->filter_frame = av_frame_alloc()))
        return AVERROR(ENOMEM);
    decoded_frame = ist->decoded_frame;

    // 2. 解码
    update_benchmark(NULL);
    ret = decode(avctx, decoded_frame, got_output, pkt);
    update_benchmark("decode_audio %d.%d", ist->file_index, ist->st->index);
    // 3. 返回值相关处理
    // 3.1 解码失败错误记录
    if (ret < 0)
        *decode_failed = 1;

    // 3.2 解码成功但采样率非法,将ret重新标记为解码失败
    if (ret >= 0 && avctx->sample_rate <= 0) {
        av_log(avctx, AV_LOG_ERROR, "Sample rate %d invalid\n", avctx->sample_rate);
        ret = AVERROR_INVALIDDATA;
    }

    // 3.3 check_decode_result处理.该函数里面若用户没指定-xerror选项,内部不会做太多处理,可以忽略它
    if (ret != AVERROR_EOF){
        check_decode_result(ist, got_output, ret);
        /*ret = check_decode_result(ist, got_output, ret);
        if(ret < 0){// 这个函数不处理返回值感觉更好
            return -1;
        }*/
    }

    // 3.4 eagain/eof/或者真正的错误(没指定-xerror时),直接返回
    if (!*got_output || ret < 0)
        return ret;

    ist->samples_decoded += decoded_frame->nb_samples;//统计已经解码的音频样本数
    ist->frames_decoded++;//统计已经解码的音频数

    // 4. 使用样本数+采样率预测next_pts、next_dts
    /* increment next_dts to use for the case where the input stream does not
       have timestamps or there are multiple frames in the packet */
    // (增加next_dts以用于输入流没有时间戳或包中有多个帧的情况)
    ist->next_pts += ((int64_t)AV_TIME_BASE * decoded_frame->nb_samples) /
                     avctx->sample_rate;// 例如样本数是1024,采样率是44.1k,那么该帧时长是0.023s.乘以AV_TIME_BASE是转成微秒
    ist->next_dts += ((int64_t)AV_TIME_BASE * decoded_frame->nb_samples) /
                     avctx->sample_rate;

    // 5. 获取解码帧的pts和tb
    if (decoded_frame->pts != AV_NOPTS_VALUE) {// 解码帧pts有效
        decoded_frame_tb   = ist->st->time_base;
    } else if (pkt && pkt->pts != AV_NOPTS_VALUE) {// 解码帧pts无效优先参考pkt
        decoded_frame->pts = pkt->pts;
        decoded_frame_tb   = ist->st->time_base;
    }else {// 否则参考流的dts
        decoded_frame->pts = ist->dts;
        decoded_frame_tb   = AV_TIME_BASE_Q;
    }

    // 6. 将解码帧pts的单位转成采样率的单位
    /* av_rescale_delta():调整时间戳的大小，同时保留已知的持续时间.
     *
     * 此函数被设计为每个音频包调用，以将输入时间戳扩展到不同的时间基数.
     * 与简单的av_rescale_q()调用相比，该函数对于可能不一致的帧持续时间具有健壮性.
     *
     * 'last'形参是一个状态变量，必须为同一流的所有后续调用保留.
     * 对于第一次调用，' *last '应该初始化为#AV_NOPTS_VALUE.
     *
     * 参1: 输入时间基
     * 参2: 输入时间戳
     * 参3: duration时间基;这通常比' in_tb '和' out_tb '粒度更细(更大).
     * 参4: 到下一次调用此函数的持续时间(即当前包/帧的持续时间)
     * 参5: 指向用' fs_tb '表示的时间戳的指针，充当状态变量
     * 参6: 输出时间基
     * 返回值: 时间戳用' out_tb '表示
     * @note: 在这个函数的上下文中，“duration”以样本为单位，而不是以秒为单位. */
    if (decoded_frame->pts != AV_NOPTS_VALUE)
        decoded_frame->pts = av_rescale_delta(decoded_frame_tb, decoded_frame->pts,
                                              (AVRational){1, avctx->sample_rate}, decoded_frame->nb_samples, &ist->filter_in_rescale_delta_last,
                                              (AVRational){1, avctx->sample_rate});

#ifdef TYYCODE_TIMESTAMP_DECODE
    static int tyycode = 1;
    if(tyycode == 1){
        mydebug(NULL, AV_LOG_INFO, "audio decode first frame success...\n");
        tyycode = 0;
    }
#endif

    // 7. 将解码帧发送到过滤器处理
    ist->nb_samples = decoded_frame->nb_samples;// 保存最近一次解码帧的样本数.
    err = send_frame_to_filters(ist, decoded_frame);

    av_frame_unref(ist->filter_frame);// 这里只是解引用filter_frame、decoded_frame,并未释放内存
    av_frame_unref(decoded_frame);
    return err < 0 ? err : ret;
}

/**
 * @brief 解码pkt,得到一帧解码帧,然后送去过滤器过滤.
 * @param ist 输入流
 * @param pkt 将要解码的pkt
 * @param got_output =1:解码一帧成功; =0:解码一帧失败,也可能遇到eagain,使用返回值判断即可.
 * @param duration_pts 传出参数,由解码后的帧成员pkt_duration得到,单位是AVStream->time_base units.
 * @param eof =0:未遇到eof; =1:eof到来
 * @param decode_failed 标记decode()调用是否失败,=1表示解码失败.
 * @return 成功-0 失败-负数
 */
int FFmpegMedia::decode_video(InputStream *ist, AVPacket *pkt, int *got_output, int64_t *duration_pts, int eof,
                        int *decode_failed)
{
    AVFrame *decoded_frame;
    int i, ret = 0, err = 0;
    int64_t best_effort_timestamp;
    int64_t dts = AV_NOPTS_VALUE;
    AVPacket avpkt;

    // With fate-indeo3-2, we're getting 0-sized packets before EOF for some
    // reason. This seems like a semi-critical bug. Don't trigger EOF, and
    // skip the packet.
    //(使用fate-indeo3-2，我们在EOF之前得到了0大小的数据包。这似乎是一个半关键的bug。不要触发EOF，并跳过包)
    if (!eof && pkt && pkt->size == 0)
        return 0;

    // 1. 给解码帧开辟内存
    if (!ist->decoded_frame && !(ist->decoded_frame = av_frame_alloc()))
        return AVERROR(ENOMEM);
    if (!ist->filter_frame && !(ist->filter_frame = av_frame_alloc()))
        return AVERROR(ENOMEM);
    decoded_frame = ist->decoded_frame;

    // 2. 重置pkt的dts
    if (ist->dts != AV_NOPTS_VALUE)
        dts = av_rescale_q(ist->dts, AV_TIME_BASE_Q, ist->st->time_base);
    if (pkt) {
        avpkt = *pkt;
        //这里看到pkt->dts的值是被重置的
        avpkt.dts = dts; // ffmpeg.c probably shouldn't do this(FFmpeg.c可能不应该这样做)
    }

    // The old code used to set dts on the drain packet, which does not work
    // with the new API anymore.(旧的代码用于设置dts的漏包，这与新的API不再工作)
    // 3. 若是eof时,给dts_buffer数组增加一个元素.一般该数组只有一个元素,就是末尾时的dts
    if (eof) {
        void *cnew = av_realloc_array(ist->dts_buffer, ist->nb_dts_buffer + 1, sizeof(ist->dts_buffer[0]));
        if (!cnew)
            return AVERROR(ENOMEM);
        ist->dts_buffer = (int64_t *)cnew;
        ist->dts_buffer[ist->nb_dts_buffer++] = dts;
    }

    // 4. 开始解码.
    update_benchmark(NULL);
    ret = decode(ist->dec_ctx, decoded_frame, got_output, pkt ? &avpkt : NULL);
    update_benchmark("decode_video %d.%d", ist->file_index, ist->st->index);
    if (ret < 0)
        *decode_failed = 1;

    // The following line may be required in some cases where there is no parser
    // or the parser does not has_b_frames correctly(在没有解析器或解析器没有正确地has_b_frames的情况下，可能需要使用下面这一行)
    // 5. 流参数的视频延迟帧数 < 解码器中帧重排序缓冲区的大小.即解码器的延迟比解复用延迟大的处理,可以理解为解码的速度跟不上解复用的速度.
    if (ist->st->codecpar->video_delay < ist->dec_ctx->has_b_frames) {
        //264解码器会使用has_b_frames给video_delay赋值,其余不做处理
        if (ist->dec_ctx->codec_id == AV_CODEC_ID_H264) {
            ist->st->codecpar->video_delay = ist->dec_ctx->has_b_frames;
        } else
            av_log(ist->dec_ctx, AV_LOG_WARNING,
                   "video_delay is larger in decoder than demuxer %d > %d.\n"
                   "If you want to help, upload a sample "
                   "of this file to ftp://upload.ffmpeg.org/incoming/ "
                   "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)\n",
                   ist->dec_ctx->has_b_frames,
                   ist->st->codecpar->video_delay);
    }

    // 6. 返回值相关处理
    // 6.1 check_decode_result处理.该函数里面若用户没指定-xerror选项,内部不会做太多处理,可以忽略它
    if (ret != AVERROR_EOF)
        check_decode_result(ist, got_output, ret);// 该函数不处理返回值感觉更好

    // 6.2 成功拿到解码一帧的判断,解码帧与解码器的参数是否一致
    if (*got_output && ret >= 0) {
        if (ist->dec_ctx->width  != decoded_frame->width ||
            ist->dec_ctx->height != decoded_frame->height ||
            ist->dec_ctx->pix_fmt != decoded_frame->format) {
            av_log(NULL, AV_LOG_DEBUG, "Frame parameters mismatch context %d,%d,%d != %d,%d,%d\n",
                decoded_frame->width,
                decoded_frame->height,
                decoded_frame->format,
                ist->dec_ctx->width,
                ist->dec_ctx->height,
                ist->dec_ctx->pix_fmt);
        }
    }

    // 6.3. eagain/eof/或者真正的错误(没指定-xerror时),直接返回
    if (!*got_output || ret < 0)
        return ret;

    if(ist->top_field_first>=0)
        decoded_frame->top_field_first = ist->top_field_first;

    ist->frames_decoded++;//统计解码器已经解码的帧数

    // 7. 硬件检索数据回调.一般为空.暂不深入研究
    if (ist->hwaccel_retrieve_data && decoded_frame->format == ist->hwaccel_pix_fmt) {
        err = ist->hwaccel_retrieve_data(ist->dec_ctx, decoded_frame);
        if (err < 0)
            goto fail;
    }
    ist->hwaccel_retrieved_pix_fmt = (AVPixelFormat)decoded_frame->format;

    // 8. best_effort_timestamp和pts的相关处理
    // 8.1 默认从解码后的一帧获取best_effort_timestamp(ffplay是走这种方式处理解码后的pts)
    best_effort_timestamp= decoded_frame->best_effort_timestamp;// 使用各种启发式算法估计帧时间戳，单位为流的时基。
    *duration_pts = decoded_frame->pkt_duration;// 传出参数,保存该帧的显示时长

    // 8.2 若输入文件指定了-r选项,则从cfr_next_pts获取best_effort_timestamp
    if (ist->framerate.num)// 输入文件的-r 25选项,注与输出文件的-r选项是不一样的.
        best_effort_timestamp = ist->cfr_next_pts++;// cfr_next_pts默认值是0

    // 8.3 若遇到eof 且 上面两步都没拿到值 且 dts_buffer有dts,则从dts_buffer数组获取best_effort_timestamp.
    // 遇到eof 且 best_effort_timestamp没有值 且eof时在dts_buffer数组存有时间戳, 则取该数组首个元素给其赋值.
    if (eof && best_effort_timestamp == AV_NOPTS_VALUE && ist->nb_dts_buffer > 0) {
        best_effort_timestamp = ist->dts_buffer[0];

        // 将数组元素往前移,覆盖首个元素.
        for (i = 0; i < ist->nb_dts_buffer - 1; i++)
            ist->dts_buffer[i] = ist->dts_buffer[i + 1];
        ist->nb_dts_buffer--;
    }

    // 8.4 保存decoded_frame->pts, ist->next_pts 以及 ist->pts.
    if(best_effort_timestamp != AV_NOPTS_VALUE) {
        // 将best_effort_timestamp赋值给decoded_frame->pts,并转单位后赋值给ts变量
        int64_t ts = av_rescale_q(decoded_frame->pts = best_effort_timestamp, ist->st->time_base, AV_TIME_BASE_Q);

        if (ts != AV_NOPTS_VALUE)
            ist->next_pts = ist->pts = ts;// 这里next_pts与pts都保存当前解码帧的pts. 会在后续加上duration来预测下一帧的pts.
    }

    // debug解码后的相关时间戳
    if (debug_ts) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_log(NULL, AV_LOG_INFO, "decoder -> ist_index:%d type:video "
               "frame_pts:%s frame_pts_time:%s best_effort_ts:%" PRId64" best_effort_ts_time:%s keyframe:%d frame_type:%d time_base:%d/%d\n",
               ist->st->index, av_ts2str_ex(str, decoded_frame->pts),
               av_ts2timestr_ex(str, decoded_frame->pts, &ist->st->time_base),
               best_effort_timestamp,
               av_ts2timestr_ex(str, best_effort_timestamp, &ist->st->time_base),
               decoded_frame->key_frame, decoded_frame->pict_type,
               ist->st->time_base.num, ist->st->time_base.den);
    }

    if (ist->st->sample_aspect_ratio.num)// 样本比例.正常不会进来
        decoded_frame->sample_aspect_ratio = ist->st->sample_aspect_ratio;

#ifdef TYYCODE_TIMESTAMP_DECODE
    static int tyycode = 1;
    if(tyycode == 1){
        mydebug(NULL, AV_LOG_INFO, "video decode first frame success...\n");
        tyycode = 0;
    }
#endif

    // 9. 发送视频解码帧到输入过滤器(视频的是: buffer).
    err = send_frame_to_filters(ist, decoded_frame);

fail:
    av_frame_unref(ist->filter_frame);//这里只是解引用filter_frame、decoded_frame,并未释放内存
    av_frame_unref(decoded_frame);
    return err < 0 ? err : ret;
}

void FFmpegMedia::sub2video_flush(InputStream *ist)
{
    int i;
    int ret;

    if (ist->sub2video.end_pts < INT64_MAX)
        sub2video_update(ist, NULL);
    for (i = 0; i < ist->nb_filters; i++) {
        ret = av_buffersrc_add_frame(ist->filters[i]->filter, NULL);
        if (ret != AVERROR_EOF && ret < 0)
            av_log(NULL, AV_LOG_WARNING, "Flush the frame error.\n");
    }
}

/*
 * Check whether a packet from ist should be written into ost at this time
 * (检查来自ist的数据包此时是否应该被写入ost)
 */
/**
 * @brief 检查来自ist的数据包此时是否应该被写入ost
 * @param ist 输入流
 * @param ost 输出流
 * @return 1-此时可以输出 0-此时不可以输出
 */
int FFmpegMedia::check_output_constraints(InputStream *ist, OutputStream *ost)
{
    OutputFile *of = output_files[ost->file_index];
    int ist_index  = input_files[ist->file_index]->ist_index + ist->st->index;//获取输入流下标

    // 1. 若输出流保存的输入流下标 与 输入流保存的不一致,返回0
    if (ost->source_index != ist_index)
        return 0;

    // 2. 输出流完成
    if (ost->finished)
        return 0;

    // 3. 当前输入流的pts 还没到达 输出流指定的开始时间
    if (of->start_time != AV_NOPTS_VALUE && ist->pts < of->start_time)
        return 0;

    return 1;
}

void FFmpegMedia::do_subtitle_out(OutputFile *of,
                            OutputStream *ost,
                            AVSubtitle *sub)
{
    int subtitle_out_max_size = 1024 * 1024;
    int subtitle_out_size, nb, i;
    AVCodecContext *enc;
    AVPacket pkt;
    int64_t pts;

    if (sub->pts == AV_NOPTS_VALUE) {
        av_log(NULL, AV_LOG_ERROR, "Subtitle packets must have a pts\n");
        if (exit_on_error)
            exit_program(1);
        return;
    }

    enc = ost->enc_ctx;

    if (!subtitle_out) {
        subtitle_out = (uint8_t *)av_malloc(subtitle_out_max_size);
        if (!subtitle_out) {
            av_log(NULL, AV_LOG_FATAL, "Failed to allocate subtitle_out\n");
            exit_program(1);
        }
    }

    /* Note: DVB subtitle need one packet to draw them and one other
       packet to clear them */
    /* XXX: signal it in the codec context ? */
    if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE)
        nb = 2;
    else
        nb = 1;

    /* shift timestamp to honor -ss and make check_recording_time() work with -t */
    pts = sub->pts;
    if (output_files[ost->file_index]->start_time != AV_NOPTS_VALUE)
        pts -= output_files[ost->file_index]->start_time;
    for (i = 0; i < nb; i++) {
        unsigned save_num_rects = sub->num_rects;

        ost->sync_opts = av_rescale_q(pts, AV_TIME_BASE_Q, enc->time_base);
        if (!check_recording_time(ost))
            return;

        sub->pts = pts;
        // start_display_time is required to be 0
        sub->pts               += av_rescale_q(sub->start_display_time, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q);
        sub->end_display_time  -= sub->start_display_time;
        sub->start_display_time = 0;
        if (i == 1)
            sub->num_rects = 0;

        ost->frames_encoded++;

        subtitle_out_size = avcodec_encode_subtitle(enc, subtitle_out,
                                                    subtitle_out_max_size, sub);
        if (i == 1)
            sub->num_rects = save_num_rects;
        if (subtitle_out_size < 0) {
            av_log(NULL, AV_LOG_FATAL, "Subtitle encoding failed\n");
            exit_program(1);
        }

        av_init_packet(&pkt);
        pkt.data = subtitle_out;
        pkt.size = subtitle_out_size;
        pkt.pts  = av_rescale_q(sub->pts, AV_TIME_BASE_Q, ost->mux_timebase);
        pkt.duration = av_rescale_q(sub->end_display_time, (AVRational){ 1, 1000 }, ost->mux_timebase);
        if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE) {
            /* XXX: the pts correction is handled here. Maybe handling
               it in the codec would be better */
            if (i == 0)
                pkt.pts += av_rescale_q(sub->start_display_time, (AVRational){ 1, 1000 }, ost->mux_timebase);
            else
                pkt.pts += av_rescale_q(sub->end_display_time, (AVRational){ 1, 1000 }, ost->mux_timebase);
        }
        pkt.dts = pkt.pts;
        output_packet(of, &pkt, ost, 0);
    }
}

int FFmpegMedia::transcode_subtitles(InputStream *ist, AVPacket *pkt, int *got_output,
                               int *decode_failed)
{
    AVSubtitle subtitle;
    int free_sub = 1;
    int i, ret = avcodec_decode_subtitle2(ist->dec_ctx,
                                          &subtitle, got_output, pkt);

    check_decode_result(NULL, got_output, ret);

    if (ret < 0 || !*got_output) {
        *decode_failed = 1;
        if (!pkt->size)
            sub2video_flush(ist);
        return ret;
    }

    if (ist->fix_sub_duration) {
        int end = 1;
        if (ist->prev_sub.got_output) {
            end = av_rescale(subtitle.pts - ist->prev_sub.subtitle.pts,
                             1000, AV_TIME_BASE);
            if (end < ist->prev_sub.subtitle.end_display_time) {
                av_log(ist->dec_ctx, AV_LOG_DEBUG,
                       "Subtitle duration reduced from %" PRId32" to %d%s\n",
                       ist->prev_sub.subtitle.end_display_time, end,
                       end <= 0 ? ", dropping it" : "");
                ist->prev_sub.subtitle.end_display_time = end;
            }
        }
        FFSWAP(int,        *got_output, ist->prev_sub.got_output);
        FFSWAP(int,        ret,         ist->prev_sub.ret);
        FFSWAP(AVSubtitle, subtitle,    ist->prev_sub.subtitle);
        if (end <= 0)
            goto out;
    }

    if (!*got_output)
        return ret;

    if (ist->sub2video.frame) {
        sub2video_update(ist, &subtitle);
    } else if (ist->nb_filters) {
        if (!ist->sub2video.sub_queue)
            ist->sub2video.sub_queue = av_fifo_alloc(8 * sizeof(AVSubtitle));
        if (!ist->sub2video.sub_queue)
            exit_program(1);
        if (!av_fifo_space(ist->sub2video.sub_queue)) {
            ret = av_fifo_realloc2(ist->sub2video.sub_queue, 2 * av_fifo_size(ist->sub2video.sub_queue));
            if (ret < 0)
                exit_program(1);
        }
        av_fifo_generic_write(ist->sub2video.sub_queue, &subtitle, sizeof(subtitle), NULL);
        free_sub = 0;
    }

    if (!subtitle.num_rects)
        goto out;

    ist->frames_decoded++;

    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost = output_streams[i];

        if (!check_output_constraints(ist, ost) || !ost->encoding_needed
            || ost->enc->type != AVMEDIA_TYPE_SUBTITLE)
            continue;

        do_subtitle_out(output_files[ost->file_index], ost, &subtitle);
    }

out:
    if (free_sub)
        avsubtitle_free(&subtitle);
    return ret;
}

/**
 * @brief 从参数结构体拷贝相关参数到InputFilter.
 */
void FFmpegMedia::ifilter_parameters_from_codecpar(InputFilter *ifilter, AVCodecParameters *par)
{
    // We never got any input. Set a fake format, which will
    // come from libavformat.
    ifilter->format                 = par->format;
    ifilter->sample_rate            = par->sample_rate;
    ifilter->channels               = par->channels;
    ifilter->channel_layout         = par->channel_layout;
    ifilter->width                  = par->width;
    ifilter->height                 = par->height;
    ifilter->sample_aspect_ratio    = par->sample_aspect_ratio;
}

/**
 * @brief 标记输入过滤器eof=1,并关闭输入过滤器
 * @param ifilter 输入过滤器
 * @param pts 当前的pts?
 * @return 成功-0 失败-负数
 */
int FFmpegMedia::ifilter_send_eof(InputFilter *ifilter, int64_t pts)
{
    int ret;

    // 1. 标记过滤器完成
    ifilter->eof = 1;

    // 2. 关闭过滤器源buffer(abuffer)
    if (ifilter->filter) {
        /* av_buffersrc_close(): EOF结束后关闭缓冲源.
         * 这类似于将NULL传递给av_buffersrc_add_frame_flags(),
         * 除了它接受EOF的时间戳，例如最后一帧结束的时间戳. */
        ret = av_buffersrc_close(ifilter->filter, pts, AV_BUFFERSRC_FLAG_PUSH);
        if (ret < 0)
            return ret;
    } else {
        // the filtergraph was never configured(从未配置过滤器)
        if (ifilter->format < 0)
            ifilter_parameters_from_codecpar(ifilter, ifilter->ist->st->codecpar);
        if (ifilter->format < 0 && (ifilter->type == AVMEDIA_TYPE_AUDIO || ifilter->type == AVMEDIA_TYPE_VIDEO)) {
            av_log(NULL, AV_LOG_ERROR, "Cannot determine format of input stream %d:%d after EOF\n", ifilter->ist->file_index, ifilter->ist->st->index);
            return AVERROR_INVALIDDATA;
        }
    }

    return 0;
}

/**
 * @brief 将输入流的每一路输入过滤器关闭.
 * @param ist 输入流
 * @return 成功-0 失败-负数
 */
int FFmpegMedia::send_filter_eof(InputStream *ist)
{
    int i, ret;
    /* TODO keep pts also in stream time base to avoid converting back(保持PTS也在流时间基中，以避免转换回) */
    int64_t pts = av_rescale_q_rnd(ist->pts, AV_TIME_BASE_Q, ist->st->time_base,
                                   (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));//转为流的时基单位

    // 1. 将输入流的每一路输入过滤器关闭.
    for (i = 0; i < ist->nb_filters; i++) {
        ret = ifilter_send_eof(ist->filters[i], pts);
        if (ret < 0)
            return ret;
    }
    return 0;
}

/**
 * @brief 后续详细分析.比转码会简单很多.
 */
int FFmpegMedia::do_streamcopy(InputStream *ist, OutputStream *ost, const AVPacket *pkt)
{
    OutputFile *of = output_files[ost->file_index];
    InputFile   *f = input_files [ist->file_index];
    int64_t start_time = (of->start_time == AV_NOPTS_VALUE) ? 0 : of->start_time;
    int64_t ost_tb_start_time = av_rescale_q(start_time, AV_TIME_BASE_Q, ost->mux_timebase);
    AVPacket opkt = { 0 };
    int ret;

    av_init_packet(&opkt);

    // 1. eof时,flush输出比特流过滤器.(后续研究)
    // EOF: flush output bitstream filters.(冲洗输出比特流过滤器)
    if (!pkt) {
        output_packet(of, &opkt, ost, 1);
        //ret = output_packet(of, &opkt, ost, 1);// 应该主要与比特流有关,比特流的可以不处理返回值
        return 0;// 暂时认为正常
    }

    // 2. (帧数=0 且 是非关键帧) 且 没指定复制最初的非关键帧
    if ((!ost->frame_number && !(pkt->flags & AV_PKT_FLAG_KEY)) &&
        !ost->copy_initial_nonkeyframes)
        return 0;// 暂时认为正常

    // 3. 帧数=0 且 没指定-copypriorss选项
    if (!ost->frame_number && !ost->copy_prior_start) {
        int64_t comp_start = start_time;
        if (copy_ts && f->start_time != AV_NOPTS_VALUE)
            comp_start = FFMAX(start_time, f->start_time + f->ts_offset);
        if (pkt->pts == AV_NOPTS_VALUE ?
            ist->pts < comp_start :
            pkt->pts < av_rescale_q(comp_start, AV_TIME_BASE_Q, ist->st->time_base))
            return 0;// 暂时认为正常
    }

    // 4. 当前时间 >= 输出文件要录像的时间,录像完毕
    if (of->recording_time != INT64_MAX &&
        ist->pts >= of->recording_time + start_time) {
        close_output_stream(ost);
        return 0;// 暂时认为正常
    }

    // 5. 当前时间 >= 输入文件要录像的时间,录像完毕.
    if (f->recording_time != INT64_MAX) {
        start_time = f->ctx->start_time;//输入流的开始时间
        if (f->start_time != AV_NOPTS_VALUE && copy_ts)
            start_time += f->start_time;
        if (ist->pts >= f->recording_time + start_time) {
            close_output_stream(ost);
            return 0;// 暂时认为正常
        }
    }

    // 6. copy下,当是视频时,sync_opts代表输出帧个数?
    /* force the input stream PTS */
    if (ost->enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
        ost->sync_opts++;

    // 7. 减去start_time是为啥?留个疑问
    if (pkt->pts != AV_NOPTS_VALUE)
        opkt.pts = av_rescale_q(pkt->pts, ist->st->time_base, ost->mux_timebase) - ost_tb_start_time;
    else
        opkt.pts = AV_NOPTS_VALUE;

    if (pkt->dts == AV_NOPTS_VALUE)
        opkt.dts = av_rescale_q(ist->dts, AV_TIME_BASE_Q, ost->mux_timebase);
    else
        opkt.dts = av_rescale_q(pkt->dts, ist->st->time_base, ost->mux_timebase);
    opkt.dts -= ost_tb_start_time;

    // 8. 音频类型 且 pkt->dts有效,会重置opkt.dts, opkt.pts
    if (ost->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && pkt->dts != AV_NOPTS_VALUE) {
        int duration = av_get_audio_frame_duration(ist->dec_ctx, pkt->size);
        if(!duration)
            duration = ist->dec_ctx->frame_size;
        opkt.dts = opkt.pts = av_rescale_delta(ist->st->time_base, pkt->dts,
                                               (AVRational){1, ist->dec_ctx->sample_rate}, duration, &ist->filter_in_rescale_delta_last,
                                               ost->mux_timebase) - ost_tb_start_time;
    }

    // 9. 时长单位转换
    opkt.duration = av_rescale_q(pkt->duration, ist->st->time_base, ost->mux_timebase);

    opkt.flags    = pkt->flags;

    // 10. buf引用和data,size的赋值处理
    if (pkt->buf) {
        opkt.buf = av_buffer_ref(pkt->buf);
        if (!opkt.buf){
            //exit_program(1);
            return -1;
        }
    }
    opkt.data = pkt->data;
    opkt.size = pkt->size;

    // 11. 边数据拷贝
    av_copy_packet_side_data(&opkt, pkt);

    // 12. 写包
    //output_packet(of, &opkt, ost, 0);
    return output_packet(of, &opkt, ost, 0);
}

/* pkt = NULL means EOF (needed to flush decoder buffers) */
/**
 * @brief 处理输入pkt.会调用解码函数进行处理.
 * @param ist 输入流
 * @param pkt 为空时,代表eof到来,需要刷空包冲刷解码器
 * @param no_eof eof到来,是由它决定是否发送eof给输入过滤器,0-发送,1-不发送.
 *
 * @return 0-eof到来; 1-eof未到来
 */
int FFmpegMedia::process_input_packet(InputStream *ist, const AVPacket *pkt, int no_eof)
{
    int ret = 0, i;
    int repeating = 0;
    int eof_reached = 0;

    AVPacket avpkt;
    // 1. 判断是否是第一次进来的时间戳，给InputStream的dts/pts赋值.
    //转码时,ist->dts参考has_b_frames; 不转码时优先参考pkt->pts,pkt->pts不存在则参考has_b_frames
    if (!ist->saw_first_ts) {
        /* 求出has_b_frames缓存大小占用的时间.例如has_b_frames=2,avg_frame_rate={24000,1001},
         * 乘以AV_TIME_BASE是转成微秒,2*1000000/(24000/1001)=83416μs.
         * has_b_frames: ffmpeg的注释是"解码器中帧重排序缓冲区的大小".
        */
        ist->dts = ist->st->avg_frame_rate.num ? - ist->dec_ctx->has_b_frames * AV_TIME_BASE / av_q2d(ist->st->avg_frame_rate) : 0;
        ist->pts = 0;
        //pkt存在 且 pkt->pts存在 且 不转码时,ist->dts由pkt->pts赋值,
        if (pkt && pkt->pts != AV_NOPTS_VALUE && !ist->decoding_needed) {
            ist->dts += av_rescale_q(pkt->pts, ist->st->time_base, AV_TIME_BASE_Q);
            ist->pts = ist->dts; //unused but better to set it to a value thats not totally wrong
        }
        ist->saw_first_ts = 1;
    }

    // 2. 当next_dts/next_pts无效,参考当前的ist->dts/ist->pts.
    // 一般是首次或者计算next_dts/next_pts无效时会进来.
    if (ist->next_dts == AV_NOPTS_VALUE)
        ist->next_dts = ist->dts;
    if (ist->next_pts == AV_NOPTS_VALUE)
        ist->next_pts = ist->pts;

    // 3. 按照是否空包来处理临时变量avpkt。
    if (!pkt) {
        /* EOF handling */
        av_init_packet(&avpkt);
        avpkt.data = NULL;
        avpkt.size = 0;
    } else {
        avpkt = *pkt;
    }

    // 4. 不是flush解码器时(pkt->dts有效时), 对ist->next_dts/ist->dts, ist->next_pts/ist->pts的处理.
    if (pkt && pkt->dts != AV_NOPTS_VALUE) {
        ist->next_dts = ist->dts = av_rescale_q(pkt->dts, ist->st->time_base, AV_TIME_BASE_Q);
        // 非视频流 或者 视频流时不转码,ist->next_pts,ist->pts由ist->dts赋值;
        // 输入流是视频流且转码,不会进入if条件
        if (ist->dec_ctx->codec_type != AVMEDIA_TYPE_VIDEO || !ist->decoding_needed)
            ist->next_pts = ist->pts = ist->dts;
    }

    // while we have more to decode or while the decoder did output something on EOF
    //(当我们有更多的东西要解码或者当解码器在EOF上输出一些东西的时候)
    // 5. 输入流要解码的流程.
    while (ist->decoding_needed) {
        int64_t duration_dts = 0;
        int64_t duration_pts = 0;
        int got_output = 0;
        int decode_failed = 0;

        // 这里的作用是为了while的下一次循环时,ist->pts,ist->dts的值可以更新为下一帧的值.
        ist->pts = ist->next_pts;
        ist->dts = ist->next_dts;

#ifdef TYYCODE_TIMESTAMP_DECODE
        // 查看解码前的详细的时间戳.主要看ist->pts/ist->dts以及ist->next_pts/ist->next_dts的变化.
        // 因为pkt.pts/pkt.dts基本是没变的.
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        mydebug(NULL, AV_LOG_INFO, "decoder before, ist_index:%d type:%s "
               "dts:%s dts_time:%s "
               "pts:%s pts_time:%s "
               "next_dts:%s next_dts_time:%s "
               "next_pts:%s next_pts_time:%s "
               "pkt_pts:%s pkt_pts_time:%s "
               "pkt_dts:%s pkt_dts_time:%s\n",
               pkt != NULL ? input_files[ist->file_index]->ist_index + pkt->stream_index : -1, av_get_media_type_string(ist->dec_ctx->codec_type),
               FFmpegMedia::av_ts2str_ex(str, ist->dts), FFmpegMedia::av_ts2timestr_ex(str, ist->dts, AV_TIME_BASE_Q),
               FFmpegMedia::av_ts2str_ex(str, ist->pts), FFmpegMedia::av_ts2timestr_ex(str, ist->pts, AV_TIME_BASE_Q),
               FFmpegMedia::av_ts2str_ex(str, ist->next_dts), FFmpegMedia::av_ts2timestr_ex(str, ist->next_dts, AV_TIME_BASE_Q),
               FFmpegMedia::av_ts2str_ex(str, ist->next_pts), FFmpegMedia::av_ts2timestr_ex(str, ist->next_pts, AV_TIME_BASE_Q),
               pkt != NULL ? av_ts2str_ex(str, pkt->pts) : "null", pkt != NULL ? av_ts2timestr_ex(str, pkt->pts, &ist->st->time_base) : "null",
               pkt != NULL ? av_ts2str_ex(str, pkt->dts) : "null", pkt != NULL ? av_ts2timestr_ex(str, pkt->dts, &ist->st->time_base) : "null");
#endif

        // 5.1 解码包
        switch (ist->dec_ctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            // 5.1.1 解码音频包
            ret = decode_audio    (ist, repeating ? NULL : &avpkt, &got_output,
                                   &decode_failed);
            break;
        case AVMEDIA_TYPE_VIDEO:
            // 5.1.2 解码视频包
            ret = decode_video    (ist, repeating ? NULL : &avpkt, &got_output, &duration_pts, !pkt,
                                   &decode_failed);
            // 预测next_dts
            if (!repeating || !pkt || got_output) {
                if (pkt && pkt->duration) {// 统计eof+正常解码一帧的dts,即pkt不为空的情况
                    duration_dts = av_rescale_q(pkt->duration, ist->st->time_base, AV_TIME_BASE_Q);
                } else if(ist->dec_ctx->framerate.num != 0 && ist->dec_ctx->framerate.den != 0) {// 统计pkt=NULL即刷空包时的dts
                    struct AVCodecParserContext *tyycode = av_stream_get_parser(ist->st);// 一般是null
                    // 这里实际思路就是: 使用帧率去获取dts.
                    // 把下面ticks和ticks_per_frame约去即可看出来.
                    int ticks= av_stream_get_parser(ist->st) ? av_stream_get_parser(ist->st)->repeat_pict+1 : ist->dec_ctx->ticks_per_frame;
                    duration_dts = ((int64_t)AV_TIME_BASE * /* 转成微秒 */
                                    ist->dec_ctx->framerate.den * ticks) /
                                    ist->dec_ctx->framerate.num / ist->dec_ctx->ticks_per_frame;
                                    // 这里是连续除以num和ticks_per_frame，优先级从左到右.例如(1000000 * 1 * 2) / 25 / 2 = 80000/2=40000.
                }

                // next_dts指向下一个dts,只要解码都会进来这里
                if(ist->dts != AV_NOPTS_VALUE && duration_dts) {
                    ist->next_dts += duration_dts;
                }else
                    ist->next_dts = AV_NOPTS_VALUE;
            }

            // 预测next_pts,只有成功解码一帧,才会进来
            if (got_output) {
                if (duration_pts > 0) {
                    ist->next_pts += av_rescale_q(duration_pts, ist->st->time_base, AV_TIME_BASE_Q);
                } else {
                    // 加负的duration_dts有什么意义?
                    ist->next_pts += duration_dts;
                }
            }
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            // 5.1.3 解码字幕包
            if (repeating)
                break;
            ret = transcode_subtitles(ist, &avpkt, &got_output, &decode_failed);
            if (!pkt && ret >= 0)
                ret = AVERROR_EOF;
            break;
        default:
            return -1;
        }//<== switch end ==>

        // 5.2 解码完成,标记并退出while
        if (ret == AVERROR_EOF) {
            eof_reached = 1;
            break;
        }

        // 5.3 失败处理
        if (ret < 0) {
            if (decode_failed) {//解码失败时会走这里
                char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_log(NULL, AV_LOG_ERROR, "Error while decoding stream #%d:%d: %s\n",
                       ist->file_index, ist->st->index, av_err2str_ex(str, ret));
            } else {//一般是发送解码帧到过滤器处理失败时会走这里
                av_log(NULL, AV_LOG_FATAL, "Error while processing the decoded "
                       "data for stream #%d:%d\n", ist->file_index, ist->st->index);
            }
            // 这里看到,当没指定-xerror选项,解码失败不会直接退出程序,只有decode_failed=0才会.
            if (!decode_failed /* ret小于0,但不是解码失败导致的小于0 */ || exit_on_error){
                //exit_program(1);
                return -1;
            }
            break;
        }

        // 5.4 成功解码一帧,标记输入流已经解码输出了.
        if (got_output)
            ist->got_output = 1;

        // 5.5 解码没报错但没有拿到解码帧,是eagain
        if (!got_output)
            break;

        // During draining, we might get multiple output frames in this loop.
        // ffmpeg.c does not drain the filter chain on configuration changes,
        // which means if we send multiple frames at once to the filters, and
        // one of those frames changes configuration, the buffered frames will
        // be lost. This can upset certain FATE tests.
        // Decode only 1 frame per call on EOF to appease these FATE tests.
        // The ideal solution would be to rewrite decoding to use the new
        // decoding API in a better way.
        /* 在引流过程中，我们可能会在这个循环中获得多个输出帧。ffmpeg.c不会在配置更改时耗尽过滤器链，
         * 这意味着如果我们一次向过滤器发送多个帧，而其中一个帧更改了配置，缓冲的帧将丢失。这可能会打乱某些FATE测试.
         * 在EOF上每次调用只解码1帧，以满足这些FATE测试。理想的解决方案是重写解码，以更好的方式使用新的解码API. */
        if (!pkt)
            break;

        repeating = 1;
    }//<== while (ist->decoding_needed) end ==>

    /* after flushing, send an EOF on all the filter inputs attached to the stream(冲洗后，对附加到流的所有过滤器输入发送EOF) */
    /* except when looping we need to flush but not to send an EOF(除非在循环时我们需要刷新但不发送EOF) */
    // 6. 刷空包 且 转码 且 eof到来 且 no_eof=0,那么发送eof给流对应的输入过滤器.
    // no_eof是关键,一般eof到来,是由它决定是否发送eof给过滤器.
    // 只有真正eof结束才会进来.若是eof但指定stream_loop循环,不会进来.
    if (!pkt && ist->decoding_needed && eof_reached && !no_eof) {
        int ret = send_filter_eof(ist);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "Error marking filters as finished\n");
            //exit_program(1);
            return -1;
        }
    }

    // 7. copy的流程.(后续详细分析,简单看了一下,不难)
    /* handle stream copy */
    if (!ist->decoding_needed && pkt) {
        ist->dts = ist->next_dts;

        // 预测各种媒体流的next_dts
        switch (ist->dec_ctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            av_assert1(pkt->duration >= 0);
            // 预测next_dts
            if (ist->dec_ctx->sample_rate) {
                ist->next_dts += ((int64_t)AV_TIME_BASE * ist->dec_ctx->frame_size) /
                                  ist->dec_ctx->sample_rate;//frame_size应该是样本数?
            } else {
                ist->next_dts += av_rescale_q(pkt->duration, ist->st->time_base, AV_TIME_BASE_Q);
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            // 若用户指定帧率,使用用户的帧率预测next_dts.
            if (ist->framerate.num) {
                // TODO: Remove work-around for c99-to-c89 issue 7
                AVRational time_base_q = AV_TIME_BASE_Q;
                int64_t next_dts = av_rescale_q(ist->next_dts, time_base_q, av_inv_q(ist->framerate));//next_dts转成帧率单位.
                // +1的意思就是预测下一帧的dts,因为帧率的间隔就是1.
                // 例如上面ist->next_dts=40000(40ms),单位是1000,000,帧率是{25,1},那么转成帧率单位后,next_dts就是1,那么+1在帧率单位中就是下一帧的dts.
                ist->next_dts = av_rescale_q(next_dts + 1, av_inv_q(ist->framerate), time_base_q);
            } else if (pkt->duration) {
                // 否则若pkt->duration存在则用其预测
                ist->next_dts += av_rescale_q(pkt->duration, ist->st->time_base, AV_TIME_BASE_Q);
            } else if(ist->dec_ctx->framerate.num != 0) {
                // 否则使用解码器上下文的帧率预测.
                // 这里实际思路就是: 使用帧率去获取dts.
                // 把下面ticks和ticks_per_frame去掉即可看出来.
                int ticks= av_stream_get_parser(ist->st) ? av_stream_get_parser(ist->st)->repeat_pict + 1 : ist->dec_ctx->ticks_per_frame;
                ist->next_dts += ((int64_t)AV_TIME_BASE *
                                  ist->dec_ctx->framerate.den * ticks) /
                                  ist->dec_ctx->framerate.num / ist->dec_ctx->ticks_per_frame;
            }
            break;
        }//<== switch end ==>

        ist->pts = ist->dts;// 可以看到copy时,pts由dts赋值,而dts由预测的next_dts得到.ffmpeg这里处理pts的方法我们可以进行参考
        ist->next_pts = ist->next_dts;
    }//<== if (!ist->decoding_needed && pkt) end ==>

    // 8. 判断是否进行copy 编码操作
    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost = output_streams[i];

        // 输入流不可以输出到输出流 或者 输出流需要编码的话,不会执行copy.
        if (!check_output_constraints(ist, ost) || ost->encoding_needed)
            continue;

        // 这里的copy可认为是相当于的编码操作
        ret = do_streamcopy(ist, ost, pkt);
        if(ret < 0){
            av_log(NULL, AV_LOG_FATAL, "do_streamcopy failed.\n");
            return ret;
        }
    }

    return !eof_reached;
}

// set duration to max(tmp, duration) in a proper time base and return duration's time_base
/**
 * @brief 比较不同单位下的两个时长,最终保存最大的时长,并返回最大时长的时基.
 *
 * @param tmp 时长1
 * @param duration 时长2,传出参数,用于保存最大的时长.
 * @param tmp_time_base tmp的时基单位
 * @param time_base duration的时基单位
 *
 * @return 返回最大时长的时基
 */
AVRational FFmpegMedia::duration_max(int64_t tmp, int64_t *duration, AVRational tmp_time_base,
                               AVRational time_base)
{
    int ret;

    // 1. 为空,直接将临时的时长和时基赋值后返回.
    if (!*duration) {
        *duration = tmp;
        return tmp_time_base;
    }

    // 2. 比较大小.
    ret = av_compare_ts(*duration, time_base, tmp, tmp_time_base);
    if (ret < 0) {
        *duration = tmp;
        return tmp_time_base;
    }

    return time_base;
}

/**
 * @brief seek到文件开始位置, 并获取该输入文件的duration,以及对应的时基.
 * @param ifile 输入文件
 * @param is 输入文件上下文
 * @return >=0-成功 负数-失败
 */
int FFmpegMedia::seek_to_start(InputFile *ifile, AVFormatContext *is)
{
    InputStream *ist;
    AVCodecContext *avctx;
    int i, ret, has_audio = 0;
    int64_t duration = 0;

    // 1. 该函数用于移动到指定时间戳的关键帧位置.
    /* 对比ffplay使用的avformat_seek_file(): 该函数是新版本的seek api,
     * 它用于移动到时间戳最近邻的位置(在min_ts与max_ts范围内),内部会调用av_seek_frame */
    ret = av_seek_frame(is, -1, is->start_time, 0);
    if (ret < 0)
        return ret;

    // 2. 判断是否有音频流 曾经解码成功过.
    for (i = 0; i < ifile->nb_streams; i++) {
        ist   = input_streams[ifile->ist_index + i];
        avctx = ist->dec_ctx;

        /* duration is the length of the last frame in a stream
         * when audio stream is present we don't care about
         * last video frame length because it's not defined exactly */
        //(持续时间是流中有音频流时的最后一帧的长度，我们不关心最后一个视频帧的长度，因为它没有确切的定义)
        // 音频流 且 最近一次音频解码样本数不为0
        if (avctx->codec_type == AVMEDIA_TYPE_AUDIO && ist->nb_samples)
            has_audio = 1;
    }

    // 3. 获取该输入文件中的duration,以及对应的时基
    for (i = 0; i < ifile->nb_streams; i++) {
        ist   = input_streams[ifile->ist_index + i];
        avctx = ist->dec_ctx;

        // 3.1 获取一帧的时长,有音频的则使用音频;只有视频则使用视频.
        if (has_audio) {
            if (avctx->codec_type == AVMEDIA_TYPE_AUDIO && ist->nb_samples) {
                // 音频以采样点数/采样率作为时长
                AVRational sample_rate = {1, avctx->sample_rate};

                duration = av_rescale_q(ist->nb_samples, sample_rate, ist->st->time_base);
            } else {
                continue;// 不是音频流则跳过,因为上面has_audio=1,所以当有音频流时,非音频流都会走这里
            }
        } else {
            // 视频以 1/帧率 作为时长
            if (ist->framerate.num) {
                //以用户指定的帧率作为时长单位,刻度是1,例如帧率是25,那么刻度1在25单位下就是0.04
                duration = av_rescale_q(1, av_inv_q(ist->framerate), ist->st->time_base);
            } else if (ist->st->avg_frame_rate.num) {
                duration = av_rescale_q(1, av_inv_q(ist->st->avg_frame_rate), ist->st->time_base);
            } else {
                duration = 1;
            }
        }

        // 下面的逻辑当存在音频流时,是以音频流的时长作为ifile的时长;
        // 只有视频流时,才会以视频流的时长作为ifile的时长;
        // 并且,当-stream_loop循环时,每次取该流的最长时长保存在ifile->duration

        // 为空先保存ifile->duration的时基
        if (!ifile->duration)
            ifile->time_base = ist->st->time_base;

        // 3.2 获取该流的总时长.
        /* the total duration of the stream, max_pts - min_pts is
         * the duration of the stream without the last frame.(流的总持续时间，max_pts - min_pts是没有最后一帧的流的持续时间) */
        // 1)ist->max_pts > ist->min_pts是验证是否正常.
        // 2)ist->max_pts - (uint64_t)ist->min_pts求出来的结果是不包含最后一帧的流的时长,
        // INT64_MAX - duration中的duration是一帧的时长,那么INT64_MAX - duration就是代表ist->max_pts - (uint64_t)ist->min_pts的最大值,
        // 所以ist->max_pts - (uint64_t)ist->min_pts < INT64_MAX - duration就是判断是否溢出.
        if (ist->max_pts > ist->min_pts && ist->max_pts - (uint64_t)ist->min_pts < INT64_MAX - duration)
            duration += ist->max_pts - ist->min_pts;// 得到该流的总时长

        // 3.3 以该流作为输入文件的时长,并保存它以及对应的时基
        // 当循环时,每次保存该流最长的duration, 作为输入文件中的时长
        ifile->time_base = duration_max(duration, &ifile->duration, ist->st->time_base,
                                        ifile->time_base);
    }

    // 4. 循环次数完成减1
    if (ifile->loop > 0)
        ifile->loop--;

    return ret;
}

/**
 * @brief 标记输出流完成编码和复用,若指定-shortest,所有输出流都会被标记.
 * @param ost 输出流
 */
void FFmpegMedia::finish_output_stream(OutputStream *ost)
{
    OutputFile *of = output_files[ost->file_index];
    int i;

    // 1. 标记编码和复用完成.
    ost->finished = (OSTFinished)(ENCODER_FINISHED | MUXER_FINISHED);

    // 2. 指定-shortest选项,还会把其它的输出流都标记完成.
    // 这里得出-shortest选项的意义:最短的流结束,其它的流也会结束.
    if (of->shortest) {
        for (i = 0; i < of->ctx->nb_streams; i++)
            output_streams[of->ost_index + i]->finished = (OSTFinished)(ENCODER_FINISHED | MUXER_FINISHED);
    }
}

/**
 * @brief 报告有新的流.简单看一下即可.
 * @param input_index 输入(文件)下标
 * @param pkt pkt
 */
void FFmpegMedia::report_new_stream(int input_index, AVPacket *pkt)
{
    InputFile *file = input_files[input_index];
    AVStream *st = file->ctx->streams[pkt->stream_index];
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};

    // 1. 包的流下标 < 流警告数,不管它.
    if (pkt->stream_index < file->nb_streams_warn)
        return;

    av_log(file->ctx, AV_LOG_WARNING,
           "New %s stream %d:%d at pos:%" PRId64" and DTS:%ss\n",
           av_get_media_type_string(st->codecpar->codec_type),
           input_index, pkt->stream_index,
           pkt->pos, av_ts2timestr_ex(str, pkt->dts, &st->time_base));
    // 2. 保存
    file->nb_streams_warn = pkt->stream_index + 1;
}

void FFmpegMedia::sub2video_heartbeat(InputStream *ist, int64_t pts)
{
    InputFile *infile = input_files[ist->file_index];
    int i, j, nb_reqs;
    int64_t pts2;

    /* When a frame is read from a file, examine all sub2video streams in
       the same file and send the sub2video frame again. Otherwise, decoded
       video frames could be accumulating in the filter graph while a filter
       (possibly overlay) is desperately waiting for a subtitle frame. */
    for (i = 0; i < infile->nb_streams; i++) {
        InputStream *ist2 = input_streams[infile->ist_index + i];
        if (!ist2->sub2video.frame)
            continue;
        /* subtitles seem to be usually muxed ahead of other streams;
           if not, subtracting a larger time here is necessary */
        pts2 = av_rescale_q(pts, ist->st->time_base, ist2->st->time_base) - 1;
        /* do not send the heartbeat frame if the subtitle is already ahead */
        if (pts2 <= ist2->sub2video.last_pts)
            continue;
        if (pts2 >= ist2->sub2video.end_pts ||
            (!ist2->sub2video.frame->data[0] && ist2->sub2video.end_pts < INT64_MAX))
            sub2video_update(ist2, NULL);
        for (j = 0, nb_reqs = 0; j < ist2->nb_filters; j++)
            nb_reqs += av_buffersrc_get_nb_failed_requests(ist2->filters[j]->filter);
        if (nb_reqs)
            sub2video_push_ref(ist2, pts2);
    }
}

/*
 * Return
 * - 0 -- one packet was read and processed
 * - AVERROR(EAGAIN) -- no packets were available for selected file,
 *   this function should be called again
 * - AVERROR_EOF -- this function should not be called again
 */
/**
 * @brief
 */
int FFmpegMedia::process_input(int file_index)
{
    InputFile *ifile = input_files[file_index];
    AVFormatContext *is;
    InputStream *ist;
    AVPacket pkt;
    int ret, thread_ret, i, j;
    int64_t duration;
    int64_t pkt_dts;

    is  = ifile->ctx;
    // 1. 从输入文件读取一个pkt
    ret = get_input_packet(ifile, &pkt);
    // 1.1 eagain,直接返回
    if (ret == AVERROR(EAGAIN)) {
        ifile->eagain = 1;
        return ret;
    }

    // 1.2 eof或者错误的情况,如果设置了循环读取,那么会直接进入循环读取的逻辑.
    if (ret < 0 && ifile->loop) {
        AVCodecContext *avctx;
        // 1.2.1 遍历每一个输入流, 若该输入流:
        // 1)是copy,那么for循环不需要处理任何内容,因为copy时根本不会用到编解码器;
        // 2)是需要解码,那么会不断往解码器刷空包,直至解码器返回eof; 注意,并不是发送一次空包后,
        //      解码器就立马返回eof,我debug这里,一般发送5-8次空包左右,解码器会返回eof.  这里debug不难.
        for (i = 0; i < ifile->nb_streams; i++) {
            ist = input_streams[ifile->ist_index + i];
            avctx = ist->dec_ctx;
            if (ist->decoding_needed) {
                ret = process_input_packet(ist, NULL, 1);//传NULL代表刷空包,参3传1代表不想输入过滤器发送eof,因为这里是循环
                if(ret < 0){// tyycode 失败直接返回.
                    av_log(NULL, AV_LOG_FATAL, "process_input_packet(1) flush NULL pkt failed.\n");
                    return ret;
                }
                if (ret>0)//1代表eof未到来,那么就先返回, 只有解码器的eof到来才会往下执行重置解码器
                    return 0;

                /* 重置内部解码器状态/刷新内部缓冲区。应该在寻找或切换到其他流时调用.
                 * @note 当被引用的帧没有被使用时(例如avctx->refcounted_frames = 0)，这会使之前从解码器返回的帧无效.
                 * 当使用被引用的帧时，解码器只是释放它可能保留在内部的任何引用，但调用方的引用仍然有效. */
                avcodec_flush_buffers(avctx);
            }
        }

        // 1.2.2 释放掉对应该输入文件的线程
#if HAVE_THREADS
        free_input_thread(file_index);
#endif

        // 1.2.3 seek到文件开始
        ret = seek_to_start(ifile, is);
#if HAVE_THREADS
        // 1.2.4 重新为该输入文件创建线程
        thread_ret = init_input_thread(file_index);
        if (thread_ret < 0)
            return thread_ret;
#endif
        if (ret < 0)//seek的失败是放在下面的错误统一处理
            av_log(NULL, AV_LOG_WARNING, "Seek to start failed.\n");
        else
            ret = get_input_packet(ifile, &pkt);// 1.2.5 seek成功,会重新再读取一个pkt
        if (ret == AVERROR(EAGAIN)) {
            ifile->eagain = 1;
            return ret;
        }
    }//<== if (ret < 0 && ifile->loop) end ==>

    // 1.3 读取包错误或者是eof
    if (ret < 0) {
        // 1.3.1 遇到真正错误并指定-xerror选项,直接退出
        if (ret != AVERROR_EOF) {
            print_error(is->url, ret);
            if (exit_on_error){
                //exit_program(1);
                return ret;
            }
        }

        // 1.3.2 遍历所有输入流做清理工作
        for (i = 0; i < ifile->nb_streams; i++) {
            // 若输入流是需要解码的,则需要刷空包,以清除解码器剩余的包.
            ist = input_streams[ifile->ist_index + i];
            if (ist->decoding_needed) {
                ret = process_input_packet(ist, NULL, 0);// 参3传0代表向过滤器发送eof,因为这里是真正结束了
                if(ret < 0){// tyycode 失败直接返回.
                    av_log(NULL, AV_LOG_FATAL, "process_input_packet(0 send eof to filter) flush NULL pkt failed.\n");
                    return ret;
                }
                if (ret>0)// 这里看到,会先清理完解码器后,再往下清理输出流.
                    return 0;
            }

            // 将输入流对应的输出流标记完成编码和复用.
            /* mark all outputs that don't go through lavfi as finished(将所有未经过lavfi(指filter?)的输出标记为已完成) */
            for (j = 0; j < nb_output_streams; j++) {
                OutputStream *ost = output_streams[j];

                if (ost->source_index == ifile->ist_index + i && /* 输出流保存的输入流下标 与 输入流下标本身一致 */
                    (ost->stream_copy || ost->enc->type == AVMEDIA_TYPE_SUBTITLE))/* 拷贝或者是字幕 */
                    finish_output_stream(ost);
            }
        }

        ifile->eof_reached = 1;// 来到这里,说明所有输入的对应的解码器都已经刷完剩余的pkt,并且已经送去过滤器然后编码.
        return AVERROR(EAGAIN);// 为啥返回eagain?不是应该返回eof好点?
                               // 答:为了返回transcode()进行下一步的清理.实际上process_input()不太注重返回值的处理,我们不用太关注返回值的处理.
    }

    // 2 重置input_files->eagain和output_streams->unavailable
    reset_eagain();

    // -dump选项
    if (do_pkt_dump) {
        av_pkt_dump_log2(NULL, AV_LOG_INFO, &pkt, do_hex_dump,
                         is->streams[pkt.stream_index]);
    }

    // 3. 出现新的流的包,报告并丢弃
    /* the following test is needed in case new streams appear
       dynamically in stream : we ignore them(如果新流在流中动态出现，则需要进行以下测试:忽略它们) */
    if (pkt.stream_index >= ifile->nb_streams) {
        report_new_stream(file_index, &pkt);
        goto discard_packet;
    }

    ist = input_streams[ifile->ist_index + pkt.stream_index];// 根据pkt保存的流下标 获取 对应输入流

    ist->data_size += pkt.size;// 统计该输入流已经读取的包大小和数目
    ist->nb_packets++;

    // 4. =1,该流读到的包都会被丢弃
    if (ist->discard)
        goto discard_packet;

    // 5. 若读到的pkt内容损坏 且 指定-xerror, 程序退出
    if (pkt.flags & AV_PKT_FLAG_CORRUPT) {
        av_log(NULL, exit_on_error ? AV_LOG_FATAL : AV_LOG_WARNING,
               "%s: corrupt input packet in stream %d\n", is->url, pkt.stream_index);
        if (exit_on_error){
            //exit_program(1);
            return -1;
        }
    }

    if (debug_ts) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_log(NULL, AV_LOG_INFO, "demuxer -> ist_index:%d type:%s "
               "next_dts:%s next_dts_time:%s next_pts:%s next_pts_time:%s pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s off:%s off_time:%s\n",
               ifile->ist_index + pkt.stream_index, av_get_media_type_string(ist->dec_ctx->codec_type),
               av_ts2str_ex(str, ist->next_dts), av_ts2timestr_ex(str, ist->next_dts, AV_TIME_BASE_Q),
               av_ts2str_ex(str, ist->next_pts), av_ts2timestr_ex(str, ist->next_pts, AV_TIME_BASE_Q),
               av_ts2str_ex(str, pkt.pts), av_ts2timestr_ex(str, pkt.pts, &ist->st->time_base),
               av_ts2str_ex(str, pkt.dts), av_ts2timestr_ex(str, pkt.dts, &ist->st->time_base),
               av_ts2str_ex(str, input_files[ist->file_index]->ts_offset),
               av_ts2timestr_ex(str, input_files[ist->file_index]->ts_offset, AV_TIME_BASE_Q));
    }

    // 6. 主要处理输入文件是 ts 流
    /* 这里主要是处理 ts 流的，能否进来由pts_wrap_bits决定,pts_wrap_bits(PTS中的位数(用于封装控制))一般值为64.
     * mp4/mkv文件转码不会跑进来. */
    if(!ist->wrap_correction_done && is->start_time != AV_NOPTS_VALUE && ist->st->pts_wrap_bits < 64){
        int64_t stime, stime2;
        // Correcting starttime based on the enabled streams
        // FIXME this ideally should be done before the first use of starttime but we do not know which are the enabled streams at that point.
        //       so we instead do it here as part of discontinuity handling
        /* 根据启用的流更正开始时间，
         * 在理想情况下，这应该在第一次使用starttime之前完成，但是我们不知道在那个时候哪些是启用的流。
         * 所以我们在这里做它作为不连续处理的一部分 */
        if (   ist->next_dts == AV_NOPTS_VALUE      // 首次进来?
            && ifile->ts_offset == -is->start_time
            && (is->iformat->flags & AVFMT_TS_DISCONT)) {// 格式允许时间戳不连续. 不允许的话ts/flv也不会跑进该if
            int64_t new_start_time = INT64_MAX;

            // 获取所有输入流中, 开始时间最小的值.
            for (i=0; i<is->nb_streams; i++) {
                AVStream *st = is->streams[i];
                if(st->discard == AVDISCARD_ALL || st->start_time == AV_NOPTS_VALUE)
                    continue;
                new_start_time = FFMIN(new_start_time, av_rescale_q(st->start_time, st->time_base, AV_TIME_BASE_Q));
            }

            // 如果所有输入流的最小的开始时间 都比 AVFormatContext的开始时间大,那么需要调整
            // is->start_time的值由AVStream->start_time的值推导出来
            if (new_start_time > is->start_time) {
                av_log(is, AV_LOG_VERBOSE, "Correcting start time by %" PRId64"\n", new_start_time - is->start_time);
                ifile->ts_offset = -new_start_time;// 直接加负号修正ts_offset
                                                   // 例如-ss指定是10s，那么ts_offset=-10s，假设输入流最小开始时间=11s,那么ts_offset=-11s
            }
        }

        // 用于下面的stime2 > stime判断.
        stime = av_rescale_q(is->start_time, AV_TIME_BASE_Q, ist->st->time_base);
        stime2= stime + (1ULL<<ist->st->pts_wrap_bits);// 1ULL特指64无符号bit,1左移pts_wrap_bits位(例如32=4,294,967,296)
                                                       // 1左移pts_wrap_bits位 等价于 2^pts_wrap_bits次方.例如1<<3位=2^3=8
        ist->wrap_correction_done = 1;

        // 主要看if的第三个条件,暂未知道意义.
        if(stime2 > stime && pkt.dts != AV_NOPTS_VALUE && pkt.dts > stime + (1LL<<(ist->st->pts_wrap_bits-1))) {
            pkt.dts -= 1ULL<<ist->st->pts_wrap_bits;
            ist->wrap_correction_done = 0;
        }
        if(stime2 > stime && pkt.pts != AV_NOPTS_VALUE && pkt.pts > stime + (1LL<<(ist->st->pts_wrap_bits-1))) {
            pkt.pts -= 1ULL<<ist->st->pts_wrap_bits;
            ist->wrap_correction_done = 0;
        }
    }

    // 7. 利用输入流的side数据 添加到 第一个pkt中
    /* add the stream-global side data to the first packet(将流全局side数据添加到第一个包中) */
    if (ist->nb_packets == 1) {
        for (i = 0; i < ist->st->nb_side_data; i++) {
            AVPacketSideData *src_sd = &ist->st->side_data[i];
            uint8_t *dst_data;

            // 不处理这个类型的边数据
            if (src_sd->type == AV_PKT_DATA_DISPLAYMATRIX)
                continue;

            // 从packet获取side info, 如果该类型在pkt已经存在数据,则不处理
            if (av_packet_get_side_data(&pkt, src_sd->type, NULL))//在wirte_packet()有对该函数说明
                continue;

            /* av_packet_new_side_data(): 分配数据包的新信息.
             * 参1: pkt; 参2: 要新开辟的边数据类型; 参3: 新开辟的边数据大小.
             * 返回值: 指向pkt中新开辟的边数据的地址. */
            // 在pkt没有该类型的边数据的,则为该类型重新开辟空间,并从ist->st->side_data拷贝数据.
            dst_data = av_packet_new_side_data(&pkt, src_sd->type, src_sd->size);
            if (!dst_data){
                //exit_program(1);
                return -1;
            }

            memcpy(dst_data, src_sd->data, src_sd->size);// 从ist->st->side_data拷贝数据到pkt的side data
        }
    }

    // 8. 加上ts_offset,ts_offset一般是0
    if (pkt.dts != AV_NOPTS_VALUE)
        pkt.dts += av_rescale_q(ifile->ts_offset, AV_TIME_BASE_Q, ist->st->time_base);
    if (pkt.pts != AV_NOPTS_VALUE)
        pkt.pts += av_rescale_q(ifile->ts_offset, AV_TIME_BASE_Q, ist->st->time_base);
#ifdef TYYCODE_TIMESTAMP_DEMUXER
    mydebug(NULL, AV_LOG_INFO, "process_input(), "
            "ist_index: %d, type: %s, "
            "ifile->ts_offset: %" PRId64", "
            "ist->st->time_base.num: %d, ist->st->time_base.den: %d, "
            "pkt.dts: %" PRId64", pkt.pts: %" PRId64"\n",
            ifile->ist_index + pkt.stream_index, av_get_media_type_string(ist->dec_ctx->codec_type),
            ifile->ts_offset,
            ist->st->time_base.num, ist->st->time_base.den, pkt.dts, pkt.pts);
#endif

    // 9. -itsscale选项,默认是1.0,设置输入ts的刻度,
    // 乘以ts_scale就是将pts,dts转成该刻度单位
    if (pkt.pts != AV_NOPTS_VALUE)
        pkt.pts *= ist->ts_scale;
    if (pkt.dts != AV_NOPTS_VALUE)
        pkt.dts *= ist->ts_scale;

#ifdef TYYCODE_TIMESTAMP_DEMUXER
            mydebug(NULL, AV_LOG_INFO,
                   "process_input(), ist_index: %d, type: %s, ifile->last_ts: %" PRId64"\n",
                   ifile->ist_index + pkt.stream_index,
                    av_get_media_type_string(ist->dec_ctx->codec_type),
                    ifile->last_ts);
#endif
    // 10. 当前pkt.dts与上一次的差值超过阈值，则调整ts_offset、pkt.dts、pkt.pts
    pkt_dts = av_rescale_q_rnd(pkt.dts, ist->st->time_base, AV_TIME_BASE_Q, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));// 这里因为相除有余数，要四舍五入
    if ((ist->dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
         ist->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) && /* 视频或者音频 */
        pkt_dts != AV_NOPTS_VALUE && ist->next_dts == AV_NOPTS_VALUE && !copy_ts /* dts有效 且 next_dts无效且 没指定-copyts */
        && (is->iformat->flags & AVFMT_TS_DISCONT) && ifile->last_ts != AV_NOPTS_VALUE) {// 允许时间戳不连续 且 last_ts有效
        int64_t delta   = pkt_dts - ifile->last_ts;// 两次dts差值
        if (delta < -1LL*dts_delta_threshold*AV_TIME_BASE ||
            delta >  1LL*dts_delta_threshold*AV_TIME_BASE){// 时间戳改变不在阈值范围[-dts_delta_threshold, dts_delta_threshold]
            ifile->ts_offset -= delta;
            av_log(NULL, AV_LOG_DEBUG,
                   "Inter stream timestamp discontinuity %" PRId64", new offset= %" PRId64"\n",
                   delta, ifile->ts_offset);
#ifdef TYYCODE_TIMESTAMP_DEMUXER
            mydebug(NULL, AV_LOG_INFO,
                   "process_input(), Inter stream timestamp discontinuity %" PRId64", new offset= %" PRId64"\n",
                   delta, ifile->ts_offset);
#endif
            pkt.dts -= av_rescale_q(delta, AV_TIME_BASE_Q, ist->st->time_base);
            if (pkt.pts != AV_NOPTS_VALUE)
                pkt.pts -= av_rescale_q(delta, AV_TIME_BASE_Q, ist->st->time_base);
        }
    }

#ifdef TYYCODE_TIMESTAMP_DEMUXER
    // 查看duration值的变化
    mydebug(NULL, AV_LOG_INFO,
           "process_input(), ist_index: %d, type: %s, "
           "ifile->duration: %" PRId64", "
           "ifile->time_base.num: %d, ifile->time_base.den: %d, "
           "ist->st->time_base.num: %d, ist->st->time_base: %d, "
           "ist->max_pts: %" PRId64", ist->min_pts: %" PRId64"\n",
           ifile->ist_index + pkt.stream_index, av_get_media_type_string(ist->dec_ctx->codec_type),
           ifile->duration,
           ifile->time_base.num, ifile->time_base.den,
           ist->st->time_base.num, ist->st->time_base.den,
           ist->max_pts, ist->min_pts);
#endif

    // 11. pkt的pts、dts加上文件时长duration的作用:
    // 当循环推流时(-stream_loop选项),每次读出来的pts、dts都需要加上输入文件的时长,原因是pts、dts要单调递增.
    // 不循环时,ifile->duration是0.
    duration = av_rescale_q(ifile->duration, ifile->time_base, ist->st->time_base);
    if (pkt.pts != AV_NOPTS_VALUE) {
        pkt.pts += duration;
        // 保存最大的pts和最小的pts, 这样后面可以根据两者相减算出文件时长.
        ist->max_pts = FFMAX(pkt.pts, ist->max_pts);
        ist->min_pts = FFMIN(pkt.pts, ist->min_pts);
    }

    if (pkt.dts != AV_NOPTS_VALUE)
        pkt.dts += duration;

    // 12. 算当前帧与下一帧的差值.
    pkt_dts = av_rescale_q_rnd(pkt.dts, ist->st->time_base, AV_TIME_BASE_Q, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    if ((ist->dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
         ist->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) &&
         pkt_dts != AV_NOPTS_VALUE && ist->next_dts != AV_NOPTS_VALUE &&
        !copy_ts) {
        int64_t delta   = pkt_dts - ist->next_dts;
        if (is->iformat->flags & AVFMT_TS_DISCONT) {
            if (delta < -1LL*dts_delta_threshold*AV_TIME_BASE ||
                delta >  1LL*dts_delta_threshold*AV_TIME_BASE ||
                pkt_dts + AV_TIME_BASE/10 < FFMAX(ist->pts, ist->dts)) {// pkt的dts与ist保存的pts,dts相差太大,也要调整
                ifile->ts_offset -= delta;
                av_log(NULL, AV_LOG_DEBUG,
                       "timestamp discontinuity for stream #%d:%d "
                       "(id=%d, type=%s): %" PRId64", new offset= %" PRId64"\n",
                       ist->file_index, ist->st->index, ist->st->id,
                       av_get_media_type_string(ist->dec_ctx->codec_type),
                       delta, ifile->ts_offset);
                pkt.dts -= av_rescale_q(delta, AV_TIME_BASE_Q, ist->st->time_base);
                if (pkt.pts != AV_NOPTS_VALUE)
                    pkt.pts -= av_rescale_q(delta, AV_TIME_BASE_Q, ist->st->time_base);
            }
        } else {
            if ( delta < -1LL*dts_error_threshold*AV_TIME_BASE ||
                 delta >  1LL*dts_error_threshold*AV_TIME_BASE) {
                av_log(NULL, AV_LOG_WARNING, "DTS %" PRId64", next:%" PRId64" st:%d invalid dropping\n", pkt.dts, ist->next_dts, pkt.stream_index);
                pkt.dts = AV_NOPTS_VALUE;
            }
            if (pkt.pts != AV_NOPTS_VALUE){
                int64_t pkt_pts = av_rescale_q(pkt.pts, ist->st->time_base, AV_TIME_BASE_Q);
                delta   = pkt_pts - ist->next_dts;
                if ( delta < -1LL*dts_error_threshold*AV_TIME_BASE ||
                     delta >  1LL*dts_error_threshold*AV_TIME_BASE) {
                    av_log(NULL, AV_LOG_WARNING, "PTS %" PRId64", next:%" PRId64" invalid dropping st:%d\n", pkt.pts, ist->next_dts, pkt.stream_index);
                    pkt.pts = AV_NOPTS_VALUE;
                }
            }
        }
    }

    // 13. 保存最近一个pkt的dts
    if (pkt.dts != AV_NOPTS_VALUE)
        ifile->last_ts = av_rescale_q(pkt.dts, ist->st->time_base, AV_TIME_BASE_Q);

    if (debug_ts) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_log(NULL, AV_LOG_INFO, "demuxer+ffmpeg -> ist_index:%d type:%s pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s off:%s off_time:%s\n",
               ifile->ist_index + pkt.stream_index, av_get_media_type_string(ist->dec_ctx->codec_type),
               av_ts2str_ex(str, pkt.pts), av_ts2timestr_ex(str, pkt.pts, ist->st->time_base),
               av_ts2str_ex(str, pkt.dts), av_ts2timestr_ex(str, pkt.dts, ist->st->time_base),
               av_ts2str_ex(str, input_files[ist->file_index]->ts_offset),
               av_ts2timestr_ex(str, input_files[ist->file_index]->ts_offset, AV_TIME_BASE_Q));
    }

    // 略,内部一般会因ist2->sub2video.frame为空返回.
    sub2video_heartbeat(ist, pkt.pts);

    // 正常处理pkt的流程.
    //process_input_packet(ist, &pkt, 0);// 可以看到ffmpeg本身正常处理是不关心返回值
    ret = process_input_packet(ist, &pkt, 0);
    if(ret < 0){
        av_log(NULL, AV_LOG_FATAL,"normal handle process_input_packet failed.\n");
        //av_packet_unref(&pkt);// 失败释不释放感觉区别不大
        return ret;
    }

discard_packet:
    av_packet_unref(&pkt);

    return 0;
}

/**
 * Run a single step of transcoding.(运行转码的单个步骤)
 *
 * @return  0 for success, <0 for error
 */
int FFmpegMedia::transcode_step(void)
{
    OutputStream *ost;
    InputStream  *ist = NULL;
    int ret;

    /* 1. 选择一个输出流 */
    ost = choose_output();
    if (!ost) {
        // 1.1 若拿到不可用的流时,且output_streams[i]->unavailable=1,会清空unavailable=0,然后返回.相当于再尝试获取输出流
        if (got_eagain()) {
            reset_eagain();
            av_usleep(10000);
            return 0;
        }

        // 1.2 没有更多的输入来读取，完成
        av_log(NULL, AV_LOG_VERBOSE, "No more inputs to read from, finishing.\n");
        return AVERROR_EOF;
    }

    /* 2. 过滤器相关，推流没用到，暂未研究 */
    if (ost->filter && !ost->filter->graph->graph) {
        if (ifilter_has_all_input_formats(ost->filter->graph)) {
            ret = configure_filtergraph(ost->filter->graph);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error reinitializing filters!\n");
                return ret;
            }
        }
    }

    /* 3. (基于输出流)选择一个输入流. */
    // 3.1 ost->filter 和 ost->filter->graph->graph都已经初始化的选择流程.
    if (ost->filter && ost->filter->graph->graph) {
        /* 3.1.1 对还没调用init_output_stream()初始化的流，则调用。
         * 注，因为在init_simple_filtergraph()是没有对
         * FilterGraph(即fg变量)里面的AVFilterGraph *graph赋值的，
         * 而ost->filter=fg->outputs[0]()即指向OutputFilter，
         * OutputFilter内部的FilterGraph *graph(这里的graph与前者graph的类型不一样)是指向fg变量，
         * 也就是说，ost->filter->graph的指向就是指向fg变量。那么由于fg->graph没赋值，
         * 所以一般不是这里调用init_output_stream()对音视频的输出流进行初始化。
        */
        if (!ost->initialized) {
            char error[1024] = {0};
            ret = init_output_stream(ost, error, sizeof(error));
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error initializing output stream %d:%d -- %s\n",
                       ost->file_index, ost->index, error);
                //exit_program(1);
                return ret;
            }
        }
        if ((ret = transcode_from_filter(ost->filter->graph, &ist)) < 0)
            return ret;
        if (!ist)// 这里看到,transcode_from_filter找不到best_ist且成功,会直接返回0
            return 0;
    } else if (ost->filter) {
        // 3.2 只有ost->filter被初始化的选择流程. 推流时,ost->filter对应fg->outputs[0](see init_simple_filtergraph())
        // 3.2.1 遍历 该输出流 对应的输入流的输入过滤器个数, 通过输入过滤器获取对应的输入流,判断是否可以作为输入
        // 正常输入流与输入过滤器个数都是1比1的(see init_simple_filtergraph()).
        int i;
        for (i = 0; i < ost->filter->graph->nb_inputs; i++) {
            InputFilter *ifilter = ost->filter->graph->inputs[i];
            if (!ifilter->ist->got_output && !input_files[ifilter->ist->file_index]->eof_reached) {//优先选择输入流还没解码成功过的 且 输入文件还没到达eof
                ist = ifilter->ist;
                break;
            }
        }
        // 3.2.2 没有找到输入流,认为处理所有输入流完成,标记inputs_done=1,函数返回.
        if (!ist) {
            ost->inputs_done = 1;
            return 0;
        }
    } else {
        //3.3 这里只要输入流下标>=0,就会选择该输入流
        av_assert0(ost->source_index >= 0);
        ist = input_streams[ost->source_index];
    }

    // 4. 读取一个pkt并解码发送到过滤器
    ret = process_input(ist->file_index);
    if (ret == AVERROR(EAGAIN)) {
        if (input_files[ist->file_index]->eagain)
            ost->unavailable = 1;
        return 0;
    }

    if (ret < 0)
        return ret == AVERROR_EOF ? 0 : ret;

    // 5. 从过滤器中读取一帧,并进行编码写帧.
    // 如果没有任何一个输入流被成功解码,会直接返回0.
    return reap_filters(0);
}

/**
 * @brief 打印完成输出后的状态.
 * @param total_size avio_size(oc->pb)得到的字节大小,我个人认为是ffmpeg写入到内存的大小.
 *                      它和写帧时的实际写入大小之差 / 写帧时的实际写入大小的值,是一个复用开销比.
 */
void FFmpegMedia::print_final_stats(int64_t total_size)
{
    uint64_t video_size = 0, audio_size = 0, extra_size = 0, other_size = 0;
    uint64_t subtitle_size = 0;
    uint64_t data_size = 0;
    float percent = -1.0;
    int i, j;
    int pass1_used = 1;

    // 1. 统计输出流已经输出到输出地址的信息.
    for (i = 0; i < nb_output_streams; i++) {
        OutputStream *ost = output_streams[i];
        // 记录单个流已写包的大小
        switch (ost->enc_ctx->codec_type) {
            case AVMEDIA_TYPE_VIDEO: video_size += ost->data_size; break;
            case AVMEDIA_TYPE_AUDIO: audio_size += ost->data_size; break;
            case AVMEDIA_TYPE_SUBTITLE: subtitle_size += ost->data_size; break;
            default:                 other_size += ost->data_size; break;
        }
        extra_size += ost->enc_ctx->extradata_size; //统计所有输出流的sps,pps等信息头大小
        data_size  += ost->data_size;               //统计所有流已经发包的大小
        //flags与运算后的结果不等于AV_CODEC_FLAG_PASS1, pass1_used=0.
        if (   (ost->enc_ctx->flags & (AV_CODEC_FLAG_PASS1 | AV_CODEC_FLAG_PASS2))
            != AV_CODEC_FLAG_PASS1)
            pass1_used = 0;
    }

    // 2. 算出复用的开销.
    // ffmpeg写入的总大小和实际发送到输出地址的差值比? muxing overhead复用开销
    if (data_size && total_size>0 && total_size >= data_size)
        percent = 100.0 * (total_size - data_size) / data_size;

    // 3. 打印各个流输出到输出文件的大小.
    av_log(NULL, AV_LOG_INFO, "video:%1.0fkB audio:%1.0fkB subtitle:%1.0fkB other streams:%1.0fkB global headers:%1.0fkB muxing overhead: ",
           video_size / 1024.0,
           audio_size / 1024.0,
           subtitle_size / 1024.0,
           other_size / 1024.0,
           extra_size / 1024.0);
    if (percent >= 0.0)
        av_log(NULL, AV_LOG_INFO, "%f%%", percent);
    else
        av_log(NULL, AV_LOG_INFO, "unknown");
    av_log(NULL, AV_LOG_INFO, "\n");

    // 4. 打印输入流详细的信息.需要设置对应的日志等级才能被打印
    /* print verbose per-stream stats(打印详细的每个流统计) */
    for (i = 0; i < nb_input_files; i++) {
        InputFile *f = input_files[i];
        uint64_t total_packets = 0, total_size = 0;

        av_log(NULL, AV_LOG_VERBOSE, "Input file #%d (%s):\n",
               i, f->ctx->url);

        for (j = 0; j < f->nb_streams; j++) {
            InputStream *ist = input_streams[f->ist_index + j];
            enum AVMediaType type = ist->dec_ctx->codec_type;

            total_size    += ist->data_size;//统计所有输入流 已经读取的总大小和总包数.
            total_packets += ist->nb_packets;

            av_log(NULL, AV_LOG_VERBOSE, "  Input stream #%d:%d (%s): ",
                   i, j, media_type_string(type));
            av_log(NULL, AV_LOG_VERBOSE, "%" PRIu64" packets read (%" PRIu64" bytes); ",
                   ist->nb_packets, ist->data_size);

            // 输入流要转码的还会把已经解码的帧数、样本数打印
            if (ist->decoding_needed) {
                av_log(NULL, AV_LOG_VERBOSE, "%" PRIu64" frames decoded",
                       ist->frames_decoded);
                if (type == AVMEDIA_TYPE_AUDIO)
                    av_log(NULL, AV_LOG_VERBOSE, " (%" PRIu64" samples)", ist->samples_decoded);
                av_log(NULL, AV_LOG_VERBOSE, "; ");
            }

            av_log(NULL, AV_LOG_VERBOSE, "\n");
        }

        av_log(NULL, AV_LOG_VERBOSE, "  Total: %" PRIu64" packets (%" PRIu64" bytes) demuxed\n",
               total_packets, total_size);
    }

    // 5. 打印输出流详细的信息, 与上面的输入同理
    for (i = 0; i < nb_output_files; i++) {
        OutputFile *of = output_files[i];
        uint64_t total_packets = 0, total_size = 0;

        av_log(NULL, AV_LOG_VERBOSE, "Output file #%d (%s):\n",
               i, of->ctx->url);

        for (j = 0; j < of->ctx->nb_streams; j++) {
            OutputStream *ost = output_streams[of->ost_index + j];
            enum AVMediaType type = ost->enc_ctx->codec_type;

            total_size    += ost->data_size;
            total_packets += ost->packets_written;

            av_log(NULL, AV_LOG_VERBOSE, "  Output stream #%d:%d (%s): ",
                   i, j, media_type_string(type));
            if (ost->encoding_needed) {
                av_log(NULL, AV_LOG_VERBOSE, "%" PRIu64" frames encoded",
                       ost->frames_encoded);
                if (type == AVMEDIA_TYPE_AUDIO)
                    av_log(NULL, AV_LOG_VERBOSE, " (%" PRIu64" samples)", ost->samples_encoded);
                av_log(NULL, AV_LOG_VERBOSE, "; ");
            }

            av_log(NULL, AV_LOG_VERBOSE, "%" PRIu64" packets muxed (%" PRIu64" bytes); ",
                   ost->packets_written, ost->data_size);

            av_log(NULL, AV_LOG_VERBOSE, "\n");
        }

        av_log(NULL, AV_LOG_VERBOSE, "  Total: %" PRIu64" packets (%" PRIu64" bytes) muxed\n",
               total_packets, total_size);
    }

    // 6. 没有内容被输出过才会进来.
    if(video_size + data_size + audio_size + subtitle_size + extra_size == 0){
        av_log(NULL, AV_LOG_WARNING, "Output file is empty, nothing was encoded ");
        if (pass1_used) {
            av_log(NULL, AV_LOG_WARNING, "\n");
        } else {
            av_log(NULL, AV_LOG_WARNING, "(check -ss / -t / -frames parameters if used)\n");
        }
    }
}

/**
 * @brief 打印相关信息到控制台.我们平时看到的帧率、编码质量、码率都是在这个函数打印的.
 *
 * @param is_last_report 是否是最后一次打印.=1是最后一次打印
 * @param timer_start 程序开始时间
 * @param cur_time 当前时间
 */
void FFmpegMedia::print_report(int is_last_report, int64_t timer_start, int64_t cur_time)
{
    AVBPrint buf, buf_script;
    OutputStream *ost;
    AVFormatContext *oc;
    int64_t total_size;
    AVCodecContext *enc;
    int frame_number, vid, i;
    double bitrate;
    double speed;
    int64_t pts = INT64_MIN + 1;
    static int64_t last_time = -1;// 静态变量
    static int qp_histogram[52];  // qp元素范围数组,静态变量
    int hours, mins, secs, us;
    const char *hours_sign;
    int ret;
    float t;

    // 1. 不打印状态 且 is_last_report=0 且 progress_avio为空
    if (!print_stats && !is_last_report && !progress_avio)
        return;

    // 2. 不是最后的一次调用本函数报告,往下走
    if (!is_last_report) {
        // 2.1 第一次进来该函数会先保存cur_time, 然后直接返回
        if (last_time == -1) {
            last_time = cur_time;
            return;
        }
        // 2.2 起码0.5s及以上才会dump
        if ((cur_time - last_time) < 500000)
            return;

        // 2.3 更新last_time
        last_time = cur_time;
    }

    // 3. 获取当前与开始时间的差值
    t = (cur_time-timer_start) / 1000000.0;

    // 4. 默认获取第一个输出文件
    oc = output_files[0]->ctx;

    // 5. 获取已经写入的文件大小.待会用于求码率
    total_size = avio_size(oc->pb);
    if (total_size <= 0) // FIXME improve avio_size() so it works with non seekable output too(改进avio_size()，使它也能处理不可查找的输出)
        total_size = avio_tell(oc->pb);

    // 6. 打印视频的frame,fps,q
    vid = 0;
    av_bprint_init(&buf, 0, AV_BPRINT_SIZE_AUTOMATIC);// 初始化两个缓存
    av_bprint_init(&buf_script, 0, AV_BPRINT_SIZE_AUTOMATIC);
    for (i = 0; i < nb_output_streams; i++) {
        float q = -1;
        ost = output_streams[i];
        enc = ost->enc_ctx;

        // 转码时获取编码质量
        if (!ost->stream_copy)
            q = ost->quality / (float) FF_QP2LAMBDA;

        // vid=1且是视频时,往缓冲区追加相关字符串(存在多个视频流时才会进来)
        if (vid && enc->codec_type == AVMEDIA_TYPE_VIDEO) {
            av_bprintf(&buf, "q=%2.1f ", q);
            av_bprintf(&buf_script, "stream_%d_%d_q=%.1f\n",
                       ost->file_index, ost->index, q);
        }

        // vid=0且是视频时,往缓冲区追加相关字符串
        if (!vid && enc->codec_type == AVMEDIA_TYPE_VIDEO) {
            float fps;

            frame_number = ost->frame_number;
            fps = t > 1 ? frame_number / t : 0;// 动态帧率求法,单位时间内写帧的数量,超过1s才会计算,否则帧率为0
            av_bprintf(&buf, "frame=%5d fps=%3.*f q=%3.1f ",
                     frame_number, fps < 9.95, fps, q);// %3.*f中的"*"可参考: https://zhidao.baidu.com/question/2269588931158396988.html
            av_bprintf(&buf_script, "frame=%d\n", frame_number);
            av_bprintf(&buf_script, "fps=%.2f\n", fps);
            av_bprintf(&buf_script, "stream_%d_%d_q=%.1f\n",
                       ost->file_index, ost->index, q);
            if (is_last_report)
                av_bprintf(&buf, "L");

            if (qp_hist) {// -qphist选项
                int j;
                int qp = lrintf(q);//lrintf()是四舍五入函数.
                if (qp >= 0 && qp < FF_ARRAY_ELEMS(qp_histogram))
                    qp_histogram[qp]++;
                for (j = 0; j < 32; j++)
                    av_bprintf(&buf, "%X", av_log2(qp_histogram[j] + 1));
            }

            // 编码器上下文包含宏AV_CODEC_FLAG_PSNR 并且 (图片类型有效或者最后一次报告)
            // 推流命令不会进来
            if ((enc->flags & AV_CODEC_FLAG_PSNR) && (ost->pict_type != AV_PICTURE_TYPE_NONE || is_last_report)) {
                int j;
                double error, error_sum = 0;
                double scale, scale_sum = 0;
                double p;
                char type[3] = { 'Y','U','V' };
                av_bprintf(&buf, "PSNR=");
                for (j = 0; j < 3; j++) {
                    if (is_last_report) {
                        error = enc->error[j];
                        scale = enc->width * enc->height * 255.0 * 255.0 * frame_number;
                    } else {
                        error = ost->error[j];
                        scale = enc->width * enc->height * 255.0 * 255.0;
                    }
                    if (j)
                        scale /= 4;
                    error_sum += error;
                    scale_sum += scale;
                    p = psnr(error / scale);
                    av_bprintf(&buf, "%c:%2.2f ", type[j], p);
                    av_bprintf(&buf_script, "stream_%d_%d_psnr_%c=%2.2f\n",
                               ost->file_index, ost->index, type[j] | 32, p);
                }
                p = psnr(error_sum / scale_sum);
                av_bprintf(&buf, "*:%2.2f ", psnr(error_sum / scale_sum));
                av_bprintf(&buf_script, "stream_%d_%d_psnr_all=%2.2f\n",
                           ost->file_index, ost->index, p);
            }

            vid = 1;
        }//<== if (!vid && enc->codec_type == AVMEDIA_TYPE_VIDEO) end ==>

        /* compute min output value(计算最小输出值) */
        /* av_stream_get_end_pts(): 返回最后一个muxed包的pts + 它的持续时间(可认为是当前时间,debug理解即可).
         * 当与demuxer一起使用时，返回值未定义. */
        int64_t tyycode = av_stream_get_end_pts(ost->st);
        if (av_stream_get_end_pts(ost->st) != AV_NOPTS_VALUE)
            pts = FFMAX(pts, av_rescale_q(av_stream_get_end_pts(ost->st),
                                          ost->st->time_base, AV_TIME_BASE_Q));// pts最终保存所有输出流的最大pts
        if (is_last_report)
            nb_frames_drop += ost->last_dropped;

    }//<== for (i = 0; i < nb_output_streams; i++) end ==>

    // 利用av_stream_get_end_pts得到的pts求出当前的时分秒
    secs = FFABS(pts) / AV_TIME_BASE;
    us = FFABS(pts) % AV_TIME_BASE;
    mins = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;
    hours_sign = (pts < 0) ? "-" : "";// 是否是负数

    // 求已经经过的时间的码率,算法: 已经写入的大小字节*8bit 除以 已经经过的时间
    bitrate = pts && total_size >= 0 ? total_size * 8 / (pts / 1000.0) : -1;

    // 求速率.pts / AV_TIME_BASE是转成单位秒; pts/t代表流写入的时间和实际时间的比.
    // 值越大,代表流写入得越快.
    speed = t != 0.0 ? (double)pts / AV_TIME_BASE / t : -1;

    // 7. 填充size和time
    if (total_size < 0) av_bprintf(&buf, "size=N/A time=");
    else                av_bprintf(&buf, "size=%8.0fkB time=", total_size / 1024.0);//除以1024是转成KB单位
    if (pts == AV_NOPTS_VALUE) {
        av_bprintf(&buf, "N/A ");
    } else {
        av_bprintf(&buf, "%s%02d:%02d:%02d.%02d ",
                   hours_sign, hours, mins, secs, (100 * us) / AV_TIME_BASE);
                    // (100 * us) / AV_TIME_BASE中, 除以AV_TIME_BASE是转成秒,乘以100是为了显示转成秒后的前两位数字.
    }

    // 8. 填充码率
    if (bitrate < 0) {
        av_bprintf(&buf, "bitrate=N/A");
        av_bprintf(&buf_script, "bitrate=N/A\n");
    }else{
        av_bprintf(&buf, "bitrate=%6.1fkbits/s", bitrate);
        av_bprintf(&buf_script, "bitrate=%6.1fkbits/s\n", bitrate);
    }

    // 填充大小,注意是往buf_script填充
    if (total_size < 0) av_bprintf(&buf_script, "total_size=N/A\n");
    else                av_bprintf(&buf_script, "total_size=%" PRId64"\n", total_size);
    if (pts == AV_NOPTS_VALUE) {
        av_bprintf(&buf_script, "out_time_us=N/A\n");
        av_bprintf(&buf_script, "out_time_ms=N/A\n");
        av_bprintf(&buf_script, "out_time=N/A\n");
    } else {
        av_bprintf(&buf_script, "out_time_us=%" PRId64"\n", pts);
        av_bprintf(&buf_script, "out_time_ms=%" PRId64"\n", pts);
        av_bprintf(&buf_script, "out_time=%s%02d:%02d:%02d.%06d\n",
                   hours_sign, hours, mins, secs, us);
    }

    if (nb_frames_dup || nb_frames_drop)
        av_bprintf(&buf, " dup=%d drop=%d", nb_frames_dup, nb_frames_drop);
    av_bprintf(&buf_script, "dup_frames=%d\n", nb_frames_dup);
    av_bprintf(&buf_script, "drop_frames=%d\n", nb_frames_drop);

    // 9. 填充speed
    if (speed < 0) {
        av_bprintf(&buf, " speed=N/A");
        av_bprintf(&buf_script, "speed=N/A\n");
    } else {
        av_bprintf(&buf, " speed=%4.3gx", speed);
        av_bprintf(&buf_script, "speed=%4.3gx\n", speed);
    }

    // 10. 最终是这里打印.可以看到是打印buf这个缓冲区.
    if (print_stats || is_last_report) {
        const char end = is_last_report ? '\n' : '\r';
        if (print_stats==1 && AV_LOG_INFO > av_log_get_level()) {
            fprintf(stderr, "%s    %c", buf.str, end);
        } else
            av_log(NULL, AV_LOG_INFO, "%s    %c", buf.str, end);

        fflush(stderr);//每次打印完毕清空输出缓冲区.
    }
    av_bprint_finalize(&buf, NULL);//参2传NULL代表释放pan_buf内开辟过的内存.

    // 11. buf_script会写到avio
    if (progress_avio) {
        av_bprintf(&buf_script, "progress=%s\n",
                   is_last_report ? "end" : "continue");
        avio_write(progress_avio, (const unsigned char *)buf_script.str,
                   FFMIN(buf_script.len, buf_script.size - 1));
        avio_flush(progress_avio);
        av_bprint_finalize(&buf_script, NULL);
        if (is_last_report) {
            if ((ret = avio_closep(&progress_avio)) < 0){
                char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_log(NULL, AV_LOG_ERROR,
                       "Error closing progress log, loss of information possible: %s\n", av_err2str_ex(str, ret));
            }
        }
    }

    // 12. 若是最后一次打印,调用print_final_stats
    if (is_last_report)
        print_final_stats(total_size);
}

/**
 * @brief 将每个输出流的编码器剩余pkt清空.
 */
int FFmpegMedia::flush_encoders(void)
{
    int i, ret;

    for (i = 0; i < nb_output_streams; i++) {
        OutputStream   *ost = output_streams[i];
        AVCodecContext *enc = ost->enc_ctx;
        OutputFile      *of = output_files[ost->file_index];

        // 1. 没编码的流直接返回
        if (!ost->encoding_needed)
            continue;

        // Try to enable encoding with no input frames.(尝试启用无输入帧的编码)
        // Maybe we should just let encoding fail instead.(也许我们应该让编码失败)
        // 2. 没有调用init_output_stream初始化输出流, 重新初始化.
        if (!ost->initialized) {
            FilterGraph *fg = ost->filter->graph;
            char error[1024] = "";

            av_log(NULL, AV_LOG_WARNING,
                   "Finishing stream %d:%d without any data written to it.\n",
                   ost->file_index, ost->st->index);

            // 2.1 开辟了OutputFilter 并且 未配置滤波图
            if (ost->filter && !fg->graph) {
                int x;
                // 2.1.1 初始化每个输入过滤器的format
                for (x = 0; x < fg->nb_inputs; x++) {
                    InputFilter *ifilter = fg->inputs[x];
                    if (ifilter->format < 0)//这里看到ffmpeg每次配置滤波图都要初始化输入过滤器的参数
                        ifilter_parameters_from_codecpar(ifilter, ifilter->ist->st->codecpar);
                }

                // 2.1.2 若上面输入过滤器配置失败,处理下一个输出流.
                if (!ifilter_has_all_input_formats(fg))
                    continue;

                // 2.1.3 配置fg
                ret = configure_filtergraph(fg);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error configuring filter graph\n");
                    //exit_program(1);
                    goto fail;
                }

                // 2.1.4 标记输出流完成编码和复用
                finish_output_stream(ost);
            }//<== if (ost->filter && !fg->graph) end ==>

            // 2.2 重新初始化输出流.
            ret = init_output_stream(ost, error, sizeof(error));
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error initializing output stream %d:%d -- %s\n",
                       ost->file_index, ost->index, error);
                //exit_program(1);
                goto fail;
            }
        }//<== if (!ost->initialized) end ==>

        // 音频的编码器的frame_size <= 1, 不处理
        if (enc->codec_type == AVMEDIA_TYPE_AUDIO && enc->frame_size <= 1)
            continue;

        // 不是音视频不处理
        if (enc->codec_type != AVMEDIA_TYPE_VIDEO && enc->codec_type != AVMEDIA_TYPE_AUDIO)
            continue;

        // 3. 清空编码器
        for (;;) {
            const char *desc = NULL;
            AVPacket pkt;
            int pkt_size;

            switch (enc->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                desc   = "audio";
                break;
            case AVMEDIA_TYPE_VIDEO:
                desc   = "video";
                break;
            default:
                /*av_assert0(0);*/
                return -1;
            }

            // 3.1 设置空包
            av_init_packet(&pkt);
            pkt.data = NULL;
            pkt.size = 0;

            update_benchmark(NULL);

            // 3.2 冲刷编码器
            // flush 编码器的实际操作: 从编码器读取eagain, 那么一直往编码器发送空帧,直至遇到非eagain
            while ((ret = avcodec_receive_packet(enc, &pkt)) == AVERROR(EAGAIN)) {
                ret = avcodec_send_frame(enc, NULL);
                if (ret < 0) {
                    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                    av_log(NULL, AV_LOG_FATAL, "%s encoding failed: %s\n",
                           desc,
                           av_err2str_ex(str, ret));
                    //exit_program(1);
                    goto fail;
                }
            }

            // 3.3 处理avcodec_receive_packet()返回eagain以外的返回值
            update_benchmark("flush_%s %d.%d", desc, ost->file_index, ost->index);
            if (ret < 0 && ret != AVERROR_EOF) {
                char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                av_log(NULL, AV_LOG_FATAL, "%s encoding failed: %s\n",
                       desc,
                       av_err2str_ex(str, ret));
                //exit_program(1);
                goto fail;
            }
            if (ost->logfile && enc->stats_out) {
                fprintf(ost->logfile, "%s", enc->stats_out);
            }

            /* 这里对剩余编码器的包的处理:
             * 1)eof: 调用output_packet,参4传1,实际上没有使用位流的话,内部没处理任何内容.
             * 2)正常读到剩余的包: 若输出流已经完成,那么不要该包了,直接释放引用,然后返回处理下一个包;
             *                  若输出流没完成,那么继续 将该包输出到输出url. */
            if (ret == AVERROR_EOF) {
                output_packet(of, &pkt, ost, 1);// 参4=1,主要是位流处理, 所以这里我们不处理返回值
                break;//该输出流flush encoder完成,退出for处理下一个输出流
            }
            // 输出流复用完成,丢弃该pkt
            if (ost->finished & MUXER_FINISHED) {
                av_packet_unref(&pkt);
                continue;
            }

            // 没有遇到eof或者输出流没完成,那么继续将该包输出到输出url
            av_packet_rescale_ts(&pkt, enc->time_base, ost->mux_timebase);
            pkt_size = pkt.size;
            ret = output_packet(of, &pkt, ost, 0);
            if(ret < 0){
                goto fail;
            }
            if (ost->enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO && vstats_filename) {
                do_video_stats(ost, pkt_size);
            }
        }//<== for (;;) end ==>

    }//<== for (i = 0; i < nb_output_streams; i++) end ==>

    ret = 0;

fail:
    return ret;
}

void FFmpegMedia::term_exit_sigsafe(void)
{
#if HAVE_TERMIOS_H
    if(restore_tty)
        tcsetattr (0, TCSANOW, &oldtty);
#endif
}

void FFmpegMedia::term_exit(void)
{
    av_log(NULL, AV_LOG_QUIET, "%s", "");
    term_exit_sigsafe();
}

/**
 * @brief The following code is the main loop of the file converter
 * (下面的代码是文件转换器的主循环)
 * @return 成功-0 失败-负数或者程序直接退出
 */
int FFmpegMedia::transcode(void)
{
    int ret, i;
    AVFormatContext *os;
    OutputStream *ost;
    InputStream *ist;
    int64_t timer_start;
    int64_t total_packets_written = 0;

    /* 1.转码前的初始化 */
    ret = transcode_init();
    if (ret < 0)
        goto fail;

    if (stdin_interaction) {
        av_log(NULL, AV_LOG_INFO, "Press [q] to stop, [?] for help\n");
    }

    timer_start = av_gettime_relative();

#if HAVE_THREADS
    /* 2.存在多个输入文件时，给每个输入文件对应的开辟一个线程. */
    if ((ret = init_input_threads()) < 0)
        goto fail;
#endif

    // 3. 循环转码
    while (!received_sigterm) {
        int64_t cur_time= av_gettime_relative();

        /* if 'q' pressed, exits */
        if (stdin_interaction)
            if (check_keyboard_interaction(cur_time) < 0)//键盘事件相关处理，这里不详细研究
                break;

        /* check if there's any stream where output is still needed */
        if (!need_output()) {
            av_log(NULL, AV_LOG_VERBOSE, "No more output streams to write to, finishing.\n");
            break;// 正常这里退出while
        }

        ret = transcode_step();
        if (ret < 0 && ret != AVERROR_EOF) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_ERROR, "Error while filtering: %s\n", av_err2str_ex(str, ret));
            break;
        }

        /* dump report by using the output first video and audio streams */
        print_report(0, timer_start, cur_time);
    }//<== while (!received_sigterm) end ==>

    // 4. 多输入文件时.回收所有输入文件线程.
#if HAVE_THREADS
    free_input_threads();
#endif

    // 5. 确认所有输入流的解码器已经使用空包刷新缓冲区.
    /* 一般在process_input()里面完成刷空包,除非发生写帧错误.
     * 1)例如在eof时,debug久一点导致写帧失败,会在这里刷NULL.
     * 2)debug时不想在这里刷空包,我们去掉与reap_filters()有关的断点,并让其快速运行,以便能快速写帧,这样就不会写帧错误,
     *      这样是为了能保持正常的逻辑去查看运行流程 */
    /* at the end of stream, we must flush the decoder buffers(在流结束时,必须刷新解码器缓冲区) */
    for (i = 0; i < nb_input_streams; i++) {
        ist = input_streams[i];
        // 文件没遇到eof.一般在process_input()被置1
        if (!input_files[ist->file_index]->eof_reached) {
            ret = process_input_packet(ist, NULL, 0);
            if(ret < 0){// tyycode 失败直接返回.
                av_log(NULL, AV_LOG_FATAL, "process_input_packet(0 transcode) flush NULL pkt failed.\n");
                return ret;
            }
        }
    }

    // 6. 清空编码器.一般是这里清空编码器,然后写剩余的帧的.
    ret = flush_encoders();
    if(ret < 0){
        av_log(NULL, AV_LOG_FATAL, "flush_encoders failed.\n");
        return -1;
    }

    //term_exit();
    av_log(NULL, AV_LOG_DEBUG, "flush_encoders excute ok.\n");

    /* write the trailer if needed and close file */
    // 7. 为每个输出文件调用av_write_trailer().
    for (i = 0; i < nb_output_files; i++) {
        os = output_files[i]->ctx;
        if (!output_files[i]->header_written) {
            av_log(NULL, AV_LOG_ERROR,
                   "Nothing was written into output file %d (%s), because "
                   "at least one of its streams received no packets.\n",
                   i, os->url);
            continue;// 没写头的文件不写尾
        }
        if ((ret = av_write_trailer(os)) < 0) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_ERROR, "Error writing trailer of %s: %s\n", os->url, av_err2str_ex(str, ret));
            if (exit_on_error){
                //exit_program(1);
                return ret;
            }
        }
    }

    /* dump report by using the first video and audio streams */
    print_report(1, timer_start, av_gettime_relative());

    // 8. 关闭编码器
    /* close each encoder */
    for (i = 0; i < nb_output_streams; i++) {
        ost = output_streams[i];
        if (ost->encoding_needed) {
            av_freep(&ost->enc_ctx->stats_in);// 编码器这样关闭就可以了吗?
        }
        total_packets_written += ost->packets_written;//统计所以输出流已经写帧的包数.
    }

    // 写入的包数为0 且 abort_on_flags 与上 1 的结果不为0, 程序退出.
    if (!total_packets_written && (abort_on_flags & ABORT_ON_FLAG_EMPTY_OUTPUT)) {
        av_log(NULL, AV_LOG_FATAL, "Empty output\n");
        //exit_program(1);
        return -1;
    }

    // 9. 关闭解码器
    /* close each decoder */
    for (i = 0; i < nb_input_streams; i++) {
        ist = input_streams[i];
        if (ist->decoding_needed) {
            avcodec_close(ist->dec_ctx);// 关闭解码器
            if (ist->hwaccel_uninit)
                ist->hwaccel_uninit(ist->dec_ctx);
        }
    }

    // 10. 释放硬件相关
    av_buffer_unref(&hw_device_ctx);
    hw_device_free_all();

    /* finished ! */
    ret = 0;

 fail:
#if HAVE_THREADS
    free_input_threads();
#endif

    if (output_streams) {
        for (i = 0; i < nb_output_streams; i++) {
            ost = output_streams[i];
            if (ost) {
                if (ost->logfile) {
                    if (fclose(ost->logfile)){
                        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
                        av_log(NULL, AV_LOG_ERROR,
                               "Error closing logfile, loss of information possible: %s\n",
                               av_err2str_ex(str, AVERROR(errno)));
                    }
                    ost->logfile = NULL;
                }
                av_freep(&ost->forced_kf_pts);
                av_freep(&ost->apad);
                av_freep(&ost->disposition);
                av_dict_free(&ost->encoder_opts);
                av_dict_free(&ost->sws_dict);
                av_dict_free(&ost->swr_opts);
                av_dict_free(&ost->resample_opts);
            }
        }
    }

    return ret;
}

int FFmpegMedia::start(){
    int i, ret;
    BenchmarkTimeStamps ti;

    init_dynload();

    //register_exit(ffmpeg_cleanup);

    setvbuf(stderr,NULL,_IONBF,0); /* win32 runtime needs this */

    av_log_set_flags(AV_LOG_SKIP_REPEATED);
    //parse_loglevel(argc, argv, options);

//    if(argc>1 && !strcmp(argv[1], "-d")){
//        run_as_daemon=1;
//        av_log_set_callback(log_callback_null);// 这样可以避免打日志？
//        argc--;
//        argv++;
//    }

#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avformat_network_init();

    //show_banner(argc, argv, options);

    /* parse options and open all input/output files */
    ret = ffmpeg_parse_options(argc, argv);
    if (ret < 0){
        //exit_program(1);
        return -1;
    }

    if (nb_output_files <= 0 && nb_input_files == 0) {
        //show_usage();
        av_log(NULL, AV_LOG_WARNING, "Use -h to get full help or, even better, run 'man %s'\n", program_name);
        //exit_program(1);
        return -1;
    }

    /* file converter / grab */
    if (nb_output_files <= 0) {
        av_log(NULL, AV_LOG_FATAL, "At least one output file must be specified\n");
        //exit_program(1);
        return -1;
    }

    for (i = 0; i < nb_output_files; i++) {
        if (strcmp(output_files[i]->ctx->oformat->name, "rtp"))
            want_sdp = 0;
    }

    current_time = ti = get_benchmark_time_stamps();
    if ((ret = transcode()) < 0){
        av_log(NULL, AV_LOG_FATAL, "transcode() failed, ret: %d.\n", ret);
        //exit_program(1);
        return ret;
    }

    if (do_benchmark) {
        int64_t utime, stime, rtime;
        current_time = get_benchmark_time_stamps();
        utime = current_time.user_usec - ti.user_usec;
        stime = current_time.sys_usec  - ti.sys_usec;
        rtime = current_time.real_usec - ti.real_usec;
        av_log(NULL, AV_LOG_INFO,
               "bench: utime=%0.3fs stime=%0.3fs rtime=%0.3fs\n",
               utime / 1000000.0, stime / 1000000.0, rtime / 1000000.0);
    }

    av_log(NULL, AV_LOG_DEBUG, "%" PRIu64" frames successfully decoded, %" PRIu64" decoding errors\n",
           decode_error_stat[0], decode_error_stat[1]);
    if ((decode_error_stat[0] + decode_error_stat[1]) * max_error_rate < decode_error_stat[1]){
        //exit_program(69);
    }

    // 调用ffmpeg_cleanup. 因为上面注册回调是使用ffmpeg_cleanup注册的.
    //exit_program(received_nb_signals ? 255 : main_return_code);// 让析构去掉

    return main_return_code;
}

int FFmpegMedia::start_async(){
    int ret = ThreadWrapper::start();
    if(ret == -1){
        printf("start_async 线程开启失败\n");
        return -1;
    }

    return 0;
}

void FFmpegMedia::loop(){
//    while(true){
//        if(_request_abort){
//            break;
//        }
//    }
    this->start();

    printf("loop线程函数退出\n");
}


/* _WIN32 means using the windows libc - cygwin doesn't define that
 * by default. HAVE_COMMANDLINETOARGVW is true on cygwin, while
 * it doesn't provide the actual command line via GetCommandLineW(). */
#if HAVE_COMMANDLINETOARGVW && defined(_WIN32)
#include <shellapi.h>
/* Will be leaked on exit */
//static char** win32_argv_utf8 = NULL;
//static int win32_argc = 0;

/**
 * Prepare command line arguments for executable.
 * For Windows - perform wide-char to UTF-8 conversion.
 * Input arguments should be main() function arguments.
 * @param argc_ptr Arguments number (including executable)
 * @param argv_ptr Arguments list.
 */
// 宽字节转成多字节
void FFmpegMedia::prepare_app_arguments(int *argc_ptr, char ***argv_ptr)
{
    char *argstr_flat;
    wchar_t **argv_w;
    int i, buffsize = 0, offset = 0;

    if (win32_argv_utf8) {
        *argc_ptr = win32_argc;
        *argv_ptr = win32_argv_utf8;
        return;
    }

    // 1. CommandLineToArgvW是获取命令行的参数以及个数，等价于main中的argc、argv.
    win32_argc = 0;
    argv_w = CommandLineToArgvW(GetCommandLineW(), &win32_argc);
    if (win32_argc <= 0 || !argv_w)
        return;
    //printf("argc: %d. argv: %s\n", win32_argc, argv_w);

    // 2. 获取命令行所有参数的总字节大小.
    // 因为这里获取到的argv是宽字节，所以需要用WideCharToMultiByte获取字节数.
    /* determine the UTF-8 buffer size (including NULL-termination symbols) */
    for (i = 0; i < win32_argc; i++)
        buffsize += WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1,
                                        NULL, 0, NULL, NULL);

    win32_argv_utf8 = (char** )av_mallocz(sizeof(char *) * (win32_argc + 1) + buffsize);
    argstr_flat     = (char *)win32_argv_utf8 + sizeof(char *) * (win32_argc + 1);
    if (!win32_argv_utf8) {
        LocalFree(argv_w);
        return;
    }

    // 3. 将宽字节转换，保存到argstr_flat中
    for (i = 0; i < win32_argc; i++) {
        win32_argv_utf8[i] = &argstr_flat[offset];
        offset += WideCharToMultiByte(CP_UTF8, 0, argv_w[i], -1,
                                      &argstr_flat[offset],
                                      buffsize - offset, NULL, NULL);
    }
    win32_argv_utf8[i] = NULL;
    LocalFree(argv_w);

    *argc_ptr = win32_argc;
    *argv_ptr = win32_argv_utf8;
}

#else

inline void FFmpegMedia::prepare_app_arguments(int *argc_ptr, char ***argv_ptr)
{
    /* nothing to do */
}

#endif /* HAVE_COMMANDLINETOARGVW */

void FFmpegMedia::init_opts(){
    // 一个与视频分辨率有关的参数.
    // detail see https://www.csdn.net/tags/MtTaEgysNDE1MDc3LWJsb2cO0O0O.html.
    av_dict_set(&sws_dict, "flags", "bicubic", 0);
}

/**
 * @brief 初始化octx
 * @param octx 参数上下文
 * @param groups 全局静态数组groups.
 * @param nb_groups 全局静态数组groups的大小，本版固定为2个.
*/
int FFmpegMedia::init_parse_context(OptionParseContext *octx,
                               const OptionGroupDef *groups, int nb_groups)
{
    static const OptionGroupDef global_group = { "global" };
    int i;

    // 1. octx清0.
    memset(octx, 0, sizeof(*octx));

    // 2. 为输入输出文件开辟空间,所以OptionGroupList就代表存储输入输出文件的链表.
    octx->nb_groups = nb_groups;
    octx->groups    = (OptionGroupList *)av_mallocz_array(octx->nb_groups, sizeof(*octx->groups));
    if (!octx->groups){
        //exit_program(1);
        return -1;
    }

    // 3. 给输入、输出、全局选项赋予OptionGroupDef值.
    for (i = 0; i < octx->nb_groups; i++)
        octx->groups[i].group_def = &groups[i];// 指向对应的def,下标0:输出,1:输入.

    octx->global_opts.group_def = &global_group;// 全局选项的def,所以上面定义global_group使用了static
    octx->global_opts.arg       = "";

    // 4. 设置视频转码参数选项.
    init_opts();

    return 0;
}

/**
 * @brief
 * @param array 要扩充的数组
 * @param elem_size 该数组单个元素的字节大小
 * @param size array数组中已有的元素个数，不过一般传0，调用完成后得到array的总元素个数.
 * @param new_size 新数组中元素的总个数，包含原有size个元素
 * @return 成功返回新开辟的数组首地址，并且总元素个数通过传出参数size传出.
 *          失败看av_realloc_array说明.
*/
void *FFmpegMedia::grow_array(void *array, int elem_size, int *size, int new_size)
{
    // 1. 判断要开辟的元素是否超过int的最大元素开辟个数.
    // INT_MAX是最大开辟字节数，elem_size是单个元素占的字节数，故除以后得到最大开辟元素个数.
    if (new_size >= INT_MAX / elem_size) {
        av_log(NULL, AV_LOG_ERROR, "Array too big.\n");
        //exit_program(1);
        return nullptr;
    }

    // 2. 若要开辟新内存的大小，小于原来内存的大小，则不做处理，返回原来的数组.
    if (*size < new_size) {
        /*
         * av_realloc_array: Allocate, reallocate, or free an array.
         * 具体看该函数说明即可.
        */
        uint8_t *tmp = (uint8_t *)av_realloc_array(array, new_size, elem_size);
        if (!tmp) {
            av_log(NULL, AV_LOG_ERROR, "Could not alloc buffer.\n");
            //exit_program(1);
            return nullptr;
        }
        // 跳过原来的数据，对重新开辟的内存清零(读者这里看不懂需要补充realloc的相关知识)
        // tmp + *size*elem_size代表跳过原来的数据，(new_size-*size) * elem_size代表在原来数据后面开辟的字节数
        memset(tmp + *size*elem_size, 0, (new_size-*size) * elem_size);
        *size = new_size;// 记录新数组的元素总个数，通过参数传出.
        return tmp;
    }
    return array;
}

/*
 * Finish parsing an option group.
 *
 * @param group_idx which group definition should this group belong to
 * @param arg argument of the group delimiting option
 */
/**
 * @brief 扫描到i选项，说明已经处理完一个输入文件的参数，那么在数组OptionGroup *groups中新建一个OptionGroup *g元素用于保存这个输入文件的参数.
 * @param octx
 * @param group_idx 0或者1，0:输入文件,1:输出文件.
 * @param arg 文件名.本程序指的文件名可以是文件或者实时流.
 */
int FFmpegMedia::finish_group(OptionParseContext *octx, int group_idx,
                         const char *arg)
{
    OptionGroupList *l = &octx->groups[group_idx];
    OptionGroup *g;

    // 1. 往OptionGroupList中的OptionGroup数组增加一个元素.
    //GROW_ARRAY(l->groups, l->nb_groups);
    GROW_ARRAY_EX(l->groups, OptionGroup *, l->nb_groups);
    //l->groups = (OptionGroup *)grow_array(l->groups, sizeof(*l->groups), &l->nb_groups, l->nb_groups + 1);
    if(!l->groups){
        return -1;
    }
    g = &l->groups[l->nb_groups - 1];// 获取新增的元素,用于临时操作.

    // 2. 赋值.
    *g             = octx->cur_group;// 这里实际赋值Option *opts以及int  nb_opts,因为临时选项cur_group在分割时只得到这两个内容
    g->arg         = arg;            // 文件名
    g->group_def   = l->group_def;   // input url或者是output url的描述
    g->sws_dict    = sws_dict;
    g->swr_opts    = swr_opts;
    g->codec_opts  = codec_opts;
    g->format_opts = format_opts;
    g->resample_opts = resample_opts;

    codec_opts  = NULL;
    format_opts = NULL;
    resample_opts = NULL;
    sws_dict    = NULL;
    swr_opts    = NULL;
    init_opts();

    // 3. 清空本次的cur_group.
    memset(&octx->cur_group, 0, sizeof(octx->cur_group));

    return 0;
}

/*
 * Check whether given option is a group separator.
 *
 * @return index of the group definition that matched or -1 if none
 */
/**
 * @brief 返回输入文件的下标,如果不是则返回-1.
 * @param groups
 * @param nb_groups 静态数组的大小，本程序固定是2
 * @param opt 用户传进的key去掉"-"后的字符串,例如-re去掉"-",opt就是re字符串.
 * @return 返回输入文件的下标,如果不是则返回-1.
*/
int FFmpegMedia::match_group_separator(const OptionGroupDef *groups, int nb_groups,
                                 const char *opt)
{
    int i;

    // 这里应该只是找输入文件在argv数组的下标.因为输出文件的sep是空，在这里会返回-1.
    for (i = 0; i < nb_groups; i++) {
        const OptionGroupDef *p = &groups[i];
        if (p->sep && !strcmp(p->sep, opt))
            return i;
    }

    return -1;
}

/**
 * @brief 判断name是否在全局静态数组po.
 * @param po 指向全局静态数组options[].
 * @param name 用户传进key，不带"-"
 * @return 找到返回对应OptionDef,否则返回NULL.
*/
const OptionDef *FFmpegMedia::find_option(const OptionDef *po, const char *name)
{
    // 1. 判断name是否有":".例如profile:v
    // 如果有":",它只会判断":"之前的长度，例如profile:v只会匹配profile这len=7的长度.
    const char *p = strchr(name, ':');// 若找到该子串,返回从该子串开始的后面所有字符串,包括该下标.
    int len = p ? p - name : strlen(name);

    while (po->name) {
        if (!strncmp(name, po->name, len) && strlen(po->name) == len)
            break;
        po++;
    }
    return po;
}

/*
 * Add an option instance to currently parsed group.
 */
/**
 * @brief 往全局选项或者临时选项的Option数组新增一个元素.
 * @param octx
 * @param opt ffmpeg官方定义的元素说明,在全局静态数组options中.
 * @param key 用户传进的key,不带"-"
 * @param val 用户或者ffmpeg添加的值
 */
int FFmpegMedia::add_opt(OptionParseContext *octx, const OptionDef *opt,
                    const char *key, const char *val)
{
    // 1. 判断该选项opt是否是全局,是g则指向全局选项,不是则指向临时选项.
    // 判断依据：只要opt->flags带有OPT_PERFILE或者OPT_SPEC或者OPT_OFFSET其中一个标志位，则说明不是全局.
    // 注：ffmpeg设置OPT_PERFILE、OPT_SPEC、OPT_OFFSET这些宏时，刚好是按照二进制的每一个bit去设计的，一个宏只会占用二进制的一个bit.共定义了19个宏.
    // 例如re参数,带有OPT_OFFSET,运算：1110 0000 0000 0000 & 0100 0000 0000 0000 = 0100 0000 0000 0000，取反后global=0,说明是非全局参数.
    int tyyb = (OPT_PERFILE | OPT_SPEC | OPT_OFFSET);
    int tyyt = opt->flags & tyyb;
    int global = !(opt->flags & (OPT_PERFILE | OPT_SPEC | OPT_OFFSET));
    OptionGroup *g = global ? &octx->global_opts : &octx->cur_group;

    // 2. 往全局或者临时选项内的Option数组新增一个元素，g->nb_opts自动加1.
    // 然后赋值，opt是ffmpeg官方定义的元素说明.key是用户的key，val是用户或者ffmpeg添加的值.
    //GROW_ARRAY(g->opts, g->nb_opts);
    GROW_ARRAY_EX(g->opts, Option *, g->nb_opts);
    //g->opts = (Option *)grow_array(g->opts, sizeof(*g->opts), &g->nb_opts, g->nb_opts + 1);
    if(!g->opts){
        return -1;
    }

    g->opts[g->nb_opts - 1].opt = opt;
    g->opts[g->nb_opts - 1].key = key;
    g->opts[g->nb_opts - 1].val = val;

    return 0;
}

const AVOption *FFmpegMedia::opt_find(void *obj, const char *name, const char *unit,
                            int opt_flags, int search_flags)
{
    /*
     av_opt_find函数:
     在对象中查找一个选项。只考虑设置了所有指定标志的选项。
     obj:指向一个第一个元素是AVClass指针的struct的指针。另外，如果设置了AV_OPT_SEARCH_FAKE_OBJ搜索标志，则指向AVClass的双指针。
     name:要查找的选项的名称.
     unit:当搜索命名常量时，它所属的单位名称。
     opt_flags:只查找所有指定标志设置的选项(AV_OPT_FLAG)。
     search_flags:AV_OPT_SEARCH_*的组合。

     return:返回一个指向找到的选项的指针，如果没有找到，则返回NULL。
     note:带有AV_OPT_SEARCH_CHILDREN标志的选项不能直接使用av_opt_set()设置。使用带有AVDictionary选项的特殊调用(例如avformat_open_input())来设置带有该标志的选项。
    */
    const AVOption *o = av_opt_find(obj, name, unit, opt_flags, search_flags);
    if(o && !o->flags)
        return NULL;
    return o;
}

/**
 * @brief 将字符串类型的数字转成对应的数值类型.
 * @param context 选项的key
 * @param numstr 选项的val
 * @param type 选项val数值的类型
 * @param min 选项val数值的类型的最小值
 * @param max 选项val数值的类型的最大值
 * @return 成功返回解析后的数值,失败退出程序.
*/
double FFmpegMedia::parse_number_or_die(const char *context, const char *numstr, int type,
                           double min, double max)
{
    char *tail;
    const char *error;
    /*
        av_strtod函数：在numstr中解析字符串并返回其值为双精度值。如果字符串为空，只包含空白，或不包含具有浮点数预期语法的初始子字符串，则不执行转换。
        在本例中，返回值为零，在tail中返回的值为numstr的值。
        1：param numstr：是一个表示数字的字符串，可能包含一个国际系统的数字后缀，例如'K'， 'M'， 'G'。
        如果在后缀后面加上'i'，则使用2的幂而不是10的幂。后缀“B”将该值乘以8，可以添加到另一个后缀之后，
        也可以单独使用。这允许使用'KB'， 'MiB'， 'G'和'B'作为后缀。
        2：param tail：如果非null将指向字符的指针放在最后一个解析字符之后。
        // 具体可以参考这篇类似的文章：https://blog.csdn.net/qianshanxue11/article/details/50728442
    */
    double d = av_strtod(numstr, &tail);
    if (*tail)// numstr出现非法字符？
        error = "Expected number for %s but found: %s\n";
    else if (d < min || d > max)// 越界
        error = "The value for %s was %s which is not within %f - %f\n";
    else if (type == OPT_INT64 && (int64_t)d != d)// 数字是64位的int，但是与doblue不匹配，同样说明越界.
        error = "Expected int64 for %s but found %s\n";
    else if (type == OPT_INT && (int)d != d)// 数字是32位的int，但是与doblue不匹配，同样说明越界.
        error = "Expected int for %s but found %s\n";
    else
        return d;

    av_log(NULL, AV_LOG_FATAL, error, context, numstr, min, max);
    //exit_program(1);
    return 0;
}

int64_t FFmpegMedia::parse_time_or_die(const char *context, const char *timestr,
                          int is_duration)
{
    int64_t us;
    if (av_parse_time(&us, timestr, is_duration) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Invalid %s specification for %s: %s\n",
               is_duration ? "duration" : "date", context, timestr);
        //exit_program(1);
        return -1;// 注意: 解析时间被我改成默认返回-1.
    }
    return us;
}

/**
 * @brief 将保存在OptionParseContext的参数写进OptionsContext o(即参数optctx)变量中.
 * @param optctx
 * @param po ffmpeg官方定义的options数组里面的元素之一.
 * @param opt 选项的key
 * @param arg 选项的val
*/
int FFmpegMedia::write_option(void *optctx, const OptionDef *po, const char *opt,
                        const char *arg)
{
    /* new-style options contain an offset into optctx, old-style address of
     * a global var*/
    // 1. 获取OptionsContext o(即optctx)成员中的偏移地址或者回调函数，
    // 当我们操作该指针时，即可保存对应的值到OptionsContext中。
    void *dst = po->flags & (OPT_OFFSET | OPT_SPEC) ?
                (uint8_t *)optctx + po->u.off : po->u.dst_ptr;//得到该成员的内存地址，例如是SpecifierOpt *codec_names，返回的是&codec_names
    int *dstcount;//如果数据是SpecifierOpt *类型，对应的存入链表成员个数的变量地址,例如int nb_codec_names，返回的是&nb_codec_names

    // 2. 判断得到的成员dts的类型.即dst在OptionsContext中对应为SpecifierOpt类型的变量
    // 看这里必须printf打印出来，因为qt debug时可能显示的值不正确!!!
    if (po->flags & OPT_SPEC) {
        SpecifierOpt **so = (SpecifierOpt **)dst;// 用临时的二级指针变量指向dst，方便操作，且因为dst是void*，不方便直接操作内部成员.
                                // 注：dst是指向成员的内存地址，所以so此时也是指向成员地址，这点非常重要.
        // 注意：这里打印很重要，因为qt debug时看到so指向0x0，这是不正确的，打印出来so不是0x0，我被qt的debug害得好惨.
        printf("write_option so addr: %#X, *so addr: %#X, dst: %#X\n",
               so, *so, dst);// 猜想:此时so、dst指向一样，都是指向成员指针的内存地址，而*so则是成员指针的值.猜想正确
                             // 以codec_names为例，so=dst=&codec_names，*so=codec_names。

        char *p = strchr(opt, ':');// 查找是否有子串':'，有则返回该下标开始及后面的字符串
        char *str;

        /*利用dst的偏移地址获取下一个成员的偏移地址，用于记录dst的数量.
         * 注意，ffmpeg的OptionsContext设计，只有带有SpecifierOpt*类型的变量，下一个成员必是int nb_xxx的成员.
        例如：
        SpecifierOpt *codec_names;
        int        nb_codec_names;*/
        dstcount = (int *)(so + 1);
        printf("write_option so addr: %#X, *so addr: %#X, dst: %#X\n",
               so, *so, dst);
        *so = (SpecifierOpt *)grow_array(*so, sizeof(**so), dstcount, *dstcount + 1);// SpecifierOpt数组扩容，每次加1个元素的大小.
        printf("write_option so addr: %#X, *so addr: %#X, so[*dstcount-1] addr: %#X, dst: %#X\n",
               so, *so, so[*dstcount-1], dst);

        // 将冒号后面的字符串存入此，p一般为:v :a :s :d，那么str就变成v a s d，av_strdup会自动开辟对应内存
        str = av_strdup(p ? p + 1 : "");
        if (!str)
            return AVERROR(ENOMEM);

        (*so)[*dstcount - 1].specifier = str;// 将str赋值给 SpecifierOpt数组末尾元素的specifier，即新开辟的SpecifierOpt元素
        dst = &(*so)[*dstcount - 1].u;// dst指向新开辟元素的共用体u的地址，方便后面进行使用该共用体保存对应的值
    }

    // 3. 到这一步，我们就发现dst指向u的内存地址的作用了，u是实际存放对应值的内容，可以存放数值型以及字符串，因为u是一个共用体.
    // 3.1 如果key是字符串类型.
    if (po->flags & OPT_STRING) {
        char *str;
        str = av_strdup(arg);// 为val值开辟内存
        av_freep(dst);//释放*dst的内存，实际上SpecifierOpt的成员specifier以及u都是没有分配内存的，需要自己分配，不过av_freep释放NULL是没问题的.
        if (!str)
            return AVERROR(ENOMEM);
        *(char **)dst = str;
    } else if (po->flags & OPT_BOOL || po->flags & OPT_INT) {// 布尔以及下面的数值型都是使用parse_number_or_die处理，比较简单
        *(int *)dst = parse_number_or_die(opt, arg, OPT_INT64, INT_MIN, INT_MAX);
    } else if (po->flags & OPT_INT64) {
        *(int64_t *)dst = parse_number_or_die(opt, arg, OPT_INT64, INT64_MIN, INT64_MAX);
    } else if (po->flags & OPT_TIME) {
        *(int64_t *)dst = parse_time_or_die(opt, arg, 1);
    } else if (po->flags & OPT_FLOAT) {
        *(float *)dst = parse_number_or_die(opt, arg, OPT_FLOAT, -INFINITY, INFINITY);
    } else if (po->flags & OPT_DOUBLE) {
        *(double *)dst = parse_number_or_die(opt, arg, OPT_DOUBLE, -INFINITY, INFINITY);
    } else if (po->u.func_arg) {// 回调参数处理
        //int ret = po->u.func_arg(optctx, opt, arg);
        int ret = po->u.func_arg(this, optctx, opt, arg);// tyycode
        if (ret < 0) {
            char str[AV_ERROR_MAX_STRING_SIZE] = {0};
            av_log(NULL, AV_LOG_ERROR,
                   "Failed to set value '%s' for option '%s': %s\n",
                   arg, opt, av_err2str_ex(str, ret));
            return ret;
        }
    }

    if (po->flags & OPT_EXIT){
        av_log(NULL, AV_LOG_WARNING, "OPT_EXIT is invaild\n");
        //exit_program(0);
    }

    return 0;
}

int FFmpegMedia::parse_optgroup(void *optctx, OptionGroup *g)
{
    int i, ret;

    av_log(NULL, AV_LOG_DEBUG, "Parsing a group of options: %s %s.\n",
           g->group_def->name, g->arg);

    // 1. 遍历该选项组(一般指一个文件)包含的选项.
    for (i = 0; i < g->nb_opts; i++) {
        Option *o = &g->opts[i];// 获取该选项组的一个选项.

        // 2. 检测用户输入参数的语法是否有误.
        // g->group_def->flags代表按语法顺序去解析用户输入的选项时得到的flags，而o->opt->flags就是官方的flags
        if (g->group_def->flags &&
            !(g->group_def->flags & o->opt->flags)) {
            av_log(NULL, AV_LOG_ERROR, "Option %s (%s) cannot be applied to "
                   "%s %s -- you are trying to apply an input option to an "
                   "output file or vice versa. Move this option before the "
                   "file it belongs to.\n", o->key, o->opt->help,
                   g->group_def->name, g->arg);
            return AVERROR(EINVAL);
        }

        av_log(NULL, AV_LOG_DEBUG, "Applying option %s (%s) with argument %s.\n",
               o->key, o->opt->help, o->val);

        // 3. 该选项无误则写入optctx,正常选项时optctx是OptionsContext,全局选项时是NULL.
        ret = write_option(optctx, o->opt, o->key, o->val);
        if (ret < 0)
            return ret;
    }

    av_log(NULL, AV_LOG_DEBUG, "Successfully parsed a group of options.\n");

    return 0;
}

#define FLAGS (o->type == AV_OPT_TYPE_FLAGS && (arg[0]=='-' || arg[0]=='+')) ? AV_DICT_APPEND : 0
int FFmpegMedia::opt_default(void *optctx, const char *opt, const char *arg)
{
    const AVOption *o;
    int consumed = 0;
    char opt_stripped[128];// 临时数组
    const char *p;
    const AVClass *cc = avcodec_get_class(), *fc = avformat_get_class();
#if CONFIG_AVRESAMPLE
    const AVClass *rc = avresample_get_class();
#endif
#if CONFIG_SWSCALE
    const AVClass *sc = sws_get_class();
#endif
#if CONFIG_SWRESAMPLE
    const AVClass *swr_class = swr_get_class();
#endif

    if (!strcmp(opt, "debug") || !strcmp(opt, "fdebug"))
        av_log_set_level(AV_LOG_DEBUG);

    // 判断opt选项是否带":",若带p则指向":"下标;若不带p则指向opt的末尾.
    if (!(p = strchr(opt, ':')))
        p = opt + strlen(opt);

    // 将opt的内容拷贝至opt_stripped临时数组.p - opt + 1，加1是确保opt后面的结束符也能被拷贝。
    // 例如opt=level参数.
    av_strlcpy(opt_stripped, opt, FFMIN(sizeof(opt_stripped), p - opt + 1));

    // 1. 查找该选项是否属于编解码器,若是则设置到codec_opts.不是则继续往下判断.
    // 例如-re -stream_loop -1 -an  -i aGanZhengChuan-av.mp4 -vcodec libx264 -profile:v main -level 3.1
    // -preset veryfast -tune zerolatency -b:v 1000K -maxrate 1000K -minrate 1000K -bufsize 2000K -s 704x576 -r 25
    // -keyint_min 50 -g 50 -sc_threshold 0 -an -shortest -f flv rtmp://192.168.1.118:1935/live/tyy
    // 这几个选项都是编解码器的选项：level preset tune maxrate、minrate、bufsize、keyint_min、g、sc_threshold.
    // 其中因为我们这里使用的编解码器是libx264，所以level preset tune所以可以去libavcodec/libx264.c文件找到对应的static const AVOption options[]定义
    // 而maxrate、minrate、bufsize、keyint_min、g、sc_threshold在libavcodec/options_table.h
    if ((o = opt_find(&cc, opt_stripped, NULL, 0,
                         AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ)) ||
        ((opt[0] == 'v' || opt[0] == 'a' || opt[0] == 's') &&
         (o = opt_find(&cc, opt + 1, NULL, 0, AV_OPT_SEARCH_FAKE_OBJ)))) {
        av_dict_set(&codec_opts, opt, arg, FLAGS);
        consumed = 1;
    }

    // 2. 查找该选项是否属于解复用器.
    if ((o = opt_find(&fc, opt, NULL, 0,
                         AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
        av_dict_set(&format_opts, opt, arg, FLAGS);
        if (consumed)
            av_log(NULL, AV_LOG_VERBOSE, "Routing option %s to both codec and muxer layer\n", opt);
        consumed = 1;
    }

    // 3. 查找该选项是否属于视频转码参数选项.
#if CONFIG_SWSCALE
    if (!consumed && (o = opt_find(&sc, opt, NULL, 0,
                         AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
        struct SwsContext *sws = sws_alloc_context();
        int ret = av_opt_set(sws, opt, arg, 0);
        sws_freeContext(sws);
        if (!strcmp(opt, "srcw") || !strcmp(opt, "srch") ||
            !strcmp(opt, "dstw") || !strcmp(opt, "dsth") ||
            !strcmp(opt, "src_format") || !strcmp(opt, "dst_format")) {
            av_log(NULL, AV_LOG_ERROR, "Directly using swscale dimensions/format options is not supported, please use the -s or -pix_fmt options\n");
            return AVERROR(EINVAL);
        }
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error setting option %s.\n", opt);
            return ret;
        }

        av_dict_set(&sws_dict, opt, arg, FLAGS);

        consumed = 1;
    }
#else
    if (!consumed && !strcmp(opt, "sws_flags")) {
        av_log(NULL, AV_LOG_WARNING, "Ignoring %s %s, due to disabled swscale\n", opt, arg);
        consumed = 1;
    }
#endif

    // 4. 查找该选项是否属于重采样选项？
#if CONFIG_SWRESAMPLE
    if (!consumed && (o=opt_find(&swr_class, opt, NULL, 0,
                                    AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
        struct SwrContext *swr = swr_alloc();
        int ret = av_opt_set(swr, opt, arg, 0);
        swr_free(&swr);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error setting option %s.\n", opt);
            return ret;
        }
        av_dict_set(&swr_opts, opt, arg, FLAGS);
        consumed = 1;
    }
#endif
#if CONFIG_AVRESAMPLE
    if ((o=opt_find(&rc, opt, NULL, 0,
                       AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
        av_dict_set(&resample_opts, opt, arg, FLAGS);
        consumed = 1;
    }
#endif

    if (consumed)
        return 0;
    return AVERROR_OPTION_NOT_FOUND;
}

/**
 * @breif 将用户输入的命令行参数进行分割.主要分为四大类，输入文件、输出文件，正常选项，AVoptions选项.
 *          如果是AVoptions选项，会直接先设置到对应的AVDictionary字典.
 * @param octx
 * @param argc 用户输入的参数个数，包含可执行程序本身.
 * @param argv 参数数组，包含可执行程序本身.
 * @param options ffmpeg定义的全局静态options数组.
 * @param groups ffmpeg定义的全局静态groups数组,只有输入输出文件两个元素.
 * @param nb_groups groups数组的个数,固定为2.
 * @return >= 成功; <0 失败
*/
int FFmpegMedia::split_commandline(OptionParseContext *octx, int argc, char *argv[],
                      const OptionDef *options,
                      const OptionGroupDef *groups, int nb_groups)
{
    int optindex = 1;
    int dashdash = -2;

    // 1. 将宽字节转成多字节.
    /* perform system-dependent conversions for arguments list */
    prepare_app_arguments(&argc, &argv);

    // 2. 初始化octx
    int ret = init_parse_context(octx, groups, nb_groups);
    if(ret < 0){
        return -1;
    }

    av_log(NULL, AV_LOG_DEBUG, "Splitting the commandline.\n");

    while (optindex < argc) {// 这里说明执行的命令至少要有一个参数才能进来.例如ffmpeg.exe表示只有程序名而无参数,这里不会进来
        const char *opt = argv[optindex++], *arg;
        const OptionDef *po;
        int ret;

        av_log(NULL, AV_LOG_DEBUG, "Reading option '%s' ...", opt);

        // 遇到--选项跳过处理?
        if (opt[0] == '-' && opt[1] == '-' && !opt[2]) {
            dashdash = optindex;
            continue;
        }

        // 3. 处理输出文件
        /* unnamed group separators, e.g. output filename */
        if (opt[0] != '-' || !opt[1] || dashdash+1 == optindex) {
            ret = finish_group(octx, 0, opt);
            if(ret < 0){
                return -1;
            }
            av_log(NULL, AV_LOG_DEBUG, " matched as %s.\n", groups[0].name);
            continue;
        }

        // 来到这里说明key是只带一个"-"的key，例如"-re"，那么我们跳过"-",执行opt++后，此时opt的值为"re"
        opt++;

#define GET_ARG(arg)                                                           \
do {                                                                           \
    arg = argv[optindex++];                                                    \
    if (!arg) {                                                                \
        av_log(NULL, AV_LOG_ERROR, "Missing argument for option '%s'.\n", opt);\
        return AVERROR(EINVAL);                                                \
    }                                                                          \
} while (0)

        // 4. 判断是否是输入文件的key，即i选项.
        /* named group separators, e.g. -i */
        if ((ret = match_group_separator(groups, nb_groups, opt)) >= 0) {
            GET_ARG(arg);
            ret = finish_group(octx, ret, arg);// 完成一个组的参数处理.
            if(ret < 0){
                return -1;
            }
            av_log(NULL, AV_LOG_DEBUG, " matched as %s with argument '%s'.\n",
                   groups[ret].name, arg);
            continue;
        }


        // 5. 从ffmpeg定义options数组中获取对应的OptionDef元素,若找到说明是正常选项，
        //      则将其放进临时选项cur_group，待找到输入或者输出文件时，再放进参数上下文octx.
        /* normal options */
        po = find_option(options, opt);
        if (po->name) {
            if (po->flags & OPT_EXIT) {
                /* optional argument, e.g. -h */
                arg = argv[optindex++];
            } else if (po->flags & HAS_ARG) {
                GET_ARG(arg);// key带val的，走这个流程
            } else {
                arg = "1";// key不带val的，走这个流程
            }

            ret = add_opt(octx, po, opt, arg);
            if(ret < 0){
                return -1;
            }
            av_log(NULL, AV_LOG_DEBUG, " matched as option '%s' (%s) with "
                   "argument '%s'.\n", po->name, po->help, arg);
            continue;
        }

        // 6. 如果是AVOptions(解复用、编解码器、重采样、视频sws相关选项)，则会直接设置到对应的AVDictionary字典.
        /* AVOptions */
        if (argv[optindex]) {
            ret = opt_default(NULL, opt, argv[optindex]);
            if (ret >= 0) {
                av_log(NULL, AV_LOG_DEBUG, " matched as AVOption '%s' with "
                       "argument '%s'.\n", opt, argv[optindex]);
                optindex++;
                continue;
            } else if (ret != AVERROR_OPTION_NOT_FOUND) {
                av_log(NULL, AV_LOG_ERROR, "Error parsing option '%s' "
                       "with argument '%s'.\n", opt, argv[optindex]);
                return ret;
            }
        }

        // 7. 这里应该布尔相关的,暂未分析.
        /* boolean -nofoo options */
        if (opt[0] == 'n' && opt[1] == 'o' &&
            (po = find_option(options, opt + 2)) &&
            po->name && po->flags & OPT_BOOL) {
            add_opt(octx, po, opt, "0");
            av_log(NULL, AV_LOG_DEBUG, " matched as option '%s' (%s) with "
                   "argument 0.\n", po->name, po->help);
            continue;
        }

        av_log(NULL, AV_LOG_ERROR, "Unrecognized option '%s'.\n", opt);
        return AVERROR_OPTION_NOT_FOUND;
    }//<== while (optindex < argc) end ==>

    // 判断输出文件后面是否有拖尾选项,有则报警告.
    // 例如ffmpeg -i input.mp4 output.mp4 -an，那么octx->cur_group.nb_opts=1,说明有一个拖尾选项，即-an
    if (octx->cur_group.nb_opts || codec_opts || format_opts || resample_opts)
        av_log(NULL, AV_LOG_WARNING, "Trailing options were found on the "
               "commandline.\n");

    // 来到这里表示完成命令行分割.
    av_log(NULL, AV_LOG_DEBUG, "Finished splitting the commandline.\n");

    return 0;
}

int FFmpegMedia::opt_map(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    StreamMap *m = NULL;
    int i, negative = 0, file_idx, disabled = 0;
    int sync_file_idx = -1, sync_stream_idx = 0;
    char *p, *sync;
    char *map;
    char *allow_unused;

    if (*arg == '-') {
        negative = 1;
        arg++;
    }
    map = av_strdup(arg);
    if (!map)
        return AVERROR(ENOMEM);

    /* parse sync stream first, just pick first matching stream */
    if (sync = strchr(map, ',')) {
        *sync = 0;
        sync_file_idx = strtol(sync + 1, &sync, 0);
        if (sync_file_idx >= nb_input_files || sync_file_idx < 0) {
            av_log(NULL, AV_LOG_FATAL, "Invalid sync file index: %d.\n", sync_file_idx);
            //exit_program(1);
            return -1;
        }
        if (*sync)
            sync++;
        for (i = 0; i < input_files[sync_file_idx]->nb_streams; i++)
            if (check_stream_specifier(input_files[sync_file_idx]->ctx,
                                       input_files[sync_file_idx]->ctx->streams[i], sync) == 1) {
                sync_stream_idx = i;
                break;
            }
        if (i == input_files[sync_file_idx]->nb_streams) {
            av_log(NULL, AV_LOG_FATAL, "Sync stream specification in map %s does not "
                                       "match any streams.\n", arg);
            //exit_program(1);
            return -1;
        }
        if (input_streams[input_files[sync_file_idx]->ist_index + sync_stream_idx]->user_set_discard == AVDISCARD_ALL) {
            av_log(NULL, AV_LOG_FATAL, "Sync stream specification in map %s matches a disabled input "
                                       "stream.\n", arg);
            //exit_program(1);
            return -1;
        }
    }


    if (map[0] == '[') {
        /* this mapping refers to lavfi output */
        const char *c = map + 1;
        //GROW_ARRAY(o->stream_maps, o->nb_stream_maps);
        GROW_ARRAY_EX(o->stream_maps, StreamMap *, o->nb_stream_maps);
        m = &o->stream_maps[o->nb_stream_maps - 1];
        m->linklabel = av_get_token(&c, "]");
        if (!m->linklabel) {
            av_log(NULL, AV_LOG_ERROR, "Invalid output link label: %s.\n", map);
            //exit_program(1);
            return -1;
        }
    } else {
        if (allow_unused = strchr(map, '?'))
            *allow_unused = 0;
        file_idx = strtol(map, &p, 0);
        if (file_idx >= nb_input_files || file_idx < 0) {
            av_log(NULL, AV_LOG_FATAL, "Invalid input file index: %d.\n", file_idx);
            //exit_program(1);
            return -1;
        }
        if (negative)
            /* disable some already defined maps */
            for (i = 0; i < o->nb_stream_maps; i++) {
                m = &o->stream_maps[i];
                if (file_idx == m->file_index &&
                    check_stream_specifier(input_files[m->file_index]->ctx,
                                           input_files[m->file_index]->ctx->streams[m->stream_index],
                                           *p == ':' ? p + 1 : p) > 0)
                    m->disabled = 1;
            }
        else
            for (i = 0; i < input_files[file_idx]->nb_streams; i++) {
                if (check_stream_specifier(input_files[file_idx]->ctx, input_files[file_idx]->ctx->streams[i],
                            *p == ':' ? p + 1 : p) <= 0)
                    continue;
                if (input_streams[input_files[file_idx]->ist_index + i]->user_set_discard == AVDISCARD_ALL) {
                    disabled = 1;
                    continue;
                }
                //GROW_ARRAY(o->stream_maps, o->nb_stream_maps);
                GROW_ARRAY_EX(o->stream_maps, StreamMap *, o->nb_stream_maps);
                m = &o->stream_maps[o->nb_stream_maps - 1];

                m->file_index   = file_idx;
                m->stream_index = i;

                if (sync_file_idx >= 0) {
                    m->sync_file_index   = sync_file_idx;
                    m->sync_stream_index = sync_stream_idx;
                } else {
                    m->sync_file_index   = file_idx;
                    m->sync_stream_index = i;
                }
            }
    }

    if (!m) {
        if (allow_unused) {
            av_log(NULL, AV_LOG_VERBOSE, "Stream map '%s' matches no streams; ignoring.\n", arg);
        } else if (disabled) {
            av_log(NULL, AV_LOG_FATAL, "Stream map '%s' matches disabled streams.\n"
                                       "To ignore this, add a trailing '?' to the map.\n", arg);
            //exit_program(1);
            return -1;
        } else {
            av_log(NULL, AV_LOG_FATAL, "Stream map '%s' matches no streams.\n"
                                       "To ignore this, add a trailing '?' to the map.\n", arg);
            //exit_program(1);
            return -1;
        }
    }

    av_freep(&map);
    return 0;
}

int FFmpegMedia::opt_map_channel(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    int n;
    AVStream *st;
    AudioChannelMap *m;
    char *allow_unused;
    char *mapchan;
    mapchan = av_strdup(arg);
    if (!mapchan)
        return AVERROR(ENOMEM);

    //GROW_ARRAY(o->audio_channel_maps, o->nb_audio_channel_maps);
    GROW_ARRAY_EX(o->audio_channel_maps, AudioChannelMap *, o->nb_audio_channel_maps);
    m = &o->audio_channel_maps[o->nb_audio_channel_maps - 1];

    /* muted channel syntax */
    n = sscanf(arg, "%d:%d.%d", &m->channel_idx, &m->ofile_idx, &m->ostream_idx);
    if ((n == 1 || n == 3) && m->channel_idx == -1) {
        m->file_idx = m->stream_idx = -1;
        if (n == 1)
            m->ofile_idx = m->ostream_idx = -1;
        av_free(mapchan);
        return 0;
    }

    /* normal syntax */
    n = sscanf(arg, "%d.%d.%d:%d.%d",
               &m->file_idx,  &m->stream_idx, &m->channel_idx,
               &m->ofile_idx, &m->ostream_idx);

    if (n != 3 && n != 5) {
        av_log(NULL, AV_LOG_FATAL, "Syntax error, mapchan usage: "
               "[file.stream.channel|-1][:syncfile:syncstream]\n");
        //exit_program(1);
        return -1;
    }

    if (n != 5) // only file.stream.channel specified
        m->ofile_idx = m->ostream_idx = -1;

    /* check input */
    if (m->file_idx < 0 || m->file_idx >= nb_input_files) {
        av_log(NULL, AV_LOG_FATAL, "mapchan: invalid input file index: %d\n",
               m->file_idx);
        //exit_program(1);
        return -1;
    }
    if (m->stream_idx < 0 ||
        m->stream_idx >= input_files[m->file_idx]->nb_streams) {
        av_log(NULL, AV_LOG_FATAL, "mapchan: invalid input file stream index #%d.%d\n",
               m->file_idx, m->stream_idx);
        //exit_program(1);
        return -1;
    }
    st = input_files[m->file_idx]->ctx->streams[m->stream_idx];
    if (st->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
        av_log(NULL, AV_LOG_FATAL, "mapchan: stream #%d.%d is not an audio stream.\n",
               m->file_idx, m->stream_idx);
        //exit_program(1);
        return -1;
    }
    /* allow trailing ? to map_channel */
    if (allow_unused = strchr(mapchan, '?'))
        *allow_unused = 0;
    if (m->channel_idx < 0 || m->channel_idx >= st->codecpar->channels ||
        input_streams[input_files[m->file_idx]->ist_index + m->stream_idx]->user_set_discard == AVDISCARD_ALL) {
        if (allow_unused) {
            av_log(NULL, AV_LOG_VERBOSE, "mapchan: invalid audio channel #%d.%d.%d\n",
                    m->file_idx, m->stream_idx, m->channel_idx);
        } else {
            av_log(NULL, AV_LOG_FATAL,  "mapchan: invalid audio channel #%d.%d.%d\n"
                    "To ignore this, add a trailing '?' to the map_channel.\n",
                    m->file_idx, m->stream_idx, m->channel_idx);
            //exit_program(1);
            return -1;
        }

    }
    av_free(mapchan);
    return 0;
}

int FFmpegMedia::opt_recording_timestamp(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    char buf[128];
    int64_t recording_timestamp = parse_time_or_die(opt, arg, 0) / 1E6;
    struct tm time = *gmtime((time_t*)&recording_timestamp);
    if (!strftime(buf, sizeof(buf), "creation_time=%Y-%m-%dT%H:%M:%S%z", &time))
        return -1;
    parse_option(o, "metadata", buf, options);

    av_log(NULL, AV_LOG_WARNING, "%s is deprecated, set the 'creation_time' metadata "
                                 "tag instead.\n", opt);
    return 0;
}

int FFmpegMedia::opt_data_frames(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "frames:d", arg, options);
}

/**
 * @brief 编写程序可读的进度信息,-progress选项的回调函数.
 */
int FFmpegMedia::opt_progress(void *optctx, const char *opt, const char *arg)
{
    AVIOContext *avio = NULL;
    int ret;

    if (!strcmp(arg, "-"))
        arg = "pipe:";
    ret = avio_open2(&avio, arg, AVIO_FLAG_WRITE, &int_cb, NULL);
    if (ret < 0) {
        char str[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_log(NULL, AV_LOG_ERROR, "Failed to open progress URL \"%s\": %s\n",
               arg, av_err2str_ex(str, ret));
        return ret;
    }
    progress_avio = avio;
    return 0;
}

int FFmpegMedia::opt_audio_codec(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "codec:a", arg, options);
}

int FFmpegMedia::opt_video_channel(void *optctx, const char *opt, const char *arg)
{
    av_log(NULL, AV_LOG_WARNING, "This option is deprecated, use -channel.\n");
    return opt_default(optctx, "channel", arg);
}

int FFmpegMedia::opt_video_standard(void *optctx, const char *opt, const char *arg)
{
    av_log(NULL, AV_LOG_WARNING, "This option is deprecated, use -standard.\n");
    return opt_default(optctx, "standard", arg);
}

int FFmpegMedia::opt_video_codec(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "codec:v", arg, options);
}

int FFmpegMedia::opt_subtitle_codec(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "codec:s", arg, options);
}

int FFmpegMedia::opt_data_codec(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "codec:d", arg, options);
}

int FFmpegMedia::opt_target(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    enum { PAL, NTSC, FILM, UNKNOWN } norm = UNKNOWN;
    static const char *const frame_rates[] = { "25", "30000/1001", "24000/1001" };

    if (!strncmp(arg, "pal-", 4)) {
        norm = PAL;
        arg += 4;
    } else if (!strncmp(arg, "ntsc-", 5)) {
        norm = NTSC;
        arg += 5;
    } else if (!strncmp(arg, "film-", 5)) {
        norm = FILM;
        arg += 5;
    } else {
        /* Try to determine PAL/NTSC by peeking in the input files */
        if (nb_input_files) {
            int i, j;
            for (j = 0; j < nb_input_files; j++) {
                for (i = 0; i < input_files[j]->nb_streams; i++) {
                    AVStream *st = input_files[j]->ctx->streams[i];
                    int64_t fr;
                    if (st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
                        continue;
                    fr = st->time_base.den * 1000LL / st->time_base.num;
                    if (fr == 25000) {
                        norm = PAL;
                        break;
                    } else if ((fr == 29970) || (fr == 23976)) {
                        norm = NTSC;
                        break;
                    }
                }
                if (norm != UNKNOWN)
                    break;
            }
        }
        if (norm != UNKNOWN)
            av_log(NULL, AV_LOG_INFO, "Assuming %s for target.\n", norm == PAL ? "PAL" : "NTSC");
    }

    if (norm == UNKNOWN) {
        av_log(NULL, AV_LOG_FATAL, "Could not determine norm (PAL/NTSC/NTSC-Film) for target.\n");
        av_log(NULL, AV_LOG_FATAL, "Please prefix target with \"pal-\", \"ntsc-\" or \"film-\",\n");
        av_log(NULL, AV_LOG_FATAL, "or set a framerate with \"-r xxx\".\n");
        //exit_program(1);
        return -1;
    }

    if (!strcmp(arg, "vcd")) {
        opt_video_codec(o, "c:v", "mpeg1video");
        opt_audio_codec(o, "c:a", "mp2");
        parse_option(o, "f", "vcd", options);

        parse_option(o, "s", norm == PAL ? "352x288" : "352x240", options);
        parse_option(o, "r", frame_rates[norm], options);
        opt_default(NULL, "g", norm == PAL ? "15" : "18");

        opt_default(NULL, "b:v", "1150000");
        opt_default(NULL, "maxrate:v", "1150000");
        opt_default(NULL, "minrate:v", "1150000");
        opt_default(NULL, "bufsize:v", "327680"); // 40*1024*8;

        opt_default(NULL, "b:a", "224000");
        parse_option(o, "ar", "44100", options);
        parse_option(o, "ac", "2", options);

        opt_default(NULL, "packetsize", "2324");
        opt_default(NULL, "muxrate", "1411200"); // 2352 * 75 * 8;

        /* We have to offset the PTS, so that it is consistent with the SCR.
           SCR starts at 36000, but the first two packs contain only padding
           and the first pack from the other stream, respectively, may also have
           been written before.
           So the real data starts at SCR 36000+3*1200. */
        o->mux_preload = (36000 + 3 * 1200) / 90000.0; // 0.44
    } else if (!strcmp(arg, "svcd")) {

        opt_video_codec(o, "c:v", "mpeg2video");
        opt_audio_codec(o, "c:a", "mp2");
        parse_option(o, "f", "svcd", options);

        parse_option(o, "s", norm == PAL ? "480x576" : "480x480", options);
        parse_option(o, "r", frame_rates[norm], options);
        parse_option(o, "pix_fmt", "yuv420p", options);
        opt_default(NULL, "g", norm == PAL ? "15" : "18");

        opt_default(NULL, "b:v", "2040000");
        opt_default(NULL, "maxrate:v", "2516000");
        opt_default(NULL, "minrate:v", "0"); // 1145000;
        opt_default(NULL, "bufsize:v", "1835008"); // 224*1024*8;
        opt_default(NULL, "scan_offset", "1");

        opt_default(NULL, "b:a", "224000");
        parse_option(o, "ar", "44100", options);

        opt_default(NULL, "packetsize", "2324");

    } else if (!strcmp(arg, "dvd")) {

        opt_video_codec(o, "c:v", "mpeg2video");
        opt_audio_codec(o, "c:a", "ac3");
        parse_option(o, "f", "dvd", options);

        parse_option(o, "s", norm == PAL ? "720x576" : "720x480", options);
        parse_option(o, "r", frame_rates[norm], options);
        parse_option(o, "pix_fmt", "yuv420p", options);
        opt_default(NULL, "g", norm == PAL ? "15" : "18");

        opt_default(NULL, "b:v", "6000000");
        opt_default(NULL, "maxrate:v", "9000000");
        opt_default(NULL, "minrate:v", "0"); // 1500000;
        opt_default(NULL, "bufsize:v", "1835008"); // 224*1024*8;

        opt_default(NULL, "packetsize", "2048");  // from www.mpucoder.com: DVD sectors contain 2048 bytes of data, this is also the size of one pack.
        opt_default(NULL, "muxrate", "10080000"); // from mplex project: data_rate = 1260000. mux_rate = data_rate * 8

        opt_default(NULL, "b:a", "448000");
        parse_option(o, "ar", "48000", options);

    } else if (!strncmp(arg, "dv", 2)) {

        parse_option(o, "f", "dv", options);

        parse_option(o, "s", norm == PAL ? "720x576" : "720x480", options);
        parse_option(o, "pix_fmt", !strncmp(arg, "dv50", 4) ? "yuv422p" :
                          norm == PAL ? "yuv420p" : "yuv411p", options);
        parse_option(o, "r", frame_rates[norm], options);

        parse_option(o, "ar", "48000", options);
        parse_option(o, "ac", "2", options);

    } else {
        av_log(NULL, AV_LOG_ERROR, "Unknown target: %s\n", arg);
        return AVERROR(EINVAL);
    }

    av_dict_copy(&o->g->codec_opts,  codec_opts, AV_DICT_DONT_OVERWRITE);
    av_dict_copy(&o->g->format_opts, format_opts, AV_DICT_DONT_OVERWRITE);

    return 0;
}

int FFmpegMedia::opt_vsync(void *optctx, const char *opt, const char *arg)
{
    if      (!av_strcasecmp(arg, "cfr"))         video_sync_method = VSYNC_CFR;
    else if (!av_strcasecmp(arg, "vfr"))         video_sync_method = VSYNC_VFR;
    else if (!av_strcasecmp(arg, "passthrough")) video_sync_method = VSYNC_PASSTHROUGH;
    else if (!av_strcasecmp(arg, "drop"))        video_sync_method = VSYNC_DROP;

    if (video_sync_method == VSYNC_AUTO)
        video_sync_method = parse_number_or_die("vsync", arg, OPT_INT, VSYNC_AUTO, VSYNC_VFR);
    return 0;
}


// 中止指定的条件标志
int FFmpegMedia::opt_abort_on(void *optctx, const char *opt, const char *arg)
{
    // C++要求opts结构体内的每个成员变量都要赋值，不然会报: Makefile error xxx这种错误
    static const AVOption opts[] = {
        { "abort_on"        , NULL, 0, AV_OPT_TYPE_FLAGS, { .i64 = 0 }, (double)INT64_MIN, (double)INT64_MAX, 0, .unit = "flags" },
        { "empty_output"    , NULL, 0, AV_OPT_TYPE_CONST, { .i64 = ABORT_ON_FLAG_EMPTY_OUTPUT     }, (double)INT64_MIN, (double)INT64_MAX, 0, .unit = "flags" },
        { NULL },
    };
    static const AVClass avclass = {
        .class_name = "",
        .item_name  = av_default_item_name,
        .option     = opts,
        .version    = LIBAVUTIL_VERSION_INT,
    };
    const AVClass *pclass = &avclass;

    return av_opt_eval_flags(&pclass, &opts[0], arg, &abort_on_flags);// 使用参3给参4赋值.
}

int FFmpegMedia::opt_qscale(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    char *s;
    int ret;
    if(!strcmp(opt, "qscale")){
        av_log(NULL, AV_LOG_WARNING, "Please use -q:a or -q:v, -qscale is ambiguous\n");
        return parse_option(o, "q:v", arg, options);
    }
    s = av_asprintf("q%s", opt + 6);
    ret = parse_option(o, s, arg, options);
    av_free(s);
    return ret;
}

int FFmpegMedia::opt_profile(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    if(!strcmp(opt, "profile")){
        av_log(NULL, AV_LOG_WARNING, "Please use -profile:a or -profile:v, -profile is ambiguous\n");
        av_dict_set(&o->g->codec_opts, "profile:v", arg, 0);
        return 0;
    }
    av_dict_set(&o->g->codec_opts, opt, arg, 0);
    return 0;
}

int FFmpegMedia::opt_video_filters(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "filter:v", arg, options);
}

int FFmpegMedia::opt_audio_filters(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "filter:a", arg, options);
}

int FFmpegMedia::opt_filter_complex(void *optctx, const char *opt, const char *arg)
{
    //GROW_ARRAY(filtergraphs, nb_filtergraphs);
    GROW_ARRAY_EX(filtergraphs, FilterGraph **, nb_filtergraphs);
    if (!(filtergraphs[nb_filtergraphs - 1] = (FilterGraph *)av_mallocz(sizeof(*filtergraphs[0]))))
        return AVERROR(ENOMEM);
    filtergraphs[nb_filtergraphs - 1]->index      = nb_filtergraphs - 1;
    filtergraphs[nb_filtergraphs - 1]->graph_desc = av_strdup(arg);
    if (!filtergraphs[nb_filtergraphs - 1]->graph_desc)
        return AVERROR(ENOMEM);

    input_stream_potentially_available = 1;

    return 0;
}

int FFmpegMedia::opt_filter_complex_script(void *optctx, const char *opt, const char *arg)
{
    uint8_t *graph_desc = read_file(arg);
    if (!graph_desc)
        return AVERROR(EINVAL);

    //GROW_ARRAY(filtergraphs, nb_filtergraphs);
    GROW_ARRAY_EX(filtergraphs, FilterGraph **, nb_filtergraphs);
    if (!(filtergraphs[nb_filtergraphs - 1] = (FilterGraph *)av_mallocz(sizeof(*filtergraphs[0]))))
        return AVERROR(ENOMEM);
    filtergraphs[nb_filtergraphs - 1]->index      = nb_filtergraphs - 1;
    filtergraphs[nb_filtergraphs - 1]->graph_desc = (const char    *)graph_desc;

    input_stream_potentially_available = 1;

    return 0;
}

int FFmpegMedia::FFmpegMedia::opt_attach(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    //GROW_ARRAY(o->attachments, o->nb_attachments);
    GROW_ARRAY_EX(o->attachments, const char **, o->nb_attachments);
    o->attachments[o->nb_attachments - 1] = arg;
    return 0;
}

int FFmpegMedia::opt_video_frames(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "frames:v", arg, options);
}

int FFmpegMedia::opt_audio_frames(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "frames:a", arg, options);
}

int FFmpegMedia::opt_sameq(void *optctx, const char *opt, const char *arg)
{
    av_log(NULL, AV_LOG_ERROR, "Option '%s' was removed. "
           "If you are looking for an option to preserve the quality (which is not "
           "what -%s was for), use -qscale 0 or an equivalent quality factor option.\n",
           opt, opt);
    return AVERROR(EINVAL);
}

int FFmpegMedia::opt_timecode(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    char *tcr = av_asprintf("timecode=%s", arg);
    int ret = parse_option(o, "metadata:g", tcr, options);
    if (ret >= 0)
        ret = av_dict_set(&o->g->codec_opts, "gop_timecode", arg, 0);
    av_free(tcr);
    return ret;
}

/**
 * @brief -vstats_file选项,设置后可以将视频流的相关信息保存到文件
 */
int FFmpegMedia::opt_vstats_file(void *optctx, const char *opt, const char *arg)
{
    av_free (vstats_filename);
    vstats_filename = av_strdup (arg);
    return 0;
}

/**
 * @brief -vstats选项,和上面-vstats_file选项的区别是,本选项自动设置文件名.
 */
int FFmpegMedia::opt_vstats(void *optctx, const char *opt, const char *arg)
{
    char filename[40];
    time_t today2 = time(NULL);
    struct tm *today = localtime(&today2);

    if (!today) { // maybe tomorrow
        av_log(NULL, AV_LOG_FATAL, "Unable to get current time: %s\n", strerror(errno));
        //exit_program(1);
        return -1;
    }

    snprintf(filename, sizeof(filename), "vstats_%02d%02d%02d.log", today->tm_hour, today->tm_min,
             today->tm_sec);
    return opt_vstats_file(NULL, opt, filename);
}

int FFmpegMedia::opt_old2new(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    char *s = av_asprintf("%s:%c", opt + 1, *opt);
    int ret = parse_option(o, s, arg, options);
    av_free(s);
    return ret;
}

int FFmpegMedia::opt_bitrate(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;

    if(!strcmp(opt, "ab")){
        av_dict_set(&o->g->codec_opts, "b:a", arg, 0);
        return 0;
    } else if(!strcmp(opt, "b")){
        av_log(NULL, AV_LOG_WARNING, "Please use -b:a or -b:v, -b is ambiguous\n");
        av_dict_set(&o->g->codec_opts, "b:v", arg, 0);
        return 0;
    }
    av_dict_set(&o->g->codec_opts, opt, arg, 0);
    return 0;
}

int FFmpegMedia::show_hwaccels(void *optctx, const char *opt, const char *arg)
{
    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    int i;

    printf("Hardware acceleration methods:\n");
    while ((type = av_hwdevice_iterate_types(type)) !=
           AV_HWDEVICE_TYPE_NONE)
        printf("%s\n", av_hwdevice_get_type_name(type));
    for (i = 0; hwaccels[i].name; i++)
        printf("%s\n", hwaccels[i].name);
    printf("\n");
    return 0;
}

int FFmpegMedia::opt_audio_qscale(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    return parse_option(o, "q:a", arg, options);
}

/* arg format is "output-stream-index:streamid-value". */
int FFmpegMedia::opt_streamid(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    int idx;
    char *p;
    char idx_str[16];

    av_strlcpy(idx_str, arg, sizeof(idx_str));
    p = strchr(idx_str, ':');
    if (!p) {
        av_log(NULL, AV_LOG_FATAL,
               "Invalid value '%s' for option '%s', required syntax is 'index:value'\n",
               arg, opt);
        //exit_program(1);
        return -1;
    }
    *p++ = '\0';
    idx = parse_number_or_die(opt, idx_str, OPT_INT, 0, MAX_STREAMS-1);
    o->streamid_map = (int   *)grow_array((void*)o->streamid_map, sizeof(*o->streamid_map), &o->nb_streamid_map, idx+1);
    o->streamid_map[idx] = parse_number_or_die(opt, p, OPT_INT, 0, INT_MAX);
    return 0;
}

int FFmpegMedia::opt_default_new(OptionsContext *o, const char *opt, const char *arg)
{
    int ret;
    AVDictionary *cbak = codec_opts;
    AVDictionary *fbak = format_opts;
    codec_opts = NULL;
    format_opts = NULL;

    ret = opt_default(NULL, opt, arg);

    av_dict_copy(&o->g->codec_opts , codec_opts, 0);
    av_dict_copy(&o->g->format_opts, format_opts, 0);
    av_dict_free(&codec_opts);
    av_dict_free(&format_opts);
    codec_opts = cbak;
    format_opts = fbak;

    return ret;
}

int FFmpegMedia::opt_channel_layout(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    char layout_str[32];
    char *stream_str;
    char *ac_str;
    int ret, channels, ac_str_size;
    uint64_t layout;

    layout = av_get_channel_layout(arg);
    if (!layout) {
        av_log(NULL, AV_LOG_ERROR, "Unknown channel layout: %s\n", arg);
        return AVERROR(EINVAL);
    }
    snprintf(layout_str, sizeof(layout_str), "%" PRIu64, layout);
    ret = opt_default_new(o, opt, layout_str);
    if (ret < 0)
        return ret;

    /* set 'ac' option based on channel layout */
    channels = av_get_channel_layout_nb_channels(layout);
    snprintf(layout_str, sizeof(layout_str), "%d", channels);
    stream_str = strchr(opt, ':');
    ac_str_size = 3 + (stream_str ? strlen(stream_str) : 0);
    ac_str = (char *)av_mallocz(ac_str_size);
    if (!ac_str)
        return AVERROR(ENOMEM);
    av_strlcpy(ac_str, "ac", 3);
    if (stream_str)
        av_strlcat(ac_str, stream_str, ac_str_size);
    ret = parse_option(o, ac_str, layout_str, options);
    av_free(ac_str);

    return ret;
}

int FFmpegMedia::opt_sdp_file(void *optctx, const char *opt, const char *arg)
{
    av_free(sdp_filename);
    sdp_filename = av_strdup(arg);
    return 0;
}

FILE *FFmpegMedia::get_preset_file(char *filename, size_t filename_size,
                      const char *preset_name, int is_path,
                      const char *codec_name)
{
    FILE *f = NULL;
    int i;
    const char *base[3] = { getenv("FFMPEG_DATADIR"),
                            getenv("HOME"),
                            FFMPEG_DATADIR, };

    if (is_path) {
        av_strlcpy(filename, preset_name, filename_size);
        f = fopen(filename, "r");
    } else {
#ifdef _WIN32
        char datadir[MAX_PATH], *ls;
        base[2] = NULL;

        if (GetModuleFileNameA(GetModuleHandleA(NULL), datadir, sizeof(datadir) - 1))
        {
            for (ls = datadir; ls < datadir + strlen(datadir); ls++)
                if (*ls == '\\') *ls = '/';

            if (ls = strrchr(datadir, '/'))
            {
                *ls = 0;
                strncat(datadir, "/ffpresets",  sizeof(datadir) - 1 - strlen(datadir));
                base[2] = datadir;
            }
        }
#endif
        for (i = 0; i < 3 && !f; i++) {
            if (!base[i])
                continue;
            snprintf(filename, filename_size, "%s%s/%s.ffpreset", base[i],
                     i != 1 ? "" : "/.ffmpeg", preset_name);
            f = fopen(filename, "r");
            if (!f && codec_name) {
                snprintf(filename, filename_size,
                         "%s%s/%s-%s.ffpreset",
                         base[i], i != 1 ? "" : "/.ffmpeg", codec_name,
                         preset_name);
                f = fopen(filename, "r");
            }
        }
    }

    return f;
}

int FFmpegMedia::opt_preset(void *optctx, const char *opt, const char *arg)
{
    OptionsContext *o = (OptionsContext *)optctx;
    FILE *f=NULL;
    char filename[1000], line[1000], tmp_line[1000];
    const char *codec_name = NULL;

    tmp_line[0] = *opt;
    tmp_line[1] = 0;
    //MATCH_PER_TYPE_OPT(codec_names, str, codec_name, NULL, tmp_line);
    MATCH_PER_TYPE_OPT_EX(codec_names, str, codec_name, const char *, NULL, tmp_line);

    if (!(f = get_preset_file(filename, sizeof(filename), arg, *opt == 'f', codec_name))) {
        if(!strncmp(arg, "libx264-lossless", strlen("libx264-lossless"))){
            av_log(NULL, AV_LOG_FATAL, "Please use -preset <speed> -qp 0\n");
        }else
            av_log(NULL, AV_LOG_FATAL, "File for preset '%s' not found\n", arg);
        //exit_program(1);
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        char *key = tmp_line, *value, *endptr;

        if (strcspn(line, "#\n\r") == 0)
            continue;
        av_strlcpy(tmp_line, line, sizeof(tmp_line));
        if (!av_strtok(key,   "=",    &value) ||
            !av_strtok(value, "\r\n", &endptr)) {
            av_log(NULL, AV_LOG_FATAL, "%s: Invalid syntax: '%s'\n", filename, line);
            //exit_program(1);
            return -1;
        }
        av_log(NULL, AV_LOG_DEBUG, "ffpreset[%s]: set '%s' = '%s'\n", filename, key, value);

        if      (!strcmp(key, "acodec")) opt_audio_codec   (o, key, value);
        else if (!strcmp(key, "vcodec")) opt_video_codec   (o, key, value);
        else if (!strcmp(key, "scodec")) opt_subtitle_codec(o, key, value);
        else if (!strcmp(key, "dcodec")) opt_data_codec    (o, key, value);
        else if (opt_default_new(o, key, value) < 0) {
            av_log(NULL, AV_LOG_FATAL, "%s: Invalid option or argument: '%s', parsed as '%s' = '%s'\n",
                   filename, line, key, value);
            //exit_program(1);
            return -1;
        }
    }

    fclose(f);

    return 0;
}

int FFmpegMedia::opt_init_hw_device(void *optctx, const char *opt, const char *arg)
{
    if (!strcmp(arg, "list")) {
        enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
        printf("Supported hardware device types:\n");
        while ((type = av_hwdevice_iterate_types(type)) !=
               AV_HWDEVICE_TYPE_NONE)
            printf("%s\n", av_hwdevice_get_type_name(type));
        printf("\n");
        //exit_program(0);
        return -1;
    } else {
        return hw_device_init_from_string(arg, NULL);
    }
}

int FFmpegMedia::opt_map_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_map(optctx, opt, arg);
}
int FFmpegMedia::opt_map_channel_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_map_channel(optctx, opt, arg);
}
int FFmpegMedia::opt_recording_timestamp_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_recording_timestamp(optctx, opt, arg);
}
int FFmpegMedia::opt_data_frames_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_data_frames(optctx, opt, arg);
}
int FFmpegMedia::opt_progress_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_progress(optctx, opt, arg);
}
int FFmpegMedia::opt_target_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_target(optctx, opt, arg);
}
int FFmpegMedia::opt_audio_codec_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_audio_codec(optctx, opt, arg);
}
int FFmpegMedia::opt_video_channel_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_video_channel(optctx, opt, arg);
}
int FFmpegMedia::opt_video_standard_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_video_standard(optctx, opt, arg);
}
int FFmpegMedia::opt_video_codec_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_video_codec(optctx, opt, arg);
}
int FFmpegMedia::opt_subtitle_codec_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_subtitle_codec(optctx, opt, arg);
}
int FFmpegMedia::opt_data_codec_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_data_codec(optctx, opt, arg);
}
int FFmpegMedia::opt_vsync_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_vsync(optctx, opt, arg);
}
int FFmpegMedia::opt_abort_on_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_abort_on(optctx, opt, arg);
}
int FFmpegMedia::opt_qscale_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_qscale(optctx, opt, arg);
}
int FFmpegMedia::opt_profile_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_profile(optctx, opt, arg);
}
int FFmpegMedia::opt_video_filters_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_video_filters(optctx, opt, arg);
}
int FFmpegMedia::opt_audio_filters_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_audio_filters(optctx, opt, arg);
}
int FFmpegMedia::opt_filter_complex_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_filter_complex(optctx, opt, arg);
}
int FFmpegMedia::opt_filter_complex_script_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_filter_complex_script(optctx, opt, arg);
}
int FFmpegMedia::opt_attach_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_attach(optctx, opt, arg);
}
int FFmpegMedia::opt_video_frames_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_video_frames(optctx, opt, arg);
}
int FFmpegMedia::opt_audio_frames_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_audio_frames(optctx, opt, arg);
}
//int opt_data_frames(void *optctx, const char *opt, const char *arg);
int FFmpegMedia::opt_sameq_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_sameq(optctx, opt, arg);
}
int FFmpegMedia::opt_timecode_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_timecode(optctx, opt, arg);
}
int FFmpegMedia::opt_vstats_file_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_vstats_file(optctx, opt, arg);
}
int FFmpegMedia::opt_vstats_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_vstats(optctx, opt, arg);
}
int FFmpegMedia::opt_old2new_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_old2new(optctx, opt, arg);
}
int FFmpegMedia::opt_bitrate_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_bitrate(optctx, opt, arg);
}
int FFmpegMedia::opt_streamid_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_streamid(optctx, opt, arg);
}
int FFmpegMedia::show_hwaccels_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->show_hwaccels(optctx, opt, arg);
}
int FFmpegMedia::opt_audio_qscale_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_audio_qscale(optctx, opt, arg);
}
int FFmpegMedia::opt_default_new_ex(void * t, OptionsContext *o, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_default_new(o, opt, arg);
}
int FFmpegMedia::opt_channel_layout_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_channel_layout(optctx, opt, arg);
}
int FFmpegMedia::opt_sdp_file_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_sdp_file(optctx, opt, arg);
}
int FFmpegMedia::opt_preset_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_preset(optctx, opt, arg);
}
int FFmpegMedia::opt_init_hw_device_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_init_hw_device(optctx, opt, arg);
}
int FFmpegMedia::opt_filter_hw_device_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_filter_hw_device(optctx, opt, arg);
}
int FFmpegMedia::opt_timelimit_ex(void * t, void *optctx, const char *opt, const char *arg){
    auto fm = (FFmpegMedia*)t;
    return fm->opt_timelimit(optctx, opt, arg);
}

int FFmpegMedia::opt_filter_hw_device(void *optctx, const char *opt, const char *arg)
{
    if (filter_hw_device) {
        av_log(NULL, AV_LOG_ERROR, "Only one filter device can be used.\n");
        return AVERROR(EINVAL);
    }
    filter_hw_device = hw_device_get_by_name(arg);
    if (!filter_hw_device) {
        av_log(NULL, AV_LOG_ERROR, "Invalid filter device %s.\n", arg);
        return AVERROR(EINVAL);
    }
    return 0;
}

int FFmpegMedia::opt_timelimit(void *optctx, const char *opt, const char *arg)
{
#if HAVE_SETRLIMIT
    int lim = parse_number_or_die(opt, arg, OPT_INT64, 0, INT_MAX);
    struct rlimit rl = { lim, lim + 1 };
    if (setrlimit(RLIMIT_CPU, &rl))
        perror("setrlimit");
#else
    av_log(NULL, AV_LOG_WARNING, "-%s not implemented on this OS\n", opt);
#endif
    return 0;
}

void FFmpegMedia::memset_zero_fm(){
    // ffmpeg.h global var
    input_streams = NULL;
    nb_input_streams = 0;         // input_streams二维数组大小

    input_files = NULL;           // 用于保存多个输入文件
    nb_input_files = 0;           // 输入文件个数

    output_streams = NULL;       // 保存各个输出流的数组
    nb_output_streams = 0;       // output_streams二维数组大小

    output_files = NULL;         // 用于保存多个输出文件
    nb_output_files = 0;         // 输出文件个数

    filtergraphs = NULL;          // 封装好的系统过滤器数组，每个FilterGraph都会包含对应输入流与输出流的的输入输出过滤器。可看init_simple_filtergraph函数
    nb_filtergraphs = 0;          // filtergraphs数组的大小

    progress_avio = NULL;
    //float max_error_rate;
    videotoolbox_pixfmt = NULL;

    //const AVIOInterruptCB int_cb;
    //memset((void*)&int_cb, 0, sizeof(int_cb));

    //OptionDef options[178];
    options = NULL;             // 指向ffmpeg的所有选项数组
    //const HWAccel hwaccels[];
    hw_device_ctx = NULL;
#if CONFIG_QSV
    qsv_device = NULL;
#endif
    filter_hw_device = NULL;          // -filter_hw_device选项


    // ffmpeg.c global var
    want_sdp = -1;
    //BenchmarkTimeStamps current_time;
    memset((void*)&current_time, 0, sizeof(BenchmarkTimeStamps));

    //decode_error_stat[2];// 记录解码的状态.下标0记录的是成功解码的参数,下标1是失败的次数.
    main_return_code = 0;//
    received_nb_signals = 0;
    //program_name[24] = "ffmpeg";

    received_sigterm = 0;
    transcode_init_done = 0;
    ffmpeg_exited = 0;

    run_as_daemon  = 0;  // 0=前台运行；1=后台运行
    nb_frames_dup = 0;
    dup_warning = 1000;
    nb_frames_drop = 0;

    // ffmpeg_cmdutil.h global var
    //program_birth_year = 2023;
    avcodec_opts[AVMEDIA_TYPE_NB];
    memset(avcodec_opts, 0, sizeof (avcodec_opts));
    avformat_opts = NULL;
    sws_dict = NULL;
    swr_opts = NULL;
    format_opts = codec_opts = resample_opts = NULL;
    //hide_banner;

    vstats_file = NULL;

    subtitle_out = NULL;

    // ffmpeg_opt.c global var
    //float max_error_rate  = 2.0/3;
    vstats_filename = NULL;
    sdp_filename = NULL;

    audio_drift_threshold = 0.1;
    dts_delta_threshold   = 10;
    dts_error_threshold   = 3600*30;  // dts错误阈值,默认30小时.帧能被解码的最大阈值大小？

    audio_volume      = 256;            // -vol选项,默认256
    audio_sync_method = 0;              // 音频同步方法.默认0
    video_sync_method = VSYNC_AUTO;
    frame_drop_threshold = 0;
    do_deinterlace    = 0;              // -deinterlace选项,默认0.
    do_benchmark      = 0;
    do_benchmark_all  = 0;              // -benchmark_all选项，默认0
    do_hex_dump       = 0;
    do_pkt_dump       = 0;
    copy_ts           = 0;              // copy timestamps,默认0
    start_at_zero     = 0;              // 使用copy_ts时，将输入时间戳移至0开始,默认0
    copy_tb           = -1;
    debug_ts          = 0;              // 是否打印相关时间戳.-debug_ts选项
    exit_on_error     = 0;              // xerror选项,默认是0
    abort_on_flags    = 0;
    print_stats       = -1;             // -stats选项,默认-1,在编码期间打印进度报告.
    qp_hist           = 0;              // -qphist选项,默认0,显示QP直方图
    stdin_interaction = 1;              // 可认为是否是交互模式.除pipe、以及/dev/stdin值为0，其它例如普通文件、实时流都是1
    frame_bits_per_raw_sample = 0;
    max_error_rate  = 2.0/3;
    filter_nbthreads = 0;               // -filter_threads选项,默认0,非复杂过滤器线程数.
    filter_complex_nbthreads = 0;       // -filter_complex_threads选项,默认0,复杂过滤器线程数.
    vstats_version = 2;                 // -vstats_version选项, 默认2, 要使用的vstats格式的版本


    intra_only         = 0;
    file_overwrite     = 0;      // -y选项，重写输出文件，即覆盖该输出文件。0=不重写，1=重写，
                                            // 不过为0时且no_file_overwrite=0时会在终端询问用户是否重写
    no_file_overwrite  = 0;      // -n选项，不重写输出文件。0=不重写，1=重写
    do_psnr            = 0;
    input_sync;
    input_stream_potentially_available = 0;// 标记，=1代表该输入文件可能是可用的
    ignore_unknown_streams = 0;
    copy_unknown_streams = 0;
    find_stream_info = 1;


    // ffmpeg_cmdutil.c global var
    win32_argv_utf8 = NULL;
    win32_argc = 0;

    // ffmpeg_hw.c global var
    nb_hw_devices;           // 用户电脑支持的硬件设备数
    hw_devices = NULL;       // 用户电脑支持的硬件设备数组

    // tyy gloabl var
    argc = 0;
    argv = NULL;
}

void FFmpegMedia::init_g_options(){
#if 1
    // offsetof是一个C函数:返回字节对齐后，成员在结构体中的偏移量.
    // 要获取OptionsContext.xxx成员的地址，我们看到write_option()时需要加上OptionsContext变量的首地址.例如(&o + options[0].off)
    #define OFFSET(x) offsetof(OptionsContext, x)
    //const OptionDef tmpoptions[] = {
    OptionDef tmpoptions[] = {
        /* main options */
        //CMDUTILS_COMMON_OPTIONS/* 暂时不要这些选项 */
        { "f",              HAS_ARG | OPT_STRING | OPT_OFFSET |
                            OPT_INPUT | OPT_OUTPUT,                      { .off       = OFFSET(format) },
            "force format", "fmt" },
        { "y",              OPT_BOOL,                                    {              &file_overwrite },
            "overwrite output files" },
        { "n",              OPT_BOOL,                                    {              &no_file_overwrite },
            "never overwrite output files" },
        { "ignore_unknown", OPT_BOOL,                                    {              &ignore_unknown_streams },
            "Ignore unknown stream types" },
        { "copy_unknown",   OPT_BOOL | OPT_EXPERT,                       {              &copy_unknown_streams },
            "Copy unknown stream types" },
        { "c",              HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                      { .off       = OFFSET(codec_names) },
            "codec name", "codec" },
        { "codec",          HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                      { .off       = OFFSET(codec_names) },
            "codec name", "codec" },
        { "pre",            HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off       = OFFSET(presets) },
            "preset name", "preset" },
        { "map",            HAS_ARG | OPT_EXPERT | OPT_PERFILE |
                            OPT_OUTPUT,                                  { .func_arg = opt_map_ex },
            "set input stream mapping",
            "[-]input_file_id[:stream_specifier][,sync_file_id[:stream_specifier]]" },
        { "map_channel",    HAS_ARG | OPT_EXPERT | OPT_PERFILE | OPT_OUTPUT, { .func_arg = opt_map_channel_ex },
            "map an audio channel from one stream to another", "file.stream.channel[:syncfile.syncstream]" },
        { "map_metadata",   HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off       = OFFSET(metadata_map) },
            "set metadata information of outfile from infile",
            "outfile[,metadata]:infile[,metadata]" },
        { "map_chapters",   HAS_ARG | OPT_INT | OPT_EXPERT | OPT_OFFSET |
                            OPT_OUTPUT,                                  { .off = OFFSET(chapters_input_file) },
            "set chapters mapping", "input_file_index" },
        { "t",              HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_INPUT | OPT_OUTPUT,                      { .off = OFFSET(recording_time) },
            "record or transcode \"duration\" seconds of audio/video",
            "duration" },
        { "to",             HAS_ARG | OPT_TIME | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT,  { .off = OFFSET(stop_time) },
            "record or transcode stop time", "time_stop" },
        { "fs",             HAS_ARG | OPT_INT64 | OPT_OFFSET | OPT_OUTPUT, { .off = OFFSET(limit_filesize) },
            "set the limit file size in bytes", "limit_size" },
        { "ss",             HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_INPUT | OPT_OUTPUT,                      { .off = OFFSET(start_time) },
            "set the start time offset", "time_off" },
        { "sseof",          HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_INPUT,                                   { .off = OFFSET(start_time_eof) },
            "set the start time offset relative to EOF", "time_off" },
        { "seek_timestamp", HAS_ARG | OPT_INT | OPT_OFFSET |
                            OPT_INPUT,                                   { .off = OFFSET(seek_timestamp) },
            "enable/disable seeking by timestamp with -ss" },
        { "accurate_seek",  OPT_BOOL | OPT_OFFSET | OPT_EXPERT |
                            OPT_INPUT,                                   { .off = OFFSET(accurate_seek) },
            "enable/disable accurate seeking with -ss" },
        { "itsoffset",      HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_EXPERT | OPT_INPUT,                      { .off = OFFSET(input_ts_offset) },
            "set the input ts offset", "time_off" },
        { "itsscale",       HAS_ARG | OPT_DOUBLE | OPT_SPEC |
                            OPT_EXPERT | OPT_INPUT,                      { .off = OFFSET(ts_scale) },
            "set the input ts scale", "scale" },
        { "timestamp",      HAS_ARG | OPT_PERFILE | OPT_OUTPUT,          { .func_arg = opt_recording_timestamp_ex },
            "set the recording timestamp ('now' to set the current time)", "time" },
        { "metadata",       HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(metadata) },
            "add metadata", "string=string" },
        { "program",        HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(program) },
            "add program with specified streams", "title=string:st=number..." },
        { "dframes",        HAS_ARG | OPT_PERFILE | OPT_EXPERT |
                            OPT_OUTPUT,                                  { .func_arg = opt_data_frames_ex },
            "set the number of data frames to output", "number" },
        { "benchmark",      OPT_BOOL | OPT_EXPERT,                       { &do_benchmark },
            "add timings for benchmarking" },
        { "benchmark_all",  OPT_BOOL | OPT_EXPERT,                       { &do_benchmark_all },
          "add timings for each task" },
        { "progress",       HAS_ARG | OPT_EXPERT,                        { .func_arg = opt_progress_ex },
          "write program-readable progress information", "url" },
        { "stdin",          OPT_BOOL | OPT_EXPERT,                       { &stdin_interaction },
          "enable or disable interaction on standard input" },
        { "timelimit",      HAS_ARG | OPT_EXPERT,                        { .func_arg = opt_timelimit_ex },
            "set max runtime in seconds", "limit" },
        { "dump",           OPT_BOOL | OPT_EXPERT,                       { &do_pkt_dump },
            "dump each input packet" },
        { "hex",            OPT_BOOL | OPT_EXPERT,                       { &do_hex_dump },
            "when dumping packets, also dump the payload" },
        { "re",             OPT_BOOL | OPT_EXPERT | OPT_OFFSET |
                            OPT_INPUT,                                   { .off = OFFSET(rate_emu) },
            "read input at native frame rate", "" },
        { "target",         HAS_ARG | OPT_PERFILE | OPT_OUTPUT,          { .func_arg = opt_target_ex },
            "specify target file type (\"vcd\", \"svcd\", \"dvd\", \"dv\" or \"dv50\" "
            "with optional prefixes \"pal-\", \"ntsc-\" or \"film-\")", "type" },
        { "vsync",          HAS_ARG | OPT_EXPERT,                        { .func_arg = opt_vsync_ex },
            "video sync method", "" },
        { "frame_drop_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,      { &frame_drop_threshold },
            "frame drop threshold", "" },
        { "async",          HAS_ARG | OPT_INT | OPT_EXPERT,              { &audio_sync_method },
            "audio sync method", "" },
        { "adrift_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,          { &audio_drift_threshold },
            "audio drift threshold", "threshold" },
        { "copyts",         OPT_BOOL | OPT_EXPERT,                       { &copy_ts },
            "copy timestamps" },
        { "start_at_zero",  OPT_BOOL | OPT_EXPERT,                       { &start_at_zero },
            "shift input timestamps to start at 0 when using copyts" },
        { "copytb",         HAS_ARG | OPT_INT | OPT_EXPERT,              { &copy_tb },
            "copy input stream time base when stream copying", "mode" },
        { "shortest",       OPT_BOOL | OPT_EXPERT | OPT_OFFSET |
                            OPT_OUTPUT,                                  { .off = OFFSET(shortest) },
            "finish encoding within shortest input" },
        { "bitexact",       OPT_BOOL | OPT_EXPERT | OPT_OFFSET |
                            OPT_OUTPUT | OPT_INPUT,                      { .off = OFFSET(bitexact) },
            "bitexact mode" },
        { "apad",           OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off = OFFSET(apad) },
            "audio pad", "" },
        { "dts_delta_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,       { &dts_delta_threshold },
            "timestamp discontinuity delta threshold", "threshold" },
        { "dts_error_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,       { &dts_error_threshold },
            "timestamp error delta threshold", "threshold" },
        { "xerror",         OPT_BOOL | OPT_EXPERT,                       { &exit_on_error },
            "exit on error", "error" },
        { "abort_on",       HAS_ARG | OPT_EXPERT,                        { .func_arg = opt_abort_on_ex },
            "abort on the specified condition flags", "flags" },
        { "copyinkf",       OPT_BOOL | OPT_EXPERT | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off = OFFSET(copy_initial_nonkeyframes) },
            "copy initial non-keyframes" },
        { "copypriorss",    OPT_INT | HAS_ARG | OPT_EXPERT | OPT_SPEC | OPT_OUTPUT,   { .off = OFFSET(copy_prior_start) },
            "copy or discard frames before start time" },
        { "frames",         OPT_INT64 | HAS_ARG | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(max_frames) },
            "set the number of frames to output", "number" },
        { "tag",            OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_EXPERT | OPT_OUTPUT | OPT_INPUT,         { .off = OFFSET(codec_tags) },
            "force codec tag/fourcc", "fourcc/tag" },
        { "q",              HAS_ARG | OPT_EXPERT | OPT_DOUBLE |
                            OPT_SPEC | OPT_OUTPUT,                       { .off = OFFSET(qscale) },
            "use fixed quality scale (VBR)", "q" },
        { "qscale",         HAS_ARG | OPT_EXPERT | OPT_PERFILE |
                            OPT_OUTPUT,                                  { .func_arg = opt_qscale_ex },
            "use fixed quality scale (VBR)", "q" },
        { "profile",        HAS_ARG | OPT_EXPERT | OPT_PERFILE | OPT_OUTPUT, { .func_arg = opt_profile_ex },
            "set profile", "profile" },
        { "filter",         HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(filters) },
            "set stream filtergraph", "filter_graph" },
        { "filter_threads",  HAS_ARG | OPT_INT,                          { &filter_nbthreads },
            "number of non-complex filter threads" },
        { "filter_script",  HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(filter_scripts) },
            "read stream filtergraph description from a file", "filename" },
        { "reinit_filter",  HAS_ARG | OPT_INT | OPT_SPEC | OPT_INPUT,    { .off = OFFSET(reinit_filters) },
            "reinit filtergraph on input parameter changes", "" },
        { "filter_complex", HAS_ARG | OPT_EXPERT,                        { .func_arg = opt_filter_complex_ex },
            "create a complex filtergraph", "graph_description" },
        { "filter_complex_threads", HAS_ARG | OPT_INT,                   { &filter_complex_nbthreads },
            "number of threads for -filter_complex" },
        { "lavfi",          HAS_ARG | OPT_EXPERT,                        { .func_arg = opt_filter_complex_ex },
            "create a complex filtergraph", "graph_description" },
        { "filter_complex_script", HAS_ARG | OPT_EXPERT,                 { .func_arg = opt_filter_complex_script_ex },
            "read complex filtergraph description from a file", "filename" },
        { "stats",          OPT_BOOL,                                    { &print_stats },
            "print progress report during encoding", },
        { "attach",         HAS_ARG | OPT_PERFILE | OPT_EXPERT |
                            OPT_OUTPUT,                                  { .func_arg = opt_attach_ex },
            "add an attachment to the output file", "filename" },
        { "dump_attachment", HAS_ARG | OPT_STRING | OPT_SPEC |
                             OPT_EXPERT | OPT_INPUT,                     { .off = OFFSET(dump_attachment) },
            "extract an attachment into a file", "filename" },
        { "stream_loop", OPT_INT | HAS_ARG | OPT_EXPERT | OPT_INPUT |
                            OPT_OFFSET,                                  { .off = OFFSET(loop) }, "set number of times input stream shall be looped", "loop count" },
        { "debug_ts",       OPT_BOOL | OPT_EXPERT,                       { &debug_ts },
            "print timestamp debugging info" },
        { "max_error_rate",  HAS_ARG | OPT_FLOAT,                        { &max_error_rate },
            "ratio of errors (0.0: no errors, 1.0: 100% errors) above which ffmpeg returns an error instead of success.", "maximum error rate" },
        { "discard",        OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_INPUT,                                   { .off = OFFSET(discard) },
            "discard", "" },
        { "disposition",    OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off = OFFSET(disposition) },
            "disposition", "" },
        { "thread_queue_size", HAS_ARG | OPT_INT | OPT_OFFSET | OPT_EXPERT | OPT_INPUT,
                                                                         { .off = OFFSET(thread_queue_size) },
            "set the maximum number of queued packets from the demuxer" },
        { "find_stream_info", OPT_BOOL | OPT_PERFILE | OPT_INPUT | OPT_EXPERT, { &find_stream_info },
            "read and decode the streams to fill missing information with heuristics" },

        /* video options */
        { "vframes",      OPT_VIDEO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = opt_video_frames_ex },
            "set the number of video frames to output", "number" },
        { "r",            OPT_VIDEO | HAS_ARG  | OPT_STRING | OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(frame_rates) },
            "set frame rate (Hz value, fraction or abbreviation)", "rate" },
        { "s",            OPT_VIDEO | HAS_ARG | OPT_SUBTITLE | OPT_STRING | OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(frame_sizes) },
            "set frame size (WxH or abbreviation)", "size" },
        { "aspect",       OPT_VIDEO | HAS_ARG  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(frame_aspect_ratios) },
            "set aspect ratio (4:3, 16:9 or 1.3333, 1.7777)", "aspect" },
        { "pix_fmt",      OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(frame_pix_fmts) },
            "set pixel format", "format" },
        { "bits_per_raw_sample", OPT_VIDEO | OPT_INT | HAS_ARG,                      { &frame_bits_per_raw_sample },
            "set the number of bits per raw sample", "number" },
        { "intra",        OPT_VIDEO | OPT_BOOL | OPT_EXPERT,                         { &intra_only },
            "deprecated use -g 1" },
        { "vn",           OPT_VIDEO | OPT_BOOL  | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT,{ .off = OFFSET(video_disable) },
            "disable video" },
        { "rc_override",  OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(rc_overrides) },
            "rate control override for specific intervals", "override" },
        { "vcodec",       OPT_VIDEO | HAS_ARG  | OPT_PERFILE | OPT_INPUT |
                          OPT_OUTPUT,                                                { .func_arg = opt_video_codec_ex },
            "force video codec ('copy' to copy stream)", "codec" },
        { "sameq",        OPT_VIDEO | OPT_EXPERT ,                                   { .func_arg = opt_sameq_ex },
            "Removed" },
        { "same_quant",   OPT_VIDEO | OPT_EXPERT ,                                   { .func_arg = opt_sameq_ex },
            "Removed" },
        { "timecode",     OPT_VIDEO | HAS_ARG | OPT_PERFILE | OPT_OUTPUT,            { .func_arg = opt_timecode_ex },
            "set initial TimeCode value.", "hh:mm:ss[:;.]ff" },
        { "pass",         OPT_VIDEO | HAS_ARG | OPT_SPEC | OPT_INT | OPT_OUTPUT,     { .off = OFFSET(pass) },
            "select the pass number (1 to 3)", "n" },
        { "passlogfile",  OPT_VIDEO | HAS_ARG | OPT_STRING | OPT_EXPERT | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(passlogfiles) },
            "select two pass log file name prefix", "prefix" },
        { "deinterlace",  OPT_VIDEO | OPT_BOOL | OPT_EXPERT,                         { &do_deinterlace },
            "this option is deprecated, use the yadif filter instead" },
        { "psnr",         OPT_VIDEO | OPT_BOOL | OPT_EXPERT,                         { &do_psnr },
            "calculate PSNR of compressed frames" },
        { "vstats",       OPT_VIDEO | OPT_EXPERT ,                                   { .func_arg = opt_vstats_ex },
            "dump video coding statistics to file" },
        { "vstats_file",  OPT_VIDEO | HAS_ARG | OPT_EXPERT ,                         { .func_arg = opt_vstats_file_ex },
            "dump video coding statistics to file", "file" },
        { "vstats_version",  OPT_VIDEO | OPT_INT | HAS_ARG | OPT_EXPERT ,            { &vstats_version },
            "Version of the vstats format to use."},
        { "vf",           OPT_VIDEO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = opt_video_filters_ex },
            "set video filters", "filter_graph" },
        { "intra_matrix", OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(intra_matrices) },
            "specify intra matrix coeffs", "matrix" },
        { "inter_matrix", OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(inter_matrices) },
            "specify inter matrix coeffs", "matrix" },
        { "chroma_intra_matrix", OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(chroma_intra_matrices) },
            "specify intra matrix coeffs", "matrix" },
        { "top",          OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_INT| OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(top_field_first) },
            "top=1/bottom=0/auto=-1 field first", "" },
        { "vtag",         OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_PERFILE |
                          OPT_INPUT | OPT_OUTPUT,                                    { .func_arg = opt_old2new_ex },
            "force video tag/fourcc", "fourcc/tag" },
        { "qphist",       OPT_VIDEO | OPT_BOOL | OPT_EXPERT ,                        { &qp_hist },
            "show QP histogram" },
        { "force_fps",    OPT_VIDEO | OPT_BOOL | OPT_EXPERT  | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(force_fps) },
            "force the selected framerate, disable the best supported framerate selection" },
        { "streamid",     OPT_VIDEO | HAS_ARG | OPT_EXPERT | OPT_PERFILE |
                          OPT_OUTPUT,                                                { .func_arg = opt_streamid_ex },
            "set the value of an outfile streamid", "streamIndex:value" },
        { "force_key_frames", OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_OUTPUT,                                 { .off = OFFSET(forced_key_frames) },
            "force key frames at specified timestamps", "timestamps" },
        { "ab",           OPT_VIDEO | HAS_ARG | OPT_PERFILE | OPT_OUTPUT,            { .func_arg = opt_bitrate_ex },
            "audio bitrate (please use -b:a)", "bitrate" },
        { "b",            OPT_VIDEO | HAS_ARG | OPT_PERFILE | OPT_OUTPUT,            { .func_arg = opt_bitrate_ex },
            "video bitrate (please use -b:v)", "bitrate" },
        { "hwaccel",          OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_INPUT,                                  { .off = OFFSET(hwaccels) },
            "use HW accelerated decoding", "hwaccel name" },
        { "hwaccel_device",   OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_INPUT,                                  { .off = OFFSET(hwaccel_devices) },
            "select a device for HW acceleration", "devicename" },
        { "hwaccel_output_format", OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_INPUT,                                  { .off = OFFSET(hwaccel_output_formats) },
            "select output format used with HW accelerated decoding", "format" },
    #if CONFIG_VIDEOTOOLBOX
        { "videotoolbox_pixfmt", HAS_ARG | OPT_STRING | OPT_EXPERT, { &videotoolbox_pixfmt}, "" },
    #endif
        { "hwaccels",         OPT_EXIT,                                              { .func_arg = show_hwaccels_ex },
            "show available HW acceleration methods" },
        { "autorotate",       HAS_ARG | OPT_BOOL | OPT_SPEC |
                              OPT_EXPERT | OPT_INPUT,                                { .off = OFFSET(autorotate) },
            "automatically insert correct rotate filters" },

        /* audio options */
        { "aframes",        OPT_AUDIO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = opt_audio_frames_ex },
            "set the number of audio frames to output", "number" },
        { "aq",             OPT_AUDIO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = opt_audio_qscale_ex },
            "set audio quality (codec-specific)", "quality", },
        { "ar",             OPT_AUDIO | HAS_ARG  | OPT_INT | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(audio_sample_rate) },
            "set audio sampling rate (in Hz)", "rate" },
        { "ac",             OPT_AUDIO | HAS_ARG  | OPT_INT | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(audio_channels) },
            "set number of audio channels", "channels" },
        { "an",             OPT_AUDIO | OPT_BOOL | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT,{ .off = OFFSET(audio_disable) },
            "disable audio" },
        { "acodec",         OPT_AUDIO | HAS_ARG  | OPT_PERFILE |
                            OPT_INPUT | OPT_OUTPUT,                                    { .func_arg = opt_audio_codec_ex },
            "force audio codec ('copy' to copy stream)", "codec" },
        { "atag",           OPT_AUDIO | HAS_ARG  | OPT_EXPERT | OPT_PERFILE |
                            OPT_OUTPUT,                                                { .func_arg = opt_old2new_ex },
            "force audio tag/fourcc", "fourcc/tag" },
        { "vol",            OPT_AUDIO | HAS_ARG  | OPT_INT,                            { &audio_volume },
            "change audio volume (256=normal)" , "volume" },
        { "sample_fmt",     OPT_AUDIO | HAS_ARG  | OPT_EXPERT | OPT_SPEC |
                            OPT_STRING | OPT_INPUT | OPT_OUTPUT,                       { .off = OFFSET(sample_fmts) },
            "set sample format", "format" },
        { "channel_layout", OPT_AUDIO | HAS_ARG  | OPT_EXPERT | OPT_PERFILE |
                            OPT_INPUT | OPT_OUTPUT,                                    { .func_arg = opt_channel_layout_ex },
            "set channel layout", "layout" },
        { "af",             OPT_AUDIO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = opt_audio_filters_ex },
            "set audio filters", "filter_graph" },
        { "guess_layout_max", OPT_AUDIO | HAS_ARG | OPT_INT | OPT_SPEC | OPT_EXPERT | OPT_INPUT, { .off = OFFSET(guess_layout_max) },
          "set the maximum number of channels to try to guess the channel layout" },

        /* subtitle options */
        { "sn",     OPT_SUBTITLE | OPT_BOOL | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT, { .off = OFFSET(subtitle_disable) },
            "disable subtitle" },
        { "scodec", OPT_SUBTITLE | HAS_ARG  | OPT_PERFILE | OPT_INPUT | OPT_OUTPUT, { .func_arg = opt_subtitle_codec_ex },
            "force subtitle codec ('copy' to copy stream)", "codec" },
        { "stag",   OPT_SUBTITLE | HAS_ARG  | OPT_EXPERT  | OPT_PERFILE | OPT_OUTPUT, { .func_arg = opt_old2new_ex }
            , "force subtitle tag/fourcc", "fourcc/tag" },
        { "fix_sub_duration", OPT_BOOL | OPT_EXPERT | OPT_SUBTITLE | OPT_SPEC | OPT_INPUT, { .off = OFFSET(fix_sub_duration) },
            "fix subtitles duration" },
        { "canvas_size", OPT_SUBTITLE | HAS_ARG | OPT_STRING | OPT_SPEC | OPT_INPUT, { .off = OFFSET(canvas_sizes) },
            "set canvas size (WxH or abbreviation)", "size" },

        /* grab options */
        { "vc", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { .func_arg = opt_video_channel_ex },
            "deprecated, use -channel", "channel" },
        { "tvstd", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { .func_arg = opt_video_standard_ex },
            "deprecated, use -standard", "standard" },
        { "isync", OPT_BOOL | OPT_EXPERT, { &input_sync }, "this option is deprecated and does nothing", "" },

        /* muxer options */
        { "muxdelay",   OPT_FLOAT | HAS_ARG | OPT_EXPERT | OPT_OFFSET | OPT_OUTPUT, { .off = OFFSET(mux_max_delay) },
            "set the maximum demux-decode delay", "seconds" },
        { "muxpreload", OPT_FLOAT | HAS_ARG | OPT_EXPERT | OPT_OFFSET | OPT_OUTPUT, { .off = OFFSET(mux_preload) },
            "set the initial demux-decode delay", "seconds" },
        { "sdp_file", HAS_ARG | OPT_EXPERT | OPT_OUTPUT, { .func_arg = opt_sdp_file_ex },
            "specify a file in which to print sdp information", "file" },

        { "time_base", HAS_ARG | OPT_STRING | OPT_EXPERT | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(time_bases) },
            "set the desired time base hint for output stream (1:24, 1:48000 or 0.04166, 2.0833e-5)", "ratio" },
        { "enc_time_base", HAS_ARG | OPT_STRING | OPT_EXPERT | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(enc_time_bases) },
            "set the desired time base for the encoder (1:24, 1:48000 or 0.04166, 2.0833e-5). "
            "two special values are defined - "
            "0 = use frame rate (video) or sample rate (audio),"
            "-1 = match source time base", "ratio" },

        { "bsf", HAS_ARG | OPT_STRING | OPT_SPEC | OPT_EXPERT | OPT_OUTPUT, { .off = OFFSET(bitstream_filters) },
            "A comma-separated list of bitstream filters", "bitstream_filters" },
        { "absf", HAS_ARG | OPT_AUDIO | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT, { .func_arg = opt_old2new_ex },
            "deprecated", "audio bitstream_filters" },
        { "vbsf", OPT_VIDEO | HAS_ARG | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT, { .func_arg = opt_old2new_ex },
            "deprecated", "video bitstream_filters" },

        { "apre", HAS_ARG | OPT_AUDIO | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT,    { .func_arg = opt_preset_ex },
            "set the audio options to the indicated preset", "preset" },
        { "vpre", OPT_VIDEO | HAS_ARG | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT,    { .func_arg = opt_preset_ex },
            "set the video options to the indicated preset", "preset" },
        { "spre", HAS_ARG | OPT_SUBTITLE | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT, { .func_arg = opt_preset_ex },
            "set the subtitle options to the indicated preset", "preset" },
        { "fpre", HAS_ARG | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT,                { .func_arg = opt_preset_ex },
            "set options from indicated preset file", "filename" },

        { "max_muxing_queue_size", HAS_ARG | OPT_INT | OPT_SPEC | OPT_EXPERT | OPT_OUTPUT, { .off = OFFSET(max_muxing_queue_size) },
            "maximum number of packets that can be buffered while waiting for all streams to initialize", "packets" },

        /* data codec support */
        { "dcodec", HAS_ARG | OPT_DATA | OPT_PERFILE | OPT_EXPERT | OPT_INPUT | OPT_OUTPUT, { .func_arg = opt_data_codec_ex },
            "force data codec ('copy' to copy stream)", "codec" },
        { "dn", OPT_BOOL | OPT_VIDEO | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT, { .off = OFFSET(data_disable) },
            "disable data" },

    #if CONFIG_VAAPI
        { "vaapi_device", HAS_ARG | OPT_EXPERT, { .func_arg = opt_vaapi_device },
            "set VAAPI hardware device (DRM path or X11 display name)", "device" },
    #endif

    #if CONFIG_QSV
        { "qsv_device", HAS_ARG | OPT_STRING | OPT_EXPERT, { &qsv_device },
            "set QSV hardware device (DirectX adapter index, DRM path or X11 display name)", "device"},
    #endif

        { "init_hw_device", HAS_ARG | OPT_EXPERT, { .func_arg = opt_init_hw_device_ex },
            "initialise hardware device", "args" },
        { "filter_hw_device", HAS_ARG | OPT_EXPERT, { .func_arg = opt_filter_hw_device_ex },
            "set hardware device used when filtering", "device" },

        { NULL, },
    };

    //const OptionDef options = tmpoptions;
    int arrLen = sizeof(tmpoptions)/sizeof(tmpoptions[0]);
    options = new OptionDef[arrLen];
    if(!options){
        av_log(NULL, AV_LOG_FATAL, "init_g_options failed, return\n");
        return;
    }

    for(int i = 0; i < arrLen; i++){
        options[i] = tmpoptions[i];
    }

    av_log(NULL, AV_LOG_DEBUG, "init_g_options successful\n");

#else
    #define OFFSET(x) offsetof(OptionsContext, x)
    const OptionDef options[] = {
        /* main options */
        //CMDUTILS_COMMON_OPTIONS
        { "f",              HAS_ARG | OPT_STRING | OPT_OFFSET |
                            OPT_INPUT | OPT_OUTPUT,                      { .off       = OFFSET(format) },
            "force format", "fmt" },
        { "y",              OPT_BOOL,                                    {              &file_overwrite },
            "overwrite output files" },
        { "n",              OPT_BOOL,                                    {              &no_file_overwrite },
            "never overwrite output files" },
        { "ignore_unknown", OPT_BOOL,                                    {              &ignore_unknown_streams },
            "Ignore unknown stream types" },
        { "copy_unknown",   OPT_BOOL | OPT_EXPERT,                       {              &copy_unknown_streams },
            "Copy unknown stream types" },
        { "c",              HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                      { .off       = OFFSET(codec_names) },
            "codec name", "codec" },
        { "codec",          HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                      { .off       = OFFSET(codec_names) },
            "codec name", "codec" },
        { "pre",            HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off       = OFFSET(presets) },
            "preset name", "preset" },
        { "map",            HAS_ARG | OPT_EXPERT | OPT_PERFILE |
                            OPT_OUTPUT,                                  { .func_arg = /*opt_map*/NULL },
            "set input stream mapping",
            "[-]input_file_id[:stream_specifier][,sync_file_id[:stream_specifier]]" },
        { "map_channel",    HAS_ARG | OPT_EXPERT | OPT_PERFILE | OPT_OUTPUT, { .func_arg = /*opt_map_channel*/NULL },
            "map an audio channel from one stream to another", "file.stream.channel[:syncfile.syncstream]" },
        { "map_metadata",   HAS_ARG | OPT_STRING | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off       = OFFSET(metadata_map) },
            "set metadata information of outfile from infile",
            "outfile[,metadata]:infile[,metadata]" },
        { "map_chapters",   HAS_ARG | OPT_INT | OPT_EXPERT | OPT_OFFSET |
                            OPT_OUTPUT,                                  { .off = OFFSET(chapters_input_file) },
            "set chapters mapping", "input_file_index" },
        { "t",              HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_INPUT | OPT_OUTPUT,                      { .off = OFFSET(recording_time) },
            "record or transcode \"duration\" seconds of audio/video",
            "duration" },
        { "to",             HAS_ARG | OPT_TIME | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT,  { .off = OFFSET(stop_time) },
            "record or transcode stop time", "time_stop" },
        { "fs",             HAS_ARG | OPT_INT64 | OPT_OFFSET | OPT_OUTPUT, { .off = OFFSET(limit_filesize) },
            "set the limit file size in bytes", "limit_size" },
        { "ss",             HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_INPUT | OPT_OUTPUT,                      { .off = OFFSET(start_time) },
            "set the start time offset", "time_off" },
        { "sseof",          HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_INPUT,                                   { .off = OFFSET(start_time_eof) },
            "set the start time offset relative to EOF", "time_off" },
        { "seek_timestamp", HAS_ARG | OPT_INT | OPT_OFFSET |
                            OPT_INPUT,                                   { .off = OFFSET(seek_timestamp) },
            "enable/disable seeking by timestamp with -ss" },
        { "accurate_seek",  OPT_BOOL | OPT_OFFSET | OPT_EXPERT |
                            OPT_INPUT,                                   { .off = OFFSET(accurate_seek) },
            "enable/disable accurate seeking with -ss" },
        { "itsoffset",      HAS_ARG | OPT_TIME | OPT_OFFSET |
                            OPT_EXPERT | OPT_INPUT,                      { .off = OFFSET(input_ts_offset) },
            "set the input ts offset", "time_off" },
        { "itsscale",       HAS_ARG | OPT_DOUBLE | OPT_SPEC |
                            OPT_EXPERT | OPT_INPUT,                      { .off = OFFSET(ts_scale) },
            "set the input ts scale", "scale" },
        { "timestamp",      HAS_ARG | OPT_PERFILE | OPT_OUTPUT,          { .func_arg = /*opt_recording_timestamp*/NULL },
            "set the recording timestamp ('now' to set the current time)", "time" },
        { "metadata",       HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(metadata) },
            "add metadata", "string=string" },
        { "program",        HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(program) },
            "add program with specified streams", "title=string:st=number..." },
        { "dframes",        HAS_ARG | OPT_PERFILE | OPT_EXPERT |
                            OPT_OUTPUT,                                  { .func_arg = /*opt_data_frames*/NULL },
            "set the number of data frames to output", "number" },
        { "benchmark",      OPT_BOOL | OPT_EXPERT,                       { &do_benchmark },
            "add timings for benchmarking" },
        { "benchmark_all",  OPT_BOOL | OPT_EXPERT,                       { &do_benchmark_all },
          "add timings for each task" },
        { "progress",       HAS_ARG | OPT_EXPERT,                        { .func_arg = /*opt_progress*/NULL },
          "write program-readable progress information", "url" },
        { "stdin",          OPT_BOOL | OPT_EXPERT,                       { &stdin_interaction },
          "enable or disable interaction on standard input" },
        { "timelimit",      HAS_ARG | OPT_EXPERT,                        { .func_arg = /*opt_timelimit*/NULL },
            "set max runtime in seconds", "limit" },
        { "dump",           OPT_BOOL | OPT_EXPERT,                       { &do_pkt_dump },
            "dump each input packet" },
        { "hex",            OPT_BOOL | OPT_EXPERT,                       { &do_hex_dump },
            "when dumping packets, also dump the payload" },
        { "re",             OPT_BOOL | OPT_EXPERT | OPT_OFFSET |
                            OPT_INPUT,                                   { .off = OFFSET(rate_emu) },
            "read input at native frame rate", "" },
        { "target",         HAS_ARG | OPT_PERFILE | OPT_OUTPUT,          { .func_arg = /*opt_target*/NULL },
            "specify target file type (\"vcd\", \"svcd\", \"dvd\", \"dv\" or \"dv50\" "
            "with optional prefixes \"pal-\", \"ntsc-\" or \"film-\")", "type" },
        { "vsync",          HAS_ARG | OPT_EXPERT,                        { .func_arg = /*opt_vsync*/NULL },
            "video sync method", "" },
        { "frame_drop_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,      { &frame_drop_threshold },
            "frame drop threshold", "" },
        { "async",          HAS_ARG | OPT_INT | OPT_EXPERT,              { &audio_sync_method },
            "audio sync method", "" },
        { "adrift_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,          { &audio_drift_threshold },
            "audio drift threshold", "threshold" },
        { "copyts",         OPT_BOOL | OPT_EXPERT,                       { &copy_ts },
            "copy timestamps" },
        { "start_at_zero",  OPT_BOOL | OPT_EXPERT,                       { &start_at_zero },
            "shift input timestamps to start at 0 when using copyts" },
        { "copytb",         HAS_ARG | OPT_INT | OPT_EXPERT,              { &copy_tb },
            "copy input stream time base when stream copying", "mode" },
        { "shortest",       OPT_BOOL | OPT_EXPERT | OPT_OFFSET |
                            OPT_OUTPUT,                                  { .off = OFFSET(shortest) },
            "finish encoding within shortest input" },
        { "bitexact",       OPT_BOOL | OPT_EXPERT | OPT_OFFSET |
                            OPT_OUTPUT | OPT_INPUT,                      { .off = OFFSET(bitexact) },
            "bitexact mode" },
        { "apad",           OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off = OFFSET(apad) },
            "audio pad", "" },
        { "dts_delta_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,       { &dts_delta_threshold },
            "timestamp discontinuity delta threshold", "threshold" },
        { "dts_error_threshold", HAS_ARG | OPT_FLOAT | OPT_EXPERT,       { &dts_error_threshold },
            "timestamp error delta threshold", "threshold" },
        { "xerror",         OPT_BOOL | OPT_EXPERT,                       { &exit_on_error },
            "exit on error", "error" },
        { "abort_on",       HAS_ARG | OPT_EXPERT,                        { .func_arg = /*opt_abort_on*/NULL },
            "abort on the specified condition flags", "flags" },
        { "copyinkf",       OPT_BOOL | OPT_EXPERT | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off = OFFSET(copy_initial_nonkeyframes) },
            "copy initial non-keyframes" },
        { "copypriorss",    OPT_INT | HAS_ARG | OPT_EXPERT | OPT_SPEC | OPT_OUTPUT,   { .off = OFFSET(copy_prior_start) },
            "copy or discard frames before start time" },
        { "frames",         OPT_INT64 | HAS_ARG | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(max_frames) },
            "set the number of frames to output", "number" },
        { "tag",            OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_EXPERT | OPT_OUTPUT | OPT_INPUT,         { .off = OFFSET(codec_tags) },
            "force codec tag/fourcc", "fourcc/tag" },
        { "q",              HAS_ARG | OPT_EXPERT | OPT_DOUBLE |
                            OPT_SPEC | OPT_OUTPUT,                       { .off = OFFSET(qscale) },
            "use fixed quality scale (VBR)", "q" },
        { "qscale",         HAS_ARG | OPT_EXPERT | OPT_PERFILE |
                            OPT_OUTPUT,                                  { .func_arg = /*opt_qscale*/NULL },
            "use fixed quality scale (VBR)", "q" },
        { "profile",        HAS_ARG | OPT_EXPERT | OPT_PERFILE | OPT_OUTPUT, { .func_arg = /*opt_profile*/NULL },
            "set profile", "profile" },
        { "filter",         HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(filters) },
            "set stream filtergraph", "filter_graph" },
        { "filter_threads",  HAS_ARG | OPT_INT,                          { &filter_nbthreads },
            "number of non-complex filter threads" },
        { "filter_script",  HAS_ARG | OPT_STRING | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(filter_scripts) },
            "read stream filtergraph description from a file", "filename" },
        { "reinit_filter",  HAS_ARG | OPT_INT | OPT_SPEC | OPT_INPUT,    { .off = OFFSET(reinit_filters) },
            "reinit filtergraph on input parameter changes", "" },
        { "filter_complex", HAS_ARG | OPT_EXPERT,                        { .func_arg = /*opt_filter_complex*/NULL },
            "create a complex filtergraph", "graph_description" },
        { "filter_complex_threads", HAS_ARG | OPT_INT,                   { &filter_complex_nbthreads },
            "number of threads for -filter_complex" },
        { "lavfi",          HAS_ARG | OPT_EXPERT,                        { .func_arg = /*opt_filter_complex*/NULL },
            "create a complex filtergraph", "graph_description" },
        { "filter_complex_script", HAS_ARG | OPT_EXPERT,                 { .func_arg = /*opt_filter_complex_script*/NULL },
            "read complex filtergraph description from a file", "filename" },
        { "stats",          OPT_BOOL,                                    { &print_stats },
            "print progress report during encoding", },
        { "attach",         HAS_ARG | OPT_PERFILE | OPT_EXPERT |
                            OPT_OUTPUT,                                  { .func_arg = /*opt_attach*/NULL },
            "add an attachment to the output file", "filename" },
        { "dump_attachment", HAS_ARG | OPT_STRING | OPT_SPEC |
                             OPT_EXPERT | OPT_INPUT,                     { .off = OFFSET(dump_attachment) },
            "extract an attachment into a file", "filename" },
        { "stream_loop", OPT_INT | HAS_ARG | OPT_EXPERT | OPT_INPUT |
                            OPT_OFFSET,                                  { .off = OFFSET(loop) }, "set number of times input stream shall be looped", "loop count" },
        { "debug_ts",       OPT_BOOL | OPT_EXPERT,                       { &debug_ts },
            "print timestamp debugging info" },
        { "max_error_rate",  HAS_ARG | OPT_FLOAT,                        { &max_error_rate },
            "ratio of errors (0.0: no errors, 1.0: 100% errors) above which ffmpeg returns an error instead of success.", "maximum error rate" },
        { "discard",        OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_INPUT,                                   { .off = OFFSET(discard) },
            "discard", "" },
        { "disposition",    OPT_STRING | HAS_ARG | OPT_SPEC |
                            OPT_OUTPUT,                                  { .off = OFFSET(disposition) },
            "disposition", "" },
        { "thread_queue_size", HAS_ARG | OPT_INT | OPT_OFFSET | OPT_EXPERT | OPT_INPUT,
                                                                         { .off = OFFSET(thread_queue_size) },
            "set the maximum number of queued packets from the demuxer" },
        { "find_stream_info", OPT_BOOL | OPT_PERFILE | OPT_INPUT | OPT_EXPERT, { &find_stream_info },
            "read and decode the streams to fill missing information with heuristics" },

        /* video options */
        { "vframes",      OPT_VIDEO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = /*opt_video_frames*/NULL },
            "set the number of video frames to output", "number" },
        { "r",            OPT_VIDEO | HAS_ARG  | OPT_STRING | OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(frame_rates) },
            "set frame rate (Hz value, fraction or abbreviation)", "rate" },
        { "s",            OPT_VIDEO | HAS_ARG | OPT_SUBTITLE | OPT_STRING | OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(frame_sizes) },
            "set frame size (WxH or abbreviation)", "size" },
        { "aspect",       OPT_VIDEO | HAS_ARG  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(frame_aspect_ratios) },
            "set aspect ratio (4:3, 16:9 or 1.3333, 1.7777)", "aspect" },
        { "pix_fmt",      OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(frame_pix_fmts) },
            "set pixel format", "format" },
        { "bits_per_raw_sample", OPT_VIDEO | OPT_INT | HAS_ARG,                      { &frame_bits_per_raw_sample },
            "set the number of bits per raw sample", "number" },
        { "intra",        OPT_VIDEO | OPT_BOOL | OPT_EXPERT,                         { &intra_only },
            "deprecated use -g 1" },
        { "vn",           OPT_VIDEO | OPT_BOOL  | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT,{ .off = OFFSET(video_disable) },
            "disable video" },
        { "rc_override",  OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(rc_overrides) },
            "rate control override for specific intervals", "override" },
        { "vcodec",       OPT_VIDEO | HAS_ARG  | OPT_PERFILE | OPT_INPUT |
                          OPT_OUTPUT,                                                { .func_arg = /*opt_video_codec*/NULL },
            "force video codec ('copy' to copy stream)", "codec" },
        { "sameq",        OPT_VIDEO | OPT_EXPERT ,                                   { .func_arg = /*opt_sameq*/NULL },
            "Removed" },
        { "same_quant",   OPT_VIDEO | OPT_EXPERT ,                                   { .func_arg = /*opt_sameq*/NULL },
            "Removed" },
        { "timecode",     OPT_VIDEO | HAS_ARG | OPT_PERFILE | OPT_OUTPUT,            { .func_arg = /*opt_timecode*/NULL },
            "set initial TimeCode value.", "hh:mm:ss[:;.]ff" },
        { "pass",         OPT_VIDEO | HAS_ARG | OPT_SPEC | OPT_INT | OPT_OUTPUT,     { .off = OFFSET(pass) },
            "select the pass number (1 to 3)", "n" },
        { "passlogfile",  OPT_VIDEO | HAS_ARG | OPT_STRING | OPT_EXPERT | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(passlogfiles) },
            "select two pass log file name prefix", "prefix" },
        { "deinterlace",  OPT_VIDEO | OPT_BOOL | OPT_EXPERT,                         { &do_deinterlace },
            "this option is deprecated, use the yadif filter instead" },
        { "psnr",         OPT_VIDEO | OPT_BOOL | OPT_EXPERT,                         { &do_psnr },
            "calculate PSNR of compressed frames" },
        { "vstats",       OPT_VIDEO | OPT_EXPERT ,                                   { .func_arg = /*opt_vstats*/NULL },
            "dump video coding statistics to file" },
        { "vstats_file",  OPT_VIDEO | HAS_ARG | OPT_EXPERT ,                         { .func_arg = /*opt_vstats_file*/NULL },
            "dump video coding statistics to file", "file" },
        { "vstats_version",  OPT_VIDEO | OPT_INT | HAS_ARG | OPT_EXPERT ,            { &vstats_version },
            "Version of the vstats format to use."},
        { "vf",           OPT_VIDEO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = /*opt_video_filters*/NULL },
            "set video filters", "filter_graph" },
        { "intra_matrix", OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(intra_matrices) },
            "specify intra matrix coeffs", "matrix" },
        { "inter_matrix", OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(inter_matrices) },
            "specify inter matrix coeffs", "matrix" },
        { "chroma_intra_matrix", OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_STRING | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(chroma_intra_matrices) },
            "specify intra matrix coeffs", "matrix" },
        { "top",          OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_INT| OPT_SPEC |
                          OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(top_field_first) },
            "top=1/bottom=0/auto=-1 field first", "" },
        { "vtag",         OPT_VIDEO | HAS_ARG | OPT_EXPERT  | OPT_PERFILE |
                          OPT_INPUT | OPT_OUTPUT,                                    { .func_arg = /*opt_old2new*/NULL },
            "force video tag/fourcc", "fourcc/tag" },
        { "qphist",       OPT_VIDEO | OPT_BOOL | OPT_EXPERT ,                        { &qp_hist },
            "show QP histogram" },
        { "force_fps",    OPT_VIDEO | OPT_BOOL | OPT_EXPERT  | OPT_SPEC |
                          OPT_OUTPUT,                                                { .off = OFFSET(force_fps) },
            "force the selected framerate, disable the best supported framerate selection" },
        { "streamid",     OPT_VIDEO | HAS_ARG | OPT_EXPERT | OPT_PERFILE |
                          OPT_OUTPUT,                                                { .func_arg = /*opt_streamid*/NULL },
            "set the value of an outfile streamid", "streamIndex:value" },
        { "force_key_frames", OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_OUTPUT,                                 { .off = OFFSET(forced_key_frames) },
            "force key frames at specified timestamps", "timestamps" },
        { "ab",           OPT_VIDEO | HAS_ARG | OPT_PERFILE | OPT_OUTPUT,            { .func_arg = /*opt_bitrate*/NULL },
            "audio bitrate (please use -b:a)", "bitrate" },
        { "b",            OPT_VIDEO | HAS_ARG | OPT_PERFILE | OPT_OUTPUT,            { .func_arg = /*opt_bitrate*/NULL },
            "video bitrate (please use -b:v)", "bitrate" },
        { "hwaccel",          OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_INPUT,                                  { .off = OFFSET(hwaccels) },
            "use HW accelerated decoding", "hwaccel name" },
        { "hwaccel_device",   OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_INPUT,                                  { .off = OFFSET(hwaccel_devices) },
            "select a device for HW acceleration", "devicename" },
        { "hwaccel_output_format", OPT_VIDEO | OPT_STRING | HAS_ARG | OPT_EXPERT |
                              OPT_SPEC | OPT_INPUT,                                  { .off = OFFSET(hwaccel_output_formats) },
            "select output format used with HW accelerated decoding", "format" },
    #if CONFIG_VIDEOTOOLBOX
        { "videotoolbox_pixfmt", HAS_ARG | OPT_STRING | OPT_EXPERT, { &videotoolbox_pixfmt}, "" },
    #endif
        { "hwaccels",         OPT_EXIT,                                              { .func_arg = /*show_hwaccels*/NULL },
            "show available HW acceleration methods" },
        { "autorotate",       HAS_ARG | OPT_BOOL | OPT_SPEC |
                              OPT_EXPERT | OPT_INPUT,                                { .off = OFFSET(autorotate) },
            "automatically insert correct rotate filters" },

        /* audio options */
        { "aframes",        OPT_AUDIO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = /*opt_audio_frames*/NULL },
            "set the number of audio frames to output", "number" },
        { "aq",             OPT_AUDIO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = /*opt_audio_qscale*/NULL },
            "set audio quality (codec-specific)", "quality", },
        { "ar",             OPT_AUDIO | HAS_ARG  | OPT_INT | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(audio_sample_rate) },
            "set audio sampling rate (in Hz)", "rate" },
        { "ac",             OPT_AUDIO | HAS_ARG  | OPT_INT | OPT_SPEC |
                            OPT_INPUT | OPT_OUTPUT,                                    { .off = OFFSET(audio_channels) },
            "set number of audio channels", "channels" },
        { "an",             OPT_AUDIO | OPT_BOOL | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT,{ .off = OFFSET(audio_disable) },
            "disable audio" },
        { "acodec",         OPT_AUDIO | HAS_ARG  | OPT_PERFILE |
                            OPT_INPUT | OPT_OUTPUT,                                    { .func_arg = /*opt_audio_codec*/NULL },
            "force audio codec ('copy' to copy stream)", "codec" },
        { "atag",           OPT_AUDIO | HAS_ARG  | OPT_EXPERT | OPT_PERFILE |
                            OPT_OUTPUT,                                                { .func_arg = /*opt_old2new*/NULL },
            "force audio tag/fourcc", "fourcc/tag" },
        { "vol",            OPT_AUDIO | HAS_ARG  | OPT_INT,                            { &audio_volume },
            "change audio volume (256=normal)" , "volume" },
        { "sample_fmt",     OPT_AUDIO | HAS_ARG  | OPT_EXPERT | OPT_SPEC |
                            OPT_STRING | OPT_INPUT | OPT_OUTPUT,                       { .off = OFFSET(sample_fmts) },
            "set sample format", "format" },
        { "channel_layout", OPT_AUDIO | HAS_ARG  | OPT_EXPERT | OPT_PERFILE |
                            OPT_INPUT | OPT_OUTPUT,                                    { .func_arg = /*opt_channel_layout*/NULL },
            "set channel layout", "layout" },
        { "af",             OPT_AUDIO | HAS_ARG  | OPT_PERFILE | OPT_OUTPUT,           { .func_arg = /*opt_audio_filters*/NULL },
            "set audio filters", "filter_graph" },
        { "guess_layout_max", OPT_AUDIO | HAS_ARG | OPT_INT | OPT_SPEC | OPT_EXPERT | OPT_INPUT, { .off = OFFSET(guess_layout_max) },
          "set the maximum number of channels to try to guess the channel layout" },

        /* subtitle options */
        { "sn",     OPT_SUBTITLE | OPT_BOOL | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT, { .off = OFFSET(subtitle_disable) },
            "disable subtitle" },
        { "scodec", OPT_SUBTITLE | HAS_ARG  | OPT_PERFILE | OPT_INPUT | OPT_OUTPUT, { .func_arg = /*opt_subtitle_codec*/NULL },
            "force subtitle codec ('copy' to copy stream)", "codec" },
        { "stag",   OPT_SUBTITLE | HAS_ARG  | OPT_EXPERT  | OPT_PERFILE | OPT_OUTPUT, { .func_arg = /*opt_old2new*/NULL }
            , "force subtitle tag/fourcc", "fourcc/tag" },
        { "fix_sub_duration", OPT_BOOL | OPT_EXPERT | OPT_SUBTITLE | OPT_SPEC | OPT_INPUT, { .off = OFFSET(fix_sub_duration) },
            "fix subtitles duration" },
        { "canvas_size", OPT_SUBTITLE | HAS_ARG | OPT_STRING | OPT_SPEC | OPT_INPUT, { .off = OFFSET(canvas_sizes) },
            "set canvas size (WxH or abbreviation)", "size" },

        /* grab options */
        { "vc", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { .func_arg = /*opt_video_channel*/NULL },
            "deprecated, use -channel", "channel" },
        { "tvstd", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { .func_arg = /*opt_video_standard*/NULL },
            "deprecated, use -standard", "standard" },
        { "isync", OPT_BOOL | OPT_EXPERT, { &input_sync }, "this option is deprecated and does nothing", "" },

        /* muxer options */
        { "muxdelay",   OPT_FLOAT | HAS_ARG | OPT_EXPERT | OPT_OFFSET | OPT_OUTPUT, { .off = OFFSET(mux_max_delay) },
            "set the maximum demux-decode delay", "seconds" },
        { "muxpreload", OPT_FLOAT | HAS_ARG | OPT_EXPERT | OPT_OFFSET | OPT_OUTPUT, { .off = OFFSET(mux_preload) },
            "set the initial demux-decode delay", "seconds" },
        { "sdp_file", HAS_ARG | OPT_EXPERT | OPT_OUTPUT, { .func_arg = /*opt_sdp_file*/NULL },
            "specify a file in which to print sdp information", "file" },

        { "time_base", HAS_ARG | OPT_STRING | OPT_EXPERT | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(time_bases) },
            "set the desired time base hint for output stream (1:24, 1:48000 or 0.04166, 2.0833e-5)", "ratio" },
        { "enc_time_base", HAS_ARG | OPT_STRING | OPT_EXPERT | OPT_SPEC | OPT_OUTPUT, { .off = OFFSET(enc_time_bases) },
            "set the desired time base for the encoder (1:24, 1:48000 or 0.04166, 2.0833e-5). "
            "two special values are defined - "
            "0 = use frame rate (video) or sample rate (audio),"
            "-1 = match source time base", "ratio" },

        { "bsf", HAS_ARG | OPT_STRING | OPT_SPEC | OPT_EXPERT | OPT_OUTPUT, { .off = OFFSET(bitstream_filters) },
            "A comma-separated list of bitstream filters", "bitstream_filters" },
        { "absf", HAS_ARG | OPT_AUDIO | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT, { .func_arg = /*opt_old2new*/NULL },
            "deprecated", "audio bitstream_filters" },
        { "vbsf", OPT_VIDEO | HAS_ARG | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT, { .func_arg = /*opt_old2new*/NULL },
            "deprecated", "video bitstream_filters" },

        { "apre", HAS_ARG | OPT_AUDIO | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT,    { .func_arg = /*opt_preset*/NULL },
            "set the audio options to the indicated preset", "preset" },
        { "vpre", OPT_VIDEO | HAS_ARG | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT,    { .func_arg = /*opt_preset*/NULL },
            "set the video options to the indicated preset", "preset" },
        { "spre", HAS_ARG | OPT_SUBTITLE | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT, { .func_arg = /*opt_preset*/NULL },
            "set the subtitle options to the indicated preset", "preset" },
        { "fpre", HAS_ARG | OPT_EXPERT| OPT_PERFILE | OPT_OUTPUT,                { .func_arg = /*opt_preset*/NULL },
            "set options from indicated preset file", "filename" },

        { "max_muxing_queue_size", HAS_ARG | OPT_INT | OPT_SPEC | OPT_EXPERT | OPT_OUTPUT, { .off = OFFSET(max_muxing_queue_size) },
            "maximum number of packets that can be buffered while waiting for all streams to initialize", "packets" },

        /* data codec support */
        { "dcodec", HAS_ARG | OPT_DATA | OPT_PERFILE | OPT_EXPERT | OPT_INPUT | OPT_OUTPUT, { .func_arg = /*opt_data_codec*/NULL },
            "force data codec ('copy' to copy stream)", "codec" },
        { "dn", OPT_BOOL | OPT_VIDEO | OPT_OFFSET | OPT_INPUT | OPT_OUTPUT, { .off = OFFSET(data_disable) },
            "disable data" },

    #if CONFIG_VAAPI
        { "vaapi_device", HAS_ARG | OPT_EXPERT, { .func_arg = opt_vaapi_device },
            "set VAAPI hardware device (DRM path or X11 display name)", "device" },
    #endif

    #if CONFIG_QSV
        { "qsv_device", HAS_ARG | OPT_STRING | OPT_EXPERT, { &qsv_device },
            "set QSV hardware device (DirectX adapter index, DRM path or X11 display name)", "device"},
    #endif

        { "init_hw_device", HAS_ARG | OPT_EXPERT, { .func_arg = /*opt_init_hw_device*/NULL },
            "initialise hardware device", "args" },
        { "filter_hw_device", HAS_ARG | OPT_EXPERT, { .func_arg = /*opt_filter_hw_device*/NULL },
            "set hardware device used when filtering", "device" },

        { NULL, },
    };
#endif
}

void FFmpegMedia::init_options(OptionsContext *o)
{
    memset(o, 0, sizeof(*o));

    o->stop_time = INT64_MAX;
    o->mux_max_delay  = 0.7;
    o->start_time     = AV_NOPTS_VALUE;
    o->start_time_eof = AV_NOPTS_VALUE;
    o->recording_time = INT64_MAX;
    o->limit_filesize = UINT64_MAX;
    o->chapters_input_file = INT_MAX;
    o->accurate_seek  = 1;
}

void FFmpegMedia::uninit_options(OptionsContext *o)
{
    const OptionDef *po = options;
    int i;

    /* all OPT_SPEC and OPT_STRING can be freed in generic way */
    while (po->name) {
        void *dst = (uint8_t*)o + po->u.off;

        // 1. 保存在SpecifierOpt结构的，需要被释放
        if (po->flags & OPT_SPEC) {
            SpecifierOpt **so = (SpecifierOpt **)dst;
            int i, *count = (int*)(so + 1);
            for (i = 0; i < *count; i++) {
                av_freep(&(*so)[i].specifier);
                if (po->flags & OPT_STRING)
                    av_freep(&(*so)[i].u.str);
            }
            av_freep(so);
            *count = 0;
        } else if (po->flags & OPT_OFFSET && po->flags & OPT_STRING)
            // 2. 具有OPT_OFFSET标志且是字符串，同样需要被释放
            av_freep(dst);
        po++;
    }

    for (i = 0; i < o->nb_stream_maps; i++)
        av_freep(&o->stream_maps[i].linklabel);
    av_freep(&o->stream_maps);
    av_freep(&o->audio_channel_maps);
    av_freep(&o->streamid_map);
    av_freep(&o->attachments);
}

int FFmpegMedia::init_complex_filters(void)
{
    int i, ret = 0;

    for (i = 0; i < nb_filtergraphs; i++) {
        ret = init_complex_filtergraph(filtergraphs[i]);
        if (ret < 0)
            return ret;
    }
    return 0;
}

AVCodec *FFmpegMedia::find_codec_or_die(const char *name, enum AVMediaType type, int encoder){
    const AVCodecDescriptor *desc;
    const char *codec_string = encoder ? "encoder" : "decoder";
    AVCodec *codec;

    // 1. 根据名字寻找编解码器
    codec = encoder ?
        avcodec_find_encoder_by_name(name) :
        avcodec_find_decoder_by_name(name);

    // 2. 如果根据名字找不到，则使用名字先找编解码器的描述符，再使用描述符寻找编解码器.
    if (!codec && (desc = avcodec_descriptor_get_by_name(name))) {// 根据名字获取编解码器的描述符
        codec = encoder ? avcodec_find_encoder(desc->id) :
                          avcodec_find_decoder(desc->id);
        if (codec)
            av_log(NULL, AV_LOG_VERBOSE, "Matched %s '%s' for codec '%s'.\n",
                   codec_string, codec->name, desc->name);
    }

    if (!codec) {
        av_log(NULL, AV_LOG_FATAL, "Unknown %s '%s'\n", codec_string, name);
        //exit_program(1);
        return nullptr;
    }

    // 3. 对比.根据用户传进的名字找到的编解码器的媒体类型，若与用户传进的不一致，退出程序.
    // 例如用户传进:-codec:a libx264，那么MATCH_PER_TYPE_OPT匹配时，audio_codec_name="libx264"
    // 虽然找到编解码器，但音频是没有libx264的，这个找到编解码器是视频的，所以退出.
    // codec->type是通过用户输入的名字找到的编解码器名字的类型
    if (codec->type != type) {
        av_log(NULL, AV_LOG_FATAL, "Invalid %s type '%s'\n", codec_string, name);
        //exit_program(1);
        return nullptr;
    }
    return codec;
}

/**
 * @brief 利用b中的key，将a中有同样key的value设置为NULL.获取时忽略后缀，设置时匹配大小写.
 * @param a 字典1
 * @param b 字典2
*/
void FFmpegMedia::remove_avoptions(AVDictionary **a, AVDictionary *b)
{
    AVDictionaryEntry *t = NULL;

    while ((t = av_dict_get(b, "", t, AV_DICT_IGNORE_SUFFIX))) {
        av_dict_set(a, t->key, NULL, AV_DICT_MATCH_CASE);
    }
}

/**
 * @brief 只有m中还有一个k-v键值对选项，那么程序就会退出.
*/
void FFmpegMedia::assert_avoptions(AVDictionary *m)
{
    AVDictionaryEntry *t;
    if ((t = av_dict_get(m, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_FATAL, "Option %s not found.\n", t->key);
        //exit_program(1);// 不退出而忽略该选项
    }
}

int FFmpegMedia::open_files(OptionGroupList *l, const char *inout,
                      int (*open_file)(void*, OptionsContext*, const char*))
{
    int i, ret;

    /* 1.经过对命令行的分析将输入文件链表放到了l中，在此逐一对输入文件进行处理 */
    /* l->nb_groups指输入文件个数 */
    for (i = 0; i < l->nb_groups; i++) {
        OptionGroup *g = &l->groups[i];// 拿到一个输入文件进行处理.
        OptionsContext o;

        /* 2.对OptionsContext 中的变量做默认设置，这个结构体很重要，它和ffmpeg_opt.c中定义的const OptionDef options[]相对应。
         * 其中设置的偏移量就是针对这个结构体*/
        init_options(&o);
        o.g = g;
        printf("open_files &o.codec_names: %#X, o.codec_names: %#X\n", &o.codec_names, o.codec_names);

        /* 3.将g中存的，从命令行中获取的针对这个输入文件的参数放到o中 */
        ret = parse_optgroup(&o, g);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error parsing options for %s file "
                   "%s.\n", inout, g->arg);
            uninit_options(&o);
            return ret;
        }

//        // 这里可以看到，推流命令测试中，经过parse_optgroup后，编解码器选项从9个变成11个.
//        AVDictionaryEntry *t = NULL;
//        while((t = av_dict_get(o.g->codec_opts, "", t, AV_DICT_IGNORE_SUFFIX))){
//            printf("tyytest, t->key: %s, t->value: %s\n", t->key, t->value);
//        }

        av_log(NULL, AV_LOG_DEBUG, "Opening an %s file: %s.\n", inout, g->arg);
        //ret = open_file(&o, g->arg);

        // 当存在多个输入输出文件时,这样做可能不合理,但对于一输入对一输出的场景是ok的.
        std::string lowstr = inout;
        if(strlwr((char*)lowstr.c_str()) == std::string("input")){
            //_input_filename = g->arg;
            ret = open_file((void*)this, &o, _input_filename.c_str());
        }else{
            ret = open_file((void*)this, &o, _output_filename.c_str());
        }

        uninit_options(&o);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error opening %s file %s.\n",
                   inout, g->arg);
            return ret;
        }
        av_log(NULL, AV_LOG_DEBUG, "Successfully opened the file.\n");
    }

    return 0;
}

/**
 * @brief 选择解码器以及对应的解码器id.两种方案：
 * 1.用户传进解码器，那么返回用户选择的解码器，并且st保存了用户选择的解码器的codec_id；
 * 2.用户无传进解码器，则以流中的codec_id作为选择解码器返回, codec_id无需改变.
 * 当用户传了 解码器选项时，若音视频类型与流的音视频类型不一致 或者 解码器不被ffmpeg支持，程序会退出.
 *
 * 实际上该函数不关心返回值的话，就是应用强制解码器id(apply forced codec ids)
*/
AVCodec *FFmpegMedia::choose_decoder(OptionsContext *o, AVFormatContext *s, AVStream *st)
{
    char *codec_name = NULL;

    //MATCH_PER_STREAM_OPT(codec_names, str, codec_name, s, st);// 判断音视频类型是否与流一致.例如-vcodec libx264,这里是判断'v'是否与st的codec_type一致
    MATCH_PER_STREAM_OPT_EX(codec_names, str, codec_name, char*, s, st);
    if (codec_name) {
        AVCodec *codec = find_codec_or_die(codec_name, st->codecpar->codec_type, 0);// 判断解码器是否被ffmpeg支持.例如-vcodec libx264,这里是判断 libx264是否被ffmpeg支持
        if(!codec){
            return nullptr;
        }

        st->codecpar->codec_id = codec->id;
        return codec;
    } else
        return avcodec_find_decoder(st->codecpar->codec_id);
}

int FFmpegMedia::open_input_file_ex(void* arg, OptionsContext *o, const char *filename){
    if(!arg){
        return -1;
    }

    FFmpegMedia *fm = (FFmpegMedia*)arg;
    return fm->open_input_file(o, filename);
}

/**
 * @brief 将传进来的字典内的键值对，去掉流说明符后，设置到新的字典进行返回，传进来的字典内容保持不变。
 * @param dict 原字典
 * @return 返回一个新的没有流说明符的字典
*/
/* return a copy of the input with the stream specifiers removed from the keys-返回输入的副本，并从键中删除流说明符 */
AVDictionary *FFmpegMedia::strip_specifiers(AVDictionary *dict)
{
    AVDictionaryEntry *e = NULL;
    AVDictionary    *ret = NULL;

    while ((e = av_dict_get(dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
        char *p = strchr(e->key, ':');

        /*将流说明符去掉.例如e->key="b:v"，p是指向":v"的，那么*p=0后，e->key="b"*/
        if (p)
            *p = 0;

        /*所以这里设置到ret的，必定是去掉流说明符"v"的(冒号是分隔符当然也会去掉)，例如e->key="b"，e->value="128K"*/
        av_dict_set(&ret, e->key, e->value, 0);

        /*上面设置完毕后，将该p指向的地址恢复为分隔符":"，此时会恢复原来的值：e->key="b:v"*/
        if (p)
            *p = ':';
    }
    return ret;
}

void FFmpegMedia::print_error(const char *filename, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}

/**
 * @brief 将用户指定输入或者输出文件的编解码器选项转移到一个临时变量，然后通过返回值返回.
 * 注意，该函数能够将这些选项过滤到不同的媒体类型，因为有av_opt_find()。例如参数-level 3.1,
 * 当音频流或者字幕流调用本函数时，因为音频和字幕的AVCodecContext的AVClass中的option表不带有这个选项，所以不会设置到ret中，
 * 而当视频时，因为视频是有这个选项的，所以会被设置到ret字典。
 * 所以说，当视频流调用时，只会将参数opts中，属于视频编解码器选项的进行返回，其余不属于视频类型的选项会被过滤掉。
 *
 * @param opts 用户指定输入或者输出文件的编解码器选项
 * @param codec_id 编解码器id，只当codec为空时有用
 * @param s 输入或者输出文件的上下文，codec为空以及check_stream_specifier匹配流与媒体类型符时有用
 * @param st 流
 * @param codec 解码器或者是编码器
 *
 * @return 成功=将用户输入或者输出文件的编解码器选项，按照不同的媒体类型进行返回，并且不带流标识符； 失败=程序退出
 *
 * @note av_opt_find的源码有点复杂.
 */
AVDictionary *FFmpegMedia::filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st, AVCodec *codec)
{
    AVDictionary    *ret = NULL;
    AVDictionaryEntry *t = NULL;
    int            flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM
                                      : AV_OPT_FLAG_DECODING_PARAM;// 没有-f参数，说明是输入文件，需要解码
    char          prefix = 0;
    const AVClass    *cc = avcodec_get_class();// 返回AVCodecContext的AVClass

    if (!codec)
        codec            = s->oformat ? avcodec_find_encoder(codec_id)
                                      : avcodec_find_decoder(codec_id);

    // 1. 使用临时变量prefix、flags记录对应媒体类型
    switch (st->codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        prefix  = 'v';
        flags  |= AV_OPT_FLAG_VIDEO_PARAM;
        break;
    case AVMEDIA_TYPE_AUDIO:
        prefix  = 'a';
        flags  |= AV_OPT_FLAG_AUDIO_PARAM;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        prefix  = 's';
        flags  |= AV_OPT_FLAG_SUBTITLE_PARAM;
        break;
    }

    // 2. 真正将用户命令行中编解码器选项设置到编解码器的流程
    while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) {
        char *p = strchr(t->key, ':');// 找子串,例"nihao:v",那么执行后p=":v"

        /*2.1 若找到":"，检测一下该specifier字符是否与流st匹配，然后把流标识符去掉.若不匹配程序退出*/
        /* check stream specification in opt name */
        if (p)
            switch (check_stream_specifier(s, st, p + 1)) {
            case  1:
                *p = 0;
                printf("p: %s\n", p);
                break;
            case  0:
                continue;
            default:
                //exit_program(1);
                return nullptr;// 成功也可能返回空,所以这里后续可能需要根据业务场景优化
            }

        /*2.2 设置选项到ret字典，通常选项走这个if*/
        if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||/*在AVCodecContext的AVClass找到该key*/
            !codec ||   /*该编解码器存在*/
            (codec->priv_class &&
             av_opt_find(&codec->priv_class, t->key, NULL, flags,
                         AV_OPT_SEARCH_FAKE_OBJ)))/*codec->priv_class存在且在codec->priv_class找到该key,codec->priv_class是称为cc的子AVClass？有兴趣请看源码*/
            av_dict_set(&ret, t->key, t->value, 0);
        else if (t->key[0] == prefix &&
                 av_opt_find(&cc, t->key + 1, NULL, flags,
                             AV_OPT_SEARCH_FAKE_OBJ))/*2.3 这个if应该是v:xxx即媒体类型是在冒号前面的。后续可以自行添加对应选项测试即可*/
            av_dict_set(&ret, t->key + 1, t->value, 0);

        // 恢复t->key带有流标识符的值.
        // 例如b:v，经过上面*p=0后，t->key="b"，这里重置*p=':'，那么就恢复为t->key="b:v"
        if (p)
            *p = ':';
    }

    return ret;
}

/**
 * @brief 为输入文件的每一个流都开辟一个字典，若用户设置了某个流的编解码器选项，则过滤后，
 *          对应的字典会保存选项。一般用户很少对输入文件添加编解码器选项。
 * @param s
 * @param codec_opts 一般是用户输入的编解码器选项
 */
AVDictionary **FFmpegMedia::setup_find_stream_info_opts(AVFormatContext *s,
                                           AVDictionary *codec_opts)
{
    int i;
    AVDictionary **opts;
    opts = NULL;// tyycode
    printf("opts: %#X\n", opts);

    if (!s->nb_streams)
        return NULL;

    // 1. 为nb_streams流各自开辟一个AVDictionary字典.
    // 开辟后,opts不为空，opts[i]每个元素都指向空
    opts = (AVDictionary **)av_mallocz_array(s->nb_streams, sizeof(*opts));
    if (!opts) {
        av_log(NULL, AV_LOG_ERROR,
               "Could not alloc memory for stream options.\n");
        return NULL;
    }
    printf("opts: %#X\n", opts);// 注，qt对二级指针的debug不是很好，不加打印，直接看堆栈时opts在qt是0x0.我已经遇到好几次了.
    if(opts == NULL){
        printf("opts is null\n");
    }
    if(*opts){
        printf("*opts not is null\n");
    }else{
        printf("*opts is null\n");
    }

    // 2. 若指定了输入文件的解码器参数，会进行过滤.
    // 过滤后会通过返回值保存到opts数组中
    // 不过一般很少支持输入文件的解码器,例如本项目当时编译时没有支持,可使用ffmpeg -codecs | findstr 264查看
    //  例如有的是支持的输出看到：decoders: h264 h264_qsv h264_cuvid，我们可在-i前加上 -codec h264
    // 部分解码器名字可以参考https://blog.csdn.net/weixin_42887343/article/details/112435362.
    for (i = 0; i < s->nb_streams; i++)
        opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id,
                                    s, s->streams[i], NULL);
    return opts;
}

/* Add all the streams from the given input file to the global
 * list of input streams. */
// 该函数实际上是对输入流中解码器相关参数的处理，然后封装到InputStream结构体中
int FFmpegMedia::add_input_streams(OptionsContext *o, AVFormatContext *ic)
{
    int i, ret;

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
        AVCodecParameters *par = st->codecpar;
        InputStream *ist = (InputStream *)av_mallocz(sizeof(*ist));
        char *framerate = NULL, *hwaccel_device = NULL;
        const char *hwaccel = NULL;
        char *hwaccel_output_format = NULL;
        char *codec_tag = NULL;
        char *next;
        char *discard_str = NULL;
        const AVClass *cc = avcodec_get_class();// 获取AVCodecContext的AVClass
        const AVOption *discard_opt = av_opt_find(&cc, "skip_frame", NULL, 0, 0);

        if (!ist){
            //exit_program(1);
            return -1;
        }

        // 1. 往二维数组input_streams**增加一个input_streams*元素
        //GROW_ARRAY(input_streams, nb_input_streams);//注：这里只是开辟了InputStream *字节大小(64bit为8字节)，而不是开辟InputStream字节大小(sizeof(InputStream))
        GROW_ARRAY_EX(input_streams, InputStream **, nb_input_streams);
        if(!input_streams){
            return -1;
        }
        input_streams[nb_input_streams - 1] = ist;

        // 2. 利用o对InputStream内部进行赋值或者直接赋默认值
        ist->st = st;
        ist->file_index = nb_input_files;
        ist->discard = 1;
        st->discard  = AVDISCARD_ALL;/*设置了AVDISCARD_ALL，表示用户丢弃该流*/
        ist->nb_samples = 0;
        ist->min_pts = INT64_MAX;
        ist->max_pts = INT64_MIN;

        ist->ts_scale = 1.0;
        MATCH_PER_STREAM_OPT(ts_scale, dbl, ist->ts_scale, ic, st);

        ist->autorotate = 1;
        MATCH_PER_STREAM_OPT(autorotate, i, ist->autorotate, ic, st);

        //MATCH_PER_STREAM_OPT(codec_tags, str, codec_tag, ic, st);
        MATCH_PER_STREAM_OPT_EX(codec_tags, str, codec_tag, char *, ic, st);
        if (codec_tag) {
            //strtlo see https://blog.csdn.net/weixin_37921201/article/details/119718403
            uint32_t tag = strtol(codec_tag, &next, 0);// next指向非法字符串开始及后面的字符串.
            if (*next)// codec_tag出现非法字符串的处理
                tag = AV_RL32(codec_tag);
            st->codecpar->codec_tag = tag;
        }

        // 3. 保存解码器
        ist->dec = choose_decoder(o, ic, st);
        if(!ist->dec){
            av_log(NULL, AV_LOG_ERROR, "choose_decoder is null.\n");
            return -1;
        }

        // 4. 保存解码器选项。将o->g->codec_opts保存的解码器选项通过返回值设置到decoder_opts中。
        ist->decoder_opts = filter_codec_opts(o->g->codec_opts, ist->st->codecpar->codec_id, ic, st, ist->dec);

        ist->reinit_filters = -1;
        MATCH_PER_STREAM_OPT(reinit_filters, i, ist->reinit_filters, ic, st);

        //MATCH_PER_STREAM_OPT(discard, str, discard_str, ic, st);
        MATCH_PER_STREAM_OPT_EX(discard, str, discard_str, char *, ic, st);
        ist->user_set_discard = AVDISCARD_NONE;/*默认该流是不丢弃的*/

        // 5. 判断是否丢弃该流.-vn,-an,-sn这些选项.
        // 只要有视频或者音频或者字幕或者数据被禁用，那么user_set_discard标记为AVDISCARD_ALL
        if ((o->video_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) ||
            (o->audio_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) ||
            (o->subtitle_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) ||
            (o->data_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_DATA))
                ist->user_set_discard = AVDISCARD_ALL;/* 设置了AVDISCARD_ALL，表示用户丢弃该流 */

        /* 检测ffmpeg设置后,user_set_discard的值是否与用户的一致，不一致则程序退出 */
        if (discard_str && av_opt_eval_int(&cc, discard_opt, discard_str, &ist->user_set_discard) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error parsing discard %s.\n",
                    discard_str);
            //exit_program(1);
            return -1;
        }

        ist->filter_in_rescale_delta_last = AV_NOPTS_VALUE;

        // 6. 为解码器上下文开辟内存
        ist->dec_ctx = avcodec_alloc_context3(ist->dec);
        if (!ist->dec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Error allocating the decoder context.\n");
            //exit_program(1);
            return -1;
        }

        // 7. 从流中拷贝参数到解码器上下文
        ret = avcodec_parameters_to_context(ist->dec_ctx, par);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error initializing the decoder context.\n");
            //exit_program(1);
            return -1;
        }

        if (o->bitexact)
            ist->dec_ctx->flags |= AV_CODEC_FLAG_BITEXACT;

        // 8. 给不同媒体类型的InputStream中的编解码器相关成员赋值.(忽略硬件相关内容)
        switch (par->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            if(!ist->dec)// 一般在choose_decoder时已经找到，注open_input_files找到的解码器是ic中的，这里是ist
                ist->dec = avcodec_find_decoder(par->codec_id);
#if FF_API_LOWRES
            if (st->codec->lowres) {
                ist->dec_ctx->lowres = st->codec->lowres;
                ist->dec_ctx->width  = st->codec->width;
                ist->dec_ctx->height = st->codec->height;
                ist->dec_ctx->coded_width  = st->codec->coded_width;
                ist->dec_ctx->coded_height = st->codec->coded_height;
            }
#endif

            // avformat_find_stream_info() doesn't set this for us anymore.
            // avformat_find_stream_info()不再为我们设置这个。
            ist->dec_ctx->framerate = st->avg_frame_rate;

            //MATCH_PER_STREAM_OPT(frame_rates, str, framerate, ic, st);
            MATCH_PER_STREAM_OPT_EX(frame_rates, str, framerate, char *, ic, st);
            if (framerate && av_parse_video_rate(&ist->framerate,
                                                 framerate) < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error parsing framerate %s.\n",
                       framerate);
                //exit_program(1);
                return -1;
            }

            ist->top_field_first = -1;
            MATCH_PER_STREAM_OPT(top_field_first, i, ist->top_field_first, ic, st);

            // 硬件加速相关.
            // 寻找硬件设备的id
            // 可参考https://blog.csdn.net/u012117034/article/details/123470108
            //MATCH_PER_STREAM_OPT(hwaccels, str, hwaccel, ic, st);
            MATCH_PER_STREAM_OPT_EX(hwaccels, str, hwaccel, const char *, ic, st);
            if (hwaccel) {
                // The NVDEC hwaccels use a CUDA device, so remap the name here.
                // NVDEC hwaccels使用CUDA设备，所以在这里重新映射名称。
                // NVDEC是英伟达提供的一种视频解码器引擎，作用是用来解码，可认为是一个SDK库。see https://blog.csdn.net/qq_18998145/article/details/108535188
                // cuda则是一种架构，使用CPU+GPU去处理各种复杂的运算。see https://baike.baidu.com/item/CUDA/1186262?fr=aladdin
                // 所以这里再重新看这段代码：因为nvdec内部使用的是cuda架构，所以重新映射hwaccel字符串
                if (!strcmp(hwaccel, "nvdec"))
                    hwaccel = "cuda";

                if (!strcmp(hwaccel, "none"))
                    ist->hwaccel_id = HWACCEL_NONE;
                else if (!strcmp(hwaccel, "auto"))
                    ist->hwaccel_id = HWACCEL_AUTO;
                else {// 用户指定了具体的硬件解码器
                    enum AVHWDeviceType type;
                    int i;
                    for (i = 0; hwaccels[i].name; i++) {
                        if (!strcmp(hwaccels[i].name, hwaccel)) {
                            ist->hwaccel_id = hwaccels[i].id;
                            break;
                        }
                    }

                    // hwaccel_id=HWACCEL_NONE时
                    if (!ist->hwaccel_id) {
                        // 根据设备类型名称查找，该函数不区分大小写。找不到会返回AV_HWDEVICE_TYPE_NONE
                        type = av_hwdevice_find_type_by_name(hwaccel);
                        if (type != AV_HWDEVICE_TYPE_NONE) {
                            ist->hwaccel_id = HWACCEL_GENERIC;
                            ist->hwaccel_device_type = type;
                        }
                    }

                    // hwaccel_id还是为HWACCEL_NONE时，打印相关错误提示然后程序退出
                    if (!ist->hwaccel_id) {
                        av_log(NULL, AV_LOG_FATAL, "Unrecognized hwaccel: %s.\n",
                               hwaccel);
                        av_log(NULL, AV_LOG_FATAL, "Supported hwaccels: ");
                        type = AV_HWDEVICE_TYPE_NONE;
                        // av_hwdevice_iterate_types函数每次会返回在hw_table中，上一个id为prev的硬件解码器id
                        // hw_table是源码中的一个硬件映射表，保存了各个解码器的处理函数
                        while ((type = av_hwdevice_iterate_types(type)) !=
                               AV_HWDEVICE_TYPE_NONE)
                            av_log(NULL, AV_LOG_FATAL, "%s ",
                                   av_hwdevice_get_type_name(type));


                        for (i = 0; hwaccels[i].name; i++)
                            av_log(NULL, AV_LOG_FATAL, "%s ", hwaccels[i].name);
                        av_log(NULL, AV_LOG_FATAL, "\n");
                        //exit_program(1);
                        return -1;
                    }
                }
            }//<== if (hwaccel) end ==>

            // 保存硬件设备名？
            //MATCH_PER_STREAM_OPT(hwaccel_devices, str, hwaccel_device, ic, st);
            MATCH_PER_STREAM_OPT_EX(hwaccel_devices, str, hwaccel_device, char *, ic, st);
            if (hwaccel_device) {
                ist->hwaccel_device = av_strdup(hwaccel_device);
                if (!ist->hwaccel_device){
                    //exit_program(1);
                    return -1;
                }

            }

            //MATCH_PER_STREAM_OPT(hwaccel_output_formats, str,
                                 //hwaccel_output_format, ic, st);
            MATCH_PER_STREAM_OPT_EX(hwaccel_output_formats, str,
                                 hwaccel_output_format, char *, ic, st);
            if (hwaccel_output_format) {
                ist->hwaccel_output_format = av_get_pix_fmt(hwaccel_output_format);
                if (ist->hwaccel_output_format == AV_PIX_FMT_NONE) {
                    av_log(NULL, AV_LOG_FATAL, "Unrecognised hwaccel output "
                           "format: %s", hwaccel_output_format);
                }
            } else {
                ist->hwaccel_output_format = AV_PIX_FMT_NONE;
            }

            ist->hwaccel_pix_fmt = AV_PIX_FMT_NONE;

            break;
        case AVMEDIA_TYPE_AUDIO:
            ist->guess_layout_max = INT_MAX;
            MATCH_PER_STREAM_OPT(guess_layout_max, i, ist->guess_layout_max, ic, st);
            guess_input_channel_layout(ist);// 设置通道布局到ist中
            break;
        case AVMEDIA_TYPE_DATA:
        case AVMEDIA_TYPE_SUBTITLE: {
            char *canvas_size = NULL;
            if(!ist->dec)// 一般在choose_decoder时已经找到
                ist->dec = avcodec_find_decoder(par->codec_id);
            MATCH_PER_STREAM_OPT(fix_sub_duration, i, ist->fix_sub_duration, ic, st);
            //MATCH_PER_STREAM_OPT(canvas_sizes, str, canvas_size, ic, st);
            MATCH_PER_STREAM_OPT_EX(canvas_sizes, str, canvas_size, char *, ic, st);
            // av_parse_video_size:解析字符串canvas_size，并将值赋值给参1、参2
            if (canvas_size &&
                av_parse_video_size(&ist->dec_ctx->width, &ist->dec_ctx->height, canvas_size) < 0) {
                av_log(NULL, AV_LOG_FATAL, "Invalid canvas size: %s.\n", canvas_size);
                //exit_program(1);
                return -1;
            }
            break;
        }
        case AVMEDIA_TYPE_ATTACHMENT:
        case AVMEDIA_TYPE_UNKNOWN:
            break;
        default:
            //abort();
            return -1;
        }//<== switch (par->codec_type) end ==>

        // 上面看到，一开始是使用avcodec_parameters_to_context(ist->dec_ctx, par)将流中
        // 的信息拷贝到解码器上下文，然后对解码器上下文里的参数赋值后，重新拷贝到流中。
        // par是st->codecpar流中的信息.
        /*9.对解码器上下文里的参数赋值后，重新拷贝到流中*/
        ret = avcodec_parameters_from_context(par, ist->dec_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error initializing the decoder context.\n");
            //exit_program(1);
            return -1;
        }
    }//<== for (i = 0; i < ic->nb_streams; i++) { end ==>
}

/**
 * @brief 这个猜测输入音频的通道布局思路很简单：
 * 1. 若输入流的解码器上下文的通道布局的值=0，则通过通道数去获取通道布局，
 *      若通道数也为0，看av_get_default_channel_layout源码知道，通道布局还是返回0，
 * 2. 若有，则直接返回1.
 * @return =1 获取输入流的通道布局成功； =0 获取输入流的通道布局失败
 */
int FFmpegMedia::guess_input_channel_layout(InputStream *ist)
{
    AVCodecContext *dec = ist->dec_ctx;

    // 1. 若通道布局为0
    if (!dec->channel_layout) {
        char layout_name[256];
        // 1.1 判断通道channels是否越界，越界则返回0，因为guess_layout_max被赋值为INT_MAX
        if (dec->channels > ist->guess_layout_max)
            return 0;

        // 1.2 通过通道数获取通道布局
        dec->channel_layout = av_get_default_channel_layout(dec->channels);
        if (!dec->channel_layout)
            return 0;

        // 1.3 通过通道数获取通道布局成功后，获取该通道布局的字符串描述.
        // 这一步只是打印警告，注释掉实际上不会影响正常逻辑
        // Return a description of a channel layout.
        av_get_channel_layout_string(layout_name, sizeof(layout_name),
                                     dec->channels, dec->channel_layout);
        av_log(NULL, AV_LOG_WARNING, "Guessed Channel Layout for Input Stream "
               "#%d.%d : %s\n", ist->file_index, ist->st->index, layout_name);
    }
    return 1;
}

//int FFmpegMedia::open_input_file(OptionsContext *o, const char *filename)
int FFmpegMedia::open_input_file(OptionsContext *o, const char *filename)
{
    InputFile *f;
    AVFormatContext *ic;
    AVInputFormat *file_iformat = NULL;
    int err, i, ret;
    int64_t timestamp;
    AVDictionary *unused_opts = NULL;
    AVDictionaryEntry *e = NULL;
    char *   video_codec_name = NULL;
    char *   audio_codec_name = NULL;
    char *subtitle_codec_name = NULL;
    char *    data_codec_name = NULL;
    int scan_all_pmts_set = 0;

    // -t and -to cannot be used together; using -t
    // -t是录制时长.例如:ffmpeg -rtsp_transport tcp -y -re -i rtsp://xxx -vcodec copy -t 00:00:10.00 -f mp4 output.mp4
    if (o->stop_time != INT64_MAX && o->recording_time != INT64_MAX) {
        o->stop_time = INT64_MAX;
        av_log(NULL, AV_LOG_WARNING, "-t and -to cannot be used together; using -t.\n");
    }

    // -to value smaller than -ss; aborting.
    if (o->stop_time != INT64_MAX && o->recording_time == INT64_MAX) {
        int64_t start_time = o->start_time == AV_NOPTS_VALUE ? 0 : o->start_time;
        if (o->stop_time <= start_time) {
            av_log(NULL, AV_LOG_ERROR, "-to value smaller than -ss; aborting.\n");
            //exit_program(1);
            return -1;
        } else {
            o->recording_time = o->stop_time - start_time;
        }
    }

    /*在options[]中对应“f”，指定输入文件的格式“-f”*/
    if (o->format) {
        if (!(file_iformat = av_find_input_format(o->format))) {
            av_log(NULL, AV_LOG_FATAL, "Unknown input format: '%s'\n", o->format);
            //exit_program(1);
            return -1;
        }
    }

    // 如果是管道,输入文件名为“-”
    if (!strcmp(filename, "-"))
        filename = "pipe:";

    /*stdin_interaction针对参数-stdin*/
    // 运算符优先级,记忆口决："单算移关与，异或逻条赋", 具体看https://blog.csdn.net/sgbl888/article/details/123997358
    // 不过下面语句等价于：stdin_interaction = stdin_interaction & (strncmp(filename, "pipe:", 5) && strcmp(filename, "/dev/stdin"));
    // 意思是：如果filename不是pipe:开头，那么再判断filename是否是/dev/stdin.
    // 故下面意思是：除了是pipe或者是/dev/stdin不是交互模式，其它都是。故文件、实时流stdin_interaction都为1
    //auto tyy1 = strncmp(filename, "pipe:", 5) && strcmp(filename, "/dev/stdin");
    //stdin_interaction = stdin_interaction & (strncmp(filename, "pipe:", 5) && strcmp(filename, "/dev/stdin"));// 等价于下面语句
    stdin_interaction &= strncmp(filename, "pipe:", 5) &&
                         strcmp(filename, "/dev/stdin");

    // 1. 为输入文件开辟相关内存
    /* get default parameters from command line */
    ic = avformat_alloc_context();
    if (!ic) {
        print_error(filename, AVERROR(ENOMEM));
        //exit_program(1);
        return -1;
    }

    // 1.1 解复用相关参数设置(不过平时输入文件很少设置过多的参数,输出文件才会)
    /*对应参数“-ar”设置音频采样率*/
    if (o->nb_audio_sample_rate) {
        av_dict_set_int(&o->g->format_opts, "sample_rate", o->audio_sample_rate[o->nb_audio_sample_rate - 1].u.i, 0);
    }

    /*对应参数"-ac"*/
    if (o->nb_audio_channels) {
        /* because we set audio_channels based on both the "ac" and
         * "channel_layout" options, we need to check that the specified
         * demuxer actually has the "channels" option before setting it */
        if (file_iformat && file_iformat->priv_class &&
            av_opt_find(&file_iformat->priv_class, "channels", NULL, 0,
                        AV_OPT_SEARCH_FAKE_OBJ)) {
            av_dict_set_int(&o->g->format_opts, "channels", o->audio_channels[o->nb_audio_channels - 1].u.i, 0);
        }
    }

    /*对应参数"r"帧率，注与re是不一样的，re对应成员是rate_emu*/
    if (o->nb_frame_rates) {
        /* set the format-level framerate option;
         * this is important for video grabbers, e.g. x11 */
        if (file_iformat && file_iformat->priv_class &&
            av_opt_find(&file_iformat->priv_class, "framerate", NULL, 0,
                        AV_OPT_SEARCH_FAKE_OBJ)) {
            av_dict_set(&o->g->format_opts, "framerate",
                       (const char *)o->frame_rates[o->nb_frame_rates - 1].u.str, 0);
        }
    }

    //对应参数"s"
    if (o->nb_frame_sizes) {
        av_dict_set(&o->g->format_opts, "video_size", (const char *)o->frame_sizes[o->nb_frame_sizes - 1].u.str, 0);
    }

    //对应参数"pix_fmt"
    if (o->nb_frame_pix_fmts)
        av_dict_set(&o->g->format_opts, "pixel_format", (const char *)o->frame_pix_fmts[o->nb_frame_pix_fmts - 1].u.str, 0);

    //对应参数"c"和"codec" 或"c:[v/a/s/d]"和"codec:[v/a/s/d]"
    //最终通过参数3传出.假设我们传-codec libx264, 那么video_codec_name="libx264"
    //因为传入时o->name[i].u.str会指向"libx264"
    MATCH_PER_TYPE_OPT_EX(codec_names, str,    video_codec_name, char*, ic, "v");
    MATCH_PER_TYPE_OPT_EX(codec_names, str,    audio_codec_name, char*, ic, "a");
    MATCH_PER_TYPE_OPT_EX(codec_names, str, subtitle_codec_name, char*, ic, "s");
    MATCH_PER_TYPE_OPT_EX(codec_names, str,     data_codec_name, char*, ic, "d");
//    MATCH_PER_TYPE_OPT(codec_names, str,    video_codec_name, ic, "v");
//    MATCH_PER_TYPE_OPT(codec_names, str,    audio_codec_name, ic, "a");
//    MATCH_PER_TYPE_OPT(codec_names, str, subtitle_codec_name, ic, "s");
//    MATCH_PER_TYPE_OPT(codec_names, str,     data_codec_name, ic, "d");

    /*
     下面第2，第3步是对ic的video_codec,audio_codec,subtitle_codec,data_codec以及
     video_codec_id,audio_codec_id,subtitle_codec_id,data_codec赋值
    */
    // 2. 根据用户输入指定的编解码器格式，寻找对应的编解码器
    // 找到则赋值给解复用结构体ic，找不到会在find_codec_or_die直接退出
    if (video_codec_name){
        ic->video_codec    = find_codec_or_die(video_codec_name   , AVMEDIA_TYPE_VIDEO   , 0);
        if(!ic->video_codec){
            return -1;
        }
    }
    if (audio_codec_name){
        ic->audio_codec    = find_codec_or_die(audio_codec_name   , AVMEDIA_TYPE_AUDIO   , 0);
        if(!ic->audio_codec){
            return -1;
        }
    }
    if (subtitle_codec_name){
        ic->subtitle_codec = find_codec_or_die(subtitle_codec_name, AVMEDIA_TYPE_SUBTITLE, 0);
        if(!ic->subtitle_codec){
            return -1;
        }
    }
    if (data_codec_name){
        ic->data_codec     = find_codec_or_die(data_codec_name    , AVMEDIA_TYPE_DATA    , 0);
        if(!ic->data_codec){
            return -1;
        }
    }

    // 来到这说明成功找到编解码器(因为失败会在find_codec_or_die直接退出出现)
    // 3. 若指定输入文件的编解码器，则将编解码器中的id赋值给解复用的id; 若无指定，解码器的id为xxx_ID_NONE
    ic->video_codec_id     = video_codec_name    ? ic->video_codec->id    : AV_CODEC_ID_NONE;
    ic->audio_codec_id     = audio_codec_name    ? ic->audio_codec->id    : AV_CODEC_ID_NONE;
    ic->subtitle_codec_id  = subtitle_codec_name ? ic->subtitle_codec->id : AV_CODEC_ID_NONE;
    ic->data_codec_id      = data_codec_name     ? ic->data_codec->id     : AV_CODEC_ID_NONE;

    /*ffmpeg的avformat_open_input()和av_read_frame()默认是阻塞的.
     * 1)用户可以通过设置"ic->flags |= AVFMT_FLAG_NONBLOCK;"设置成非阻塞(通常是不推荐的)；
     * 2)或者是设置超时时间；
     * 3)或者是设置interrupt_callback定义返回机制。
    */
    ic->flags |= AVFMT_FLAG_NONBLOCK;
    if (o->bitexact)
        ic->flags |= AVFMT_FLAG_BITEXACT;
    ic->interrupt_callback = int_cb;// 设置中断函数

    // 4. avformat_open_input打开输入文件，打开时可以设置解复用的相关参数
    // scan_all_pmts是mpegts的一个选项，表示扫描全部的ts流的"Program Map Table"表。这里在没有设定该选项的时候，强制设为1。
    if (!av_dict_get(o->g->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&o->g->format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
    /* open the input file with generic avformat function */
    err = avformat_open_input(&ic, filename, file_iformat, &o->g->format_opts);// 这里看到，解复用的参数最终使用该函数去设置的
    if (err < 0) {
        print_error(filename, err);
        if (err == AVERROR_PROTOCOL_NOT_FOUND)
            av_log(NULL, AV_LOG_ERROR, "Did you mean file:%s?\n", filename);

        FMMessage *fmmsg = new FMMessage();
        fmmsg->what = FM_MSG_OPEN_INPUT_FAILED;
        fmmsg->ifilename = _input_filename;
        fmmsg->ofilename = _output_filename;
        _msg_queue->push(fmmsg);

        //exit_program(1);
        return -1;
    }

    // 打开输入文件后，将该选项置空
    if (scan_all_pmts_set)
        av_dict_set(&o->g->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
    remove_avoptions(&o->g->format_opts, o->g->codec_opts);
    // 上面我们会将有效的条目置空，所以此时还有条目，代表该条目是不合法的。
    assert_avoptions(o->g->format_opts);

    // 5. 若用户设置解码器，会查找解码器，并把找到解码器的id赋值到streams中
    /* apply forced codec ids */
    for (i = 0; i < ic->nb_streams; i++){
        auto codec = choose_decoder(o, ic, ic->streams[i]);
        if(!codec){
            av_log(NULL, AV_LOG_ERROR, "choose_decoder failed, %s\n", filename);
            return -1;
        }
    }

    // 6. 是否查找流信息.
    if (find_stream_info) {
        /*这里会为输入流开辟每一个编解码器选项，以返回值进行返回*/
        AVDictionary **opts = setup_find_stream_info_opts(ic, o->g->codec_opts);
        int orig_nb_streams = ic->nb_streams;

        /*avformat_find_stream_info这里会用到解码器选项o->g->codec_opts，因为opts就是从
        o->g->codec_opts过滤得到的*/
        /* If not enough info to get the stream parameters, we decode the
           first frames to get it. (used in mpeg case for example) */
        ret = avformat_find_stream_info(ic, opts);

        // 释放setup_find_stream_info_opts内部开辟的内存
        for (i = 0; i < orig_nb_streams; i++)
            av_dict_free(&opts[i]);
        av_freep(&opts);

        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "%s: could not find codec parameters\n", filename);
            if (ic->nb_streams == 0) {
                avformat_close_input(&ic);
                FMMessage *fmmsg = new FMMessage();
                fmmsg->what = FM_MSG_FIND_STREAM_INFO_FAILED;
                fmmsg->ifilename = _input_filename;
                fmmsg->ofilename = _output_filename;
                _msg_queue->push(fmmsg);
                //exit_program(1);
                return -1;
            }
        }
    }

    //对应参数"sseof",设置相对于结束的开始时间
    if (o->start_time != AV_NOPTS_VALUE && o->start_time_eof != AV_NOPTS_VALUE) {
        av_log(NULL, AV_LOG_WARNING, "Cannot use -ss and -sseof both, using -ss for %s\n", filename);
        o->start_time_eof = AV_NOPTS_VALUE;
    }

    if (o->start_time_eof != AV_NOPTS_VALUE) {
        if (o->start_time_eof >= 0) {
            av_log(NULL, AV_LOG_ERROR, "-sseof value must be negative; aborting\n");
            //exit_program(1);
            return -1;
        }
        if (ic->duration > 0) {
            o->start_time = o->start_time_eof + ic->duration;
            if (o->start_time < 0) {
                av_log(NULL, AV_LOG_WARNING, "-sseof value seeks to before start of file %s; ignored\n", filename);
                o->start_time = AV_NOPTS_VALUE;
            }
        } else
            av_log(NULL, AV_LOG_WARNING, "Cannot use -sseof, duration of %s not known\n", filename);
    }
    timestamp = (o->start_time == AV_NOPTS_VALUE) ? 0 : o->start_time;// -ss选项
    /* add the stream start time */
    if (!o->seek_timestamp && ic->start_time != AV_NOPTS_VALUE)
        timestamp += ic->start_time;

    // 如果开始时间指定的话, 需要做seek
    /* if seeking requested, we execute it */
    if (o->start_time != AV_NOPTS_VALUE) {
        int64_t seek_timestamp = timestamp;

        if (!(ic->iformat->flags & AVFMT_SEEK_TO_PTS)) {
            int dts_heuristic = 0;
            for (i=0; i<ic->nb_streams; i++) {
                const AVCodecParameters *par = ic->streams[i]->codecpar;
                if (par->video_delay) {
                    dts_heuristic = 1;
                    break;
                }
            }
            if (dts_heuristic) {
                //笔者猜测这里意思是：若ffmpeg存在视频帧延迟(估计是ffmpeg内部缓冲？还是解码造成dts、pts的差距？)，
                //那么seek时，应当稍微减少一点seek_timestamp. 3/23大概是3帧的时长，*AV_TIME_BASE是单位转成秒.
                seek_timestamp -= 3*AV_TIME_BASE / 23;
            }
        }
        ret = avformat_seek_file(ic, -1, INT64_MIN, seek_timestamp, seek_timestamp, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                   filename, (double)timestamp / AV_TIME_BASE);
        }
    }

    /* update the current parameters so that they match the one of the input stream */
    // 7. 将输入文件的每个流封装到InputStream中。InputStream->st是直接指向输入文件的AVStream的，不会再开辟内存.
    // 执行完该函数后，输入文件中各个流就会被保存到input_streams**这个二维数组
    // (ic->streams[]与input_streams都保存这输入文件的各个输入流)
    ret = add_input_streams(o, ic);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "add_input_streams failed.\n");
        return -1;
    }

    /* dump the file content */
    // 参4：0-代表dump输入文件，1-代表dump输出文件。可以看源码.
    av_dump_format(ic, nb_input_files, filename, 0);

    /*8.对InputFile内剩余成员的相关初始化工作*/
    //int tyysize = sizeof (*input_files);//8
    //GROW_ARRAY(input_files, nb_input_files);// 开辟sizeof(InputFile*)大小的内存，且输入文件个数+1
    GROW_ARRAY_EX(input_files, InputFile **, nb_input_files);
    //input_files = (InputFile **)grow_array(input_files, sizeof(*input_files), &nb_input_files, nb_input_files + 1);
    if(!input_files){
        return -1;
    }
    f = (InputFile *)av_mallocz(sizeof(*f));// 真正开辟InputFile结构体的内存
    if (!f){
        //exit_program(1);
        return -1;
    }
    input_files[nb_input_files - 1] = f;

    // 下面就是给InputFile结构体赋值
    f->ctx        = ic;
    f->ist_index  = nb_input_streams - ic->nb_streams;/*输入文件的流的第一个流下标.基本是0，因为在add_input_streams中看到，
                                                        nb_input_streams的个数由ic->nb_streams决定*/
    f->start_time = o->start_time;
    f->recording_time = o->recording_time;
    f->input_ts_offset = o->input_ts_offset;
    f->ts_offset  = o->input_ts_offset - (copy_ts ? (start_at_zero && ic->start_time != AV_NOPTS_VALUE ? ic->start_time : 0) : timestamp);
    f->nb_streams = ic->nb_streams;
    f->rate_emu   = o->rate_emu;// -re
    f->accurate_seek = o->accurate_seek;
    f->loop = o->loop;
    f->duration = 0;
    f->time_base = (AVRational){ 1, 1 };
#if HAVE_THREADS
    f->thread_queue_size = o->thread_queue_size > 0 ? o->thread_queue_size : 8;
#ifndef TYY_POINT_THREAD
#else
    f->thread = (pthread_t *)av_mallocz(sizeof(pthread_t));//tyycode
#endif // TYY_POINT_THREAD

#endif

    /*9.检测所有编解码器选项是否已经被使用(看open_output_file)*/
    /* check if all codec options have been used */
    // 因输入文件一般很少设置编解码器选项， 这里可以后续读者自行研究，看懂意思就行，不难
    unused_opts = strip_specifiers(o->g->codec_opts);// 将用户输入的参数去掉流分隔符
    /*这里意思是：因为在add_input_streams时，内部已经将用户输入的参数过滤后，匹配给对应的流(一般情
     * 况各个输入流中都保存一份o->g->codec_opts副本)，所以这里根据各个输入流保存的选项，去清除用户实际
     * 输入的选项unused_opts，当发现unused_opts还有选项时，那么该选项就是未使用的。
    */
    for (i = f->ist_index; i < nb_input_streams; i++) {
        e = NULL;
        while ((e = av_dict_get(input_streams[i]->decoder_opts, "", e,
                                AV_DICT_IGNORE_SUFFIX)))
            av_dict_set(&unused_opts, e->key, NULL, 0);
    }

    /*这个while就是对多余的选项进行一些警告处理*/
    e = NULL;
    while ((e = av_dict_get(unused_opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
        const AVClass *avclass = avcodec_get_class();// 获取编解码器的AVClass
        const AVOption *option = av_opt_find(&avclass, e->key, NULL, 0,
                                             AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
        const AVClass *fclass = avformat_get_class();// 获取解复用器的AVClass
        const AVOption *foption = av_opt_find(&fclass, e->key, NULL, 0,
                                             AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
        if (!option || foption)
            continue;

        // 若选项option即e->key不是解码器选项，程序退出
        if (!(option->flags & AV_OPT_FLAG_DECODING_PARAM)) {
            av_log(NULL, AV_LOG_ERROR, "Codec AVOption %s (%s) specified for "
                   "input file #%d (%s) is not a decoding option.\n", e->key,
                   option->help ? option->help : "", nb_input_files - 1,
                   filename);
            //exit_program(1);
            return -1;
        }

        av_log(NULL, AV_LOG_WARNING, "Codec AVOption %s (%s) specified for "
               "input file #%d (%s) has not been used for any stream. The most "
               "likely reason is either wrong type (e.g. a video option with "
               "no video streams) or that it is a private option of some decoder "
               "which was not actually used for any stream.\n", e->key,
               option->help ? option->help : "", nb_input_files - 1, filename);
    }
    av_dict_free(&unused_opts);

    //对应参数"dump_attachment"
    for (i = 0; i < o->nb_dump_attachment; i++) {
        int j;
        for (j = 0; j < ic->nb_streams; j++) {
            AVStream *st = ic->streams[j];
            if (check_stream_specifier(ic, st, o->dump_attachment[i].specifier) == 1){
                ret = dump_attachment(st, (const char *)o->dump_attachment[i].u.str);
                if(ret < 0){
                    return -1;
                }
            }
        }
    }

    /*10.input_stream_potentially_available 置为1，记录该输入文件可能是能用的*/
    // 打开后，记录该输入文件可能是能用的
    input_stream_potentially_available = 1;

    return 0;
}

/**
 * @brief 判断当前st是否与用户的spec匹配。
 * @param s
 * @param st 流
 * @param spec 流字符描述，一般是用户输入，ffmpeg分割命令行保存在结构体中
 * @return >0 匹配; =0 不匹配; <0 失败
 */
int FFmpegMedia::check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec)
{
    /*检查s中包含的流st是否与流说明符spec匹配。
    有关规范的语法，请参阅文档中的“流说明符”一章。
    >0 if st is matched by spec;
    =0  if st is not matched by spec;
    AVERROR code if spec is invalid*/
    // avformat_match_stream_specifier支持使用下标('0'-'9')、音视频类型字符('v','a','s','d','t','V')以及其他类型判断流.
    // 这里ffmpeg.c使用'v','a','s'等字符去判断, ffplay使用下标去判断.
    // 例如ffplay的调用:if (avformat_match_stream_specifier(ic, st, wanted_stream_spec[type]) > 0)
    // 这是笔者看avformat_match_stream_specifier源码得到的信息, 看源码得出，使用音视频类型字符判断是最简单.
    // 备注：源码中spec是空字符串的话，默认是返回1匹配成功的
    int ret = avformat_match_stream_specifier(s, st, spec);
    if (ret < 0)
        av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
    return ret;
}

/**
 * @brief 判断用户输入的字符是否为Y，是返回1，否返回0.
 * @note 有个while的原因是：例如我们输入N后，需要按下enter(即'\n')，所以实际上在缓冲区中是存储了两个字符，
 * 一个是'N'，一个是'\n'，而我们在int c = getchar();实际上只读了一个字符'N'，所以我们需要再读一个，以便让函数返回
 * 而直接按下enter，则缓冲区只有一个字符'\n'，所以直接按下enter会返回0.
*/
int FFmpegMedia::read_yesno(void)
{
    int c = getchar();
    int yesno = (av_toupper(c) == 'Y');

    // 只要输入的字符不是\n或者EOF，那么就进入while；否则不进入
    while (c != '\n' && c != EOF)
        c = getchar();

    return yesno;
}

/**
 * @brief 判断是否重写文件。对实时流无效。
 * @param filename 输出文件名
 * @return 成功=0，代表输出文件可以重写。失败=程序退出
 *
 * @note 该函数很简单，就是用于判断文件是否重写，只要函数能返回，
 *          就代表该输出文件可以重写。重写即重新创建文件是在调用该函数后，在调
 *          用avio_open2完成的。
 * 并且注意，重写是指输出文件，而不是输入文件。
*/
int FFmpegMedia::assert_file_overwrite(const char *filename)
{
    // 1. 获取协议名字，例如rtmp://xxx，那么proto_name="rtmp"
    const char *proto_name = avio_find_protocol_name(filename);

    // 2. -y以及-n都提供，程序退出
    if (file_overwrite && no_file_overwrite) {
        fprintf(stderr, "Error, both -y and -n supplied. Exiting.\n");
        //exit_program(1);
        return -1;
    }

    /* 3.若没指定-y且-n也没指定，那么会使用avio_check判断该文件是否存在，若存在会在终端询问用户是否重写，
         用户输入y代表重写，那么该函数返回后会在avio_open2将该文件重新创建。
         指定了-y，那么直接跳过判断，说明可以直接写了.
    */
    /*
    * avio_check：返回AVIO_FLAG_*访问标志，对应url中资源的访问权限，失败时返回负值，对应AVERROR代码。
    * 返回的访问标志被flags中的值屏蔽。
    * @note这个函数本质上是不安全的，因为被检查的资源可能会在一次调用到另一次调用时改变它的存在或权限状态。
    * 因此，您不应该信任返回的值，除非您确定没有其他进程正在访问所检查的资源。
    * 即意思是：你刚好检测完权限，下一秒就被人改掉这个权限了。
    * 并且笔者测试过，avio_check内部会检测该输出文件是否存在，若存在返回0；不存在返回其它.
    */
    if (!file_overwrite) {
        /*协议存在 且 是文件 且 avio_check(filename, 0) == 0*/
        //int tyycodeCheck = avio_check(filename, 0);//实时流要注掉，不然会卡主
        if (proto_name && !strcmp(proto_name, "file") && avio_check(filename, 0) == 0) {
            /*若交互模式且没强制不重写文件，会主动询问用户是否要重写文件*/
            if (stdin_interaction && !no_file_overwrite) {
                fprintf(stderr,"File '%s' already exists. Overwrite ? [y/N] ", filename);//在界面提示该字符串
                fflush(stderr);//清空stderr的缓冲区
                //term_exit();
                signal(SIGINT, SIG_DFL);
                if (!read_yesno()) {/*返回0不重写文件；1重写*/
                    av_log(NULL, AV_LOG_FATAL, "Not overwriting - exiting\n");
                    //exit_program(1);
                    return -1;
                }
                //term_init();// 该函数很简单，就是注册相关信号回调函数
            }
            else {
                av_log(NULL, AV_LOG_FATAL, "File '%s' already exists. Exiting.\n", filename);
                //exit_program(1);
                return -1;
            }
        }
    }

    // 4. 检测输入文件与输出文件名是否一致，若一致则程序退出。一些特定于设备的特殊文件不考虑.
    /*文件流程，实时流不会进该if*/
    if (proto_name && !strcmp(proto_name, "file")) {
        /*判断每个输入文件是否是特殊的文件*/
        for (int i = 0; i < nb_input_files; i++) {
             InputFile *file = input_files[i];
             /*输入文件的flags是否包含AVFMT_NOFILE
                AVFMT_NOFILE：一些特定于设备的特殊文件*/
             if (file->ctx->iformat->flags & AVFMT_NOFILE)
                 continue;

             if (!strcmp(filename, file->ctx->url)) {/*判断输出文件名与输入上下文保存的文件名(输入的文件名)是否一致*/
                 av_log(NULL, AV_LOG_FATAL, "Output %s same as Input #%d - exiting\n", filename, i);
                 av_log(NULL, AV_LOG_WARNING, "FFmpeg cannot edit existing files in-place.\n");
                 //exit_program(1);
                 return -1;
             }
        }
    }
}

int FFmpegMedia::dump_attachment(AVStream *st, const char *filename)
{
    int ret;
    AVIOContext *out = NULL;
    AVDictionaryEntry *e;

    if (!st->codecpar->extradata_size) {
        av_log(NULL, AV_LOG_WARNING, "No extradata to dump in stream #%d:%d.\n",
               nb_input_files - 1, st->index);
        return -1;
    }
    if (!*filename && (e = av_dict_get(st->metadata, "filename", NULL, 0)))
        filename = e->value;
    if (!*filename) {
        av_log(NULL, AV_LOG_FATAL, "No filename specified and no 'filename' tag"
               "in stream #%d:%d.\n", nb_input_files - 1, st->index);
        //exit_program(1);
        return -1;
    }

    ret = assert_file_overwrite(filename);
    if(ret < 0){
        av_log(NULL, AV_LOG_FATAL, "assert_file_overwrite failed\n");
        return -1;
    }

    if ((ret = avio_open2(&out, filename, AVIO_FLAG_WRITE, &int_cb, NULL)) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Could not open file %s for writing.\n",
               filename);
        //exit_program(1);
        return -1;
    }

    // 写sps pps
    avio_write(out, st->codecpar->extradata, st->codecpar->extradata_size);
    avio_flush(out);
    avio_close(out);
}

/**
 * @brief 选择编码器，找到编码器最终保存在ost->enc中
 * @param o 保存着用户命令行的参数上下文
 * @param s 输出文件上下文
 * @param ost ffmpeg封装的输出流
 * @return 0=成功；没有指定编码器但找不到编码器=返回负数；指定编码器但找不到编码器=程序退出
 */
int FFmpegMedia::choose_encoder(OptionsContext *o, AVFormatContext *s, OutputStream *ost)
{
    enum AVMediaType type = ost->st->codecpar->codec_type;
    char *codec_name = NULL;

    /*1.只有音视频、字幕才能进行转码，否则其它类型默认为copy选项不转码*/
    if (type == AVMEDIA_TYPE_VIDEO || type == AVMEDIA_TYPE_AUDIO || type == AVMEDIA_TYPE_SUBTITLE) {
        /*2.获取用户指定输出的编码类型*/
        //MATCH_PER_STREAM_OPT(codec_names, str, codec_name, s, ost->st);
        MATCH_PER_STREAM_OPT_EX(codec_names, str, codec_name, char *, s, ost->st);
        /*2.1若用户没指定，则通过stream的编码器id查找*/
        if (!codec_name) {
            /*av_guess_codec: 猜测编解码器id,依赖oc->oformat->name(-f参数，例如flv)以及文件名.
             视频通过av_guess_codec处理后，看源码得出，视频一般返回的是AVOutputFormat->video_codec*/
            ost->st->codecpar->codec_id = av_guess_codec(s->oformat, NULL, s->url,
                                                         NULL, ost->st->codecpar->codec_type);
            ost->enc = avcodec_find_encoder(ost->st->codecpar->codec_id);
            if (!ost->enc) {
                av_log(NULL, AV_LOG_FATAL, "Automatic encoder selection failed for "
                       "output stream #%d:%d. Default encoder for format %s (codec %s) is "
                       "probably disabled. Please choose an encoder manually.\n",
                       ost->file_index, ost->index, s->oformat->name,
                       avcodec_get_name(ost->st->codecpar->codec_id));
                return AVERROR_ENCODER_NOT_FOUND;
            }
        } else if (!strcmp(codec_name, "copy"))/*2.2用户指定不转码*/
            ost->stream_copy = 1;
        else {
            /*2.3指定编解码器，则使用find_codec_or_die查找，找不到会直接退出程序*/
            ost->enc = find_codec_or_die(codec_name, ost->st->codecpar->codec_type, 1);
            if(!ost->enc){
                return -1;
            }
            ost->st->codecpar->codec_id = ost->enc->id;
        }
        ost->encoding_needed = !ost->stream_copy;
    } else {
        /* no encoding supported for other media types */
        ost->stream_copy     = 1;
        ost->encoding_needed = 0;
    }

    return 0;
}

int FFmpegMedia::get_preset_file_2(const char *preset_name, const char *codec_name, AVIOContext **s)
{
    int i, ret = -1;
    char filename[1000];
    const char *base[3] = { getenv("AVCONV_DATADIR"),
                            getenv("HOME"),
                            AVCONV_DATADIR,
                            };

    for (i = 0; i < FF_ARRAY_ELEMS(base) && ret < 0; i++) {
        if (!base[i])
            continue;
        if (codec_name) {
            snprintf(filename, sizeof(filename), "%s%s/%s-%s.avpreset", base[i],
                     i != 1 ? "" : "/.avconv", codec_name, preset_name);
            ret = avio_open2(s, filename, AVIO_FLAG_READ, &int_cb, NULL);
        }
        if (ret < 0) {
            snprintf(filename, sizeof(filename), "%s%s/%s.avpreset", base[i],
                     i != 1 ? "" : "/.avconv", preset_name);
            ret = avio_open2(s, filename, AVIO_FLAG_READ, &int_cb, NULL);
        }
    }
    return ret;
}

uint8_t *FFmpegMedia::get_line(AVIOContext *s)
{
    AVIOContext *line;
    uint8_t *buf;
    char c;

    if (avio_open_dyn_buf(&line) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Could not alloc buffer for reading preset.\n");
        //exit_program(1);
        return nullptr;
    }

    while ((c = avio_r8(s)) && c != '\n')
        avio_w8(line, c);
    avio_w8(line, 0);
    avio_close_dyn_buf(line, &buf);

    return buf;
}

/**
 * @brief 创建一个OutputStream类型的输出流，并使用用户传进的参数即o对该输出流的成员进行初始化
 * @param o 用户输入的参数上下文
 * @param oc 输出文件上下文
 * @param type 媒体类型
 * @param source_index 输入文件的流下标
 *
 * @return 成功=返回创建好的OutputStream*； 失败=程序退出
*/
OutputStream *FFmpegMedia::new_output_stream(OptionsContext *o, AVFormatContext *oc, enum AVMediaType type, int source_index)
{
    OutputStream *ost;
    /*1.创建新的输出流*/
    AVStream *st = avformat_new_stream(oc, NULL);//这里调用后，oc->nb_streams自动加1.例如avformat_alloc_output_context2时是0，这里后变成1
    int idx      = oc->nb_streams - 1, ret = 0;
    const char *bsfs = NULL, *time_base = NULL;
    char *next, *codec_tag = NULL;
    double qscale = -1;
    int i;

    if (!st) {
        av_log(NULL, AV_LOG_FATAL, "Could not alloc stream.\n");
        //exit_program(1);
        return nullptr;
    }

    if (oc->nb_streams - 1 < o->nb_streamid_map)
        st->id = o->streamid_map[oc->nb_streams - 1];

    //GROW_ARRAY(output_streams, nb_output_streams);//在二级数组中添加一个一级指针大小的元素
    GROW_ARRAY_EX(output_streams, OutputStream **, nb_output_streams);
    if(!output_streams) {
        return nullptr;
    }
    if (!(ost = (OutputStream *)av_mallocz(sizeof(*ost)))){
        //exit_program(1);
        return nullptr;
    }
    output_streams[nb_output_streams - 1] = ost;// 给一级指针赋值

    /*2.给OutputStream相关成员赋值*/
    ost->file_index = nb_output_files - 1;//这里有机会可以试试多个输出时，debug file_index的值
    ost->index      = idx;// 使用流下标赋值
    ost->st         = st;
    ost->forced_kf_ref_pts = AV_NOPTS_VALUE;
    st->codecpar->codec_type = type;

    /*3.寻找编码器*/
    ret = choose_encoder(o, oc, ost);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error selecting an encoder for stream "
               "%d:%d\n", ost->file_index, ost->index);
        //exit_program(1);
        return nullptr;
    }

    /*4.开辟编码器上下文*/
    ost->enc_ctx = avcodec_alloc_context3(ost->enc);
    if (!ost->enc_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Error allocating the encoding context.\n");
        //exit_program(1);
        return nullptr;
    }
    ost->enc_ctx->codec_type = type;

    /*5.给OutputStream中的编码器相关参数开辟空间*/
    ost->ref_par = avcodec_parameters_alloc();
    if (!ost->ref_par) {
        av_log(NULL, AV_LOG_ERROR, "Error allocating the encoding parameters.\n");
        //exit_program(1);
        return nullptr;
    }

    /*6.从o->g->codec_opts过滤选项，保存到OutputStream的encoder_opts中*/
    if (ost->enc) {
        AVIOContext *s = NULL;
        char *buf = NULL, *arg = NULL, *preset = NULL;

        /*这里是将o中编码器选项转移到OutputStream中，很重要*/
        ost->encoder_opts  = filter_codec_opts(o->g->codec_opts, ost->enc->id, oc, st, ost->enc);

        //该presets应该与选项-preset veryfast无关因为-preset是被存放在o->g->codec_opts中.故这里后续分析
        //MATCH_PER_STREAM_OPT(presets, str, preset, oc, st);
        MATCH_PER_STREAM_OPT_EX(presets, str, preset, char *, oc, st);
        if (preset && (!(ret = get_preset_file_2(preset, ost->enc->name, &s)))) {
            do  {
                buf = (char *)get_line(s);
                if (!buf[0] || buf[0] == '#') {
                    av_free(buf);
                    continue;
                }
                if (!(arg = strchr(buf, '='))) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid line found in the preset file.\n");
                    //exit_program(1);
                    return nullptr;
                }
                *arg++ = 0;
                av_dict_set(&ost->encoder_opts, buf, arg, AV_DICT_DONT_OVERWRITE);
                av_free(buf);
            } while (!s->eof_reached);
            avio_closep(&s);
        }
        if (ret) {
            av_log(NULL, AV_LOG_FATAL,
                   "Preset %s specified for stream %d:%d, but could not be opened.\n",
                   preset, ost->file_index, ost->index);
            //exit_program(1);
            return nullptr;
        }
    } else {
        ost->encoder_opts = filter_codec_opts(o->g->codec_opts, AV_CODEC_ID_NONE, oc, st, NULL);
    }

    /*tyycode*/
    AVDictionaryEntry *t = NULL;
    while((t = av_dict_get(ost->encoder_opts, "", t, AV_DICT_IGNORE_SUFFIX))){
        printf("tyytest, t->key: %s, t->value: %s\n", t->key, t->value);
    }

    /*7.这里到函数结尾都是给OutputStream相关成员赋值*/
    if (o->bitexact)
        ost->enc_ctx->flags |= AV_CODEC_FLAG_BITEXACT;

    /*是否设置流的时基，一般不设置默认是{0,0}*/
    /**
     * av_parse_ratio: 解析str并将解析的比率存储在q中。请注意，无穷大(1/0)或负数的比率为被认为是有效的，
     * 想要排除这些值,所以你应该检查返回值。未定义的值可以使用“0:0”字符串表示。
     *
     @param[in,out] q 解析字符串后，得到的时基从该参数传出
     @param[in] str 它必须是一个字符串的格式，格式可以是: num:den或者浮点数或表达式
     @param[in] max 最大允许的分子和分母
     @param[in] log_offset 日志级别偏移量，应用于log_ctx的日志级别
     @param[in] log_ctx 父日志上下文
     @return >= 0表示成功，否则返回负的错误码 **/
    //MATCH_PER_STREAM_OPT(time_bases, str, time_base, oc, st);
    MATCH_PER_STREAM_OPT_EX(time_bases, str, time_base, const char *, oc, st);
    if (time_base) {
        AVRational q;
        if (av_parse_ratio(&q, time_base, INT_MAX, 0, NULL) < 0 ||
            q.num <= 0 || q.den <= 0) {
            av_log(NULL, AV_LOG_FATAL, "Invalid time base: %s\n", time_base);
            //exit_program(1);
            return nullptr;
        }
        /*AVStream结构体每个字段解释 see https://blog.csdn.net/weixin_44517656/article/details/109645489*/
        st->time_base = q;
    }

    /*是否设置编码时的时基，一般不设置默认是{0,0}*/
    //MATCH_PER_STREAM_OPT(enc_time_bases, str, time_base, oc, st);
    MATCH_PER_STREAM_OPT_EX(enc_time_bases, str, time_base, const char *, oc, st);
    if (time_base) {
        AVRational q;
        if (av_parse_ratio(&q, time_base, INT_MAX, 0, NULL) < 0 ||
            q.den <= 0) {
            av_log(NULL, AV_LOG_FATAL, "Invalid time base: %s\n", time_base);
            //exit_program(1);
            return nullptr;
        }
        ost->enc_timebase = q;
    }

    /*-frames选项？暂时未遇到，不过应该与截图相关*/
    //对应参数"frames"，"vframes"，设置输出的帧数。
    ost->max_frames = INT64_MAX;
    MATCH_PER_STREAM_OPT(max_frames, i64, ost->max_frames, oc, st);
    for (i = 0; i<o->nb_max_frames; i++) {
        char *p = o->max_frames[i].specifier;
        if (!*p && type != AVMEDIA_TYPE_VIDEO) {
            av_log(NULL, AV_LOG_WARNING, "Applying unspecific -frames to non video streams, maybe you meant -vframes ?\n");
            break;
        }
    }

    //对应参数"copypriorss"，"copy or discard frames before start time"
    ost->copy_prior_start = -1;
    MATCH_PER_STREAM_OPT(copy_prior_start, i, ost->copy_prior_start, oc ,st);

    //对应参数"bsf"，"absf"，"vbsf"
    /*这里应该与转码+滤镜相关，后续详细分析，可简单参考https://www.cnblogs.com/zhibei/p/12551810.html*/
    //MATCH_PER_STREAM_OPT(bitstream_filters, str, bsfs, oc, st);
    MATCH_PER_STREAM_OPT_EX(bitstream_filters, str, bsfs, const char *, oc, st);
    while (bsfs && *bsfs) {
        const AVBitStreamFilter *filter;
        char *bsf, *bsf_options_str, *bsf_name;

        bsf = av_get_token(&bsfs, ",");
        if (!bsf){
            //exit_program(1);
            return nullptr;
        }
        bsf_name = av_strtok(bsf, "=", &bsf_options_str);
        if (!bsf_name){
            //exit_program(1);
            return nullptr;
        }

        filter = av_bsf_get_by_name(bsf_name);
        if (!filter) {
            av_log(NULL, AV_LOG_FATAL, "Unknown bitstream filter %s\n", bsf_name);
            //exit_program(1);
            return nullptr;
        }

        ost->bsf_ctx = (AVBSFContext **)av_realloc_array(ost->bsf_ctx,
                                        ost->nb_bitstream_filters + 1,
                                        sizeof(*ost->bsf_ctx));
        if (!ost->bsf_ctx){
            //exit_program(1);
            return nullptr;
        }

        ret = av_bsf_alloc(filter, &ost->bsf_ctx[ost->nb_bitstream_filters]);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error allocating a bitstream filter context\n");
            //exit_program(1);
            return nullptr;
        }

        ost->nb_bitstream_filters++;

        if (bsf_options_str && filter->priv_class) {
            const AVOption *opt = av_opt_next(ost->bsf_ctx[ost->nb_bitstream_filters-1]->priv_data, NULL);
            const char * shorthand[2] = {NULL};

            if (opt)
                shorthand[0] = opt->name;

            ret = av_opt_set_from_string(ost->bsf_ctx[ost->nb_bitstream_filters-1]->priv_data, bsf_options_str, shorthand, "=", ":");
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error parsing options for bitstream filter %s\n", bsf_name);
                //exit_program(1);
                return nullptr;
            }
        }
        av_freep(&bsf);

        if (*bsfs)
            bsfs++;
    }

    //对应参数"tag"
    //MATCH_PER_STREAM_OPT(codec_tags, str, codec_tag, oc, st);
    MATCH_PER_STREAM_OPT_EX(codec_tags, str, codec_tag, char *, oc, st);
    if (codec_tag) {
        uint32_t tag = strtol(codec_tag, &next, 0);
        if (*next)
            tag = AV_RL32(codec_tag);
        ost->st->codecpar->codec_tag =
        ost->enc_ctx->codec_tag = tag;
    }

    //对应参数"qscale:[v:a:s:d]"/"q" 以<数值>质量为基础的VBR，取值0.01-255，越小质量越好
    /*qscale、disposition、max_muxing_queue_size暂时未研究*/
    MATCH_PER_STREAM_OPT(qscale, dbl, qscale, oc, st);
    if (qscale >= 0) {
        ost->enc_ctx->flags |= AV_CODEC_FLAG_QSCALE;
        ost->enc_ctx->global_quality = FF_QP2LAMBDA * qscale;
    }

    //对应参数"disposition"
    //MATCH_PER_STREAM_OPT(disposition, str, ost->disposition, oc, st);
    MATCH_PER_STREAM_OPT_EX(disposition, str, ost->disposition, char *, oc, st);
    ost->disposition = av_strdup(ost->disposition);

    ost->max_muxing_queue_size = 128;
    MATCH_PER_STREAM_OPT(max_muxing_queue_size, i, ost->max_muxing_queue_size, oc, st);
    ost->max_muxing_queue_size *= sizeof(AVPacket);

    /*编码器设置全局头，一般推rtmp都会默认设置*/
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        ost->enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /*拷贝对应的字典到OutputStream，编解码器选项的字段在上面的filter_codec_opts处理了*/
    av_dict_copy(&ost->sws_dict, o->g->sws_dict, 0);

    av_dict_copy(&ost->swr_opts, o->g->swr_opts, 0);
    if (ost->enc && av_get_exact_bits_per_sample(ost->enc->id) == 24)
        av_dict_set(&ost->swr_opts, "output_sample_bits", "24", 0);

    av_dict_copy(&ost->resample_opts, o->g->resample_opts, 0);

    ost->source_index = source_index;// 输入文件对应的流下标.视频时，该流下标就是对应该文件中的视频流下标
    if (source_index >= 0) {
        ost->sync_ist = input_streams[source_index];//要同步的输入流，这里看到，输出流中会有成员指向对应输入流的信息
        input_streams[source_index]->discard = 0;
        input_streams[source_index]->st->discard = (AVDiscard)input_streams[source_index]->user_set_discard;
    }
    ost->last_mux_dts = AV_NOPTS_VALUE;

    /*给复用队列开辟内存*/
    ost->muxing_queue = av_fifo_alloc(8 * sizeof(AVPacket));
    if (!ost->muxing_queue){
        //exit_program(1);
        return nullptr;
    }

    return ost;
}

void FFmpegMedia::free_output_stream(OutputStream *ost){

    if (!ost)
        return;

    int j;

    for (j = 0; j < ost->nb_bitstream_filters; j++)
        av_bsf_free(&ost->bsf_ctx[j]);
    av_freep(&ost->bsf_ctx);

    av_frame_free(&ost->filtered_frame);
    av_frame_free(&ost->last_frame);
    av_dict_free(&ost->encoder_opts);

    av_freep(&ost->forced_keyframes);
    av_expr_free(ost->forced_keyframes_pexpr);
    av_freep(&ost->avfilter);
    av_freep(&ost->logfile_prefix);

    av_freep(&ost->audio_channels_map);
    ost->audio_channels_mapped = 0;

    av_dict_free(&ost->sws_dict);
    av_dict_free(&ost->swr_opts);

    avcodec_free_context(&ost->enc_ctx);// 释放编码器上下文
    avcodec_parameters_free(&ost->ref_par);

    if (ost->muxing_queue) {
        while (av_fifo_size(ost->muxing_queue)) {
            AVPacket pkt;
            av_fifo_generic_read(ost->muxing_queue, &pkt, sizeof(pkt), NULL);
            av_packet_unref(&pkt);
        }
        av_fifo_freep(&ost->muxing_queue);
    }

    av_freep(&ost);
}

int FFmpegMedia::parse_matrix_coeffs(uint16_t *dest, const char *str)
{
    int i;
    const char *p = str;
    for (i = 0;; i++) {
        dest[i] = atoi(p);
        if (i == 63)
            break;
        p = strchr(p, ',');
        if (!p) {
            av_log(NULL, AV_LOG_FATAL, "Syntax error in matrix \"%s\" at coeff %d\n", str, i);
            //exit_program(1);
            return -1;
        }
        p++;
    }

    return 0;
}

/**
 * @brief 简单了解一下即可
*/
/* read file contents into a string */
uint8_t *FFmpegMedia::read_file(const char *filename)
{
    AVIOContext *pb      = NULL;
    AVIOContext *dyn_buf = NULL;

    /**
    *avio_open：创建并初始化AVIOContext用于访问url表示的资源。
    @note 当url表示的资源以 读+写 方式打开后，AVIOContext只能用于写。
    @param s 用于返回创建的AVIOContext的指针。在失败的情况下，指向的值被设置为NULL。
    @param url url.
    @param flags 控制url所指示的资源如何打开的标志
    * @return >= 0表示成功，如果失败则为一个负值，对应一个AVERROR代码
    */
    /*1.只读方式打开*/
    int ret = avio_open(&pb, filename, AVIO_FLAG_READ);
    uint8_t buf[1024], *str;
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error opening file %s.\n", filename);
        return NULL;
    }

    /**
     * avio_open_dyn_buf：Open a write only memory stream.
     *
     * @param s new IO context
     * @return zero if no error.
     */
    /*2.打开一个只写的内存流*/
    ret = avio_open_dyn_buf(&dyn_buf);
    if (ret < 0) {
        avio_closep(&pb);
        return NULL;
    }

    /**
     * avio_read：Read size bytes from AVIOContext into buf.
     * @return number of bytes read or AVERROR
     */
    /*3.从pb中读sizeof(buf)字节到buf中后，ret是实际读到的字节数，然后将读到的实际字节数，写进dyn_buf*/
    while ((ret = avio_read(pb, buf, sizeof(buf))) > 0)
        avio_write(dyn_buf, buf, ret);
    avio_w8(dyn_buf, 0);
    avio_closep(&pb);

    /*4.关闭dyn_buf并得到读到的内容，str指向该内容*/
    ret = avio_close_dyn_buf(dyn_buf, &str);
    if (ret < 0)
        return NULL;
    return str;
}

/**
 * @brief 创建一个OutputStream，并将o中的参数转移到OutputStream中，以此初始化OutputStream.
 * @param o 参数上下文
 * @param oc 输出文件上下文
 * @param source_index 输入文件中视频流的下标
 * @return 成功=返回OutputStream*；失败=程序退出
*/
OutputStream *FFmpegMedia::new_video_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    AVStream *st;
    OutputStream *ost;
    AVCodecContext *video_enc;
    char *frame_rate = NULL, *frame_aspect_ratio = NULL;
    int ret;

    /*1.为视频创建一个输出流OutputStream，通过返回值返回.
    注，oc->streams同样会有该AVStream *st，即OutputStream与oc结构都保存了该AVStream *st.
    ost->st与oc->streams中的指向肯定不一样，因为前者是一级指针，后者是二级指针(可取oc->streams[0]看指向是一样的)
    */
    ost = new_output_stream(o, oc, AVMEDIA_TYPE_VIDEO, source_index);
    if(!ost){
        av_log(NULL, AV_LOG_FATAL, "new_output_stream(AVMEDIA_TYPE_VIDEO) failed.\n");
        ret = -1;
        goto fail;
    }
    st  = ost->st;
    video_enc = ost->enc_ctx;

    /*获取帧率-r选项*/
    //MATCH_PER_STREAM_OPT(frame_rates, str, frame_rate, oc, st);
    MATCH_PER_STREAM_OPT_EX(frame_rates, str, frame_rate, char *, oc, st);
    if (frame_rate && av_parse_video_rate(&ost->frame_rate, frame_rate) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Invalid framerate value: %s\n", frame_rate);
        //exit_program(1);
        ret = -1;
        goto fail;
    }
    /*使用-vsync 0和-r会产生无效的输出文件，两者不能同时使用*/
    if (frame_rate && video_sync_method == VSYNC_PASSTHROUGH)
        av_log(NULL, AV_LOG_ERROR, "Using -vsync 0 and -r can produce invalid output files\n");

    /*对应参数"aspect"，设置指定的显示比例*/
    /*aspect可以是浮点数字的字符串，或一个字符串的比值，比值分别是纵横比的分子和分母。
     For example "4:3", "16:9", "1.3333", and "1.7777" are valid argument values.*/
    //MATCH_PER_STREAM_OPT(frame_aspect_ratios, str, frame_aspect_ratio, oc, st);
    MATCH_PER_STREAM_OPT_EX(frame_aspect_ratios, str, frame_aspect_ratio, char *, oc, st);
    if (frame_aspect_ratio) {
        AVRational q;
        if (av_parse_ratio(&q, frame_aspect_ratio, 255, 0, NULL) < 0 ||
            q.num <= 0 || q.den <= 0) {
            av_log(NULL, AV_LOG_FATAL, "Invalid aspect ratio: %s\n", frame_aspect_ratio);
            //exit_program(1);
            ret = -1;
            goto fail;
        }
        ost->frame_aspect_ratio = q;
    }

    /*过滤器相关*/
    /*对应参数"filter_script"和"filter"*/
    //MATCH_PER_STREAM_OPT(filter_scripts, str, ost->filters_script, oc, st);//new_audio_stream也会调用到
    MATCH_PER_STREAM_OPT_EX(filter_scripts, str, ost->filters_script, char *, oc, st);
    //MATCH_PER_STREAM_OPT(filters,        str, ost->filters,        oc, st);
    MATCH_PER_STREAM_OPT_EX(filters,        str, ost->filters, char *,       oc, st);
    if (o->nb_filters > 1)
        av_log(NULL, AV_LOG_ERROR, "Only '-vf %s' read, ignoring remaining -vf options: Use ',' to separate filters\n", ost->filters);

    /*2.用户没有输入-xxx copy不转码，那么就转码*/
    if (!ost->stream_copy) {
        const char *p = NULL;
        char *frame_size = NULL;
        char *frame_pix_fmt = NULL;
        char *intra_matrix = NULL, *inter_matrix = NULL;
        char *chroma_intra_matrix = NULL;
        int do_pass = 0;
        int i;

        /*解析-s分辨率选项到编解码器上下文video_enc中*/
        //MATCH_PER_STREAM_OPT(frame_sizes, str, frame_size, oc, st);
        MATCH_PER_STREAM_OPT_EX(frame_sizes, str, frame_size, char *, oc, st);
        if (frame_size && av_parse_video_size(&video_enc->width, &video_enc->height, frame_size) < 0) {
            av_log(NULL, AV_LOG_FATAL, "Invalid frame size: %s.\n", frame_size);
            //exit_program(1);
            ret = -1;
            goto fail;
        }

        /*对应参数"bits_per_raw_sample"，暂未研究*/
        video_enc->bits_per_raw_sample = frame_bits_per_raw_sample;
        /*对应参数"pix_fmt"，视频帧格式，暂未研究，但是平时比较常用*/
        //MATCH_PER_STREAM_OPT(frame_pix_fmts, str, frame_pix_fmt, oc, st);
        MATCH_PER_STREAM_OPT_EX(frame_pix_fmts, str, frame_pix_fmt, char *, oc, st);
        if (frame_pix_fmt && *frame_pix_fmt == '+') {
            ost->keep_pix_fmt = 1;
            if (!*++frame_pix_fmt)
                frame_pix_fmt = NULL;
        }
        if (frame_pix_fmt && (video_enc->pix_fmt = av_get_pix_fmt(frame_pix_fmt)) == AV_PIX_FMT_NONE) {
            av_log(NULL, AV_LOG_FATAL, "Unknown pixel format requested: %s.\n", frame_pix_fmt);
            //exit_program(1);
            ret = -1;
            goto fail;
        }
        /*video_enc->sample_aspect_ratio参数暂未研究.默认值是{0,1}*/
        st->sample_aspect_ratio = video_enc->sample_aspect_ratio;

        //对应参数"intra"，“-g 1”，只有I帧
        if (intra_only)
            video_enc->gop_size = 0;//video_enc->gop_size默认是-1

        /*以下三个矩阵相关的参数暂未研究.推流没用到，读者有兴趣可自行研究*/
        //对应参数"intra_matrix"
        //MATCH_PER_STREAM_OPT(intra_matrices, str, intra_matrix, oc, st);
        MATCH_PER_STREAM_OPT_EX(intra_matrices, str, intra_matrix, char *, oc, st);
        if (intra_matrix) {
            if (!(video_enc->intra_matrix = (uint16_t *)av_mallocz(sizeof(*video_enc->intra_matrix) * 64))) {
                av_log(NULL, AV_LOG_FATAL, "Could not allocate memory for intra matrix.\n");
                //exit_program(1);
                ret = -1;
                goto fail;
            }
            ret = parse_matrix_coeffs(video_enc->intra_matrix, intra_matrix);
            if(ret < 0){
                return nullptr;
            }
        }
        //对应参数"chroma_intra_matrix"
        //MATCH_PER_STREAM_OPT(chroma_intra_matrices, str, chroma_intra_matrix, oc, st);
        MATCH_PER_STREAM_OPT_EX(chroma_intra_matrices, str, chroma_intra_matrix, char *, oc, st);
        if (chroma_intra_matrix) {
            uint16_t *p = (uint16_t *)av_mallocz(sizeof(*video_enc->chroma_intra_matrix) * 64);
            if (!p) {
                av_log(NULL, AV_LOG_FATAL, "Could not allocate memory for intra matrix.\n");
                //exit_program(1);
                ret = -1;
                goto fail;
            }
            video_enc->chroma_intra_matrix = p;
            ret = parse_matrix_coeffs(p, chroma_intra_matrix);
            if(ret < 0){
                return nullptr;
            }
        }
        //对应参数"inter_matrix"
        //MATCH_PER_STREAM_OPT(inter_matrices, str, inter_matrix, oc, st);
        MATCH_PER_STREAM_OPT_EX(inter_matrices, str, inter_matrix, char *, oc, st);
        if (inter_matrix) {
            if (!(video_enc->inter_matrix = (uint16_t *)av_mallocz(sizeof(*video_enc->inter_matrix) * 64))) {
                av_log(NULL, AV_LOG_FATAL, "Could not allocate memory for inter matrix.\n");
                //exit_program(1);
                ret = -1;
                goto fail;
            }
            ret = parse_matrix_coeffs(video_enc->inter_matrix, inter_matrix);
            if(ret < 0){
                ret = -1;
                goto fail;
            }
        }

        /*rc_overrides参数暂未研究.推流没用到*/
        //对应参数"rc_override"
        /*-rc_override[:stream_specifier] override (output,per-stream)
        速率控制，覆盖指定的时间间隔，以'逗号分隔的int,int,int'列表格式。
        前两个值是开始和结束的帧号，最后一个如果是整数，表示用量。如果是负数表示品质因素*/
        //MATCH_PER_STREAM_OPT(rc_overrides, str, p, oc, st);
        MATCH_PER_STREAM_OPT_EX(rc_overrides, str, p, const char *, oc, st);
        for (i = 0; p; i++) {
            int start, end, q;
            int e = sscanf(p, "%d,%d,%d", &start, &end, &q);
            if (e != 3) {
                av_log(NULL, AV_LOG_FATAL, "error parsing rc_override\n");
                //exit_program(1);
                ret = -1;
                goto fail;
            }
            video_enc->rc_override =
                (RcOverride *)av_realloc_array(video_enc->rc_override,
                                 i + 1, sizeof(RcOverride));
            if (!video_enc->rc_override) {
                av_log(NULL, AV_LOG_FATAL, "Could not (re)allocate memory for rc_override.\n");
                //exit_program(1);
                ret = -1;
                goto fail;
            }
            video_enc->rc_override[i].start_frame = start;
            video_enc->rc_override[i].end_frame   = end;
            if (q > 0) {
                video_enc->rc_override[i].qscale         = q;
                video_enc->rc_override[i].quality_factor = 1.0;
            }
            else {
                video_enc->rc_override[i].qscale         = 0;
                video_enc->rc_override[i].quality_factor = -q/100.0;
            }
            p = strchr(p, '/');
            if (p) p++;
        }
        video_enc->rc_override_count = i;//没设置一般默认是0

        /*是否要或上AV_CODEC_FLAG_PSNR宏*/
        //对应参数"psnr"，计算压缩的帧PSNR，表示视频的质量
        if (do_psnr)
            video_enc->flags|= AV_CODEC_FLAG_PSNR;

        /* two pass mode */
        /*do_passs参数暂未研究.推流没用到*/
        MATCH_PER_STREAM_OPT(pass, i, do_pass, oc, st);
        if (do_pass) {
            if (do_pass & 1) {
                video_enc->flags |= AV_CODEC_FLAG_PASS1;
                av_dict_set(&ost->encoder_opts, "flags", "+pass1", AV_DICT_APPEND);
            }
            if (do_pass & 2) {
                video_enc->flags |= AV_CODEC_FLAG_PASS2;
                av_dict_set(&ost->encoder_opts, "flags", "+pass2", AV_DICT_APPEND);
            }
        }

        /*passlogfile参数暂未研究.推流没用到*/
        //对应参数“passlogfile”
        //MATCH_PER_STREAM_OPT(passlogfiles, str, ost->logfile_prefix, oc, st);
        MATCH_PER_STREAM_OPT_EX(passlogfiles, str, ost->logfile_prefix, char *, oc, st);
        if (ost->logfile_prefix &&
            !(ost->logfile_prefix = av_strdup(ost->logfile_prefix))){
            //exit_program(1);
            ret = -1;
            goto fail;
        }

        /*do_passs参数暂未研究.推流没用到*/
        if (do_pass) {
            char logfilename[1024];
            FILE *f;

            // 拼接日志文件名
            snprintf(logfilename, sizeof(logfilename), "%s-%d.log",
                     ost->logfile_prefix ? ost->logfile_prefix :
                                           DEFAULT_PASS_LOGFILENAME_PREFIX,
                     i);
            if (!strcmp(ost->enc->name, "libx264")) {
                /*若编码器是libx264,重写stats=logfilename*/
                av_dict_set(&ost->encoder_opts, "stats", logfilename, AV_DICT_DONT_OVERWRITE);
            } else {
                if (video_enc->flags & AV_CODEC_FLAG_PASS2) {
                    char  *logbuffer = (char  *)read_file(logfilename);

                    if (!logbuffer) {
                        av_log(NULL, AV_LOG_FATAL, "Error reading log file '%s' for pass-2 encoding\n",
                               logfilename);
                        //exit_program(1);
                        ret = -1;
                        goto fail;
                    }
                    video_enc->stats_in = logbuffer;
                }
                if (video_enc->flags & AV_CODEC_FLAG_PASS1) {
                    /*打开文件并保存该文件句柄到ost->logfile*/
                    f = av_fopen_utf8(logfilename, "wb");
                    if (!f) {
                        av_log(NULL, AV_LOG_FATAL,
                               "Cannot write log file '%s' for pass-1 encoding: %s\n",
                               logfilename, strerror(errno));
                        //exit_program(1);
                        ret = -1;
                        goto fail;
                    }
                    ost->logfile = f;
                }
            }
        }

        /*检测是否有强制关键帧参数，若有则需要重新开辟一份内存.
        因为未开辟时，ost->forced_keyframes指向的是o中的内存.参数暂未研究.*/
        //对应参数"force_key_frames"，
        /*
        ‘-force_key_frames[:stream_specifier] time[,time...] (output,per-stream)’
        ‘-force_key_frames[:stream_specifier] expr:expr (output,per-stream)’在指定的时间戳强制关键帧
        */
        //MATCH_PER_STREAM_OPT(forced_key_frames, str, ost->forced_keyframes, oc, st);
        MATCH_PER_STREAM_OPT_EX(forced_key_frames, str, ost->forced_keyframes, char *, oc, st);
        if (ost->forced_keyframes)
            ost->forced_keyframes = av_strdup(ost->forced_keyframes);

        /*该参数暂未研究.*/
        //对应参数"force_fps"，强制设置视频帧率
        MATCH_PER_STREAM_OPT(force_fps, i, ost->force_fps, oc, st);

        /*该参数暂未研究.*/
        //对应参数"top"
        ost->top_field_first = -1;
        MATCH_PER_STREAM_OPT(top_field_first, i, ost->top_field_first, oc, st);

        /*get_ost_filters流程很简单*/
        ost->avfilter = get_ost_filters(o, oc, ost);
        if (!ost->avfilter){
            //exit_program(1);
            ret = -1;
            goto fail;
        }
    } else {
        /*未研究过该参数*/
        //流拷贝，不需要重新编码的处理
        //对应参数"copyinkf"，复制初始非关键帧
        MATCH_PER_STREAM_OPT(copy_initial_nonkeyframes, i, ost->copy_initial_nonkeyframes, oc ,st);
    }

    /*3.不转码流程*/
    if (ost->stream_copy) {
        ret = check_streamcopy_filters(o, oc, ost, AVMEDIA_TYPE_VIDEO);
        if(ret < 0){
            ret = -1;
            goto fail;
        }
    }

    ret = 0;

fail:
    if(ret < 0){
        av_log(NULL, AV_LOG_FATAL, "new_video_stream failed.\n");
        // 这里不需要释放new_output_stream()开辟的内容.
        // 析构调ffmpeg_cleanup()时会释放,因为ost=new_output_stream()成功会保存到 类成员output_streams
        return nullptr;
    }

    return ost;
}

/**
 * @brief 检测指定copy不转码时，是否使用了过滤器相关选项.
 * @param o 参数上下文
 * @param oc 输出文件上下文
 * @param ost 输出流封装结构体
 * @param type 媒体类型
 * @return void
 *
 * @note 若指定了copy，但还使用了过滤器相关功能，程序会直接退出
*/
int FFmpegMedia::check_streamcopy_filters(OptionsContext *o, AVFormatContext *oc,
                                     const OutputStream *ost, enum AVMediaType type)
{
    /*1.这里看到，若使用copy不转码，就无法再使用过滤器相关的功能*/
    if (ost->filters_script || ost->filters) {
        av_log(NULL, AV_LOG_ERROR,
               "%s '%s' was defined for %s output stream %d:%d but codec copy was selected.\n"
               "Filtering and streamcopy cannot be used together.\n",
               ost->filters ? "Filtergraph" : "Filtergraph script",
               ost->filters ? ost->filters : ost->filters_script,
               av_get_media_type_string(type), ost->file_index, ost->index);
        //exit_program(1);
        return -1;
    }

    return 0;
}

/**
 * @brief 获取输出流对应过滤器字符串的描述。
 * @param o 参数上下文
 * @param oc 输出文件上下文
 * @param ost 输出流封装结构体
 * @return 成功=得到对应的字符串描述，注意没有设置过滤器描述时，会返回"null"或者"anull"，这也是成功的； 失败=程序退出
*/
char *FFmpegMedia::get_ost_filters(OptionsContext *o, AVFormatContext *oc,
                             OutputStream *ost)
{
    AVStream *st = ost->st;

    /* 1.同时设置-filter and -filter_scrip会报错，程序退出 */
    if (ost->filters_script && ost->filters) {
        av_log(NULL, AV_LOG_ERROR, "Both -filter and -filter_script set for "
               "output stream #%d:%d.\n", nb_output_files, st->index);
        //exit_program(1);
        return nullptr;
    }

    /* 2.若过滤器描述是使用脚本，则读取里面的内容进行返回 */
    if (ost->filters_script)
        return (char *)read_file(ost->filters_script);
    else if (ost->filters)/* 3.若过滤器描述是使用字符串直接描述，则开辟内存后直接返回 */
        return av_strdup(ost->filters);

    /* 4.若没设置过滤器描述，则返回对应媒体类型的空字符串描述 */
    return av_strdup(st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ?
                     "null" : "anull");
}

int FFmpegMedia::init_output_filter(OutputFilter *ofilter, OptionsContext *o,
                               AVFormatContext *oc)
{
    OutputStream *ost;

    switch (ofilter->type) {
    case AVMEDIA_TYPE_VIDEO: ost = new_video_stream(o, oc, -1); break;
    case AVMEDIA_TYPE_AUDIO: ost = new_audio_stream(o, oc, -1); break;
    default:
        av_log(NULL, AV_LOG_FATAL, "Only video and audio filters are supported "
               "currently.\n");
        //exit_program(1);
        return -1;
    }

    if(!ost){
        return -1;
    }

    ost->source_index = -1;
    ost->filter       = ofilter;

    ofilter->ost      = ost;
    ofilter->format   = -1;

    if (ost->stream_copy) {
        av_log(NULL, AV_LOG_ERROR, "Streamcopy requested for output stream %d:%d, "
               "which is fed from a complex filtergraph. Filtering and streamcopy "
               "cannot be used together.\n", ost->file_index, ost->index);
        //exit_program(1);
        return -1;
    }

    if (ost->avfilter && (ost->filters || ost->filters_script)) {
        const char *opt = ost->filters ? "-vf/-af/-filter" : "-filter_script";
        av_log(NULL, AV_LOG_ERROR,
               "%s '%s' was specified through the %s option "
               "for output stream %d:%d, which is fed from a complex filtergraph.\n"
               "%s and -filter_complex cannot be used together for the same stream.\n",
               ost->filters ? "Filtergraph" : "Filtergraph script",
               ost->filters ? ost->filters : ost->filters_script,
               opt, ost->file_index, ost->index, opt);
        //exit_program(1);
        return -1;
    }

    avfilter_inout_free(&ofilter->out_tmp);
    return 0;
}

OutputStream *FFmpegMedia::new_audio_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    int n;
    AVStream *st;
    OutputStream *ost;
    AVCodecContext *audio_enc;
    int ret;

    /*1.根据输入音频流创建一个输出音频流*/
    ost = new_output_stream(o, oc, AVMEDIA_TYPE_AUDIO, source_index);
    if (!ost) {
        av_log(NULL, AV_LOG_FATAL, "new_output_stream(AVMEDIA_TYPE_AUDIO) failed.\n");
        ret = -1;
        goto fail;
    }
    st  = ost->st;

    audio_enc = ost->enc_ctx;
    audio_enc->codec_type = AVMEDIA_TYPE_AUDIO;

    //MATCH_PER_STREAM_OPT(filter_scripts, str, ost->filters_script, oc, st);
    //MATCH_PER_STREAM_OPT(filters,        str, ost->filters,        oc, st);
    MATCH_PER_STREAM_OPT_EX(filter_scripts, str, ost->filters_script, char *, oc, st);
    MATCH_PER_STREAM_OPT_EX(filters,        str, ost->filters,        char *, oc, st);
    if (o->nb_filters > 1)
        av_log(NULL, AV_LOG_ERROR, "Only '-af %s' read, ignoring remaining -af options: Use ',' to separate filters\n", ost->filters);

    /*2.若转码，则初始化音频流的流程*/
    /*若指定-vcodec libx264，音频也会进来这里(因为视频使stream_copy=0了)，但实际不会音频没有太多实际的代码运行.
    可以额外添加对应通道、采样格式、采样率的参数*/
    /*例如可以在推流命令基础上添加：-ar 48000 -ac 2 -sample_fmt s16p。
    但是注意sample_fmt选项，可以通过ffmpegC -sample_fmts查看ffmpeg支持的采样格式.
    例如笔者查看后，ffmpeg是支持s16格式的，将上面s16p改成s16会报错，原因是笔者的输入文件的音频是mp3，而ffmpeg会用到libmp3lame这个编解码库，
    它是不支持s16格式的，所以会报错：Specified sample format s16 is invalid or not supported*/
    if (!ost->stream_copy) {
        /*音频转码无非就是音频三元组（采样率，采样大小和通道数）的改变.采样大小也叫采样位数(采样格式)*/
        char *sample_fmt = NULL;

        /*-ac通道号选项*/
        MATCH_PER_STREAM_OPT(audio_channels, i, audio_enc->channels, oc, st);

        /*-sample_fmt采样格式选项，例如s16p*/
        //MATCH_PER_STREAM_OPT(sample_fmts, str, sample_fmt, oc, st);
        MATCH_PER_STREAM_OPT_EX(sample_fmts, str, sample_fmt, char *, oc, st);
        if (sample_fmt &&
            (audio_enc->sample_fmt = av_get_sample_fmt(sample_fmt)) == AV_SAMPLE_FMT_NONE) {
            av_log(NULL, AV_LOG_FATAL, "Invalid sample format '%s'\n", sample_fmt);
            //exit_program(1);
            ret = -1;
            goto fail;
        }

        /*-ar采样率选项*/
        MATCH_PER_STREAM_OPT(audio_sample_rate, i, audio_enc->sample_rate, oc, st);

        //MATCH_PER_STREAM_OPT(apad, str, ost->apad, oc, st);
        MATCH_PER_STREAM_OPT_EX(apad, str, ost->apad, char *, oc, st);
        ost->apad = av_strdup(ost->apad);

        /*获取音视频过滤器描述*/
        ost->avfilter = get_ost_filters(o, oc, ost);
        if (!ost->avfilter){
            //exit_program(1);
            ret = -1;
            goto fail;
        }

        /* check for channel mapping for this audio stream */
        for (n = 0; n < o->nb_audio_channel_maps; n++) {
            /*map参数暂未研究，后续补充，推流没用到*/
            AudioChannelMap *map = &o->audio_channel_maps[n];/*从获取输入的channel mapping数组获取一个*/
            if ((map->ofile_idx   == -1 || ost->file_index == map->ofile_idx) &&
                (map->ostream_idx == -1 || ost->st->index  == map->ostream_idx)) {
                InputStream *ist;

                if (map->channel_idx == -1) {
                    ist = NULL;
                } else if (ost->source_index < 0) {
                    av_log(NULL, AV_LOG_FATAL, "Cannot determine input stream for channel mapping %d.%d\n",
                           ost->file_index, ost->st->index);
                    continue;
                } else {
                    ist = input_streams[ost->source_index];
                }

                if (!ist || (ist->file_index == map->file_idx && ist->st->index == map->stream_idx)) {
                    if (av_reallocp_array(&ost->audio_channels_map,
                                          ost->audio_channels_mapped + 1,
                                          sizeof(*ost->audio_channels_map)
                                          ) < 0 ){
                        //exit_program(1);
                        ret = -1;
                        goto fail;
                    }

                    ost->audio_channels_map[ost->audio_channels_mapped++] = map->channel_idx;
                }
            }
        }
    }//<== if (!ost->stream_copy) end ==>

    /*3.若不转码，则初始化音频流的流程比较简单*/
    if (ost->stream_copy){
        ret = check_streamcopy_filters(o, oc, ost, AVMEDIA_TYPE_AUDIO);
        if(ret < 0){
            ret = -1;
            goto fail;
        }
    }

    ret = 0;

fail:
    if(ret < 0){
        av_log(NULL, AV_LOG_FATAL, "new_audio_stream failed.\n");
        return nullptr;
    }

    return ost;
}

/**
 * @brief 创建一个输出字幕流。
 * @param o
 * @param oc
 * @param source_index
 */
OutputStream *FFmpegMedia::new_subtitle_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    AVStream *st;
    OutputStream *ost;
    AVCodecContext *subtitle_enc;
    int ret;

    /*1.利用输入字幕流创建一个输出字幕流*/
    ost = new_output_stream(o, oc, AVMEDIA_TYPE_SUBTITLE, source_index);
    if(!ost){
        av_log(NULL, AV_LOG_FATAL, "new_output_stream(AVMEDIA_TYPE_SUBTITLE) failed.\n");
        ret = -1;
        goto fail;
    }
    st  = ost->st;
    subtitle_enc = ost->enc_ctx;

    subtitle_enc->codec_type = AVMEDIA_TYPE_SUBTITLE;

    MATCH_PER_STREAM_OPT(copy_initial_nonkeyframes, i, ost->copy_initial_nonkeyframes, oc, st);

    /*2.若转码，则进入if*/
    if (!ost->stream_copy) {
        char *frame_size = NULL;

        /*这里看到，若输入了-s选项，字幕也会使用*/
        //MATCH_PER_STREAM_OPT(frame_sizes, str, frame_size, oc, st);
        MATCH_PER_STREAM_OPT_EX(frame_sizes, str, frame_size, char *, oc, st);
        if (frame_size && av_parse_video_size(&subtitle_enc->width, &subtitle_enc->height, frame_size) < 0) {
            av_log(NULL, AV_LOG_FATAL, "Invalid frame size: %s.\n", frame_size);
            //exit_program(1);
            ret = -1;
            goto fail;
        }
    }

    ret = 0;

fail:
    if(ret < 0){
        av_log(NULL, AV_LOG_FATAL, "new_subtitle_stream failed.\n");
        return nullptr;
    }

    return ost;
}

OutputStream *FFmpegMedia::new_data_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    OutputStream *ost;

    ost = new_output_stream(o, oc, AVMEDIA_TYPE_DATA, source_index);
    if(!ost){
        av_log(NULL, AV_LOG_FATAL, "new_output_stream(AVMEDIA_TYPE_DATA) failed.\n");
        return nullptr;
    }
    if (!ost->stream_copy) {
        av_log(NULL, AV_LOG_FATAL, "Data stream encoding not supported yet (only streamcopy)\n");
        //exit_program(1);
        return nullptr;
    }

    return ost;
}

OutputStream *FFmpegMedia::new_attachment_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    OutputStream *ost = new_output_stream(o, oc, AVMEDIA_TYPE_ATTACHMENT, source_index);
    if(!ost){
        av_log(NULL, AV_LOG_FATAL, "new_output_stream(AVMEDIA_TYPE_ATTACHMENT) failed.\n");
        return nullptr;
    }
    ost->stream_copy = 1;
    ost->finished    = (OSTFinished)1;
    return ost;
}

OutputStream *FFmpegMedia::new_unknown_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    OutputStream *ost;

    ost = new_output_stream(o, oc, AVMEDIA_TYPE_UNKNOWN, source_index);
    if(!ost){
        av_log(NULL, AV_LOG_FATAL, "new_output_stream(AVMEDIA_TYPE_UNKNOWN) failed.\n");
        return nullptr;
    }
    if (!ost->stream_copy) {
        av_log(NULL, AV_LOG_FATAL, "Unknown stream encoding not supported yet (only streamcopy)\n");
        //exit_program(1);
        return nullptr;
    }

    return ost;
}

/**
 * @brief 输出流会创建一个OutputFilter过滤器，输入流会创建一个InputFilter过滤器，
 *          它们最终保存在新创建的FilterGraph系统过滤器中。
 * @param ist 输入流
 * @param ost 与输入流对应的输出流
 * @return 成功=0 失败=程序退出
 *
 * @note 这个函数看到，虽然在FilterGraph的成员outputs、inputs都是二级指针，但都是只使用第一个元素，
 * 即下面看到fg->outputs[0]、fg->inputs[0]都是固定下标0，所以这个函数只被调用一次(for循环为每个输出流调用一次)，实际上ffmpeg
 * 也是这样调的，读者可以自行搜索，看到该函数只会被调用一次.
*/
int FFmpegMedia::init_simple_filtergraph(InputStream *ist, OutputStream *ost)
{
    /*1.给封装好的系统过滤器FilterGraph开辟内存*/
    FilterGraph *fg = (FilterGraph *)av_mallocz(sizeof(*fg));
    if (!fg){
        //exit_program(1);
        return -1;
    }
    fg->index = nb_filtergraphs;

    /*2.开辟一个OutputFilter *指针和开辟一个OutputFilter结构体，
     并让该指针指向该结构体，然后对该结构体进行相关赋值*/
    //GROW_ARRAY(fg->outputs, fg->nb_outputs);//开辟OutputFilter *指针
    GROW_ARRAY_EX(fg->outputs, OutputFilter **, fg->nb_outputs);
    if (!fg->outputs){
        return -1;
    }
    if (!(fg->outputs[0] = (OutputFilter *)av_mallocz(sizeof(*fg->outputs[0])))){// 开辟一个OutputFilter结构体
        //exit_program(1);
        return -1;
    }
    fg->outputs[0]->ost   = ost;//保存输出流
    fg->outputs[0]->graph = fg;//保存该系统过滤器
    fg->outputs[0]->format = -1;

    ost->filter = fg->outputs[0];//同样ost中也会保存该OutputFilter.(建议画图容易理解)

    /*3.开辟一个InputFilter *指针和开辟一个InputFilter结构体，
     并让该指针指向该结构体，然后对该结构体进行相关赋值*/
    //GROW_ARRAY(fg->inputs, fg->nb_inputs);
    GROW_ARRAY_EX(fg->inputs, InputFilter **, fg->nb_inputs);
    if (!fg->inputs){
        return -1;
    }
    if (!(fg->inputs[0] = (InputFilter *)av_mallocz(sizeof(*fg->inputs[0])))){
        //exit_program(1);
        return -1;
    }

    /*与输出过滤器赋值同理*/
    fg->inputs[0]->ist   = ist;
    fg->inputs[0]->graph = fg;
    fg->inputs[0]->format = -1;
    /*给输入过滤器中的帧队列开辟内存，注意开辟的是指针大小的内存*/
    fg->inputs[0]->frame_queue = av_fifo_alloc(8 * sizeof(AVFrame*));/*开辟内存，只不过是使用结构体AVFifoBuffer进行返回，
                                                                        av_fifo_alloc源码很简单，可以看看*/
    if (!fg->inputs[0]->frame_queue){
        //exit_program(1);
        return -1;
    }

    //GROW_ARRAY(ist->filters, ist->nb_filters);// 给输入流的过滤器开辟一个指针
    GROW_ARRAY_EX(ist->filters, InputFilter **, ist->nb_filters);
    if(!ist->filters){
        return -1;
    }
    ist->filters[ist->nb_filters - 1] = fg->inputs[0];// 给输入流的过滤器赋值，对比输出流OutputStream可以看到，输入流可以有多个输入过滤器

    /*4.保存FilterGraph fg，其中该fg保存了新开辟的输出过滤器OutputFilter 以及 新开辟的输入过滤器InputFilter*/
    //GROW_ARRAY(filtergraphs, nb_filtergraphs);
    GROW_ARRAY_EX(filtergraphs, FilterGraph **, nb_filtergraphs);
    if(!filtergraphs){
        return -1;
    }
    filtergraphs[nb_filtergraphs - 1] = fg;

    return 0;
}

char *FFmpegMedia::describe_filter_link(FilterGraph *fg, AVFilterInOut *inout, int in)
{
    AVFilterContext *ctx = inout->filter_ctx;
    AVFilterPad *pads = in ? ctx->input_pads  : ctx->output_pads;
    int       nb_pads = in ? ctx->nb_inputs   : ctx->nb_outputs;
    AVIOContext *pb;
    uint8_t *res = NULL;

    if (avio_open_dyn_buf(&pb) < 0){
        //exit_program(1);
        return nullptr;
    }

    avio_printf(pb, "%s", ctx->filter->name);
    if (nb_pads > 1)
        avio_printf(pb, ":%s", avfilter_pad_get_name(pads, inout->pad_idx));
    avio_w8(pb, 0);
    avio_close_dyn_buf(pb, &res);
    return (char*)res;
}

int FFmpegMedia::init_input_filter(FilterGraph *fg, AVFilterInOut *in)
{
    InputStream *ist = NULL;
    enum AVMediaType type = avfilter_pad_get_type(in->filter_ctx->input_pads, in->pad_idx);
    int i;

    // TODO: support other filter types
    if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
        av_log(NULL, AV_LOG_FATAL, "Only video and audio filters supported "
               "currently.\n");
        //exit_program(1);
        return -1;
    }

    if (in->name) {
        AVFormatContext *s;
        AVStream       *st = NULL;
        char *p;
        int file_idx = strtol(in->name, &p, 0);

        if (file_idx < 0 || file_idx >= nb_input_files) {
            av_log(NULL, AV_LOG_FATAL, "Invalid file index %d in filtergraph description %s.\n",
                   file_idx, fg->graph_desc);
            //exit_program(1);
            return -1;
        }
        s = input_files[file_idx]->ctx;

        for (i = 0; i < s->nb_streams; i++) {
            enum AVMediaType stream_type = s->streams[i]->codecpar->codec_type;
            if (stream_type != type &&
                !(stream_type == AVMEDIA_TYPE_SUBTITLE &&
                  type == AVMEDIA_TYPE_VIDEO /* sub2video hack */))
                continue;
            if (check_stream_specifier(s, s->streams[i], *p == ':' ? p + 1 : p) == 1) {
                st = s->streams[i];
                break;
            }
        }
        if (!st) {
            av_log(NULL, AV_LOG_FATAL, "Stream specifier '%s' in filtergraph description %s "
                   "matches no streams.\n", p, fg->graph_desc);
            //exit_program(1);
            return -1;
        }
        ist = input_streams[input_files[file_idx]->ist_index + st->index];
        if (ist->user_set_discard == AVDISCARD_ALL) {
            av_log(NULL, AV_LOG_FATAL, "Stream specifier '%s' in filtergraph description %s "
                   "matches a disabled input stream.\n", p, fg->graph_desc);
            //exit_program(1);
            return -1;
        }
    } else {
        /* find the first unused stream of corresponding type */
        for (i = 0; i < nb_input_streams; i++) {
            ist = input_streams[i];
            if (ist->user_set_discard == AVDISCARD_ALL)
                continue;
            if (ist->dec_ctx->codec_type == type && ist->discard)
                break;
        }
        if (i == nb_input_streams) {
            av_log(NULL, AV_LOG_FATAL, "Cannot find a matching stream for "
                   "unlabeled input pad %d on filter %s\n", in->pad_idx,
                   in->filter_ctx->name);
            //exit_program(1);
            return -1;
        }
    }
    //av_assert0(ist);
    if(!ist){
        return -1;
    }

    ist->discard         = 0;
    ist->decoding_needed |= DECODING_FOR_FILTER;
    ist->st->discard = AVDISCARD_NONE;

    //GROW_ARRAY(fg->inputs, fg->nb_inputs);
    GROW_ARRAY_EX(fg->inputs, InputFilter   **, fg->nb_inputs);
    if(!fg->inputs){
        return -1;
    }
    if (!(fg->inputs[fg->nb_inputs - 1] = (InputFilter*)av_mallocz(sizeof(*fg->inputs[0])))){
        //exit_program(1);
        return -1;
    }
    fg->inputs[fg->nb_inputs - 1]->ist   = ist;
    fg->inputs[fg->nb_inputs - 1]->graph = fg;
    fg->inputs[fg->nb_inputs - 1]->format = -1;
    fg->inputs[fg->nb_inputs - 1]->type = ist->st->codecpar->codec_type;
    fg->inputs[fg->nb_inputs - 1]->name = (uint8_t *)describe_filter_link(fg, in, 1);// 先不处理describe_filter_link的返回值

    fg->inputs[fg->nb_inputs - 1]->frame_queue = av_fifo_alloc(8 * sizeof(AVFrame*));
    if (!fg->inputs[fg->nb_inputs - 1]->frame_queue){
        //exit_program(1);
        return -1;
    }

    //GROW_ARRAY(ist->filters, ist->nb_filters);
    GROW_ARRAY_EX(ist->filters, InputFilter **, ist->nb_filters);
    if(!ist->filters){
        return -1;
    }
    ist->filters[ist->nb_filters - 1] = fg->inputs[fg->nb_inputs - 1];

    return 0;
}

int FFmpegMedia::init_complex_filtergraph(FilterGraph *fg)
{
    AVFilterInOut *inputs, *outputs, *cur;
    AVFilterGraph *graph;
    int ret = 0;

    /* this graph is only used for determining the kinds of inputs
     * and outputs we have, and is discarded on exit from this function */
    graph = avfilter_graph_alloc();
    if (!graph)
        return AVERROR(ENOMEM);
    graph->nb_threads = 1;

    ret = avfilter_graph_parse2(graph, fg->graph_desc, &inputs, &outputs);
    if (ret < 0)
        goto fail;

    for (cur = inputs; cur; cur = cur->next){
        ret = init_input_filter(fg, cur);
        if(ret < 0){
            ret = -1;
            goto fail;
        }
    }

    for (cur = outputs; cur;) {
        //GROW_ARRAY(fg->outputs, fg->nb_outputs);
        GROW_ARRAY_EX(fg->outputs, OutputFilter **, fg->nb_outputs);
        if(!fg->outputs){
            ret = -1;
            goto fail;
        }
        fg->outputs[fg->nb_outputs - 1] = (OutputFilter *)av_mallocz(sizeof(*fg->outputs[0]));
        if (!fg->outputs[fg->nb_outputs - 1]){
            //exit_program(1);
            ret = -1;
            goto fail;
        }

        fg->outputs[fg->nb_outputs - 1]->graph   = fg;
        fg->outputs[fg->nb_outputs - 1]->out_tmp = cur;
        fg->outputs[fg->nb_outputs - 1]->type    = avfilter_pad_get_type(cur->filter_ctx->output_pads,
                                                                         cur->pad_idx);
        fg->outputs[fg->nb_outputs - 1]->name = (uint8_t *)describe_filter_link(fg, cur, 0);
        cur = cur->next;
        fg->outputs[fg->nb_outputs - 1]->out_tmp->next = NULL;
    }

fail:
    avfilter_inout_free(&inputs);
    avfilter_graph_free(&graph);
    return ret;
}


/**
 * Parse a metadata specifier passed as 'arg' parameter.
 * @param arg  metadata string to parse
 * @param type metadata type is written here -- g(lobal)/s(tream)/c(hapter)/p(rogram)
 * @param index for type c/p, chapter/program index is written here
 * @param stream_spec for type s, the stream specifier is written here
 */
int FFmpegMedia::parse_meta_type(char *arg, char *type, int *index, const char **stream_spec)
{
    if (*arg) {
        *type = *arg;
        switch (*arg) {
        case 'g':
            break;
        case 's':
            if (*(++arg) && *arg != ':') {
                av_log(NULL, AV_LOG_FATAL, "Invalid metadata specifier %s.\n", arg);
                //exit_program(1);
                return -1;
            }
            *stream_spec = *arg == ':' ? arg + 1 : "";
            break;
        case 'c':
        case 'p':
            if (*(++arg) == ':')
                *index = strtol(++arg, NULL, 0);
            break;
        default:
            av_log(NULL, AV_LOG_FATAL, "Invalid metadata type %c.\n", *arg);
            //exit_program(1);
            return -1;
        }
    } else
        *type = 'g';

    return 0;
}

int FFmpegMedia::copy_metadata(char *outspec, char *inspec, AVFormatContext *oc, AVFormatContext *ic, OptionsContext *o)
{
    AVDictionary **meta_in = NULL;
    AVDictionary **meta_out = NULL;
    int i, ret = 0;
    char type_in, type_out;
    const char *istream_spec = NULL, *ostream_spec = NULL;
    int idx_in = 0, idx_out = 0;

    ret = parse_meta_type(inspec,  &type_in,  &idx_in,  &istream_spec);
    if(ret < 0){
        return -1;
    }
    ret = parse_meta_type(outspec, &type_out, &idx_out, &ostream_spec);
    if(ret < 0){
        return -1;
    }

    if (!ic) {
        if (type_out == 'g' || !*outspec)
            o->metadata_global_manual = 1;
        if (type_out == 's' || !*outspec)
            o->metadata_streams_manual = 1;
        if (type_out == 'c' || !*outspec)
            o->metadata_chapters_manual = 1;
        return 0;
    }

    if (type_in == 'g' || type_out == 'g')
        o->metadata_global_manual = 1;
    if (type_in == 's' || type_out == 's')
        o->metadata_streams_manual = 1;
    if (type_in == 'c' || type_out == 'c')
        o->metadata_chapters_manual = 1;

    /* ic is NULL when just disabling automatic mappings */
    if (!ic)
        return 0;

//#define METADATA_CHECK_INDEX(index, nb_elems, desc)\
//    if ((index) < 0 || (index) >= (nb_elems)) {\
//        av_log(NULL, AV_LOG_FATAL, "Invalid %s index %d while processing metadata maps.\n",\
//                (desc), (index));\
//        exit_program(1);\
//    }
#define METADATA_CHECK_INDEX(index, nb_elems, desc)\
    if ((index) < 0 || (index) >= (nb_elems)) {\
        av_log(NULL, AV_LOG_FATAL, "Invalid %s index %d while processing metadata maps.\n",\
                (desc), (index));\
        return -1;\
    }

#define SET_DICT(type, meta, context, index)\
        switch (type) {\
        case 'g':\
            meta = &context->metadata;\
            break;\
        case 'c':\
            METADATA_CHECK_INDEX(index, context->nb_chapters, "chapter")\
            meta = &context->chapters[index]->metadata;\
            break;\
        case 'p':\
            METADATA_CHECK_INDEX(index, context->nb_programs, "program")\
            meta = &context->programs[index]->metadata;\
            break;\
        case 's':\
            break; /* handled separately below */ \
        default: /* av_assert0(0); */ return -1;\
        }\

    SET_DICT(type_in, meta_in, ic, idx_in);
    SET_DICT(type_out, meta_out, oc, idx_out);

    /* for input streams choose first matching stream */
    if (type_in == 's') {
        for (i = 0; i < ic->nb_streams; i++) {
            if ((ret = check_stream_specifier(ic, ic->streams[i], istream_spec)) > 0) {
                meta_in = &ic->streams[i]->metadata;
                break;
            } else if (ret < 0){
                //exit_program(1);
                return -1;
            }

        }
        if (!meta_in) {
            av_log(NULL, AV_LOG_FATAL, "Stream specifier %s does not match  any streams.\n", istream_spec);
            //exit_program(1);
            return -1;
        }
    }

    if (type_out == 's') {
        for (i = 0; i < oc->nb_streams; i++) {
            if ((ret = check_stream_specifier(oc, oc->streams[i], ostream_spec)) > 0) {
                meta_out = &oc->streams[i]->metadata;
                av_dict_copy(meta_out, *meta_in, AV_DICT_DONT_OVERWRITE);
            } else if (ret < 0){
                //exit_program(1);
                return -1;
            }
        }
    } else
        av_dict_copy(meta_out, *meta_in, AV_DICT_DONT_OVERWRITE);

    return 0;
}

int FFmpegMedia::copy_chapters(InputFile *ifile, OutputFile *ofile, int copy_metadata)
{
    AVFormatContext *is = ifile->ctx;
    AVFormatContext *os = ofile->ctx;
    AVChapter **tmp;
    int i;

    tmp = (AVChapter **)av_realloc_f(os->chapters, is->nb_chapters + os->nb_chapters, sizeof(*os->chapters));
    if (!tmp)
        return AVERROR(ENOMEM);
    os->chapters = tmp;

    for (i = 0; i < is->nb_chapters; i++) {
        AVChapter *in_ch = is->chapters[i], *out_ch;
        int64_t start_time = (ofile->start_time == AV_NOPTS_VALUE) ? 0 : ofile->start_time;
        int64_t ts_off   = av_rescale_q(start_time - ifile->ts_offset,
                                       AV_TIME_BASE_Q, in_ch->time_base);
        int64_t rt       = (ofile->recording_time == INT64_MAX) ? INT64_MAX :
                           av_rescale_q(ofile->recording_time, AV_TIME_BASE_Q, in_ch->time_base);


        if (in_ch->end < ts_off)
            continue;
        if (rt != INT64_MAX && in_ch->start > rt + ts_off)
            break;

        out_ch = (AVChapter *)av_mallocz(sizeof(AVChapter));
        if (!out_ch)
            return AVERROR(ENOMEM);

        out_ch->id        = in_ch->id;
        out_ch->time_base = in_ch->time_base;
        out_ch->start     = FFMAX(0,  in_ch->start - ts_off);
        out_ch->end       = FFMIN(rt, in_ch->end   - ts_off);

        if (copy_metadata)
            av_dict_copy(&out_ch->metadata, in_ch->metadata, 0);

        os->chapters[os->nb_chapters++] = out_ch;
    }
    return 0;
}

void FFmpegMedia::tyy_print_AVDirnary(AVDictionary *d){
    if(!d){
        return;
    }

    AVDictionaryEntry *t = NULL;
    while((t = av_dict_get(d, "", t, AV_DICT_IGNORE_SUFFIX))){
        printf("tyy_print_AVDirnary, t->key: %s, t->value: %s\n", t->key, t->value);
    }
}

int FFmpegMedia::open_output_file(OptionsContext *o, const char *filename)
{
    AVFormatContext *oc;
    int i, j, err, ret;
    OutputFile *of;
    OutputStream *ost;
    InputStream  *ist;
    AVDictionary *unused_opts = NULL;
    AVDictionaryEntry *e = NULL;
    int format_flags = 0;

    //o->stop_time对应参数"-to"，o->recording_time对应参数"t"
    if (o->stop_time != INT64_MAX && o->recording_time != INT64_MAX) {
        o->stop_time = INT64_MAX;
        av_log(NULL, AV_LOG_WARNING, "-t and -to cannot be used together; using -t.\n");
    }

    if (o->stop_time != INT64_MAX && o->recording_time == INT64_MAX) {
        //o->start_time对应参数"-ss"
        int64_t start_time = o->start_time == AV_NOPTS_VALUE ? 0 : o->start_time;
        if (o->stop_time <= start_time) {
            av_log(NULL, AV_LOG_ERROR, "-to value smaller than -ss; aborting.\n");
            //exit_program(1);
            return -1;
        } else {
            o->recording_time = o->stop_time - start_time;
        }
    }

    /*1.为一个输出文件开辟内存，并进行一些必要的初始化*/
    //GROW_ARRAY(output_files, nb_output_files);// 开辟sizeof(OutputFile*)大小的内存，nb_output_files自动+1
    GROW_ARRAY_EX(output_files, OutputFile **, nb_output_files);
    if (!output_files){
        return -1;
    }
    of = (OutputFile *)av_mallocz(sizeof(*of));//开辟OutputFile结构体
    if (!of){
        //exit_program(1);
        return -1;
    }
    output_files[nb_output_files - 1] = of;

    /* 对of赋值 */
    of->ost_index      = nb_output_streams;         // 保存首个输出流的流下标
    of->recording_time = o->recording_time;         // 输出文件的-t录像时长
    of->start_time     = o->start_time;
    of->limit_filesize = o->limit_filesize;
    of->shortest       = o->shortest;               // -shortest选项
    av_dict_copy(&of->opts, o->g->format_opts, 0);  // 解复用选项.这里看到输出会保留一份解复用字典，
                                                    // 而输入直接在avformat_open_input就使用，不会保存到InputFile

    if (!strcmp(filename, "-"))
        filename = "pipe:";

    // 2. 给输出文件开辟解复用上下文.
    // o->format就是-f选项的值,例如-f flv，那么o->format="flv"
    err = avformat_alloc_output_context2(&oc, NULL, o->format, filename);
    if (!oc) {
        print_error(filename, err);
        //exit_program(1);
        return -1;
    }

    of->ctx = oc;// 保存解复用的上下文
    if (o->recording_time != INT64_MAX)
        oc->duration = o->recording_time;// 设置流的时长.

    oc->interrupt_callback = int_cb;

    // 判断用户是否设置了fflags解复用选项.这里要具体看懂，还是得去看av_opt_find+av_opt_eval_flags的源码
    e = av_dict_get(o->g->format_opts, "fflags", NULL, 0);
    if (e) {
        // 在oc中的AVClass查找fflags.注：ffmpeg的第一个成员都是AVClass
        // 所以传oc就是等价于传AVClass.
        const AVOption *o = av_opt_find(oc, "fflags", NULL, 0, 0);
        /*
            这组函数可用于评估选项字符串并从中获取数字。
            它们与av_opt_set（）执行相同的操作，除了将结果写入到调用者提供的指针中。
            参数：obj：一个结构体，其第一个元素是指向AVClass的指针。
           o：要评估字符串的选项。
           val：要评估的字符串。
           *_val:字符串的值将写在这里。
            返回：0成功，负数为失败。
        */
        av_opt_eval_flags(oc, o, e->value, &format_flags);// 这个函数实现在源码找不到
    }
    if (o->bitexact) {
        format_flags |= AVFMT_FLAG_BITEXACT;
        oc->flags    |= AVFMT_FLAG_BITEXACT;
    }

    /* 3. create streams for all unlabeled output pads */
    /* 滤镜相关.
        参数"filter", "filter_script", "reinit_filter", "filter_complex",
        "lavfi", "filter_complex_script"会涉及到此处处理*/
    // 推流未使用到，后续再研究.
    for (i = 0; i < nb_filtergraphs; i++) {
        FilterGraph *fg = filtergraphs[i];
        for (j = 0; j < fg->nb_outputs; j++) {
            OutputFilter *ofilter = fg->outputs[j];

            if (!ofilter->out_tmp || ofilter->out_tmp->name)
                continue;

            switch (ofilter->type) {
            case AVMEDIA_TYPE_VIDEO:    o->video_disable    = 1; break;
            case AVMEDIA_TYPE_AUDIO:    o->audio_disable    = 1; break;
            case AVMEDIA_TYPE_SUBTITLE: o->subtitle_disable = 1; break;
            }

            ret = init_output_filter(ofilter, o, oc);
            if(ret < 0){
                av_log(NULL, AV_LOG_ERROR, "init_output_filter failed.\n");
                return -1;
            }
        }
    }

    /*4.对应参数"-map".*/
    /*1.不传map参数时，默认nb_stream_maps=0，传时，则不为0*/
    /*例如-y -i 1.mkv -map 0:0 -map 0:1 -map 0:2 -c:v libx264 -c:a:0 aac -b:a:0 128k -c:s mov_text map.mp4
    备注，mp4不支持ass、srt这些类型的字幕，需要转成mov_text，不想转则将输出格式mp4换成mkv即可.*/
    /*map相关可参考 https://blog.csdn.net/m0_60259116/article/details/125642026*/
    if (!o->nb_stream_maps) {
        char *subtitle_codec_name = NULL;

        /* pick the "best" stream of each type */

        /*4.1. 若存在多个输入视频流，则选择一个最好的视频流，来创建新的输出视频流*/
        /* video: highest resolution */
        // 没有禁用视频，那么就去猜测编解码器id,依赖oc->oformat->name(-f参数，例如flv)以及文件名，且猜测的id不能是AV_CODEC_ID_NONE.
        // 视频通过av_guess_codec处理后，看源码得出，视频一般返回的是AVOutputFormat->video_codec
        if (!o->video_disable && av_guess_codec(oc->oformat, NULL, filename, NULL, AVMEDIA_TYPE_VIDEO) != AV_CODEC_ID_NONE) {
            int area = 0, idx = -1;
            /*
             * 测试给定的容器是否可以存储该编解码器(该函数不用看源码，虽然不复杂，但是有回调函数的判断，且该函数意思可以直接看懂)。
             * @param ofmt 用来检查兼容性的容器
             * @param codec_id 可能存储在容器中的编解码器
             * @param std_compliance 标准遵从级别，FF_COMPLIANCE_*之一
             * @return 如果codec_id的编解码器可以存储在ofmt中，则返回1，如果不能，则返回0。如果该信息不可用，则为负数。
            */
            int qcr = avformat_query_codec(oc->oformat, oc->oformat->video_codec, 0);

            for (i = 0; i < nb_input_streams; i++) {
                int new_area;
                ist = input_streams[i];
                // codec_info_nb_frames:avformat_find_stream_info()过程中被解复用的帧数
                // disposition:?
                // 单目运算符即逻辑非! 是比 算术运算符乘号* 优先级高的。see https://blog.csdn.net/sgbl888/article/details/123997358
                // auto tyycode = !ist->st->codec_info_nb_frames;
                // auto tyycode1 = !!ist->st->codec_info_nb_frames;
                // 100000000*!!ist->st->codec_info_nb_frames意思：codec_info_nb_frames为非零，取值100000000；为零，取值0.
                // 5000000*!!(ist->st->disposition & AV_DISPOSITION_DEFAULT)意思：含有AV_DISPOSITION_DEFAULT，取值5000000；不含，取值0.
                new_area = ist->st->codecpar->width * ist->st->codecpar->height + 100000000*!!ist->st->codec_info_nb_frames
                           + 5000000*!!(ist->st->disposition & AV_DISPOSITION_DEFAULT);/*new_area的作用与音频的score一样，请看下面音频的解释*/
                if (ist->user_set_discard == AVDISCARD_ALL)// 若用户要丢弃该输入流，那么输出流则跳过不处理
                    continue;

                // 包含附属图，new_area标记为1
                // 在这里qcr肯定不等于MKTAG('A', 'P', 'I', 'C')
                if((qcr!=MKTAG('A', 'P', 'I', 'C')) && (ist->st->disposition & AV_DISPOSITION_ATTACHED_PIC))
                    new_area = 1;

                /* MKTAG宏示例：
                 * ascii码：'A'=65,'P'=80,'I'=73,'C'=67*/
                // 01000001 | (01010000 << 8) | (01001001 << 16) | (01000011 << 24)
                if (ist->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
                    new_area > area) {
                    // 这里qcr必定不等于MKTAG('A', 'P', 'I', 'C')，故不会进入(这种特殊的视频比较少见，大家可以自行研究)
                    if((qcr==MKTAG('A', 'P', 'I', 'C')) && !(ist->st->disposition & AV_DISPOSITION_ATTACHED_PIC))
                        continue;

                    // 视频正常往这里走
                    area = new_area;
                    idx = i;
                }
            }//<== for (i = 0; i < nb_input_streams; i++) end ==>

            if (idx >= 0){
                //idx是视频流的下标.不需要处理返回值是因为，oc中也保存着一份新创建的流的地址.即oc->streams[]二级指针中
                if(new_video_stream(o, oc, idx) == nullptr){
                    av_log(NULL, AV_LOG_ERROR, "new_video_stream failed.\n");
                    return -1;
                }
            }

        }//<== if (!o->video_disable && av_guess_codec(oc->oformat, NULL, filename, NULL, AVMEDIA_TYPE_VIDEO) != AV_CODEC_ID_NONE) end ==>

        /*4.2. 若存在多个输入音频流，则选择一个最好的音频流，来创建新的输出音频流*/
        /*进入这里要把-an去掉(输入输出文件都去掉)，且输入文件要有音频*/
        /* audio: most channels */
        enum AVCodecID tyycode1 = av_guess_codec(oc->oformat, NULL, filename, NULL, AVMEDIA_TYPE_AUDIO);//例如0X15001是AV_CODEC_ID_MP3
        if (!o->audio_disable && av_guess_codec(oc->oformat, NULL, filename, NULL, AVMEDIA_TYPE_AUDIO) != AV_CODEC_ID_NONE) {
            int best_score = 0, idx = -1;/*best_score用于保存输入音频流得分最高的分数，从而选择最好的输入音频流*/
            for (i = 0; i < nb_input_streams; i++) {
                int score;
                ist = input_streams[i];
                score = ist->st->codecpar->channels + 100000000*!!ist->st->codec_info_nb_frames
                        + 5000000*!!(ist->st->disposition & AV_DISPOSITION_DEFAULT);
                if (ist->user_set_discard == AVDISCARD_ALL)
                    continue;

                /*从这里看到，socre的作用是：
                 * 当输入文件存在多个音频流，会对比各个音频流的得分，选择得分最高的音频流作为输入文件的音频流。
                 * 得分规则：
                 * 1)默认加上ist->st->codecpar->channels通道数的分数。
                 * 2)codec_info_nb_frames不为0：加10^8分.
                 * 3)disposition存在AV_DISPOSITION_DEFAULT宏：加5x10^6分.
                 * 上面看到，存在codec_info_nb_frames优先选择的，其次是AV_DISPOSITION_DEFAULT，若都不存在，则考虑通道数。
                  */
                if (ist->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
                    score > best_score) {
                    best_score = score;
                    /*这里看到，若存在多个输入音频流，idx只会保存最后一个有效的音频流下标*/
                    idx = i;
                }
            }
            if (idx >= 0){
                if(new_audio_stream(o, oc, idx) == nullptr){
                    av_log(NULL, AV_LOG_ERROR, "new_audio_stream failed.\n");
                    return -1;
                }
            }

        }
        /*所以经过上面两步，ffmpeg为你选择了最好的视频流+音频流来创建输出文件*/


        /*4.3. 为字幕流开辟输出流，若存在多个字幕流，ffmpeg只会取第一个字幕流*/
        /*字幕相关只是简单分析，不会深入研究*/
        /* subtitles: pick first(选择第一个字幕流) */
        /*获取用户指定的字幕编解码器名字*/
        //MATCH_PER_TYPE_OPT(codec_names, str, subtitle_codec_name, oc, "s");
        MATCH_PER_TYPE_OPT_EX(codec_names, str, subtitle_codec_name, char *, oc, "s");
        AVCodec * tyycodeSc = avcodec_find_encoder(oc->oformat->subtitle_codec);
        /*这个if条件很简单，1.不管有无字幕，都先判断有无禁用该字幕；2.无禁用，则再判断输入文件是否包含字幕流.*/
        /*这里可以在推流命令基础添加：-scodec text.注意，因为我们推rtmp，-f是指定为flv，而在ffmpeg中的flv是不支持text以外的编解码器，
        所以当你-scodec ass指定字幕编解码器为ass，会报错：Subtitle codec '%s' for stream %d is not compatible with FLV.
        这是笔者从ffmpeg源码分析得出的.
        不过有个奇怪的点，当用基础推流命令加上-scodec text，vlc有画面输出(没字幕)，但ffplay播放rtmp却只能显示音频图，而没画面输出，
        播flv、hls则没问题，流媒体服务器是srs.
        笔者找到原因是加上了-scodec text，去掉后ffplay播放重新有画面.根本原因有兴趣的可以自行看ffplay源码分析.这里是为了使流程往下走*/
        if (!o->subtitle_disable && (avcodec_find_encoder(oc->oformat->subtitle_codec) || subtitle_codec_name)) {
            for (i = 0; i < nb_input_streams; i++)
                /*1.判断该输入流是否字幕流*/
                if (input_streams[i]->st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                    /*1.1利用输入的编解码器id获取输入描述符*/
                    AVCodecDescriptor const *input_descriptor =
                        avcodec_descriptor_get(input_streams[i]->st->codecpar->codec_id);
                    AVCodecDescriptor const *output_descriptor = NULL;

                    AVCodec const *output_codec =
                        avcodec_find_encoder(oc->oformat->subtitle_codec);
                    int input_props = 0, output_props = 0;

                    /*如果流数据的user_set_discard设为AVDISCARD_ALL，那么该流被丢弃，不做处理*/
                    if (input_streams[i]->user_set_discard == AVDISCARD_ALL)
                        continue;

                    /*1.2利用输出的编解码器id获取输出描述符*/
                    if (output_codec)
                        output_descriptor = avcodec_descriptor_get(output_codec->id);

                    /*1.3获取输入以及输出的属性，保存到局部变量中*/
                    /*注，与上这两个宏目的是为了清除其它宏的标志位*/
                    if (input_descriptor)
                        input_props = input_descriptor->props & (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);
                    if (output_descriptor)
                        output_props = output_descriptor->props & (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);

                    /*1.4用户指定字幕编解码器(且输入文件存在字幕即上面的if条件)，就会new一个字幕流*/
                    /* &&、||、!即与或非，相同的逻辑表达式时，求值顺序是从左往右；不同的逻辑运算符时，非最高，然后是&&，再到||*/
                    /*int a = TRUE && !FALSE;// 1
                    int a1 = TRUE && (!FALSE);// 1 故通过a、a1的测试得出，当存在&&以及!运算时，!的优先级比&&高
                    int x = TRUE || TRUE && FALSE;// 1
                    int x1 = TRUE || (TRUE && FALSE);// 1
                    int x2 = (TRUE || TRUE) && FALSE;// 0 故通过x、x1、x2的测试得出，当存在||以及&&运算时，&&的优先级比||高
                    */
                    /*int tyycodeSub = ((subtitle_codec_name || input_props & output_props) ||
                            ((input_descriptor && output_descriptor) &&
                            (!input_descriptor->props ||
                             !output_descriptor->props)));*///等价于下面的if语句.

                    if (subtitle_codec_name ||/*用户指定字幕编解码器*/
                        input_props & output_props ||/*input_props & output_props代表：它们是否都同时至少具有一个宏AV_CODEC_PROP_TEXT_SUB或者AV_CODEC_PROP_BITMAP_SUB*/
                        // Map dvb teletext which has neither property to any output subtitle encoder
                        input_descriptor && output_descriptor && /*输入输出描述符都存在*/
                        (!input_descriptor->props ||
                         !output_descriptor->props))//注意，当output_descriptor为空不会来到这里，所以不会存在段错误
                    {
                        if(new_subtitle_stream(o, oc, i) == nullptr){
                            av_log(NULL, AV_LOG_ERROR, "new_subtitle_stream failed.\n");
                            return -1;
                        }
                        break;//这里看到，只会拿第一个字幕流
                    }
                }
        }

        /*4.4 为数据流开辟输出流。若存在输入文件多个数据流，同样会为其开辟相同个数的数据流。
         * 暂未研究，且比较少见*/
        /* Data only if codec id match */
        if (!o->data_disable ) {
            enum AVCodecID codec_id = av_guess_codec(oc->oformat, NULL, filename, NULL, AVMEDIA_TYPE_DATA);
            for (i = 0; codec_id != AV_CODEC_ID_NONE && i < nb_input_streams; i++) {
                if (input_streams[i]->user_set_discard == AVDISCARD_ALL)
                    continue;
                if (input_streams[i]->st->codecpar->codec_type == AVMEDIA_TYPE_DATA
                    && input_streams[i]->st->codecpar->codec_id == codec_id ){
                    if(new_data_stream(o, oc, i) == nullptr){
                        av_log(NULL, AV_LOG_ERROR, "new_data_stream failed.\n");
                        return -1;
                    }
                }
            }
        }

    } else {
        /*暂不研究map，后续补充*/
        for (i = 0; i < o->nb_stream_maps; i++) {
            StreamMap *map = &o->stream_maps[i];

            if (map->disabled)
                continue;

            if (map->linklabel) {
                FilterGraph *fg;
                OutputFilter *ofilter = NULL;
                int j, k;

                for (j = 0; j < nb_filtergraphs; j++) {
                    fg = filtergraphs[j];
                    for (k = 0; k < fg->nb_outputs; k++) {
                        AVFilterInOut *out = fg->outputs[k]->out_tmp;
                        if (out && !strcmp(out->name, map->linklabel)) {
                            ofilter = fg->outputs[k];
                            goto loop_end;
                        }
                    }
                }
loop_end:
                if (!ofilter) {
                    av_log(NULL, AV_LOG_FATAL, "Output with label '%s' does not exist "
                           "in any defined filter graph, or was already used elsewhere.\n", map->linklabel);
                    //exit_program(1);
                    return -1;
                }
                ret = init_output_filter(ofilter, o, oc);
                if(ret < 0){
                    av_log(NULL, AV_LOG_FATAL, "init_output_filter failed.\n");
                    return -1;
                }
            } else {
                int src_idx = input_files[map->file_index]->ist_index + map->stream_index;

                ist = input_streams[input_files[map->file_index]->ist_index + map->stream_index];
                if (ist->user_set_discard == AVDISCARD_ALL) {
                    av_log(NULL, AV_LOG_FATAL, "Stream #%d:%d is disabled and cannot be mapped.\n",
                           map->file_index, map->stream_index);
                    //exit_program(1);
                    return -1;
                }
                if(o->subtitle_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
                    continue;
                if(o->   audio_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                    continue;
                if(o->   video_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
                    continue;
                if(o->    data_disable && ist->st->codecpar->codec_type == AVMEDIA_TYPE_DATA)
                    continue;

                ost = NULL;
                switch (ist->st->codecpar->codec_type) {
                case AVMEDIA_TYPE_VIDEO:      ost = new_video_stream     (o, oc, src_idx); break;
                case AVMEDIA_TYPE_AUDIO:      ost = new_audio_stream     (o, oc, src_idx); break;
                case AVMEDIA_TYPE_SUBTITLE:   ost = new_subtitle_stream  (o, oc, src_idx); break;
                case AVMEDIA_TYPE_DATA:       ost = new_data_stream      (o, oc, src_idx); break;
                case AVMEDIA_TYPE_ATTACHMENT: ost = new_attachment_stream(o, oc, src_idx); break;
                case AVMEDIA_TYPE_UNKNOWN:
                    if (copy_unknown_streams) {
                        ost = new_unknown_stream   (o, oc, src_idx);
                        break;
                    }
                default:
                    av_log(NULL, ignore_unknown_streams ? AV_LOG_WARNING : AV_LOG_FATAL,
                           "Cannot map stream #%d:%d - unsupported type.\n",
                           map->file_index, map->stream_index);
                    if (!ignore_unknown_streams) {
                        av_log(NULL, AV_LOG_FATAL,
                               "If you want unsupported types ignored instead "
                               "of failing, please use the -ignore_unknown option\n"
                               "If you want them copied, please use -copy_unknown\n");
                        //exit_program(1);
                        return -1;
                    }
                }

                if(!ost){// tyycode check ost that new stream successful
                    return -1;
                }
                if (ost)
                    ost->sync_ist = input_streams[  input_files[map->sync_file_index]->ist_index
                                                  + map->sync_stream_index];
            }
        }
    }

    /*5. 对应参数"attach"，添加一个附件到输出文件*/
    /*处理附加文件，推流没用到，暂未研究，读者可自行研究*/
    /* handle attached files */
    for (i = 0; i < o->nb_attachments; i++) {
        AVIOContext *pb;
        uint8_t *attachment;
        const char *p;
        int64_t len;

        if ((err = avio_open2(&pb, o->attachments[i], AVIO_FLAG_READ, &int_cb, NULL)) < 0) {
            av_log(NULL, AV_LOG_FATAL, "Could not open attachment file %s.\n",
                   o->attachments[i]);
            //exit_program(1);
            return -1;
        }
        if ((len = avio_size(pb)) <= 0) {
            av_log(NULL, AV_LOG_FATAL, "Could not get size of the attachment %s.\n",
                   o->attachments[i]);
            //exit_program(1);
            return -1;
        }
        if (len > INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE ||
            !(attachment = (uint8_t *)av_malloc(len + AV_INPUT_BUFFER_PADDING_SIZE))) {
            av_log(NULL, AV_LOG_FATAL, "Attachment %s too large.\n",
                   o->attachments[i]);
            //exit_program(1);
            return -1;
        }
        avio_read(pb, attachment, len);
        memset(attachment + len, 0, AV_INPUT_BUFFER_PADDING_SIZE);

        ost = new_attachment_stream(o, oc, -1);
        if(!ost){
            return -1;
        }
        ost->stream_copy               = 0;
        ost->attachment_filename       = o->attachments[i];
        ost->st->codecpar->extradata      = attachment;
        ost->st->codecpar->extradata_size = len;

        p = strrchr(o->attachments[i], '/');
        av_dict_set(&ost->st->metadata, "filename", (p && *p) ? p + 1 : o->attachments[i], AV_DICT_DONT_OVERWRITE);
        avio_closep(&pb);
    }

#if FF_API_LAVF_AVCTX
    /*6. 若满足一定条件，则为该输出流的编解码器设置flags选项*/
    /*nb_output_streams一般与oc->nb_streams相等*/
    /*推流没用到，暂未研究*/
    for (i = nb_output_streams - oc->nb_streams; i < nb_output_streams; i++) { //for all streams of this output file(对于该输出文件的所有流)
        AVDictionaryEntry *e;
        ost = output_streams[i];

        if ((ost->stream_copy || ost->attachment_filename)/*有不转码请求或者附属文件名不为空*/
            && (e = av_dict_get(o->g->codec_opts, "flags", NULL, AV_DICT_IGNORE_SUFFIX))/*存在忽略大小写的flags编解码器选项*/
            && (!e->key[5] || check_stream_specifier(oc, ost->st, e->key+6))){/*因为上面调用后，e->key可能是父串，"flags"作为子串，
            故!e->key[5]的意思是排除"flags"作为子串的可能，即刚好是"flags"，则程序退出.
            或者e->key+6后的字符串与对应的流不匹配(+6是因为跳过对应分隔符)，程序也退出.*/
            if (av_opt_set(ost->st->codec, "flags", e->value, 0) < 0){
                //exit_program(1);
                return -1;
            }
        }

    }
#endif

    /*7. 检测，当输出流数为0，且(对(格式不需要任何流)取反)即格式需要流的情况，那么程序退出*/
    if (!oc->nb_streams && !(oc->oformat->flags & AVFMT_NOSTREAMS)) {
        av_dump_format(oc, nb_output_files - 1, oc->url, 1);
        av_log(NULL, AV_LOG_ERROR, "Output file #%d does not contain any stream\n", nb_output_files - 1);
        //exit_program(1);
        return -1;
    }

    /* check if all codec options have been used */
    /*8. 得到用户输入并且去掉流说明符的字典*/
    unused_opts = strip_specifiers(o->g->codec_opts);
    /*遍历每个输出流，利用每个输出流保存的编解码器选项，将用户输入的选项unused_opts设置为空
    因为会在调用以视频为例，new_video_stream的new_output_stream中，看到：
    ost->encoder_opts  = filter_codec_opts(o->g->codec_opts, ost->enc->id, oc, st, ost->enc);调用，
    这样每个流都保存了属于自己的编解码器选项。
    那么，现在，只要将保存的选项去清除用户的选项，就知道用户输入的选项剩余哪些没使用了*/
    for (i = of->ost_index; i < nb_output_streams; i++) {
        e = NULL;
        while ((e = av_dict_get(output_streams[i]->encoder_opts, "", e,
                                AV_DICT_IGNORE_SUFFIX)))
            av_dict_set(&unused_opts, e->key, NULL, 0);
    }

    /*9. 检测用户剩余未使用的选项，对剩余的选项进行检测判断忽略该选项或者程序退出*/
    /*实际下面的判断逻辑很简单，1.判断该选项是否是编解码器选项，不是则直接忽略；
     * 是则再判断是不是解复用选项，是的话，那么也忽略；是编解码器选项且不是的解复用的话，那么肯定是编解码器选项，且这里是输出流需要编码，
     * 所以继续往下判断，若不是编码选项，则程序退出*/
    e = NULL;
    while ((e = av_dict_get(unused_opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
        const AVClass *avclass = avcodec_get_class();
        const AVOption *option = av_opt_find(&avclass, e->key, NULL, 0,
                                             AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
        const AVClass *fclass = avformat_get_class();
        const AVOption *foption = av_opt_find(&fclass, e->key, NULL, 0,
                                              AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
        /*1.在avcodec的AVClass找不到该选项，则忽略它；
          2.在avcodec的AVClass找到该选项，则看它是否在avformat的AVClass中找到：
                2.1）若在avformat的AVClass中找到，则忽略；
                2.2）若在avformat的AVClass中找不到，则往下。说明是avcodec的AVClass的选项*/
        if (!option || foption)
            continue;

        /*该选项不是编码选项(换句话说是解码器选项)，程序退出*/
        if (!(option->flags & AV_OPT_FLAG_ENCODING_PARAM)) {
            av_log(NULL, AV_LOG_ERROR, "Codec AVOption %s (%s) specified for "
                   "output file #%d (%s) is not an encoding option.\n", e->key,
                   option->help ? option->help : "", nb_output_files - 1,
                   filename);
            //exit_program(1);
            return -1;
        }

        // gop_timecode is injected by generic code but not always used
        // gop_timecode是由泛型代码注入的，但并不总是使用
        if (!strcmp(e->key, "gop_timecode"))
            continue;

        av_log(NULL, AV_LOG_WARNING, "Codec AVOption %s (%s) specified for "
               "output file #%d (%s) has not been used for any stream. The most "
               "likely reason is either wrong type (e.g. a video option with "
               "no video streams) or that it is a private option of some encoder "
               "which was not actually used for any stream.\n", e->key,
               option->help ? option->help : "", nb_output_files - 1, filename);
    }
    av_dict_free(&unused_opts);

    /* set the decoding_needed flags and create simple filtergraphs */
    /*10. 设置decoing_needed标志并创建简单的过滤器*/
    /* 这个for循环主要工作：判断每个输出流是否需要编码，是的话会对该输出流的过滤器进行初始化,
     * 但只能是视频或者音频流*/
    /*10.1遍历输出文件的每个输出流*/
    for (i = of->ost_index; i < nb_output_streams; i++) {
        OutputStream *ost = output_streams[i];

        /*10.2判断该输出流是否需要重新编码，并且要求该输出流对应的输入流下标>=0*/
        if (ost->encoding_needed && ost->source_index >= 0) {
            /*10.3获取该输出流对应的输入流ist，并标记该输入流需要解码*/
            InputStream *ist = input_streams[ost->source_index];
            ist->decoding_needed |= DECODING_FOR_OST;

            /*10.4媒体类型是视频或者音频，需要利用输入输出流初始化简单过滤器.
            所以字幕一般来到这里，但不属于音视频类型，所以字幕不会创建过滤器*/
            if (ost->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ||
                ost->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                err = init_simple_filtergraph(ist, ost);/*利用输入输出流创建过滤器*/
                if (err < 0) {
                    av_log(NULL, AV_LOG_ERROR,
                           "Error initializing a simple filtergraph between streams "
                           "%d:%d->%d:%d\n", ist->file_index, ost->source_index,
                           nb_output_files - 1, ost->st->index);
                    //exit_program(1);
                    return -1;
                }
            }
        }

        /* 10.5 当上面需要初始化过滤器后，这里会利用编码器的参数设置过滤器的输出条件。
         * 主要是：视频的帧率、分辨率、帧格式；音频的采样格式、采样率、通道布局*/
        /* set the filter output constraints */
        if (ost->filter) {
            OutputFilter *f = ost->filter;//fg->outputs[0]
            int count;
            // 下面全是为输出过滤器f赋值的
            switch (ost->enc_ctx->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                f->frame_rate = ost->frame_rate;
                f->width      = ost->enc_ctx->width;
                f->height     = ost->enc_ctx->height;
                /* 利用编码器上下文或者编码器，给输出过滤器获取支持的像素格式。
                 * 若编码器上下文已经有像素格式则直接获取(保存在format变量)，否则从编码器中获取(保存在formats数组) */
                if (ost->enc_ctx->pix_fmt != AV_PIX_FMT_NONE) {
                    f->format = ost->enc_ctx->pix_fmt;// 注，这里是变量
                } else if (ost->enc->pix_fmts) {
                    count = 0;
                    /* 遍历该编码器支持的像素格式，得到支持像素格式数组的大小count */
                    /* 以推流使用libx264编码器为例：我们在libavcodec/allcodecs.c文件找到，extern AVCodec ff_libx264_encoder编码器对象，
                     * 在X264_init_static()中，看到会根据x264的版本，支持不同的像素格式，我这里是x264dll是164版本，
                     * 所以指向pix_fmts_all，共15个元素(不包含AV_PIX_FMT_NONE) */
                    while (ost->enc->pix_fmts[count] != AV_PIX_FMT_NONE)
                        count++;
                    f->formats = (int *)av_mallocz_array(count + 1, sizeof(*f->formats));
                    if (!f->formats){
                        //exit_program(1);
                        return -1;
                    }

                    memcpy(f->formats, ost->enc->pix_fmts, (count + 1) * sizeof(*f->formats));// 注，这里是formats数组
                }
                break;
            case AVMEDIA_TYPE_AUDIO:
                /* 1.获取音频像素格式、采样率、通道布局.获取方法与视频同理 */
                if (ost->enc_ctx->sample_fmt != AV_SAMPLE_FMT_NONE) {
                    f->format = ost->enc_ctx->sample_fmt;
                } else if (ost->enc->sample_fmts) {
                    count = 0;
                    while (ost->enc->sample_fmts[count] != AV_SAMPLE_FMT_NONE)
                        count++;
                    f->formats = (int *)av_mallocz_array(count + 1, sizeof(*f->formats));
                    if (!f->formats){
                        //exit_program(1);
                        return -1;
                    }
                    memcpy(f->formats, ost->enc->sample_fmts, (count + 1) * sizeof(*f->formats));
                }
                /* 获取音频的采样率 */
                if (ost->enc_ctx->sample_rate) {
                    f->sample_rate = ost->enc_ctx->sample_rate;
                } else if (ost->enc->supported_samplerates) {
                    count = 0;
                    while (ost->enc->supported_samplerates[count])
                        count++;
                    f->sample_rates = (int *)av_mallocz_array(count + 1, sizeof(*f->sample_rates));
                    if (!f->sample_rates){
                        //exit_program(1);
                        return -1;
                    }
                    memcpy(f->sample_rates, ost->enc->supported_samplerates,
                           (count + 1) * sizeof(*f->sample_rates));
                }
                /* 获取音频的通道布局 */
                if (ost->enc_ctx->channels) {
                    f->channel_layout = av_get_default_channel_layout(ost->enc_ctx->channels);
                } else if (ost->enc->channel_layouts) {
                    count = 0;
                    while (ost->enc->channel_layouts[count])
                        count++;
                    f->channel_layouts = (uint64_t *)av_mallocz_array(count + 1, sizeof(*f->channel_layouts));
                    if (!f->channel_layouts){
                        //exit_program(1);
                        return -1;
                    }
                    memcpy(f->channel_layouts, ost->enc->channel_layouts,
                           (count + 1) * sizeof(*f->channel_layouts));
                }
                break;
            }//<== switch (ost->enc_ctx->codec_type) end ==>
        }//<== if (ost->filter) end ==>
    }//<== for (i = of->ost_index; i < nb_output_streams; i++) emd ==>

    /*11. 如果需要图像编号，请检查文件名.有兴趣的请看av_filename_number_test源码，推流没用到*/
    /* check filename in case of an image number is expected */
    if (oc->oformat->flags & AVFMT_NEEDNUMBER) {
        if (!av_filename_number_test(oc->url)) {
            print_error(oc->url, AVERROR(EINVAL));
            //exit_program(1);
            return -1;;
        }
    }

    /*12. 输出需要流 且 输入流为空，程序退出。
    input_stream_potentially_available=0代表输入文件没有输入流*/
    if (!(oc->oformat->flags & AVFMT_NOSTREAMS) && !input_stream_potentially_available) {
        av_log(NULL, AV_LOG_ERROR,
               "No input streams but output needs an input stream\n");
        //exit_program(1);
        return -1;
    }

    /*13. 判断该输出文件能否重写，若不能程序直接退出；能则将输出文件重置，等待后面的流程输入数据*/
    /*输出文件的AVOutputFormat不包含AVFMT_NOFILE，那么进入if流程(实时流和文件一般都会进入)*/
    /*AVFMT_NOFILE：大概搜了源码以及百度，意思是：指一些特定于设备的特殊文件，
    且AVIOContext表示字节流输入/输出的上下文，在muxers和demuxers的数据成员flags有设置AVFMT_NOFILE时，
    这个成员变量pb就不需要设置，因为muxers和demuxers会使用其它的方式处理输入/输出。*/
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        /* test if it already exists to avoid losing precious files */
        ret = assert_file_overwrite(filename);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "assert_file_overwrite failed, file: %s has exist?\n", filename);
            return -1;
        }

        /* open the file */
        /*因为没有AVFMT_NOFILE，所以需要调avio_open2给oc->pb赋值*/
        /*经过assert_file_overwrite断言后，说明用户是认可重写该文件的，那么经过avio_open2处理后，
        输入文件就会被重新创建，debug此时看到文件变成0KB的大小*/
        if ((err = avio_open2(&oc->pb, filename, AVIO_FLAG_WRITE,
                              &oc->interrupt_callback,
                              &of->opts)) < 0) {
            print_error(filename, err);
            //exit_program(1);
            return -1;
        }
    } else if (strcmp(oc->oformat->name, "image2")==0 && !av_filename_number_test(filename)){
        ret = assert_file_overwrite(filename);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "assert_file_overwrite(image2) failed, file: %s has exist?\n", filename);
            return -1;
        }
    }


    if (o->mux_preload) {
        av_dict_set_int(&of->opts, "preload", o->mux_preload*AV_TIME_BASE, 0);
    }
    oc->max_delay = (int)(o->mux_max_delay * AV_TIME_BASE);

    /* copy metadata */
    /*14. copy_metadata()拷贝元数据，推流没用到，有兴趣可参考：
    https://blog.csdn.net/feiyu5323/article/details/118352974以及
    https://www.jianshu.com/p/bd752c86f3e7
    */
    for (i = 0; i < o->nb_metadata_map; i++) {
        char *p;
        int in_file_index = strtol((const char *)o->metadata_map[i].u.str, &p, 0);

        if (in_file_index >= nb_input_files) {
            av_log(NULL, AV_LOG_FATAL, "Invalid input file index %d while processing metadata maps\n", in_file_index);
            //exit_program(1);
            return -1;
        }

        ret = copy_metadata(o->metadata_map[i].specifier, *p ? p + 1 : p, oc,
                      in_file_index >= 0 ?
                      input_files[in_file_index]->ctx : NULL, o);
        if (ret < 0) {
            av_log(NULL, AV_LOG_FATAL, "copy_metadata failed.\n");
            return -1;
        }
    }

    /* 15. copy chapters */
    /*推流没用到，暂不研究*/
    if (o->chapters_input_file >= nb_input_files) {
        if (o->chapters_input_file == INT_MAX) {
            /* copy chapters from the first input file that has them*/
            o->chapters_input_file = -1;
            for (i = 0; i < nb_input_files; i++)
                if (input_files[i]->ctx->nb_chapters) {
                    o->chapters_input_file = i;
                    break;
                }
        } else {
            av_log(NULL, AV_LOG_FATAL, "Invalid input file index %d in chapter mapping.\n",
                   o->chapters_input_file);
            //exit_program(1);
            return -1;
        }
    }
    /*推流没用到，暂不研究*/
    if (o->chapters_input_file >= 0)
        copy_chapters(input_files[o->chapters_input_file], of,
                      !o->metadata_chapters_manual);// ffmpeg并未处理copy_chapters返回值,所以我们也不管

    /*16. 这里就是将输入文件的全局元数据拷贝到输出文件的全局元数据*/
    /* copy global metadata by default */
    printf("============ global metadata ===========\n");
    printf("copy before, input_files[0]->ctx->metadata: \n");
    if(input_files[0]->ctx->metadata)
        tyy_print_AVDirnary(input_files[0]->ctx->metadata);
    else
        printf("input_files[0]->ctx->metadata is null\n");

    printf("copy before, oc->metadata: \n");
    if(oc->metadata)
        tyy_print_AVDirnary(oc->metadata);
    else
        printf("oc->metadata is null\n");
    if (!o->metadata_global_manual && nb_input_files){
        av_dict_copy(&oc->metadata, input_files[0]->ctx->metadata,
                     AV_DICT_DONT_OVERWRITE);
        if(o->recording_time != INT64_MAX)
            av_dict_set(&oc->metadata, "duration", NULL, 0);
        av_dict_set(&oc->metadata, "creation_time", NULL, 0);
    }
    printf("+++++++++ copy after, input_files[0]->ctx->metadata: \n");
    tyy_print_AVDirnary(input_files[0]->ctx->metadata);
    printf("copy after, oc->metadata: \n");
    tyy_print_AVDirnary(oc->metadata);
    printf("============ global metadata ===========\n");

    /*17. 这里就是将输入文件的每一个流的元数据拷贝到输出文件对应每一个流的元数据中*/
    if (!o->metadata_streams_manual)
        for (i = of->ost_index; i < nb_output_streams; i++) {
            InputStream *ist;
            if (output_streams[i]->source_index < 0)         /* this is true e.g. for attached files */
                continue;

            ist = input_streams[output_streams[i]->source_index];//根据输出流中保存的输入流下标，得到对应的输入流

            printf("============ stream metadata %d ===========\n", i);
            printf("copy before, ist->st->metadata: \n");
            if(ist->st->metadata)
                tyy_print_AVDirnary(ist->st->metadata);
            else
                printf("ist->st->metadata is null\n");

            printf("copy before, output_streams[i]->st->metadata: \n");
            if(output_streams[i]->st->metadata)
                tyy_print_AVDirnary(output_streams[i]->st->metadata);
            else
                printf("output_streams[i]->st->metadata is null\n");
            av_dict_copy(&output_streams[i]->st->metadata, ist->st->metadata, AV_DICT_DONT_OVERWRITE);//就是这里进行拷贝的

            // 拷贝之后应该也要检查是否为空的，这里就不写得这么仔细了
            printf("+++++++++ copy after, ist->st->metadata: \n");
            tyy_print_AVDirnary(ist->st->metadata);
            printf("copy after, output_streams[i]->st->metadata: \n");
            tyy_print_AVDirnary(output_streams[i]->st->metadata);
            printf("============ stream metadata %d ===========\n", i);

            if (!output_streams[i]->stream_copy) {
                /*转码则把元数据的encoder删掉，因为后续会在set_encoder_id()中为每个流增加该选项*/
                av_dict_set(&output_streams[i]->st->metadata, "encoder", NULL, 0);
            }
        }

    /*18 process manually set programs，推流没用到，暂不研究*/
    /* process manually set programs */
    for (i = 0; i < o->nb_program; i++) {
        const char *p = (const char *)o->program[i].u.str;
        int progid = i+1;
        AVProgram *program;

        while(*p) {
            const char *p2 = av_get_token(&p, ":");
            const char *to_dealloc = p2;
            char *key;
            if (!p2)
                break;

            if(*p) p++;

            key = av_get_token(&p2, "=");
            if (!key || !*p2) {
                av_freep(&to_dealloc);
                av_freep(&key);
                break;
            }
            p2++;

            if (!strcmp(key, "program_num"))
                progid = strtol(p2, NULL, 0);
            av_freep(&to_dealloc);
            av_freep(&key);
        }

        program = av_new_program(oc, progid);

        p = (const char *)o->program[i].u.str;
        while(*p) {
            const char *p2 = av_get_token(&p, ":");
            const char *to_dealloc = p2;
            char *key;
            if (!p2)
                break;
            if(*p) p++;

            key = av_get_token(&p2, "=");
            if (!key) {
                av_log(NULL, AV_LOG_FATAL,
                       "No '=' character in program string %s.\n",
                       p2);
                //exit_program(1);
                return -1;
            }
            if (!*p2){
                //exit_program(1);
                return -1;
            }

            p2++;

            if (!strcmp(key, "title")) {
                av_dict_set(&program->metadata, "title", p2, 0);
            } else if (!strcmp(key, "program_num")) {
            } else if (!strcmp(key, "st")) {
                int st_num = strtol(p2, NULL, 0);
                av_program_add_stream_index(oc, progid, st_num);
            } else {
                av_log(NULL, AV_LOG_FATAL, "Unknown program key %s.\n", key);
                //exit_program(1);
                return -1;
            }
            av_freep(&to_dealloc);
            av_freep(&key);
        }
    }//<== for (i = 0; i < o->nb_program; i++) end ==>

    /*
     * 看上面提到的这两篇文章：
      https://blog.csdn.net/feiyu5323/article/details/118352974以及
      https://www.jianshu.com/p/bd752c86f3e7
      推流没用到，暂不研究
      只要我们不考虑元数据(除了必要的拷贝外)，这部分代码基本可以去掉。
    */
    /* 19. process manually set metadata */
    for (i = 0; i < o->nb_metadata; i++) {
        AVDictionary **m;
        char type, *val;
        const char *stream_spec;
        int index = 0, j, ret = 0;

        val = strchr((const char *)o->metadata[i].u.str, '=');
        if (!val) {
            av_log(NULL, AV_LOG_FATAL, "No '=' character in metadata string %s.\n",
                   o->metadata[i].u.str);
            //exit_program(1);
            return -1;
        }
        *val++ = 0;

        ret = parse_meta_type(o->metadata[i].specifier, &type, &index, &stream_spec);
        if(ret < 0){
            //exit_program(1);
            return -1;
        }
        if (type == 's') {
            for (j = 0; j < oc->nb_streams; j++) {
                ost = output_streams[nb_output_streams - oc->nb_streams + j];
                if ((ret = check_stream_specifier(oc, oc->streams[j], stream_spec)) > 0) {
                    if (!strcmp((const char *)o->metadata[i].u.str, "rotate")) {
                        char *tail;
                        double theta = av_strtod(val, &tail);
                        if (!*tail) {
                            ost->rotate_overridden = 1;
                            ost->rotate_override_value = theta;
                        }
                    } else {
                        av_dict_set(&oc->streams[j]->metadata, (const char *)o->metadata[i].u.str, *val ? val : NULL, 0);
                    }
                } else if (ret < 0){
                    //exit_program(1);
                    return -1;
                }
            }
        }
        else {
            switch (type) {
            case 'g':
                m = &oc->metadata;
                break;
            case 'c':
                if (index < 0 || index >= oc->nb_chapters) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid chapter index %d in metadata specifier.\n", index);
                    //exit_program(1);
                    return -1;
                }
                m = &oc->chapters[index]->metadata;
                break;
            case 'p':
                if (index < 0 || index >= oc->nb_programs) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid program index %d in metadata specifier.\n", index);
                    //exit_program(1);
                    return -1;
                }
                m = &oc->programs[index]->metadata;
                break;
            default:
                av_log(NULL, AV_LOG_FATAL, "Invalid metadata specifier %s.\n", o->metadata[i].specifier);
                //exit_program(1);
                return -1;
            }
            av_dict_set(m, (const char *)o->metadata[i].u.str, *val ? val : NULL, 0);
        }
    }

    return 0;
}

int FFmpegMedia::open_output_file_ex(void* arg, OptionsContext *o, const char *filename){
    if(!arg){
        return -1;
    }

    FFmpegMedia *fm = (FFmpegMedia*)arg;
    return fm->open_output_file(o, filename);
}

/**
 * @brief 检查系统过滤器数组中，输出过滤器保存的输出流是否为空.
 *      可看init_simple_filtergraph函数对FilterGraph **filtergraphs的初始化。
 * @return 成功=void； 失败=程序退出
*/
int FFmpegMedia::check_filter_outputs(void)
{
    int i;
    //1. 遍历系统过滤器数组
    for (i = 0; i < nb_filtergraphs; i++) {
        int n;
        // 2. 遍历系统过滤器数组中的输出过滤器数组
        for (n = 0; n < filtergraphs[i]->nb_outputs; n++) {
            OutputFilter *output = filtergraphs[i]->outputs[n];
            if (!output->ost) {
                av_log(NULL, AV_LOG_FATAL, "Filter %s has an unconnected output\n", output->name);
                //exit_program(1);
                return -1;
            }
        }
    }

    return 0;
}

void FFmpegMedia::uninit_opts(void)
{
    av_dict_free(&swr_opts);
    av_dict_free(&sws_dict);
    av_dict_free(&format_opts);
    av_dict_free(&codec_opts);
    av_dict_free(&resample_opts);
}

void FFmpegMedia::uninit_parse_context(OptionParseContext *octx)
{
    int i, j;

    for (i = 0; i < octx->nb_groups; i++) {
        OptionGroupList *l = &octx->groups[i];

        for (j = 0; j < l->nb_groups; j++) {
            av_freep(&l->groups[j].opts);
            av_dict_free(&l->groups[j].codec_opts);
            av_dict_free(&l->groups[j].format_opts);
            av_dict_free(&l->groups[j].resample_opts);

            av_dict_free(&l->groups[j].sws_dict);
            av_dict_free(&l->groups[j].swr_opts);
        }
        av_freep(&l->groups);
    }
    av_freep(&octx->groups);

    av_freep(&octx->cur_group.opts);
    av_freep(&octx->global_opts.opts);

    uninit_opts();
}

int FFmpegMedia::parse_option(void *optctx, const char *opt, const char *arg,
                 const OptionDef *options)
{
    const OptionDef *po;
    int ret;

    po = find_option(options, opt);
    if (!po->name && opt[0] == 'n' && opt[1] == 'o') {
        /* handle 'no' bool option */
        po = find_option(options, opt + 2);
        if ((po->name && (po->flags & OPT_BOOL)))
            arg = "0";
    } else if (po->flags & OPT_BOOL)
        arg = "1";

    if (!po->name)
        po = find_option(options, "default");
    if (!po->name) {
        av_log(NULL, AV_LOG_ERROR, "Unrecognized option '%s'\n", opt);
        return AVERROR(EINVAL);
    }
    if (po->flags & HAS_ARG && !arg) {
        av_log(NULL, AV_LOG_ERROR, "Missing argument for option '%s'\n", opt);
        return AVERROR(EINVAL);
    }

    ret = write_option(optctx, po, opt, arg);
    if (ret < 0)
        return ret;

    return !!(po->flags & HAS_ARG);
}

int FFmpegMedia::ffmpeg_parse_options(int argc, char **argv){
    OptionParseContext octx;
    uint8_t error[128];
    int ret;

    memset(&octx, 0, sizeof(octx));

    // 1. 分割命令行
    /* split the commandline into an internal representation */
    ret = split_commandline(&octx, argc, argv, options, groups,
                            FF_ARRAY_ELEMS(groups));
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error splitting the argument list: ");
        goto fail;
    }

    // 2. 应用全局选项
    /* apply global options */
    ret = parse_optgroup(NULL, &octx.global_opts);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error parsing global options: ");
        goto fail;
    }

    // 3. 相关信号注册.这里不深入研究.
    /* configure terminal and setup signal handlers */
    //term_init();

    // 4. 打开一个或者多个输入文件.
    /* open input files */
    //ret = open_files(&octx.groups[GROUP_INFILE], "input", open_input_file);
    ret = open_files(&octx.groups[GROUP_INFILE], "input", open_input_file_ex);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error opening input files: ");
        goto fail;
    }

    /* 5. create the complex filtergraphs-创建复杂的过滤图 */
    // 推流模块没用到滤镜，后续再将滤镜相关.
    // 没用到的话，nb_filtergraphs=0，内部不会没有任何处理
    ret = init_complex_filters();
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error initializing complex filters.\n");
        goto fail;
    }

    // 6. 打开一个或者多个输出文件.
    /* open output files */
    ret = open_files(&octx.groups[GROUP_OUTFILE], "output", open_output_file_ex);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error opening output files: ");
        goto fail;
    }

    ret = check_filter_outputs();
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error check_filter_outputs files: ");
        goto fail;
    }

fail:
    uninit_parse_context(&octx);
    if (ret < 0) {
        av_strerror(ret, (char *)error, sizeof(error));// 上面一些错误返回值是-1,传进该函数不准确,忽略即可
        av_log(NULL, AV_LOG_FATAL, "%s\n", error);
    }
    return ret;
}

void FFmpegMedia::fm_set_input_filename(const char* filename){
    _input_filename = filename;
}

void FFmpegMedia::fm_set_output_filename(const char* filename){
    _output_filename = filename;
}
