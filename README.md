# HCMFFmpegMedia版本迭代
0.0.2上一版本：与ffmpeg.c一样，只支持从命令行读取参数，并且只支持一路流。<br> 
0.0.2：支持推多路流。<br> 
0.0.3：通过config.h去解决ffmpeg的libavutil/thread.h 与 qt mingw编译器中的pthread_t结构体和相关函数重定义的问题，这样做是更好的。    并且添加线程封装抽象类以及消息队列<br> 
0.0.4：去掉exit_program()会导致进程退出的代码，使用返回值处理.  目前该版本基本去掉，剩下部分解析命令行参数的回调还没处理。  后续版本继续优化。<br> 
0.0.5：完全去掉ffmpeg.c本身调用exit_program()函数的代码。<br> 
0.0.6：增加消息队列的处理。目前以写帧失败进行测试，没问题。后续补充一个流管理的类进行处理。<br> 
0.0.7：增加test_msg_queue()消息队列测试，以打开输出流失败以及写帧失败为例子，进行重推，目前已经测试通过。<br> 

0.1.0：
	1. 支持msvc编译器调试ffmpeg.c。
	2. 增加poco、mysql库。注，poco库只支持msvc编译器，mingw调试ffmpeg+poco的话，需要用mingw额外编译poco。<br> 
	3. 修复轮流推不同文件到同一输出流地址时(即一个文件推到rtmp://xxx/t1并结束，另一个文件继续推到rtmp://xxx/t1)，写帧会失败的问题。原因是在start()的transcode()结束后，<br> 
		应该也调用ffmpeg_cleanup()回收,不应该只在析构调用,因为调用者不一定会有delete操作。<br> 
	4. 增加FFmpegMedia类的stop_async()停止推流函数。<br> 
	5. 完善队列类，增加中断队列的操作。<br> 
	