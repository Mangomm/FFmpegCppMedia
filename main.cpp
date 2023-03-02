#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <map>
#include "ffmpeg.h"
#include "version.h"
#include "msgqueue.hpp"
using namespace std;
using namespace HCMFFmpegMedia;

HCMFFmpegMedia::Queue<FMMessage*> HCMFFmpegMedia::g_msg_queue;

#ifdef __cplusplus
extern "C"{
#endif

#ifdef __cplusplus
}
#endif

void test_one(int argc, char **argv){
    // 1路推流测试，目前可以成功播放.
    // 参数: -re -stream_loop -1 -an  -i 704x576-1Mbps.mp4 -vcodec libx264 -profile:v main -level 3.1 -preset veryfast -tune zerolatency -b:v 1000K -maxrate 1000K -minrate 1000K -bufsize 2000K -s 704x576 -r 25 -keyint_min 50 -g 50 -sc_threshold 0 -an -shortest -f flv rtmp://192.168.1.118:1935/live/stream1
    FFmpegMedia tyy;
    tyy.fm_set_input_filename("Titanic.ts");
    tyy.fm_set_output_filename("rtmp://192.168.1.118:1935/live/t1");
    tyy.argc = argc;
    tyy.argv = argv;
    tyy.start();
}

void test_mul(int argc, char **argv){
    // 推多路流
    auto Lambda = [=](int i){
        if(i == 1){
            FFmpegMedia tyy;
            //tyy.fm_set_input_filename("704x576-1Mbps.mp4");Titanic.ts
            tyy.fm_set_input_filename("Titanic.ts");
            tyy.fm_set_output_filename("rtmp://192.168.1.10:1935/live/t11");
            tyy.argc = argc;
            tyy.argv = argv;
            tyy.start();
        }else if(i == 2){
            // 简单测试推多路(用两路进行模拟)
            // 注意：命令行参数的输出文件暂时用_output_filename的值代替, 后续版本修改成函数来设置. 详细看open_files()
            FFmpegMedia tyy;
            //tyy.fm_set_input_filename("704x576-1Mbps.mp4");
            tyy.fm_set_input_filename("Titanic.ts");
            tyy.fm_set_output_filename("rtmp://192.168.1.10:1935/live/t12");
            tyy.argc = argc;
            tyy.argv = argv;
            tyy.start();
        }
    };

    cout << "push 1" << endl;
    std::thread th1(Lambda, 1);

    _sleep(3000);
    cout << "push 2" << endl;
    std::thread th2(Lambda, 2);

    while(1){
        //cout << "while" << endl;
        _sleep(3000);
    }
}

void test_msg_queue(int argc, char **argv){
    int ret = 0;
    FFmpegMedia *tyy = new FFmpegMedia(&g_msg_queue);
    if(!tyy){
        cout << "new memory failed" << endl;
        return;
    }
    tyy->fm_set_input_filename("Titanic.ts");
    tyy->fm_set_output_filename("rtmp://192.168.1.10:1935/live/t1");
    tyy->argc = argc;
    tyy->argv = argv;
    ret = tyy->start_async();
    if(ret < 0){
        cout << "start_async failed" << endl;
        return;
    }

    // 流管理类,后续需要自定义map的uid,方便重推和其它操作.这样以固定uid=1为例
    std::map<std::string, FFmpegMedia*> mfm;
    mfm.insert(std::pair<std::string, FFmpegMedia*>("1", tyy));

    FMMessage *msg = NULL;
    while(g_msg_queue.wait_and_pop(msg, -1) > 0){
        cout << "get msg type: " << msg->what << endl;

        auto it = mfm.find("1");
        //auto tyy_stream = mfm.find(msg->id);
        if(it == mfm.end()){
            // 流不存在,已经被删除应当检查错误
            cout << "stream not exist!!!" << endl;
            continue;
        }

        std::string in, out;
        FFmpegMedia *stream;
        stream = it->second;
        mfm.erase(it);
        if(!stream){
            // 流为空,已经被删除应当检查错误
            cout << "stream is null!!!" << endl;
            continue;
        }
        in = msg->ifilename;
        out = msg->ofilename;
        //uid = stream->_uid;
        if(stream){
            delete stream;
            stream = NULL;
        }

        switch (msg->what) {
        case FM_MSG_OPEN_INPUT_FAILED :
            // 一般是流地址失败
        break;
        case FM_MSG_FIND_STREAM_INFO_FAILED :
        break;
        case FM_MSG_INTERLEAVED_WRITE_HEADER_FAILED :
        break;
        case FM_MSG_INTERLEAVED_WRITE_FRAME_FAILED :
            // 以写帧失败为例处理
            tyy = new FFmpegMedia(&g_msg_queue);
            if(!tyy){
                cout << "new memory failed" << endl;
                return;
            }
            tyy->fm_set_input_filename(in.c_str());
            tyy->fm_set_output_filename(out.c_str());
            tyy->argc = argc;
            tyy->argv = argv;
            ret = tyy->start_async();
            if(ret < 0){
                cout << "start_async failed" << endl;
                return;
            }
            mfm.insert(std::pair<std::string, FFmpegMedia*>("1", tyy));

        break;
        default:
            cout << "unkown msg type: " << msg->what << endl;
            break;
        }

        // 消息是否要处理?后续再调
//        delete msg;
//        msg = NULL;
    }
}

int main(int argc, char **argv)
{
    cout << "Hello World!" << endl;
    cout << "version:" << fm_get_version() << endl;

#if 0
   test_one(argc, argv);
#elif 0
    test_mul(argc, argv);
#elif true
    test_msg_queue(argc, argv);
#endif

    cout << "main function end!" << endl;
    return 0;
}
