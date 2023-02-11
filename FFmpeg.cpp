#ifdef __cplusplus

//ffmpeg.c的头文件

//fixme: 修复InitializeSRWLock等api没有声明的问题
#define _WIN32_WINNT 0x0600
//#include <synchapi.h>

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
//#include "libavutil/timestamp.h"
#include "libavutil/bprint.h"
#include "libavutil/time.h"
#include "libavutil/thread.h"
#include "libavutil/threadmessage.h"
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

#if false
extern "C"{
//#endif

//tyycode
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

///* Include only the enabled headers since some compilers (namely, Sun
//   Studio) will not omit unused inline functions and create undefined
//   references to libraries that are not being built. */

#include "config.h"

//tyycode
//#ifdef HAVE_THREADS
//#include "libavutil/thread.h"
//#endif

#include "compat/va_copy.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavdevice/avdevice.h"
#include "libavresample/avresample.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libpostproc/postprocess.h"
#include "libavutil/attributes.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/bprint.h"
#include "libavutil/display.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavutil/libm.h"
#include "libavutil/parseutils.h"
#include "libavutil/pixdesc.h"
#include "libavutil/eval.h"
#include "libavutil/dict.h"
#include "libavutil/fifo.h"
#include "libavutil/opt.h"
#include "libavutil/cpu.h"
#include "libavutil/ffversion.h"
#include "libavutil/version.h"
#include "libavutil/intreadwrite.h"
#include "cmdutils.h"
#if CONFIG_NETWORK
#include "libavformat/network.h"
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include "libavutil/time.h"

//eee

//#include "config.h"
//#include <ctype.h>
//#include <string.h>
//#include <math.h>
//#include <stdlib.h>
//#include <errno.h>
//#include <limits.h>
//#include <stdatomic.h>
//#include <stdint.h>

//#if HAVE_IO_H
//#include <io.h>
//#endif
//#if HAVE_UNISTD_H
//#include <unistd.h>
//#endif

//#include "libavformat/avformat.h"
//#include "libavdevice/avdevice.h"
//#include "libswresample/swresample.h"
//#include "libavutil/opt.h"
//#include "libavutil/channel_layout.h"
//#include "libavutil/parseutils.h"
//#include "libavutil/samplefmt.h"
//#include "libavutil/fifo.h"
//#include "libavutil/hwcontext.h"
//#include "libavutil/internal.h"
//#include "libavutil/intreadwrite.h"
//#include "libavutil/dict.h"
//#include "libavutil/display.h"
//#include "libavutil/mathematics.h"
//#include "libavutil/pixdesc.h"
//#include "libavutil/avstring.h"
//#include "libavutil/libm.h"
//#include "libavutil/imgutils.h"
//#include "libavutil/timestamp.h"
//#include "libavutil/bprint.h"
//#include "libavutil/time.h"
//#include "libavutil/thread.h"
//#include "libavutil/threadmessage.h"
//#include "libavcodec/mathops.h"
//#include "libavformat/os_support.h"

//# include "libavfilter/avfilter.h"
//# include "libavfilter/buffersrc.h"
# include "libavfilter/buffersink.h"

//#if HAVE_SYS_RESOURCE_H
//#include <sys/time.h>
//#include <sys/types.h>
//#include <sys/resource.h>
//#elif HAVE_GETPROCESSTIMES
//#include <windows.h>
//#endif
//#if HAVE_GETPROCESSMEMORYINFO
//#include <windows.h>
//#include <psapi.h>
//#endif
//#if HAVE_SETCONSOLECTRLHANDLER
//#include <windows.h>
//#endif


//#if HAVE_SYS_SELECT_H
//#include <sys/select.h>
//#endif

//#if HAVE_TERMIOS_H
//#include <fcntl.h>
//#include <sys/ioctl.h>
//#include <sys/time.h>
//#include <termios.h>
//#elif HAVE_KBHIT
//#include <conio.h>
//#endif

//#include <time.h>

//#include "ffmpeg.h"
//#include "cmdutils.h"

//#include "libavutil/avassert.h"


#ifdef __cplusplus
}
#endif

#include "ffmpeg.h"

#endif

// ffmpeg.c
const char *const forced_keyframes_const_names[] = {
    "n",
    "n_forced",
    "prev_forced_n",
    "prev_forced_t",
    "t",
    NULL
};


// ffmpeg_opt.c
#define DEFAULT_PASS_LOGFILENAME_PREFIX "ffmpeg2pass"

