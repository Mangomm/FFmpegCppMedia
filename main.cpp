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
    FFmpegMedia tyy;
    tyy.argc = argc;
    tyy.argv = argv;
    tyy.start();

    cout << "Hello World!2" << endl;

    return 0;
}
