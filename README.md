# HCMFFmpegMedia版本迭代
0.0.2上一版本：与ffmpeg.c一样，只支持从命令行读取参数，并且只支持一路流。<br> 
0.0.2：支持推多路流。<br> 
0.0.3：通过config.h去解决ffmpeg的libavutil/thread.h 与 qt mingw编译器中的pthread_t结构体和相关函数重定义的问题，这样做是更好的。    并且添加线程封装抽象类以及消息队列<br> 
0.0.4：去掉exit_program()会导致进程退出的代码，使用返回值处理.  目前该版本基本去掉，剩下部分解析命令行参数的回调还没处理。  后续版本继续优化。<br> 