const HWAccel hwaccels[] = {
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


FFmpegMedia::FFmpegMedia():
    //int_cb({ decode_interrupt_cb, NULL }),
    int_cb({ decode_interrupt_cb, this }),
    program_birth_year(2000)
{
    init_g_options();
    transcode_init_done = 0;
}

FFmpegMedia::~FFmpegMedia(){
//    if(options){
//        delete options;
//        options = NULL;
//    }
}

void FFmpegMedia::init_dynload(){
#ifdef _WIN32
    /* Calling SetDllDirectory with the empty string (but not NULL) removes the
     * current working directory from the DLL search path as a security pre-caution. */
    SetDllDirectory((LPCWSTR)"");
#endif
}

//void FFmpegMedia::ffmpeg_init(){
//    int i, ret;
//    //BenchmarkTimeStamps ti;

//    init_dynload();

//    //register_exit(ffmpeg_cleanup);

//    setvbuf(stderr,NULL,_IONBF,0); /* win32 runtime needs this */

//    av_log_set_flags(AV_LOG_SKIP_REPEATED);
//    //parse_loglevel(argc, argv, options);

////    if(argc>1 && !strcmp(argv[1], "-d")){
////        run_as_daemon=1;
////        av_log_set_callback(log_callback_null);
////        argc--;
////        argv++;
////    }

//#if CONFIG_AVDEVICE
//    avdevice_register_all();
//#endif
//    avformat_network_init();
//}

//enum OptGroup {
//    GROUP_OUTFILE,
//    GROUP_INFILE,
//};

//static const OptionGroupDef groups[] = {
//    [GROUP_OUTFILE] = { "output url",  NULL, OPT_OUTPUT },
//    [GROUP_INFILE]  = { "input url",   "i",  OPT_INPUT },
//};

//int FFmpegMedia::open_input_file(OptionsContext *o, const char *filename)
////int FFmpegMedia::open_input_file(const char *filename)
//{
//    InputFile *f;
//    AVFormatContext *ic;
//    AVInputFormat *file_iformat = NULL;
//    int err, i, ret;
//    int64_t timestamp;
//    AVDictionary *unused_opts = NULL;
//    AVDictionaryEntry *e = NULL;
//    char *   video_codec_name = NULL;
//    char *   audio_codec_name = NULL;
//    char *subtitle_codec_name = NULL;
//    char *    data_codec_name = NULL;
//    int scan_all_pmts_set = 0;

//    // -t and -to cannot be used together; using -t
//    // -t是录制时长.例如:ffmpeg -rtsp_transport tcp -y -re -i rtsp://xxx -vcodec copy -t 00:00:10.00 -f mp4 output.mp4
//    if (o->stop_time != INT64_MAX && o->recording_time != INT64_MAX) {
//        o->stop_time = INT64_MAX;
//        av_log(NULL, AV_LOG_WARNING, "-t and -to cannot be used together; using -t.\n");
//    }

//    // -to value smaller than -ss; aborting.
//    if (o->stop_time != INT64_MAX && o->recording_time == INT64_MAX) {
//        int64_t start_time = o->start_time == AV_NOPTS_VALUE ? 0 : o->start_time;
//        if (o->stop_time <= start_time) {
//            av_log(NULL, AV_LOG_ERROR, "-to value smaller than -ss; aborting.\n");
//            exit_program(1);
//        } else {
//            o->recording_time = o->stop_time - start_time;
//        }
//    }

//    /*在options[]中对应“f”，指定输入文件的格式“-f”*/
//    if (o->format) {
//        if (!(file_iformat = av_find_input_format(o->format))) {
//            av_log(NULL, AV_LOG_FATAL, "Unknown input format: '%s'\n", o->format);
//            exit_program(1);
//        }
//    }

//    // 如果是管道,输入文件名为“-”
//    if (!strcmp(filename, "-"))
//        filename = "pipe:";

//    /*stdin_interaction针对参数-stdin*/
//    // 运算符优先级,记忆口决："单算移关与，异或逻条赋", 具体看https://blog.csdn.net/sgbl888/article/details/123997358
//    // 不过下面语句等价于：stdin_interaction = stdin_interaction & (strncmp(filename, "pipe:", 5) && strcmp(filename, "/dev/stdin"));
//    // 意思是：如果filename不是pipe:开头，那么再判断filename是否是/dev/stdin.
//    // 故下面意思是：除了是pipe或者是/dev/stdin不是交互模式，其它都是。故文件、实时流stdin_interaction都为1
//    //auto tyy1 = strncmp(filename, "pipe:", 5) && strcmp(filename, "/dev/stdin");
//    //stdin_interaction = stdin_interaction & (strncmp(filename, "pipe:", 5) && strcmp(filename, "/dev/stdin"));// 等价于下面语句
//    stdin_interaction &= strncmp(filename, "pipe:", 5) &&
//                         strcmp(filename, "/dev/stdin");

//    // 1. 为输入文件开辟相关内存
//    /* get default parameters from command line */
//    ic = avformat_alloc_context();
//    if (!ic) {
//        print_error(filename, AVERROR(ENOMEM));
//        exit_program(1);
//    }

//    // 1.1 解复用相关参数设置(不过平时输入文件很少设置过多的参数,输出文件才会)
//    /*对应参数“-ar”设置音频采样率*/
//    if (o->nb_audio_sample_rate) {
//        av_dict_set_int(&o->g->format_opts, "sample_rate", o->audio_sample_rate[o->nb_audio_sample_rate - 1].u.i, 0);
//    }

//    /*对应参数"-ac"*/
//    if (o->nb_audio_channels) {
//        /* because we set audio_channels based on both the "ac" and
//         * "channel_layout" options, we need to check that the specified
//         * demuxer actually has the "channels" option before setting it */
//        if (file_iformat && file_iformat->priv_class &&
//            av_opt_find(&file_iformat->priv_class, "channels", NULL, 0,
//                        AV_OPT_SEARCH_FAKE_OBJ)) {
//            av_dict_set_int(&o->g->format_opts, "channels", o->audio_channels[o->nb_audio_channels - 1].u.i, 0);
//        }
//    }

//    /*对应参数"r"帧率，注与re是不一样的，re对应成员是rate_emu*/
//    if (o->nb_frame_rates) {
//        /* set the format-level framerate option;
//         * this is important for video grabbers, e.g. x11 */
//        if (file_iformat && file_iformat->priv_class &&
//            av_opt_find(&file_iformat->priv_class, "framerate", NULL, 0,
//                        AV_OPT_SEARCH_FAKE_OBJ)) {
//            av_dict_set(&o->g->format_opts, "framerate",
//                        o->frame_rates[o->nb_frame_rates - 1].u.str, 0);
//        }
//    }

//    //对应参数"s"
//    if (o->nb_frame_sizes) {
//        av_dict_set(&o->g->format_opts, "video_size", o->frame_sizes[o->nb_frame_sizes - 1].u.str, 0);
//    }

//    //对应参数"pix_fmt"
//    if (o->nb_frame_pix_fmts)
//        av_dict_set(&o->g->format_opts, "pixel_format", o->frame_pix_fmts[o->nb_frame_pix_fmts - 1].u.str, 0);

//    //对应参数"c"和"codec" 或"c:[v/a/s/d]"和"codec:[v/a/s/d]"
//    //最终通过参数3传出.假设我们传-codec libx264, 那么video_codec_name="libx264"
//    //因为传入时o->name[i].u.str会指向"libx264"
//    MATCH_PER_TYPE_OPT(codec_names, str,    video_codec_name, ic, "v");
//    MATCH_PER_TYPE_OPT(codec_names, str,    audio_codec_name, ic, "a");
//    MATCH_PER_TYPE_OPT(codec_names, str, subtitle_codec_name, ic, "s");
//    MATCH_PER_TYPE_OPT(codec_names, str,     data_codec_name, ic, "d");

//    /*
//     下面第2，第3步是对ic的video_codec,audio_codec,subtitle_codec,data_codec以及
//     video_codec_id,audio_codec_id,subtitle_codec_id,data_codec赋值
//    */
//    // 2. 根据用户输入指定的编解码器格式，寻找对应的编解码器
//    // 找到则赋值给解复用结构体ic，找不到会在find_codec_or_die直接退出
//    if (video_codec_name)
//        ic->video_codec    = find_codec_or_die(video_codec_name   , AVMEDIA_TYPE_VIDEO   , 0);
//    if (audio_codec_name)
//        ic->audio_codec    = find_codec_or_die(audio_codec_name   , AVMEDIA_TYPE_AUDIO   , 0);
//    if (subtitle_codec_name)
//        ic->subtitle_codec = find_codec_or_die(subtitle_codec_name, AVMEDIA_TYPE_SUBTITLE, 0);
//    if (data_codec_name)
//        ic->data_codec     = find_codec_or_die(data_codec_name    , AVMEDIA_TYPE_DATA    , 0);

//    // 来到这说明成功找到编解码器(因为失败会在find_codec_or_die直接退出出现)
//    // 3. 若指定输入文件的编解码器，则将编解码器中的id赋值给解复用的id; 若无指定，解码器的id为xxx_ID_NONE
//    ic->video_codec_id     = video_codec_name    ? ic->video_codec->id    : AV_CODEC_ID_NONE;
//    ic->audio_codec_id     = audio_codec_name    ? ic->audio_codec->id    : AV_CODEC_ID_NONE;
//    ic->subtitle_codec_id  = subtitle_codec_name ? ic->subtitle_codec->id : AV_CODEC_ID_NONE;
//    ic->data_codec_id      = data_codec_name     ? ic->data_codec->id     : AV_CODEC_ID_NONE;

//    /*ffmpeg的avformat_open_input()和av_read_frame()默认是阻塞的.
//     * 1)用户可以通过设置"ic->flags |= AVFMT_FLAG_NONBLOCK;"设置成非阻塞(通常是不推荐的)；
//     * 2)或者是设置超时时间；
//     * 3)或者是设置interrupt_callback定义返回机制。
//    */
//    ic->flags |= AVFMT_FLAG_NONBLOCK;
//    if (o->bitexact)
//        ic->flags |= AVFMT_FLAG_BITEXACT;
//    ic->interrupt_callback = int_cb;// 设置中断函数

//    // 4. avformat_open_input打开输入文件，打开时可以设置解复用的相关参数
//    // scan_all_pmts是mpegts的一个选项，表示扫描全部的ts流的"Program Map Table"表。这里在没有设定该选项的时候，强制设为1。
//    if (!av_dict_get(o->g->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
//        av_dict_set(&o->g->format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
//        scan_all_pmts_set = 1;
//    }
//    /* open the input file with generic avformat function */
//    err = avformat_open_input(&ic, filename, file_iformat, &o->g->format_opts);// 这里看到，解复用的参数最终使用该函数去设置的
//    if (err < 0) {
//        print_error(filename, err);
//        if (err == AVERROR_PROTOCOL_NOT_FOUND)
//            av_log(NULL, AV_LOG_ERROR, "Did you mean file:%s?\n", filename);
//        exit_program(1);
//    }
//    // 打开输入文件后，将该选项置空
//    if (scan_all_pmts_set)
//        av_dict_set(&o->g->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
//    remove_avoptions(&o->g->format_opts, o->g->codec_opts);
//    // 上面我们会将有效的条目置空，所以此时还有条目，代表该条目是不合法的。
//    assert_avoptions(o->g->format_opts);

//    // 5. 若用户设置解码器，会查找解码器，并把找到解码器的id赋值到streams中
//    /* apply forced codec ids */
//    for (i = 0; i < ic->nb_streams; i++)
//        choose_decoder(o, ic, ic->streams[i]);

//    // 6. 是否查找流信息.
//    if (find_stream_info) {
//        /*这里会为输入流开辟每一个编解码器选项，以返回值进行返回*/
//        AVDictionary **opts = setup_find_stream_info_opts(ic, o->g->codec_opts);
//        int orig_nb_streams = ic->nb_streams;

//        /*avformat_find_stream_info这里会用到解码器选项o->g->codec_opts，因为opts就是从
//        o->g->codec_opts过滤得到的*/
//        /* If not enough info to get the stream parameters, we decode the
//           first frames to get it. (used in mpeg case for example) */
//        ret = avformat_find_stream_info(ic, opts);

//        // 释放setup_find_stream_info_opts内部开辟的内存
//        for (i = 0; i < orig_nb_streams; i++)
//            av_dict_free(&opts[i]);
//        av_freep(&opts);

//        if (ret < 0) {
//            av_log(NULL, AV_LOG_FATAL, "%s: could not find codec parameters\n", filename);
//            if (ic->nb_streams == 0) {
//                avformat_close_input(&ic);
//                exit_program(1);
//            }
//        }
//    }

//    //对应参数"sseof",设置相对于结束的开始时间
//    if (o->start_time != AV_NOPTS_VALUE && o->start_time_eof != AV_NOPTS_VALUE) {
//        av_log(NULL, AV_LOG_WARNING, "Cannot use -ss and -sseof both, using -ss for %s\n", filename);
//        o->start_time_eof = AV_NOPTS_VALUE;
//    }

//    if (o->start_time_eof != AV_NOPTS_VALUE) {
//        if (o->start_time_eof >= 0) {
//            av_log(NULL, AV_LOG_ERROR, "-sseof value must be negative; aborting\n");
//            exit_program(1);
//        }
//        if (ic->duration > 0) {
//            o->start_time = o->start_time_eof + ic->duration;
//            if (o->start_time < 0) {
//                av_log(NULL, AV_LOG_WARNING, "-sseof value seeks to before start of file %s; ignored\n", filename);
//                o->start_time = AV_NOPTS_VALUE;
//            }
//        } else
//            av_log(NULL, AV_LOG_WARNING, "Cannot use -sseof, duration of %s not known\n", filename);
//    }
//    timestamp = (o->start_time == AV_NOPTS_VALUE) ? 0 : o->start_time;// -ss选项
//    /* add the stream start time */
//    if (!o->seek_timestamp && ic->start_time != AV_NOPTS_VALUE)
//        timestamp += ic->start_time;

//    // 如果开始时间指定的话, 需要做seek
//    /* if seeking requested, we execute it */
//    if (o->start_time != AV_NOPTS_VALUE) {
//        int64_t seek_timestamp = timestamp;

//        if (!(ic->iformat->flags & AVFMT_SEEK_TO_PTS)) {
//            int dts_heuristic = 0;
//            for (i=0; i<ic->nb_streams; i++) {
//                const AVCodecParameters *par = ic->streams[i]->codecpar;
//                if (par->video_delay) {
//                    dts_heuristic = 1;
//                    break;
//                }
//            }
//            if (dts_heuristic) {
//                //笔者猜测这里意思是：若ffmpeg存在视频帧延迟(估计是ffmpeg内部缓冲？还是解码造成dts、pts的差距？)，
//                //那么seek时，应当稍微减少一点seek_timestamp. 3/23大概是3帧的时长，*AV_TIME_BASE是单位转成秒.
//                seek_timestamp -= 3*AV_TIME_BASE / 23;
//            }
//        }
//        ret = avformat_seek_file(ic, -1, INT64_MIN, seek_timestamp, seek_timestamp, 0);
//        if (ret < 0) {
//            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
//                   filename, (double)timestamp / AV_TIME_BASE);
//        }
//    }

//    /* update the current parameters so that they match the one of the input stream */
//    // 7. 将输入文件的每个流封装到InputStream中。InputStream->st是直接指向输入文件的AVStream的，不会再开辟内存.
//    // 执行完该函数后，输入文件中各个流就会被保存到input_streams**这个二维数组
//    // (ic->streams[]与input_streams都保存这输入文件的各个输入流)
//    add_input_streams(o, ic);

//    /* dump the file content */
//    // 参4：0-代表dump输入文件，1-代表dump输出文件。可以看源码.
//    av_dump_format(ic, nb_input_files, filename, 0);

//    /*8.对InputFile内剩余成员的相关初始化工作*/
//    //int tyysize = sizeof (*input_files);//8
//    GROW_ARRAY(input_files, nb_input_files);// 开辟sizeof(InputFile*)大小的内存，且输入文件个数+1
//    f = av_mallocz(sizeof(*f));// 真正开辟InputFile结构体的内存
//    if (!f)
//        exit_program(1);
//    input_files[nb_input_files - 1] = f;

//    // 下面就是给InputFile结构体赋值
//    f->ctx        = ic;
//    f->ist_index  = nb_input_streams - ic->nb_streams;/*输入文件的流的第一个流下标.基本是0，因为在add_input_streams中看到，
//                                                        nb_input_streams的个数由ic->nb_streams决定*/
//    f->start_time = o->start_time;
//    f->recording_time = o->recording_time;
//    f->input_ts_offset = o->input_ts_offset;
//    f->ts_offset  = o->input_ts_offset - (copy_ts ? (start_at_zero && ic->start_time != AV_NOPTS_VALUE ? ic->start_time : 0) : timestamp);
//    f->nb_streams = ic->nb_streams;
//    f->rate_emu   = o->rate_emu;// -re
//    f->accurate_seek = o->accurate_seek;
//    f->loop = o->loop;
//    f->duration = 0;
//    f->time_base = (AVRational){ 1, 1 };
//#if HAVE_THREADS
//    f->thread_queue_size = o->thread_queue_size > 0 ? o->thread_queue_size : 8;
//#endif

//    /*9.检测所有编解码器选项是否已经被使用(看open_output_file)*/
//    /* check if all codec options have been used */
//    // 因输入文件一般很少设置编解码器选项， 这里可以后续读者自行研究，看懂意思就行，不难
//    unused_opts = strip_specifiers(o->g->codec_opts);// 将用户输入的参数去掉流分隔符
//    /*这里意思是：因为在add_input_streams时，内部已经将用户输入的参数过滤后，匹配给对应的流(一般情
//     * 况各个输入流中都保存一份o->g->codec_opts副本)，所以这里根据各个输入流保存的选项，去清除用户实际
//     * 输入的选项unused_opts，当发现unused_opts还有选项时，那么该选项就是未使用的。
//    */
//    for (i = f->ist_index; i < nb_input_streams; i++) {
//        e = NULL;
//        while ((e = av_dict_get(input_streams[i]->decoder_opts, "", e,
//                                AV_DICT_IGNORE_SUFFIX)))
//            av_dict_set(&unused_opts, e->key, NULL, 0);
//    }

//    /*这个while就是对多余的选项进行一些警告处理*/
//    e = NULL;
//    while ((e = av_dict_get(unused_opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
//        const AVClass *class = avcodec_get_class();// 获取编解码器的AVClass
//        const AVOption *option = av_opt_find(&class, e->key, NULL, 0,
//                                             AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
//        const AVClass *fclass = avformat_get_class();// 获取解复用器的AVClass
//        const AVOption *foption = av_opt_find(&fclass, e->key, NULL, 0,
//                                             AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
//        if (!option || foption)
//            continue;

//        // 若选项option即e->key不是解码器选项，程序退出
//        if (!(option->flags & AV_OPT_FLAG_DECODING_PARAM)) {
//            av_log(NULL, AV_LOG_ERROR, "Codec AVOption %s (%s) specified for "
//                   "input file #%d (%s) is not a decoding option.\n", e->key,
//                   option->help ? option->help : "", nb_input_files - 1,
//                   filename);
//            exit_program(1);
//        }

//        av_log(NULL, AV_LOG_WARNING, "Codec AVOption %s (%s) specified for "
//               "input file #%d (%s) has not been used for any stream. The most "
//               "likely reason is either wrong type (e.g. a video option with "
//               "no video streams) or that it is a private option of some decoder "
//               "which was not actually used for any stream.\n", e->key,
//               option->help ? option->help : "", nb_input_files - 1, filename);
//    }
//    av_dict_free(&unused_opts);

//    //对应参数"dump_attachment"
//    for (i = 0; i < o->nb_dump_attachment; i++) {
//        int j;

//        for (j = 0; j < ic->nb_streams; j++) {
//            AVStream *st = ic->streams[j];

//            if (check_stream_specifier(ic, st, o->dump_attachment[i].specifier) == 1)
//                dump_attachment(st, o->dump_attachment[i].u.str);
//        }
//    }

//    /*10.input_stream_potentially_available 置为1，记录该输入文件可能是能用的*/
//    // 打开后，记录该输入文件可能是能用的
//    input_stream_potentially_available = 1;

//    return 0;
//}


////int FFmpegMedia::open_files(OptionGroupList *l, const char *inout,
////                      int (*open_file)(const char*))
//int FFmpegMedia::open_files(OptionGroupList *l, const char *inout,
//                            int (*open_file)(OptionsContext*, const char*))
//{
//    int i, ret;

//    for (i = 0; i < l->nb_groups; i++) {
//        OptionGroup *g = &l->groups[i];
//        OptionsContext o;

////        init_options(&o);
////        o.g = g;

////        ret = parse_optgroup(&o, g);
////        if (ret < 0) {
////            av_log(NULL, AV_LOG_ERROR, "Error parsing options for %s file "
////                   "%s.\n", inout, g->arg);
////            uninit_options(&o);
////            return ret;
////        }

//        av_log(NULL, AV_LOG_DEBUG, "Opening an %s file: %s.\n", inout, g->arg);
//        ret = open_file(&o, g->arg);
////        uninit_options(&o);
////        if (ret < 0) {
////            av_log(NULL, AV_LOG_ERROR, "Error opening %s file %s.\n",
////                   inout, g->arg);
////            return ret;
////        }
//        av_log(NULL, AV_LOG_DEBUG, "Successfully opened the file.\n");
//    }

//    return 0;
//}

////int FFmpegMedia::split_commandline(OptionParseContext *octx, int argc, char *argv[],
////                      const OptionDef *options,
////                      const OptionGroupDef *groups, int nb_groups)
////{
////    int optindex = 1;
////    int dashdash = -2;

////    /* perform system-dependent conversions for arguments list */
////    prepare_app_arguments(&argc, &argv);

////    init_parse_context(octx, groups, nb_groups);
////    av_log(NULL, AV_LOG_DEBUG, "Splitting the commandline.\n");

////    while (optindex < argc) {
////        const char *opt = argv[optindex++], *arg;
////        const OptionDef *po;
////        int ret;

////        av_log(NULL, AV_LOG_DEBUG, "Reading option '%s' ...", opt);

////        if (opt[0] == '-' && opt[1] == '-' && !opt[2]) {
////            dashdash = optindex;
////            continue;
////        }
////        /* unnamed group separators, e.g. output filename */
////        if (opt[0] != '-' || !opt[1] || dashdash+1 == optindex) {
////            finish_group(octx, 0, opt);
////            av_log(NULL, AV_LOG_DEBUG, " matched as %s.\n", groups[0].name);
////            continue;
////        }
////        opt++;

////#define GET_ARG(arg)                                                           \
////do {                                                                           \
////    arg = argv[optindex++];                                                    \
////    if (!arg) {                                                                \
////        av_log(NULL, AV_LOG_ERROR, "Missing argument for option '%s'.\n", opt);\
////        return AVERROR(EINVAL);                                                \
////    }                                                                          \
////} while (0)

////        /* named group separators, e.g. -i */
////        if ((ret = match_group_separator(groups, nb_groups, opt)) >= 0) {
////            GET_ARG(arg);
////            finish_group(octx, ret, arg);
////            av_log(NULL, AV_LOG_DEBUG, " matched as %s with argument '%s'.\n",
////                   groups[ret].name, arg);
////            continue;
////        }

////        /* normal options */
////        po = find_option(options, opt);
////        if (po->name) {
////            if (po->flags & OPT_EXIT) {
////                /* optional argument, e.g. -h */
////                arg = argv[optindex++];
////            } else if (po->flags & HAS_ARG) {
////                GET_ARG(arg);
////            } else {
////                arg = "1";
////            }

////            add_opt(octx, po, opt, arg);
////            av_log(NULL, AV_LOG_DEBUG, " matched as option '%s' (%s) with "
////                   "argument '%s'.\n", po->name, po->help, arg);
////            continue;
////        }

////        /* AVOptions */
////        if (argv[optindex]) {
////            ret = opt_default(NULL, opt, argv[optindex]);
////            if (ret >= 0) {
////                av_log(NULL, AV_LOG_DEBUG, " matched as AVOption '%s' with "
////                       "argument '%s'.\n", opt, argv[optindex]);
////                optindex++;
////                continue;
////            } else if (ret != AVERROR_OPTION_NOT_FOUND) {
////                av_log(NULL, AV_LOG_ERROR, "Error parsing option '%s' "
////                       "with argument '%s'.\n", opt, argv[optindex]);
////                return ret;
////            }
////        }

////        /* boolean -nofoo options */
////        if (opt[0] == 'n' && opt[1] == 'o' &&
////            (po = find_option(options, opt + 2)) &&
////            po->name && po->flags & OPT_BOOL) {
////            add_opt(octx, po, opt, "0");
////            av_log(NULL, AV_LOG_DEBUG, " matched as option '%s' (%s) with "
////                   "argument 0.\n", po->name, po->help);
////            continue;
////        }

////        av_log(NULL, AV_LOG_ERROR, "Unrecognized option '%s'.\n", opt);
////        return AVERROR_OPTION_NOT_FOUND;
////    }

////    if (octx->cur_group.nb_opts || codec_opts || format_opts || resample_opts)
////        av_log(NULL, AV_LOG_WARNING, "Trailing options were found on the "
////               "commandline.\n");

////    av_log(NULL, AV_LOG_DEBUG, "Finished splitting the commandline.\n");

////    return 0;
////}

//int FFmpegMedia::ffmpeg_parse_options(int argc, char **argv){
//    OptionParseContext octx;
//    uint8_t error[128];
//    int ret;

//    memset(&octx, 0, sizeof(octx));

////    /* split the commandline into an internal representation */
////    ret = split_commandline(&octx, argc, argv, options, groups,
////                            FF_ARRAY_ELEMS(groups));
////    if (ret < 0) {
////        av_log(NULL, AV_LOG_FATAL, "Error splitting the argument list: ");
////        goto fail;
////    }

////    /* apply global options */
////    ret = parse_optgroup(NULL, &octx.global_opts);
////    if (ret < 0) {
////        av_log(NULL, AV_LOG_FATAL, "Error parsing global options: ");
////        goto fail;
////    }

////    /* configure terminal and setup signal handlers */
////    term_init();

//    /* open input files */
//    ret = open_files(&octx.groups[GROUP_INFILE], "input", open_input_file);
//    if (ret < 0) {
//        av_log(NULL, AV_LOG_FATAL, "Error opening input files: ");
//        goto fail;
//    }

//    /* create the complex filtergraphs */
////    ret = init_complex_filters();
////    if (ret < 0) {
////        av_log(NULL, AV_LOG_FATAL, "Error initializing complex filters.\n");
////        goto fail;
////    }

////    /* open output files */
////    ret = open_files(&octx.groups[GROUP_OUTFILE], "output", open_output_file);
////    if (ret < 0) {
////        av_log(NULL, AV_LOG_FATAL, "Error opening output files: ");
////        goto fail;
////    }

////    check_filter_outputs();

//fail:
//    uninit_parse_context(&octx);
//    if (ret < 0) {
//        //av_strerror(ret, error, sizeof(error));
//        av_log(NULL, AV_LOG_FATAL, "%s\n", error);
//    }
//    return ret;
//}

//int FFmpegMedia::ffmpeg_transcode(){
////    int i, ret;
//    //BenchmarkTimeStamps ti;

////    init_dynload();

//    //register_exit(ffmpeg_cleanup);

////    setvbuf(stderr,NULL,_IONBF,0); /* win32 runtime needs this */

////    av_log_set_flags(AV_LOG_SKIP_REPEATED);
//    //parse_loglevel(argc, argv, options);

////    if(argc>1 && !strcmp(argv[1], "-d")){
////        run_as_daemon=1;
////        av_log_set_callback(log_callback_null);
////        argc--;
////        argv++;
////    }

////#if CONFIG_AVDEVICE
////    avdevice_register_all();
////#endif
////    avformat_network_init();

//    //show_banner(argc, argv, options);

//    /* parse options and open all input/output files */
////    ret = ffmpeg_parse_options(argc, argv);
////    if (ret < 0)
////        exit_program(1);

////    if (nb_output_files <= 0 && nb_input_files == 0) {
////        show_usage();
////        av_log(NULL, AV_LOG_WARNING, "Use -h to get full help or, even better, run 'man %s'\n", program_name);
////        exit_program(1);
////    }

//    /* file converter / grab */
////    if (nb_output_files <= 0) {
////        av_log(NULL, AV_LOG_FATAL, "At least one output file must be specified\n");
////        exit_program(1);
////    }

////    for (i = 0; i < nb_output_files; i++) {
////        if (strcmp(output_files[i]->ctx->oformat->name, "rtp"))
////            _want_sdp = 0;
////    }

//    //current_time = ti = get_benchmark_time_stamps();
//    //if (transcode() < 0)
//        //exit_program(1);

////    if (do_benchmark) {
////        int64_t utime, stime, rtime;
////        current_time = get_benchmark_time_stamps();
////        utime = current_time.user_usec - ti.user_usec;
////        stime = current_time.sys_usec  - ti.sys_usec;
////        rtime = current_time.real_usec - ti.real_usec;
////        av_log(NULL, AV_LOG_INFO,
////               "bench: utime=%0.3fs stime=%0.3fs rtime=%0.3fs\n",
////               utime / 1000000.0, stime / 1000000.0, rtime / 1000000.0);
////    }

////    av_log(NULL, AV_LOG_DEBUG, "%"PRIu64" frames successfully decoded, %"PRIu64" decoding errors\n",
////           decode_error_stat[0], decode_error_stat[1]);
////    if ((decode_error_stat[0] + decode_error_stat[1]) * max_error_rate < decode_error_stat[1])
////        exit_program(69);

////    exit_program(received_nb_signals ? 255 : main_return_code);
////    return main_return_code;
//    return 0;
//}


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

void FFmpegMedia::exit_program(int ret)
{
    // 1. 回收相关内容
//        if (program_exit)
//            program_exit(ret);

    // 2. 退出进程
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
//            snprintf(error, error_len, "Device setup failed for "
//                     "decoder on input stream #%d:%d : %s",
//                     ist->file_index, ist->st->index, av_err2str(ret));
            return ret;
        }

        /* 1.4编解码器上下文与编解码器关联.
         * 这里看到，编解码器的相关选项是在这里被应用的 */
        if ((ret = avcodec_open2(ist->dec_ctx, codec, &ist->decoder_opts)) < 0) {
            if (ret == AVERROR_EXPERIMENTAL)
                abort_codec_experimental(codec, 0);

//            snprintf(error, error_len,
//                     "Error while opening decoder for input stream "
//                     "#%d:%d : %s",
//                     ist->file_index, ist->st->index, av_err2str(ret));
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
void FFmpegMedia::set_encoder_id(OutputFile *of, OutputStream *ost)
{
    AVDictionaryEntry *e;

    uint8_t *encoder_string;
    int encoder_string_len;
    int format_flags = 0;
    int codec_flags = ost->enc_ctx->flags;

    /* 1.若ost->st->metadata已经存在encoder选项，则返回,不做处理.
     * 因为在open_output_file()时会将每一个流的元数据的encoder置为NULL,所以不会直接返回. */
    if (av_dict_get(ost->st->metadata, "encoder",  NULL, 0))
        return;


    {
        //tyycode
        /* 测试av_opt_eval_flags():可以看到，从of-ctx找到fflags后，就是libavformat/options_table.h文件里的变量
         * static const AVOption avformat_options[]的fflags选项. */
        const AVOption *o = av_opt_find(of->ctx, "fflags", NULL, 0, 0);
        if (!o)
            return;
        /* 假设解复用选项of->opts的fflags的value值被设为"32"，那么通过av_opt_eval_flags()后，将里面的数字会设置到format_flags中，
         * 所以看到，format_flags的值 从0变成32.注，因为fflags是int64(具体类型需要看源码)，
         * 所以必须是该类型是字符串，例下面"32"写成"32.2"是无法读到数字的 */
        av_opt_eval_flags(of->ctx, o, "32", &format_flags);
    }

    /* 2.若用户设置了解复用的fflags选项，则进行获取，然后保存在临时变量format_flags中 */
    /* 关于AVOption模块，可参考: https://blog.csdn.net/ericbar/article/details/79872779 */
    e = av_dict_get(of->opts, "fflags", NULL, 0);
    if (e) {
        // 若解复用不支持fflags选项，则返回
        const AVOption *o = av_opt_find(of->ctx, "fflags", NULL, 0, 0);
        if (!o)
            return;
        /* opt_eval_funcs: 计算选项字符串。这组函数可用于计算选项字符串并从中获取数字。它们所做的事情与av_opt_set()相同，只是结果被写入调用方提供的指针 */
        av_opt_eval_flags(of->ctx, o, e->value, &format_flags);
    }

    /* 3.同理，若用户设置了编码器的flags选项，则进行获取，然后保存在临时变量codec_flags中 */
    e = av_dict_get(ost->encoder_opts, "flags", NULL, 0);
    if (e) {
        const AVOption *o = av_opt_find(ost->enc_ctx, "flags", NULL, 0, 0);
        if (!o)
            return;
        av_opt_eval_flags(ost->enc_ctx, o, e->value, &codec_flags);
    }

    /* 4.获取编码器名字encoder选项的长度.LIBAVCODEC_IDENT宏里面的 ## 是拼接前后两个宏参数的意思. */
    encoder_string_len = sizeof(LIBAVCODEC_IDENT) + strlen(ost->enc->name) + 2;
    encoder_string     = (uint8_t *)av_mallocz(encoder_string_len);
    if (!encoder_string)
        exit_program(1);

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
void FFmpegMedia::parse_forced_key_frames(char *kf, OutputStream *ost,
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
        exit_program(1);
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
                exit_program(1);
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
    set_encoder_id(output_files[ost->file_index], ost);

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
                parse_forced_key_frames(ost->forced_keyframes, ost, ost->enc_ctx);
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
        abort();
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
            exit_program(1);
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
//        av_log(NULL, AV_LOG_ERROR,
//               "Could not write header for output file #%d "
//               "(incorrect codec parameters ?): %s\n",
//               file_index, av_err2str(ret));
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
            //1111write_packet(of, &pkt, ost, 1);
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
//                snprintf(error, error_len, "Device setup failed for "
//                         "encoder on output stream #%d:%d : %s",
//                     ost->file_index, ost->index, av_err2str(ret));
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
            if (ret == AVERROR_EXPERIMENTAL)
                abort_codec_experimental(codec, 1);
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
            exit_program(1);
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
            if (ret != AVERROR_EOF)
//                av_log(f->ctx, AV_LOG_ERROR,
//                       "Unable to send packet to main thread: %s\n",
//                       av_err2str(ret));

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
    //pthread_join(f->thread, NULL);
    pthread_join(*f->thread, NULL);
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
    //if ((ret = pthread_create(&f->thread, NULL, input_thread, f))) {
    if ((ret = pthread_create((pthread_t*)f->thread, NULL, input_thread, f))) {
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
//    while (!received_sigterm) {
//        int64_t cur_time= av_gettime_relative();

//        /* if 'q' pressed, exits */
//        if (stdin_interaction)
//            if (check_keyboard_interaction(cur_time) < 0)//键盘事件相关处理，这里不详细研究
//                break;

//        /* check if there's any stream where output is still needed */
//        if (!need_output()) {
//            av_log(NULL, AV_LOG_VERBOSE, "No more output streams to write to, finishing.\n");
//            break;// 正常这里退出while
//        }

//        ret = transcode_step();
//        if (ret < 0 && ret != AVERROR_EOF) {
//            av_log(NULL, AV_LOG_ERROR, "Error while filtering: %s\n", av_err2str(ret));
//            break;
//        }

//        /* dump report by using the output first video and audio streams */
//        print_report(0, timer_start, cur_time);
//    }//<== while (!received_sigterm) end ==>

//    // 4. 多输入文件时.回收所有输入文件线程.
//#if HAVE_THREADS
//    free_input_threads();
//#endif

//    // 5. 确认所有输入流的解码器已经使用空包刷新缓冲区.
//    /* 一般在process_input()里面完成刷空包,除非发生写帧错误.
//     * 1)例如在eof时,debug久一点导致写帧失败,会在这里刷NULL.
//     * 2)debug时不想在这里刷空包,我们去掉与reap_filters()有关的断点,并让其快速运行,以便能快速写帧,这样就不会写帧错误,
//     *      这样是为了能保持正常的逻辑去查看运行流程 */
//    /* at the end of stream, we must flush the decoder buffers(在流结束时,必须刷新解码器缓冲区) */
//    for (i = 0; i < nb_input_streams; i++) {
//        ist = input_streams[i];
//        // 文件没遇到eof.一般在process_input()被置1
//        if (!input_files[ist->file_index]->eof_reached) {
//            process_input_packet(ist, NULL, 0);
//        }
//    }

//    // 6. 清空编码器.一般是这里清空编码器,然后写剩余的帧的.
//    flush_encoders();

//    term_exit();

//    /* write the trailer if needed and close file */
//    // 7. 为每个输出文件调用av_write_trailer().
//    for (i = 0; i < nb_output_files; i++) {
//        os = output_files[i]->ctx;
//        if (!output_files[i]->header_written) {
//            av_log(NULL, AV_LOG_ERROR,
//                   "Nothing was written into output file %d (%s), because "
//                   "at least one of its streams received no packets.\n",
//                   i, os->url);
//            continue;// 没写头的文件不写尾
//        }
//        if ((ret = av_write_trailer(os)) < 0) {
//            av_log(NULL, AV_LOG_ERROR, "Error writing trailer of %s: %s\n", os->url, av_err2str(ret));
//            if (exit_on_error)
//                exit_program(1);
//        }
//    }

//    /* dump report by using the first video and audio streams */
//    print_report(1, timer_start, av_gettime_relative());

//    // 8. 关闭编码器
//    /* close each encoder */
//    for (i = 0; i < nb_output_streams; i++) {
//        ost = output_streams[i];
//        if (ost->encoding_needed) {
//            av_freep(&ost->enc_ctx->stats_in);// 编码器这样关闭就可以了吗?
//        }
//        total_packets_written += ost->packets_written;//统计所以输出流已经写帧的包数.
//    }

//    // 写入的包数为0 且 abort_on_flags 与上 1 的结果不为0, 程序退出.
//    if (!total_packets_written && (abort_on_flags & ABORT_ON_FLAG_EMPTY_OUTPUT)) {
//        av_log(NULL, AV_LOG_FATAL, "Empty output\n");
//        exit_program(1);
//    }

//    // 9. 关闭解码器
//    /* close each decoder */
//    for (i = 0; i < nb_input_streams; i++) {
//        ist = input_streams[i];
//        if (ist->decoding_needed) {
//            avcodec_close(ist->dec_ctx);// 关闭解码器
//            if (ist->hwaccel_uninit)
//                ist->hwaccel_uninit(ist->dec_ctx);
//        }
//    }

//    // 10. 释放硬件相关
//    av_buffer_unref(&hw_device_ctx);
//    hw_device_free_all();

    /* finished ! */
    ret = 0;

 fail:
//#if HAVE_THREADS
//    free_input_threads();
//#endif

    if (output_streams) {
        for (i = 0; i < nb_output_streams; i++) {
            ost = output_streams[i];
            if (ost) {
                if (ost->logfile) {
                    if (fclose(ost->logfile))
//                        av_log(NULL, AV_LOG_ERROR,
//                               "Error closing logfile, loss of information possible: %s\n",
//                               av_err2str(AVERROR(errno)));
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
//        av_log_set_callback(log_callback_null);
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
    if (ret < 0)
        exit_program(1);

    if (nb_output_files <= 0 && nb_input_files == 0) {
        //show_usage();
        av_log(NULL, AV_LOG_WARNING, "Use -h to get full help or, even better, run 'man %s'\n", program_name);
        exit_program(1);
    }

    /* file converter / grab */
    if (nb_output_files <= 0) {
        av_log(NULL, AV_LOG_FATAL, "At least one output file must be specified\n");
        exit_program(1);
    }

    for (i = 0; i < nb_output_files; i++) {
        if (strcmp(output_files[i]->ctx->oformat->name, "rtp"))
            want_sdp = 0;
    }

    current_time = ti = get_benchmark_time_stamps();
    if (transcode() < 0)
        exit_program(1);
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
//    av_log(NULL, AV_LOG_DEBUG, "%"PRIu64" frames successfully decoded, %"PRIu64" decoding errors\n",
//           decode_error_stat[0], decode_error_stat[1]);
    if ((decode_error_stat[0] + decode_error_stat[1]) * max_error_rate < decode_error_stat[1])
        exit_program(69);

    exit_program(received_nb_signals ? 255 : main_return_code);
    return main_return_code;
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
 * @param nb_groups 全局静态数组groups的大小，本版本为2个.
*/
void FFmpegMedia::init_parse_context(OptionParseContext *octx,
                               const OptionGroupDef *groups, int nb_groups)
{
    static const OptionGroupDef global_group = { "global" };
    int i;

    // 1. octx清0.
    memset(octx, 0, sizeof(*octx));

    // 2. 为输入输出文件开辟空间,所以OptionGroupList就代表存储输入输出文件的链表.
    octx->nb_groups = nb_groups;
    octx->groups    = (OptionGroupList *)av_mallocz_array(octx->nb_groups, sizeof(*octx->groups));
    if (!octx->groups)
        exit_program(1);

    // 3. 给输入、输出、全局选项赋予OptionGroupDef值.
    for (i = 0; i < octx->nb_groups; i++)
        octx->groups[i].group_def = &groups[i];// 指向对应的def,下标0:输出,1:输入.

    octx->global_opts.group_def = &global_group;// 全局选项的def,所以上面定义global_group使用了static
    octx->global_opts.arg       = "";

    // 4. 设置视频转码参数选项.
    init_opts();
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
        exit_program(1);
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
            exit_program(1);
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
void FFmpegMedia::finish_group(OptionParseContext *octx, int group_idx,
                         const char *arg)
{
    OptionGroupList *l = &octx->groups[group_idx];
    OptionGroup *g;

    // 1. 往OptionGroupList中的OptionGroup数组增加一个元素.
    //GROW_ARRAY(l->groups, l->nb_groups);
    l->groups = (OptionGroup *)grow_array(l->groups, sizeof(*l->groups), &l->nb_groups, l->nb_groups + 1);
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
void FFmpegMedia::add_opt(OptionParseContext *octx, const OptionDef *opt,
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
    g->opts = (Option *)grow_array(g->opts, sizeof(*g->opts), &g->nb_opts, g->nb_opts + 1);
    g->opts[g->nb_opts - 1].opt = opt;
    g->opts[g->nb_opts - 1].key = key;
    g->opts[g->nb_opts - 1].val = val;
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
    exit_program(1);
    return 0;
}

int64_t FFmpegMedia::parse_time_or_die(const char *context, const char *timestr,
                          int is_duration)
{
    int64_t us;
    if (av_parse_time(&us, timestr, is_duration) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Invalid %s specification for %s: %s\n",
               is_duration ? "duration" : "date", context, timestr);
        exit_program(1);
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
    // 看这里必须printf打印出来，因为qt debug时可能显示的值不正确.
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
        int ret = po->u.func_arg(optctx, opt, arg);
        if (ret < 0) {
//            av_log(NULL, AV_LOG_ERROR,
//                   "Failed to set value '%s' for option '%s': %s\n",
//                   arg, opt, av_err2str(ret));
            return ret;
        }
    }
    if (po->flags & OPT_EXIT)
        exit_program(0);

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
    init_parse_context(octx, groups, nb_groups);
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
            finish_group(octx, 0, opt);
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
            finish_group(octx, ret, arg);// 完成一个组的参数处理.
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

            add_opt(octx, po, opt, arg);
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

void FFmpegMedia::init_g_options(){
    // offsetof是一个C函数:返回字节对齐后，成员在结构体中的偏移量.
    // detail see https://blog.csdn.net/weixin_45275802/article/details/113528695
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
        exit_program(1);
    }

    // 3. 对比.根据用户传进的名字找到的编解码器的媒体类型，若与用户传进的不一致，退出程序.
    // 例如用户传进:-codec:a libx264，那么MATCH_PER_TYPE_OPT匹配时，audio_codec_name="libx264"
    // 虽然找到编解码器，但音频是没有libx264的，这个找到编解码器是视频的，所以退出.
    // codec->type是通过用户输入的名字找到的编解码器名字的类型
    if (codec->type != type) {
        av_log(NULL, AV_LOG_FATAL, "Invalid %s type '%s'\n", codec_string, name);
        exit_program(1);
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
        exit_program(1);
    }
}


// ret = open_files(&octx.groups[GROUP_INFILE], "input", open_input_file);
//int FFmpegMedia::open_files(OptionGroupList *l, const char *inout,
//                      int (*open_file)(OptionsContext*, const char*))
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
        ret = open_file((void*)this, &o, g->arg);
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
            case  1: *p = 0; printf("p: %s\n", p);break;
            case  0:         continue;
            default:         exit_program(1);
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
void FFmpegMedia::add_input_streams(OptionsContext *o, AVFormatContext *ic)
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

        if (!ist)
            exit_program(1);

        // 1. 往二维数组input_streams**增加一个input_streams*元素
        //GROW_ARRAY(input_streams, nb_input_streams);//注：这里只是开辟了InputStream *字节大小(64bit为8字节)，而不是开辟InputStream字节大小(sizeof(InputStream))
        GROW_ARRAY_EX(input_streams, InputStream **, nb_input_streams);
        //input_streams = (InputStream **)grow_array(input_streams, sizeof(*input_streams), &nb_input_streams, nb_input_streams + 1)
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

        // 4. 保存解码器选项。将o->g->codec_opts保存的解码器选项通过返回值设置到decoder_opts中。
        /*这里看到，每个流都保存了一份o->g->codec_opts*/
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
                ist->user_set_discard = AVDISCARD_ALL;/*设置了AVDISCARD_ALL，表示用户丢弃该流*/
        /*检测ffmpeg设置后user_set_discard的值是否与用户的一致，不一致则程序退出*/
        if (discard_str && av_opt_eval_int(&cc, discard_opt, discard_str, &ist->user_set_discard) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error parsing discard %s.\n",
                    discard_str);
            exit_program(1);
        }

        ist->filter_in_rescale_delta_last = AV_NOPTS_VALUE;

        // 6. 为解码器上下文开辟内存
        ist->dec_ctx = avcodec_alloc_context3(ist->dec);
        if (!ist->dec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Error allocating the decoder context.\n");
            exit_program(1);
        }

        // 7. 从流中拷贝参数到解码器上下文
        ret = avcodec_parameters_to_context(ist->dec_ctx, par);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error initializing the decoder context.\n");
            exit_program(1);
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
                exit_program(1);
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
                        exit_program(1);
                    }
                }
            }//<== if (hwaccel) end ==>

            // 保存硬件设备名？
            //MATCH_PER_STREAM_OPT(hwaccel_devices, str, hwaccel_device, ic, st);
            MATCH_PER_STREAM_OPT_EX(hwaccel_devices, str, hwaccel_device, char *, ic, st);
            if (hwaccel_device) {
                ist->hwaccel_device = av_strdup(hwaccel_device);
                if (!ist->hwaccel_device)
                    exit_program(1);
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
                exit_program(1);
            }
            break;
        }
        case AVMEDIA_TYPE_ATTACHMENT:
        case AVMEDIA_TYPE_UNKNOWN:
            break;
        default:
            abort();
        }//<== switch (par->codec_type) end ==>

        // 上面看到，一开始是使用avcodec_parameters_to_context(ist->dec_ctx, par)将流中
        // 的信息拷贝到解码器上下文，然后对解码器上下文里的参数赋值后，重新拷贝到流中。
        // par是st->codecpar流中的信息.
        /*9.对解码器上下文里的参数赋值后，重新拷贝到流中*/
        ret = avcodec_parameters_from_context(par, ist->dec_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error initializing the decoder context.\n");
            exit_program(1);
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
            exit_program(1);
        } else {
            o->recording_time = o->stop_time - start_time;
        }
    }

    /*在options[]中对应“f”，指定输入文件的格式“-f”*/
    if (o->format) {
        if (!(file_iformat = av_find_input_format(o->format))) {
            av_log(NULL, AV_LOG_FATAL, "Unknown input format: '%s'\n", o->format);
            exit_program(1);
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
        exit_program(1);
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
    if (video_codec_name)
        ic->video_codec    = find_codec_or_die(video_codec_name   , AVMEDIA_TYPE_VIDEO   , 0);
    if (audio_codec_name)
        ic->audio_codec    = find_codec_or_die(audio_codec_name   , AVMEDIA_TYPE_AUDIO   , 0);
    if (subtitle_codec_name)
        ic->subtitle_codec = find_codec_or_die(subtitle_codec_name, AVMEDIA_TYPE_SUBTITLE, 0);
    if (data_codec_name)
        ic->data_codec     = find_codec_or_die(data_codec_name    , AVMEDIA_TYPE_DATA    , 0);

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
        exit_program(1);
    }
    // 打开输入文件后，将该选项置空
    if (scan_all_pmts_set)
        av_dict_set(&o->g->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
    remove_avoptions(&o->g->format_opts, o->g->codec_opts);
    // 上面我们会将有效的条目置空，所以此时还有条目，代表该条目是不合法的。
    assert_avoptions(o->g->format_opts);

    // 5. 若用户设置解码器，会查找解码器，并把找到解码器的id赋值到streams中
    /* apply forced codec ids */
    for (i = 0; i < ic->nb_streams; i++)
        choose_decoder(o, ic, ic->streams[i]);

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
                exit_program(1);
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
            exit_program(1);
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
    add_input_streams(o, ic);

    /* dump the file content */
    // 参4：0-代表dump输入文件，1-代表dump输出文件。可以看源码.
    av_dump_format(ic, nb_input_files, filename, 0);

    /*8.对InputFile内剩余成员的相关初始化工作*/
    //int tyysize = sizeof (*input_files);//8
    //GROW_ARRAY(input_files, nb_input_files);// 开辟sizeof(InputFile*)大小的内存，且输入文件个数+1
    input_files = (InputFile **)grow_array(input_files, sizeof(*input_files), &nb_input_files, nb_input_files + 1);
    f = (InputFile *)av_mallocz(sizeof(*f));// 真正开辟InputFile结构体的内存
    if (!f)
        exit_program(1);
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
    f->thread = (pthread_t *)av_mallocz(sizeof(pthread_t));//tyycode
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
            exit_program(1);
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

            if (check_stream_specifier(ic, st, o->dump_attachment[i].specifier) == 1)
                dump_attachment(st, (const char *)o->dump_attachment[i].u.str);
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
void FFmpegMedia::assert_file_overwrite(const char *filename)
{
    // 1. 获取协议名字，例如rtmp://xxx，那么proto_name="rtmp"
    const char *proto_name = avio_find_protocol_name(filename);

    // 2. -y以及-n都提供，程序退出
    if (file_overwrite && no_file_overwrite) {
        fprintf(stderr, "Error, both -y and -n supplied. Exiting.\n");
        exit_program(1);
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
                    exit_program(1);
                }
                //term_init();// 该函数很简单，就是注册相关信号回调函数
            }
            else {
                av_log(NULL, AV_LOG_FATAL, "File '%s' already exists. Exiting.\n", filename);
                exit_program(1);
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
                 exit_program(1);
             }
        }
    }
}

void FFmpegMedia::dump_attachment(AVStream *st, const char *filename)
{
    int ret;
    AVIOContext *out = NULL;
    AVDictionaryEntry *e;

    if (!st->codecpar->extradata_size) {
        av_log(NULL, AV_LOG_WARNING, "No extradata to dump in stream #%d:%d.\n",
               nb_input_files - 1, st->index);
        return;
    }
    if (!*filename && (e = av_dict_get(st->metadata, "filename", NULL, 0)))
        filename = e->value;
    if (!*filename) {
        av_log(NULL, AV_LOG_FATAL, "No filename specified and no 'filename' tag"
               "in stream #%d:%d.\n", nb_input_files - 1, st->index);
        exit_program(1);
    }

    assert_file_overwrite(filename);

    if ((ret = avio_open2(&out, filename, AVIO_FLAG_WRITE, &int_cb, NULL)) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Could not open file %s for writing.\n",
               filename);
        exit_program(1);
    }

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
        exit_program(1);
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
        exit_program(1);
    }

    if (oc->nb_streams - 1 < o->nb_streamid_map)
        st->id = o->streamid_map[oc->nb_streams - 1];

    //GROW_ARRAY(output_streams, nb_output_streams);//在二级数组中添加一个一级指针大小的元素
    GROW_ARRAY_EX(output_streams, OutputStream **, nb_output_streams);
    if (!(ost = (OutputStream *)av_mallocz(sizeof(*ost))))
        exit_program(1);
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
        exit_program(1);
    }

    /*4.开辟编码器上下文*/
    ost->enc_ctx = avcodec_alloc_context3(ost->enc);
    if (!ost->enc_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Error allocating the encoding context.\n");
        exit_program(1);
    }
    ost->enc_ctx->codec_type = type;

    /*5.给OutputStream中的编码器相关参数开辟空间*/
    ost->ref_par = avcodec_parameters_alloc();
    if (!ost->ref_par) {
        av_log(NULL, AV_LOG_ERROR, "Error allocating the encoding parameters.\n");
        exit_program(1);
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
                    exit_program(1);
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
            exit_program(1);
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
            exit_program(1);
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
            exit_program(1);
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
        if (!bsf)
            exit_program(1);
        bsf_name = av_strtok(bsf, "=", &bsf_options_str);
        if (!bsf_name)
            exit_program(1);

        filter = av_bsf_get_by_name(bsf_name);
        if (!filter) {
            av_log(NULL, AV_LOG_FATAL, "Unknown bitstream filter %s\n", bsf_name);
            exit_program(1);
        }

        ost->bsf_ctx = (AVBSFContext **)av_realloc_array(ost->bsf_ctx,
                                        ost->nb_bitstream_filters + 1,
                                        sizeof(*ost->bsf_ctx));
        if (!ost->bsf_ctx)
            exit_program(1);

        ret = av_bsf_alloc(filter, &ost->bsf_ctx[ost->nb_bitstream_filters]);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error allocating a bitstream filter context\n");
            exit_program(1);
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
                exit_program(1);
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
    if (!ost->muxing_queue)
        exit_program(1);

    return ost;
}

