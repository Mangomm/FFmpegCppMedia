#include "thread_wrapper.h"
using namespace HCMFFmpegMedia;

ThreadWrapper::ThreadWrapper() :
    _thread(NULL),
    _request_abort(false),
    _running(false)
{
}

ThreadWrapper::~ThreadWrapper()
{
    // stop is safe if multiple call
    stop();
}

void* ThreadWrapper::transfer(void *p)
{
    if(!p){
        return nullptr;
    }

    auto tw = (ThreadWrapper*)p;
    tw->set_running(true);
    tw->loop();
    tw->set_running(false);

    return nullptr;
}

int ThreadWrapper::start()
{
    // this指的是谁调用该函数的对象，而不是只表示ThreadWrapper的对象，例如ffmpegwrapper->start()，那么this就是ffmpegwrapper类的对象?
    _thread = new std::thread(transfer, this);
    if(!_thread) {
        return -1;
    }

    return 0;
}

void ThreadWrapper::stop()
{
    _request_abort = true;
    //_running = false;     // _running统一在transfer管理即可,这里注不注释一样
    if(_thread && _thread->joinable()) {
        _thread->join();
        delete _thread;
        _thread = nullptr;
    }
}

bool ThreadWrapper::is_running()
{
    return _running;
}

void ThreadWrapper::set_running(bool running)
{
    _running = running;
}


