#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <map>
#include "ffmpeg.h"
#include "version.h"
#include "msgqueue.hpp"
#include "poco_mysql.h"
#include "web_http_server.h"
using namespace std;
using namespace HCMFFmpegMedia;

using namespace Poco::Data;
using namespace Poco::Data::Keywords;
using Poco::Data::MySQL::ConnectionException;
using Poco::Data::MySQL::StatementException;
using Poco::format;
using Poco::Tuple;
using Poco::DateTime;
using Poco::Any;
using Poco::AnyCast;
using Poco::NotFoundException;
using Poco::InvalidAccessException;
using Poco::BadCastException;
using Poco::RangeException;

using Poco::LocalDateTime;
using Poco::DateTime;
using Poco::DateTimeFormat;
using Poco::DateTimeFormatter;
using Poco::DateTimeParser;

HCMFFmpegMedia::Queue<FMMessage> HCMFFmpegMedia::g_msg_queue;
std::map<std::string, FFmpegMedia*> mfm;
// 定义一个全局连接池变量
MysqlSessionPool g_mysql_pool("host=localhost;"
                              "port=3306;"
                              "user=root;"
                              "password=123456;"
                              "db=test_db;"
                              "compress=true;"
                              "auto-reconnect=true"
                              ";secure-auth=true"
                              ";ssl-mode=1"
                              ";protocol=tcp"
                              "default-character-set=default");


void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received, program exit." << std::endl;
    /// let test_msg_queue while break;
    g_msg_queue.abort();
}

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
            //tyy.fm_set_input_filename("704x576-1Mbps.mp4");
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
    auto tmpargv = argv;
    FFmpegMedia *tyy = new FFmpegMedia(&g_msg_queue);
    if(!tyy){
        std::cout << "new memory failed" << endl;
        return;
    }
    tyy->fm_set_input_filename("Titanic.ts");
    tyy->fm_set_output_filename("rtmp://192.168.1.118:1935/live/t1");
    tyy->argc = argc;
    tyy->argv = argv;
    ret = tyy->start_async();
    if(ret < 0){
        delete tyy;
        tyy = NULL;
        std::cout << "start_async failed" << endl;
        return;
    }

    // 流管理类,后续需要自定义map的uid,方便重推和其它操作.这样以固定uid=1为例
    mfm.insert(std::pair<std::string, FFmpegMedia*>("1", tyy));

    FMMessage msg;
    while(g_msg_queue.wait_and_pop(msg, -1) > 0){
        std::cout << "get msg type: " << msg.what << endl;

        auto it = mfm.find("1");
        // 1. 流不存在,已经被删除应当检查错误
        if(it == mfm.end()){
            std::cout << "stream not exist!!!" << endl;
            continue;
        }

        // 2. 从map删除,并回收.
        std::string in, out;
        FFmpegMedia *stream;
        stream = it->second;
        mfm.erase(it);
        if(!stream){
            // 流为空,已经被删除应当检查错误
            std::cout << "stream is null!!!" << endl;
            continue;
        }
        in = msg.ifilename;
        out = msg.ofilename;
        //uid = stream->_uid;
        if(stream){
            delete stream;
            stream = NULL;
        }

        // 3. 重连
        switch (msg.what) {
        case FM_MSG_OPEN_INPUT_FAILED :
            // 打开输入流地址失败
        break;
        case FM_MSG_OPEN_OUTPUT_FAILED :
            // 打开输出流地址失败
            tyy = new FFmpegMedia(&g_msg_queue);
            if(!tyy){
                std::cout << "new memory failed" << endl;
                return;
            }
            tyy->fm_set_input_filename(in.c_str());
            tyy->fm_set_output_filename(out.c_str());
            tyy->argc = argc;
            tyy->argv = tmpargv;
            ret = tyy->start_async();
            if(ret < 0){
                delete tyy;
                tyy = NULL;
                std::cout << "start_async failed" << endl;
                return;
            }
            mfm.insert(std::pair<std::string, FFmpegMedia*>("1", tyy));
        break;
        case FM_MSG_FIND_STREAM_INFO_FAILED :
        break;
        case FM_MSG_INTERLEAVED_WRITE_HEADER_FAILED :
        break;
        case FM_MSG_INTERLEAVED_WRITE_FRAME_FAILED :
            // 以写帧失败为例处理
            tyy = new FFmpegMedia(&g_msg_queue);
            if(!tyy){
                std::cout << "new memory failed" << endl;
                return;
            }
            tyy->fm_set_input_filename(in.c_str());
            tyy->fm_set_output_filename(out.c_str());
            tyy->argc = argc;
            tyy->argv = tmpargv;
            ret = tyy->start_async();
            if(ret < 0){
                delete tyy;
                tyy = NULL;
                std::cout << "start_async failed" << endl;
                return;
            }
            mfm.insert(std::pair<std::string, FFmpegMedia*>("1", tyy));

        break;
        default:
            std::cout << "unkown msg type: " << msg.what << std::endl;
            break;
        }

    }

    std::cout << "test_msg_queue function end" << std::endl;
}