void FFmpegMedia::parse_matrix_coeffs(uint16_t *dest, const char *str)
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
            exit_program(1);
        }
        p++;
    }
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

    /*1.为视频创建一个输出流OutputStream，通过返回值返回.
    注，oc->streams同样会有该AVStream *st，即OutputStream与oc结构都保存了该AVStream *st.
    ost->st与oc->streams中的指向肯定不一样，因为前者是一级指针，后者是二级指针(可取oc->streams[0]看指向是一样的)
    */
    ost = new_output_stream(o, oc, AVMEDIA_TYPE_VIDEO, source_index);
    st  = ost->st;
    video_enc = ost->enc_ctx;

    /*获取帧率-r选项*/
    //MATCH_PER_STREAM_OPT(frame_rates, str, frame_rate, oc, st);
    MATCH_PER_STREAM_OPT_EX(frame_rates, str, frame_rate, char *, oc, st);
    if (frame_rate && av_parse_video_rate(&ost->frame_rate, frame_rate) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Invalid framerate value: %s\n", frame_rate);
        exit_program(1);
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
            exit_program(1);
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
            exit_program(1);
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
            exit_program(1);
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
                exit_program(1);
            }
            parse_matrix_coeffs(video_enc->intra_matrix, intra_matrix);
        }
        //对应参数"chroma_intra_matrix"
        //MATCH_PER_STREAM_OPT(chroma_intra_matrices, str, chroma_intra_matrix, oc, st);
        MATCH_PER_STREAM_OPT_EX(chroma_intra_matrices, str, chroma_intra_matrix, char *, oc, st);
        if (chroma_intra_matrix) {
            uint16_t *p = (uint16_t *)av_mallocz(sizeof(*video_enc->chroma_intra_matrix) * 64);
            if (!p) {
                av_log(NULL, AV_LOG_FATAL, "Could not allocate memory for intra matrix.\n");
                exit_program(1);
            }
            video_enc->chroma_intra_matrix = p;
            parse_matrix_coeffs(p, chroma_intra_matrix);
        }
        //对应参数"inter_matrix"
        //MATCH_PER_STREAM_OPT(inter_matrices, str, inter_matrix, oc, st);
        MATCH_PER_STREAM_OPT_EX(inter_matrices, str, inter_matrix, char *, oc, st);
        if (inter_matrix) {
            if (!(video_enc->inter_matrix = (uint16_t *)av_mallocz(sizeof(*video_enc->inter_matrix) * 64))) {
                av_log(NULL, AV_LOG_FATAL, "Could not allocate memory for inter matrix.\n");
                exit_program(1);
            }
            parse_matrix_coeffs(video_enc->inter_matrix, inter_matrix);
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
                exit_program(1);
            }
            video_enc->rc_override =
                (RcOverride *)av_realloc_array(video_enc->rc_override,
                                 i + 1, sizeof(RcOverride));
            if (!video_enc->rc_override) {
                av_log(NULL, AV_LOG_FATAL, "Could not (re)allocate memory for rc_override.\n");
                exit_program(1);
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
            !(ost->logfile_prefix = av_strdup(ost->logfile_prefix)))
            exit_program(1);

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
                        exit_program(1);
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
                        exit_program(1);
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
        if (!ost->avfilter)
            exit_program(1);
    } else {
        /*未研究过该参数*/
        //流拷贝，不需要重新编码的处理
        //对应参数"copyinkf"，复制初始非关键帧
        MATCH_PER_STREAM_OPT(copy_initial_nonkeyframes, i, ost->copy_initial_nonkeyframes, oc ,st);
    }

    /*3.不转码流程*/
    if (ost->stream_copy)
        check_streamcopy_filters(o, oc, ost, AVMEDIA_TYPE_VIDEO);

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
void FFmpegMedia::check_streamcopy_filters(OptionsContext *o, AVFormatContext *oc,
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
        exit_program(1);
    }
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
        exit_program(1);
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

