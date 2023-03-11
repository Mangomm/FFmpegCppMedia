#ifndef MSGDEF_H
#define MSGDEF_H

namespace HCMFFmpegMedia {


#define             FM_MSG_OPEN_INPUT_FAILED                        100     /* avformat_open_input failed */
#define             FM_MSG_OPEN_OUTPUT_FAILED                       101     /* avio_open2 failed */
#define             FM_MSG_FIND_STREAM_INFO_FAILED                  102
#define             FM_MSG_INTERLEAVED_WRITE_HEADER_FAILED          103
#define             FM_MSG_INTERLEAVED_WRITE_FRAME_FAILED           104
//#define             FM_MSG_INTERLEAVED_WRITE_TRAILER_FAILED         105
}

#endif // MSGDEF_H