void test_mysql() {

    try {
        // 从连接池中获取一个session
        Session session(g_mysql_pool.get());
        std::string id("1");
        int uid;
        std::string name;
        bool isok;

        // 方式1
        session << "SELECT  USERID, MYNAME, ISOK from USERINFO where id = ?",
            use(id), into(uid), into(name), into(isok), now;
        std::cout << "uid: " << uid << ", name: " << name << ", isok: " << isok << std::endl;

        // 方式2: 建立查询执行器
        id = "2";
        Statement select(session);
        select << "SELECT USERID, MYNAME, ISOK from USERINFO where id = ?",
            use(id),
            into(uid),
            into(name),
            into(isok),
            range(0, 1); //  每次放一条纪录
        while (!select.done())
        {
            select.execute();
            std::cout << "uid: " << uid << ", name: " << name << ", isok: " << isok << std::endl;
        }

        session.close();  // <推荐> 显式关闭连接
    }
    catch (Exception& e) {
        std::cerr << e.displayText() << std::endl;
    }

    return;
}

void test_poco_http(){
    int port = 8088;

    // Need Handler Factory
    Poco::Net::HTTPRequestHandlerFactory::Ptr factory(new Web::WebRequestHandlerFactory());

    // Net ServerSocket
    Poco::Net::ServerSocket ss(port);
    std::cout<< "listen port: " << port << std::endl;

    // Need a ThreadPool
    Poco::ThreadPool pool;

    // Need HTTPServerParams::Ptr
    Poco::Net::HTTPServerParams::Ptr pParams(new Poco::Net::HTTPServerParams());
    Poco::Net::HTTPServer server(factory, pool,  ss, pParams);
    server.start();

    // This is a bit of a hack to have the server running until a key is pressed
    std::string s;
    std::cin >> s;// block here

    server.stop();
}

int main(int argc, char **argv)
{
    cout << "Hello World!" << endl;
    cout << "version:" << fm_get_version() << endl;

#if 0
   test_one(argc, argv);
#elif 0
    test_mul(argc, argv);
#elif 0
    /// 注意,debug模式触发信号会报段错误,这是正常的.
    signal(SIGINT, signalHandler);
    test_msg_queue(argc, argv);

exit:
    std::cout<< "g_msg_queue size: " << g_msg_queue.size() << ", mfm size: " << mfm.size() << std::endl;
    g_msg_queue.clear();
    for(auto it = mfm.begin(); it != mfm.end();){
        auto fm = it->second;
        if(fm){
            fm->stop_async();
            delete fm;
            fm = NULL;
        }
        it = mfm.erase(it);
    }
#elif 0
    test_mysql();
#elif 1
    test_poco_http();
#endif

    cout << "main function end!" << endl;
    return 0;
}
