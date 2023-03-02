#ifndef MSGQUEUE_HPP
#define MSGQUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

namespace HCMFFmpegMedia {

typedef struct FMMessage{
    int what;
    std::string ifilename;
    std::string ofilename;
} FMMessage;


template<typename T>
class Queue
{
public:
    Queue() {}
    virtual ~Queue() {}

    void push(T new_value)// 实参传到形参这里会发生拷贝
    {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));// 这里不会发生拷贝
        std::lock_guard<std::mutex> lk(_mut);
        _data_queue.push(data);// 这里不会发生拷贝，但引用数加1
        _data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(_mut);
        _data_cond.wait(lk, [this] {
            return !_data_queue.empty();
        });
        value = std::move(*_data_queue.front());
        _data_queue.pop();
    }

    /**
    * @brief 获取一个value。 队列有value直接返回，没value依据参数 timeout 去处理。
    * @param value 队列元素，传入传出参数。
    * @param timeout -1代表阻塞等待; 0; 代表非阻塞等待; >0 代表有超时的等待; -2 代表参数异常。
    * @return -2 代表未知错误; 0 代表队列没有元素;  1代表读取到了元素。
    */
    int wait_and_pop(T& value, int timeout)
    {
        std::unique_lock<std::mutex> lock(_mut);
        int ret;

        for (;;) {
            if (!_data_queue.empty()) {
                value = std::move(*_data_queue.front());
                _data_queue.pop();

                ret = 1;
                break;
            }

            else if (0 == timeout) {
                ret = 0;
                break;
            }

            else if (timeout < 0) {
                _data_cond.wait(lock, [this] {
                    return !_data_queue.empty();
                });
            }

            else if (timeout > 0) {
                _data_cond.wait_for(lock, std::chrono::milliseconds(timeout), [this] {
                    return !_data_queue.empty();
                });

                if (_data_queue.empty()) {
                    ret = 0;
                    break;
                }
            }
            else {
                // UNKOWN
                ret = -2;
                break;
            }
        }

        return ret;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(_mut);
        _data_cond.wait(lk, [this] {
            return !_data_queue.empty();
        });
        std::shared_ptr<T> res = _data_queue.front();
        _data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(_mut);
        if (_data_queue.empty())
            return false;
        value = std::move(*_data_queue.front());
        _data_queue.pop();
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(_mut);
        if (_data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res = _data_queue.front();
        _data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(_mut);
        return _data_queue.empty();
    }

    void notify_one()
    {
        _data_cond.notify_one();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(_mut);
        while (!_data_queue.empty()) {
            _data_queue.pop();
            //delete xxx; 智能指针不需要处理,引用计数为0时会自动回收
        }
    }

private:
    mutable std::mutex _mut;
    std::queue< std::shared_ptr<T> > _data_queue;
    std::condition_variable _data_cond;
};


extern Queue<FMMessage*> g_msg_queue;
}// HCMFFmpegMedia

#endif // MSGQUEUE_HPP