void FFmpegMedia::init_output_filter(OutputFilter *ofilter, OptionsContext *o,
                               AVFormatContext *oc)
{
    OutputStream *ost;

    switch (ofilter->type) {
    case AVMEDIA_TYPE_VIDEO: ost = new_video_stream(o, oc, -1); break;
    case AVMEDIA_TYPE_AUDIO: ost = new_audio_stream(o, oc, -1); break;
    default:
        av_log(NULL, AV_LOG_FATAL, "Only video and audio filters are supported "
               "currently.\n");
        exit_program(1);
    }

    ost->source_index = -1;
    ost->filter       = ofilter;

    ofilter->ost      = ost;
    ofilter->format   = -1;

    if (ost->stream_copy) {
        av_log(NULL, AV_LOG_ERROR, "Streamcopy requested for output stream %d:%d, "
               "which is fed from a complex filtergraph. Filtering and streamcopy "
               "cannot be used together.\n", ost->file_index, ost->index);
        exit_program(1);
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
        exit_program(1);
    }

    avfilter_inout_free(&ofilter->out_tmp);
}

OutputStream *FFmpegMedia::new_audio_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    int n;
    AVStream *st;
    OutputStream *ost;
    AVCodecContext *audio_enc;

    /*1.根据输入音频流创建一个输出音频流*/
    ost = new_output_stream(o, oc, AVMEDIA_TYPE_AUDIO, source_index);
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
            exit_program(1);
        }

        /*-ar采样率选项*/
        MATCH_PER_STREAM_OPT(audio_sample_rate, i, audio_enc->sample_rate, oc, st);

        //MATCH_PER_STREAM_OPT(apad, str, ost->apad, oc, st);
        MATCH_PER_STREAM_OPT_EX(apad, str, ost->apad, char *, oc, st);
        ost->apad = av_strdup(ost->apad);

        /*获取音视频过滤器描述*/
        ost->avfilter = get_ost_filters(o, oc, ost);
        if (!ost->avfilter)
            exit_program(1);

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
                                          ) < 0 )
                        exit_program(1);

                    ost->audio_channels_map[ost->audio_channels_mapped++] = map->channel_idx;
                }
            }
        }
    }//<== if (!ost->stream_copy) end ==>

    /*3.若不转码，则初始化音频流的流程比较简单*/
    if (ost->stream_copy)
        check_streamcopy_filters(o, oc, ost, AVMEDIA_TYPE_AUDIO);

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

    /*1.利用输入字幕流创建一个输出字幕流*/
    ost = new_output_stream(o, oc, AVMEDIA_TYPE_SUBTITLE, source_index);
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
            exit_program(1);
        }
    }

    return ost;
}

