#ifndef THREAD_WRAPPER_H
#define THREAD_WRAPPER_H

#include <thread>

namespace HCMFFmpegMedia {

class ThreadWrapper
{
public:
    ThreadWrapper();
    virtual  ~ThreadWrapper();
    virtual int start();
    virtual void stop();
public:
    virtual void loop() = 0;                // 由派生实现的函数，真正的回调函数

public:
    virtual bool is_running();
    virtual void set_running(bool running); // 设置线程状态

private:
    static void *transfer(void *p);         // thread的回调函数，作为中转，内部调用Loop。

protected:
    std::thread *_thread = NULL;            // 线程
    bool _request_abort = false;            // 请求退出线程的标志，因为loop在派生实现，所以在派生的loop中才有用
    bool _running = false;                  // 线程是否在运行

};

}// HCMFFmpegMedia



#endif // THREAD_WRAPPER_H
