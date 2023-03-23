TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        ffmpeg.cpp \
        main.cpp \
        thread_wrapper.cpp \
        web_http_server.cpp

HEADERS += \
    cmdutils.h \
    config.h \
    ffmpeg.h \
    globalheader.h \
    msgdef.h \
    msgqueue.hpp \
    poco_mysql.h \
    thread_wrapper.h \
    version.h \
    web_http_server.h


win32 {

    #自定义变量.注释使用64位，否则使用32位
    #DEFINES += USE_X32

    #contains(QT_ARCH, i386) {#系统的变量，使用自己的变量去控制感觉更好
    contains(DEFINES, USE_X32) {
    # x86环境
#    INCLUDEPATH += $$PWD/ffmpeg-4.2/include
#    INCLUDEPATH += $$PWD/SDL2/include

#    LIBS += $$PWD/ffmpeg-4.2/lib/x86/avformat.lib   \
#            $$PWD/ffmpeg-4.2/lib/x86/avcodec.lib    \
#            $$PWD/ffmpeg-4.2/lib/x86/avdevice.lib   \
#            $$PWD/ffmpeg-4.2/lib/x86/avfilter.lib   \
#            $$PWD/ffmpeg-4.2/lib/x86/avutil.lib     \
#            $$PWD/ffmpeg-4.2/lib/x86/postproc.lib   \
#            $$PWD/ffmpeg-4.2/lib/x86/swresample.lib \
#            $$PWD/ffmpeg-4.2/lib/x86/swscale.lib    \
#            $$PWD/SDL2/lib/x86/SDL2.lib
    message("win32")

    }else{
    # x64环境
    INCLUDEPATH += $$PWD/3rdlib/ffmpeg-4.2/include
    INCLUDEPATH += $$PWD/3rdlib/SDL2/include
    INCLUDEPATH += $$PWD/3rdlib/poco/include/Foundation/include
    INCLUDEPATH += $$PWD/3rdlib/poco/include/Data/MYSQL/include
    INCLUDEPATH += $$PWD/3rdlib/poco/include/Data/include
    INCLUDEPATH += $$PWD/3rdlib/poco/include/Net/include
    INCLUDEPATH += $$PWD/3rdlib/poco/include/JSON/include
    INCLUDEPATH += $$PWD/3rdlib/mysql-8.0.30-winx64/include

    LIBS += $$PWD/3rdlib/ffmpeg-4.2/lib/x64/avformat.lib   \
            $$PWD/3rdlib/ffmpeg-4.2/lib/x64/avcodec.lib    \
            $$PWD/3rdlib/ffmpeg-4.2/lib/x64/avdevice.lib   \
            $$PWD/3rdlib/ffmpeg-4.2/lib/x64/avfilter.lib   \
            $$PWD/3rdlib/ffmpeg-4.2/lib/x64/avutil.lib     \
            $$PWD/3rdlib/ffmpeg-4.2/lib/x64/postproc.lib   \
            $$PWD/3rdlib/ffmpeg-4.2/lib/x64/swresample.lib \
            $$PWD/3rdlib/ffmpeg-4.2/lib/x64/swscale.lib    \
            $$PWD/3rdlib/SDL2/lib/x64/SDL2.lib

    CONFIG(debug,debug|release)
    {
        message("Debug++++++++++++++++++++++++++++++++++++++++")
        LIBS += $$PWD/3rdlib/poco/lib/win64/Debug/PocoActiveRecordd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoCryptod.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoDatad.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoDataMySQLd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoDataODBCd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoDataSQLited.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoEncodingsd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoFoundationd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoJSONd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoJWTd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoMongoDBd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoNetd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoNetSSLd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoPrometheusd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoRedisd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoUtild.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoXMLd.lib   \
                $$PWD/3rdlib/poco/lib/win64/Debug/PocoZipd.lib   \
                $$PWD/3rdlib/mysql-8.0.30-winx64/lib/libmysql.lib \
                $$PWD/3rdlib/mysql-8.0.30-winx64/lib/mysqlclient.lib
    }
    CONFIG(release,debug|release)
    {
        message("release++++++++++++++++++++++++++++++++++++++++")
        LIBS += $$PWD/3rdlib/poco/lib/win64/Release/PocoActiveRecord.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoCrypto.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoData.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoDataMySQL.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoDataODBC.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoDataSQLite.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoEncodings.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoFoundation.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoJSON.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoJWT.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoMongoDB.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoNet.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoNetSSL.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoPrometheus.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoRedis.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoUtil.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoXML.lib   \
                $$PWD/3rdlib/poco/lib/win64/Release/PocoZip.lib   \
                $$PWD/3rdlib/mysql-8.0.30-winx64/lib/mysqlclient.lib
    }

    message("win64")
    }

}

win32-msvc* {
    LIBS += psapi.lib
}
