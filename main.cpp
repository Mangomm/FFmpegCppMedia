#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "ffmpeg.h"
using namespace std;

#ifdef __cplusplus
extern "C"{
#endif

#ifdef __cplusplus
}
#endif

int main(int argc, char **argv)
{
    cout << "Hello World!1" << endl;

    // 1路推流测试，目前可以成功播放.
    // 参数: -re -stream_loop -1 -an  -i 704x576-1Mbps.mp4 -vcodec libx264 -profile:v main -level 3.1 -preset veryfast -tune zerolatency -b:v 1000K -maxrate 1000K -minrate 1000K -bufsize 2000K -s 704x576 -r 25 -keyint_min 50 -g 50 -sc_threshold 0 -an -shortest -f flv rtmp://192.168.1.118:1935/live/stream1
//    FFmpegMedia tyy;
//    tyy.argc = argc;
//    tyy.argv = argv;
//    tyy.start();

    auto Lambda = [=](int i){
        if(i == 1){
            FFmpegMedia tyy;
            //tyy._input_filename = "704x576-1Mbps.mp4";
            tyy._output_filename = "rtmp://192.168.1.118:1935/live/t1";
            tyy.argc = argc;
            tyy.argv = argv;
            tyy.start();
        }else if(i == 2){
            // 简单测试推多路(用两路进行模拟)
            // 注意：命令行参数的输出文件暂时用_output_filename的值代替, 后续版本修改成函数来设置. 详细看open_files()
            FFmpegMedia tyy;
            //tyy._input_filename = "704x576-1Mbps.mp4";
            tyy._output_filename = "rtmp://192.168.1.118:1935/live/t2";
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

    cout << "Hello World!2" << endl;

    return 0;
}
