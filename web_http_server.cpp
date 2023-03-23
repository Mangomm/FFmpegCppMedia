#include "web_http_server.h"
#include "Poco/URI.h"
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTMLForm.h>
using namespace Web;
using namespace Poco::Net;

BasicHandler::BasicHandler()
    : Poco::Net::HTTPRequestHandler(){

}

BasicHandler::BasicHandler(std::function<void()> f)
    : Poco::Net::HTTPRequestHandler()
        , _f(f){

}

BasicHandler::~BasicHandler(){

}

void BasicHandler::handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response){

    auto method = request.getMethod();
    if(method == "GET"){
        _f();
        response.setContentType("text/txt");
        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        std::ostream& os = response.send();// only write header, return value can write body.
        os << "BasicHandler::handleRequest: Server reponse" << std::endl;
    }else if(method == "POST"){
        std::cout<<"can't handle method: " << method << std::endl;
        response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        std::ostream& os = response.send();
    }else if(method == "PUT"){
    }
    else if(method == "PATCH"){
    }
    else if(method == "DELETE"){
    }
    else{
        std::cout<<"can't handle method: " << method << std::endl;
        response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        std::ostream& os = response.send();
    }
}
//<== BasicHandler end ==>

PostRequestHandler::PostRequestHandler()
    : Poco::Net::HTTPRequestHandler(){

}

PostRequestHandler::~PostRequestHandler(){

}

void PostRequestHandler::handleRequest( Poco::Net::HTTPServerRequest & request, Poco::Net::HTTPServerResponse & response){
    std::cout << "Post Request Received!" << std::endl;

    std::istream& in = request.stream();
    int reqsize = request.getContentLength();
    auto buffer = new char[reqsize + 1]{0};
    //char buffer[reqsize + 1];// mingw ok, but msvc error
    in.read(buffer, reqsize);
    buffer[reqsize] = '\0';
    if (in)
      std::cout << "Size " << reqsize << " " << buffer << std::endl;

    response.setContentType("text/txt");
    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
    std::ostream& os = response.send();
    os << buffer << std::endl;

    if(buffer){
        delete buffer;
        buffer = NULL;
    }
}
//<== PostRequestHandler end ==>

PushStreamRequestHandler::PushStreamRequestHandler()
    : BasicHandler(){

}

PushStreamRequestHandler::PushStreamRequestHandler(void(PushStreamRequestHandler::*f)())
    : BasicHandler(std::bind(f, this)){
}

PushStreamRequestHandler::~PushStreamRequestHandler(){

}

//void PushStreamRequestHandler::handleRequest( Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response){

//}

void PushStreamRequestHandler::all(){
    std::cout<<"PushStreamRequestHandler::all()"<<std::endl;
}

void PushStreamRequestHandler::add(){
    std::cout<<"PushStreamRequestHandler::add()"<<std::endl;
    //Sleep(5000);
    std::cout<<"PushStreamRequestHandler::sss()"<<std::endl;
}

void PushStreamRequestHandler::del(){
    std::cout<<"PushStreamRequestHandler::stop()"<<std::endl;
}
//<== PushStreamRequestHandler end ==>

WebRequestHandlerFactory::WebRequestHandlerFactory()
    : HTTPRequestHandlerFactory(){
    registerRouter();
}

WebRequestHandlerFactory::~WebRequestHandlerFactory(){

}

void WebRequestHandlerFactory::registerRouter(){
    // bind url and callback functions
    createRequestHandler<PushStreamRequestHandler, void>("/api/v1/stream/", "all", &PushStreamRequestHandler::all);
    createRequestHandler<PushStreamRequestHandler, void>("/api/v1/stream/", "add", &PushStreamRequestHandler::add);
    createRequestHandler<PushStreamRequestHandler, void>("/api/v1/stream/", "del", &PushStreamRequestHandler::del);
}

template<typename T, typename R, typename ...Args>
void WebRequestHandlerFactory::createRequestHandler(std::string group, std::string action, R(T::*f)(Args...)){
    std::string url = group + action;
    _apis_cb.insert(std::make_pair(group + action, [=](){
        return new T(f);
    }));
}

Poco::Net::HTTPRequestHandler* WebRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest &request){

    auto uri = request.getURI();
    std::vector<std::string> uriNoParVec;
    Poco::URI uriParse(uri);
    uriParse.getPathSegments(uriNoParVec);

    std::string uriNoPar;
    uriNoPar.append("/");
    for(auto it = uriNoParVec.begin(); it != uriNoParVec.end(); it++){
        uriNoPar.append(it->c_str());
        uriNoPar.append("/");
    }
    uriNoPar = uriNoPar.substr(0, uriNoPar.length() - 1);
    std::cout << "uri: " << uri << ", uriNoPar: " << uriNoPar <<std::endl;

    auto func = _apis_cb.find(uriNoPar);
    if(func == _apis_cb.end()){
        // 后续可以加一个页面处理器,读取本地的错误html,以处理不存在的请求url
        return NULL;
    }

    return func->second();
}
//<== WebRequestHandlerFactory end ==>