OutputStream *FFmpegMedia::new_data_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    OutputStream *ost;

    ost = new_output_stream(o, oc, AVMEDIA_TYPE_DATA, source_index);
    if (!ost->stream_copy) {
        av_log(NULL, AV_LOG_FATAL, "Data stream encoding not supported yet (only streamcopy)\n");
        exit_program(1);
    }

    return ost;
}

OutputStream *FFmpegMedia::new_attachment_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    OutputStream *ost = new_output_stream(o, oc, AVMEDIA_TYPE_ATTACHMENT, source_index);
    ost->stream_copy = 1;
    ost->finished    = (OSTFinished)1;
    return ost;
}

OutputStream *FFmpegMedia::new_unknown_stream(OptionsContext *o, AVFormatContext *oc, int source_index)
{
    OutputStream *ost;

    ost = new_output_stream(o, oc, AVMEDIA_TYPE_UNKNOWN, source_index);
    if (!ost->stream_copy) {
        av_log(NULL, AV_LOG_FATAL, "Unknown stream encoding not supported yet (only streamcopy)\n");
        exit_program(1);
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

    if (!fg)
        exit_program(1);
    fg->index = nb_filtergraphs;

    /*2.开辟一个OutputFilter *指针和开辟一个OutputFilter结构体，
     并让该指针指向该结构体，然后对该结构体进行相关赋值*/
    //GROW_ARRAY(fg->outputs, fg->nb_outputs);//开辟OutputFilter *指针
    GROW_ARRAY_EX(fg->outputs, OutputFilter **, fg->nb_outputs);
    if (!(fg->outputs[0] = (OutputFilter *)av_mallocz(sizeof(*fg->outputs[0]))))// 开辟一个OutputFilter结构体
        exit_program(1);
    fg->outputs[0]->ost   = ost;//保存输出流
    fg->outputs[0]->graph = fg;//保存该系统过滤器
    fg->outputs[0]->format = -1;

    ost->filter = fg->outputs[0];//同样ost中也会保存该OutputFilter.(建议画图容易理解)

    /*3.开辟一个InputFilter *指针和开辟一个InputFilter结构体，
     并让该指针指向该结构体，然后对该结构体进行相关赋值*/
    //GROW_ARRAY(fg->inputs, fg->nb_inputs);
    GROW_ARRAY_EX(fg->inputs, InputFilter **, fg->nb_inputs);
    if (!(fg->inputs[0] = (InputFilter *)av_mallocz(sizeof(*fg->inputs[0]))))
        exit_program(1);
    /*与输出过滤器赋值同理*/
    fg->inputs[0]->ist   = ist;
    fg->inputs[0]->graph = fg;
    fg->inputs[0]->format = -1;
    /*给输入过滤器中的帧队列开辟内存，注意开辟的是指针大小的内存*/
    fg->inputs[0]->frame_queue = av_fifo_alloc(8 * sizeof(AVFrame*));/*开辟内存，只不过是使用结构体AVFifoBuffer进行返回，
                                                                        av_fifo_alloc源码很简单，可以看看*/
    if (!fg->inputs[0]->frame_queue)
        exit_program(1);

    //GROW_ARRAY(ist->filters, ist->nb_filters);// 给输入流的过滤器开辟一个指针
    GROW_ARRAY_EX(ist->filters, InputFilter **, ist->nb_filters);
    ist->filters[ist->nb_filters - 1] = fg->inputs[0];// 给输入流的过滤器赋值，对比输出流OutputStream可以看到，输入流可以有多个输入过滤器

    /*4.保存FilterGraph fg，其中该fg保存了新开辟的输出过滤器OutputFilter 以及 新开辟的输入过滤器InputFilter*/
    //GROW_ARRAY(filtergraphs, nb_filtergraphs);
    GROW_ARRAY_EX(filtergraphs, FilterGraph **, nb_filtergraphs);
    filtergraphs[nb_filtergraphs - 1] = fg;

    return 0;
}

/**
 * Parse a metadata specifier passed as 'arg' parameter.
 * @param arg  metadata string to parse
 * @param type metadata type is written here -- g(lobal)/s(tream)/c(hapter)/p(rogram)
 * @param index for type c/p, chapter/program index is written here
 * @param stream_spec for type s, the stream specifier is written here
 */
void FFmpegMedia::parse_meta_type(char *arg, char *type, int *index, const char **stream_spec)
{
    if (*arg) {
        *type = *arg;
        switch (*arg) {
        case 'g':
            break;
        case 's':
            if (*(++arg) && *arg != ':') {
                av_log(NULL, AV_LOG_FATAL, "Invalid metadata specifier %s.\n", arg);
                exit_program(1);
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
            exit_program(1);
        }
    } else
        *type = 'g';
}

int FFmpegMedia::copy_metadata(char *outspec, char *inspec, AVFormatContext *oc, AVFormatContext *ic, OptionsContext *o)
{
    AVDictionary **meta_in = NULL;
    AVDictionary **meta_out = NULL;
    int i, ret = 0;
    char type_in, type_out;
    const char *istream_spec = NULL, *ostream_spec = NULL;
    int idx_in = 0, idx_out = 0;

    parse_meta_type(inspec,  &type_in,  &idx_in,  &istream_spec);
    parse_meta_type(outspec, &type_out, &idx_out, &ostream_spec);

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

#define METADATA_CHECK_INDEX(index, nb_elems, desc)\
    if ((index) < 0 || (index) >= (nb_elems)) {\
        av_log(NULL, AV_LOG_FATAL, "Invalid %s index %d while processing metadata maps.\n",\
                (desc), (index));\
        exit_program(1);\
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
        default: av_assert0(0);\
        }\

    SET_DICT(type_in, meta_in, ic, idx_in);
    SET_DICT(type_out, meta_out, oc, idx_out);

    /* for input streams choose first matching stream */
    if (type_in == 's') {
        for (i = 0; i < ic->nb_streams; i++) {
            if ((ret = check_stream_specifier(ic, ic->streams[i], istream_spec)) > 0) {
                meta_in = &ic->streams[i]->metadata;
                break;
            } else if (ret < 0)
                exit_program(1);
        }
        if (!meta_in) {
            av_log(NULL, AV_LOG_FATAL, "Stream specifier %s does not match  any streams.\n", istream_spec);
            exit_program(1);
        }
    }

    if (type_out == 's') {
        for (i = 0; i < oc->nb_streams; i++) {
            if ((ret = check_stream_specifier(oc, oc->streams[i], ostream_spec)) > 0) {
                meta_out = &oc->streams[i]->metadata;
                av_dict_copy(meta_out, *meta_in, AV_DICT_DONT_OVERWRITE);
            } else if (ret < 0)
                exit_program(1);
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
    int i, j, err;
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
            exit_program(1);
        } else {
            o->recording_time = o->stop_time - start_time;
        }
    }

    /*1.为一个输出文件开辟内存，并进行一些必要的初始化*/
    //GROW_ARRAY(output_files, nb_output_files);// 开辟sizeof(OutputFile*)大小的内存，nb_output_files自动+1
    GROW_ARRAY_EX(output_files, OutputFile **, nb_output_files);
    of = (OutputFile *)av_mallocz(sizeof(*of));//开辟OutputFile结构体
    if (!of)
        exit_program(1);
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
        exit_program(1);
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
            init_output_filter(ofilter, o, oc);
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

            if (idx >= 0)
                new_video_stream(o, oc, idx);//idx是视频流的下标.不需要处理返回值是因为，oc中也保存着一份新创建的流的地址.即oc->streams[]二级指针中

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
            if (idx >= 0)
                new_audio_stream(o, oc, idx);
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
                        new_subtitle_stream(o, oc, i);
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
                    && input_streams[i]->st->codecpar->codec_id == codec_id )
                    new_data_stream(o, oc, i);
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
                    exit_program(1);
                }
                init_output_filter(ofilter, o, oc);
            } else {
                int src_idx = input_files[map->file_index]->ist_index + map->stream_index;

                ist = input_streams[input_files[map->file_index]->ist_index + map->stream_index];
                if (ist->user_set_discard == AVDISCARD_ALL) {
                    av_log(NULL, AV_LOG_FATAL, "Stream #%d:%d is disabled and cannot be mapped.\n",
                           map->file_index, map->stream_index);
                    exit_program(1);
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
                        exit_program(1);
                    }
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
            exit_program(1);
        }
        if ((len = avio_size(pb)) <= 0) {
            av_log(NULL, AV_LOG_FATAL, "Could not get size of the attachment %s.\n",
                   o->attachments[i]);
            exit_program(1);
        }
        if (len > INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE ||
            !(attachment = (uint8_t *)av_malloc(len + AV_INPUT_BUFFER_PADDING_SIZE))) {
            av_log(NULL, AV_LOG_FATAL, "Attachment %s too large.\n",
                   o->attachments[i]);
            exit_program(1);
        }
        avio_read(pb, attachment, len);
        memset(attachment + len, 0, AV_INPUT_BUFFER_PADDING_SIZE);

        ost = new_attachment_stream(o, oc, -1);
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
            if (av_opt_set(ost->st->codec, "flags", e->value, 0) < 0)
                exit_program(1);
        }

    }
