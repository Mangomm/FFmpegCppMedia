#include <iostream>
#include <string>
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

    FFmpegMedia tyy;
    tyy.argc = argc;
    tyy.argv = argv;
    tyy.start();

    cout << "Hello World!2" << endl;

    return 0;
}