#endif

    /*7. 检测，当输出流数为0，且(对(格式不需要任何流)取反)即格式需要流的情况，那么程序退出*/
    if (!oc->nb_streams && !(oc->oformat->flags & AVFMT_NOSTREAMS)) {
        av_dump_format(oc, nb_output_files - 1, oc->url, 1);
        av_log(NULL, AV_LOG_ERROR, "Output file #%d does not contain any stream\n", nb_output_files - 1);
        exit_program(1);
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
            exit_program(1);
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
                    exit_program(1);
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
                    if (!f->formats)
                        exit_program(1);
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
                    if (!f->formats)
                        exit_program(1);
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
                    if (!f->sample_rates)
                        exit_program(1);
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
                    if (!f->channel_layouts)
                        exit_program(1);
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
            exit_program(1);
        }
    }

    /*12. 输出需要流 且 输入流为空，程序退出。
    input_stream_potentially_available=0代表输入文件没有输入流*/
    if (!(oc->oformat->flags & AVFMT_NOSTREAMS) && !input_stream_potentially_available) {
        av_log(NULL, AV_LOG_ERROR,
               "No input streams but output needs an input stream\n");
        exit_program(1);
    }

    /*13. 判断该输出文件能否重写，若不能程序直接退出；能则将输出文件重置，等待后面的流程输入数据*/
    /*输出文件的AVOutputFormat不包含AVFMT_NOFILE，那么进入if流程(实时流和文件一般都会进入)*/
    /*AVFMT_NOFILE：大概搜了源码以及百度，意思是：指一些特定于设备的特殊文件，
    且AVIOContext表示字节流输入/输出的上下文，在muxers和demuxers的数据成员flags有设置AVFMT_NOFILE时，
    这个成员变量pb就不需要设置，因为muxers和demuxers会使用其它的方式处理输入/输出。*/
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        /* test if it already exists to avoid losing precious files */
        assert_file_overwrite(filename);

        /* open the file */
        /*因为没有AVFMT_NOFILE，所以需要调avio_open2给oc->pb赋值*/
        /*经过assert_file_overwrite断言后，说明用户是认可重写该文件的，那么经过avio_open2处理后，
        输入文件就会被重新创建，debug此时看到文件变成0KB的大小*/
        if ((err = avio_open2(&oc->pb, filename, AVIO_FLAG_WRITE,
                              &oc->interrupt_callback,
                              &of->opts)) < 0) {
            print_error(filename, err);
            exit_program(1);
        }
    } else if (strcmp(oc->oformat->name, "image2")==0 && !av_filename_number_test(filename))
        assert_file_overwrite(filename);

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
            exit_program(1);
        }
        copy_metadata(o->metadata_map[i].specifier, *p ? p + 1 : p, oc,
                      in_file_index >= 0 ?
                      input_files[in_file_index]->ctx : NULL, o);
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
            exit_program(1);
        }
    }
    /*推流没用到，暂不研究*/
    if (o->chapters_input_file >= 0)
        copy_chapters(input_files[o->chapters_input_file], of,
                      !o->metadata_chapters_manual);

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
        //av_dict_set(&oc->metadata, "MAJOR_BRAND", "cwj", 0);
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
                exit_program(1);
            }
            if (!*p2)
                exit_program(1);
            p2++;

            if (!strcmp(key, "title")) {
                av_dict_set(&program->metadata, "title", p2, 0);
            } else if (!strcmp(key, "program_num")) {
            } else if (!strcmp(key, "st")) {
                int st_num = strtol(p2, NULL, 0);
                av_program_add_stream_index(oc, progid, st_num);
            } else {
                av_log(NULL, AV_LOG_FATAL, "Unknown program key %s.\n", key);
                exit_program(1);
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
            exit_program(1);
        }
        *val++ = 0;

        parse_meta_type(o->metadata[i].specifier, &type, &index, &stream_spec);
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
                } else if (ret < 0)
                    exit_program(1);
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
                    exit_program(1);
                }
                m = &oc->chapters[index]->metadata;
                break;
            case 'p':
                if (index < 0 || index >= oc->nb_programs) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid program index %d in metadata specifier.\n", index);
                    exit_program(1);
                }
                m = &oc->programs[index]->metadata;
                break;
            default:
                av_log(NULL, AV_LOG_FATAL, "Invalid metadata specifier %s.\n", o->metadata[i].specifier);
                exit_program(1);
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
void FFmpegMedia::check_filter_outputs(void)
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
                exit_program(1);
            }
        }
    }
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
//    ret = init_complex_filters();
//    if (ret < 0) {
//        av_log(NULL, AV_LOG_FATAL, "Error initializing complex filters.\n");
//        goto fail;
//    }

    // 6. 打开一个或者多个输出文件.
    /* open output files */
    ret = open_files(&octx.groups[GROUP_OUTFILE], "output", open_output_file_ex);
    if (ret < 0) {
        av_log(NULL, AV_LOG_FATAL, "Error opening output files: ");
        goto fail;
    }

    check_filter_outputs();

fail:
    uninit_parse_context(&octx);
    if (ret < 0) {
        av_strerror(ret, (char *)error, sizeof(error));
        av_log(NULL, AV_LOG_FATAL, "%s\n", error);
    }
    return ret;
}
