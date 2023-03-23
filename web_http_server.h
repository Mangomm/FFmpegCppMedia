#ifndef WEB_HTTP_SERVER_H
#define WEB_HTTP_SERVER_H

#include <iostream>
#include <string>
#include <functional>
#include <map>

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include <Poco/ThreadPool.h>

#include "Poco/JSON/Array.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/String.h"

namespace Web
{
    /// wrap HTTPRequestHandler about HTTPServerRequest/HTTPServerResponse
    class THttpContext{
    public:
        THttpContext(std::shared_ptr<Poco::Net::HTTPServerRequest> req, std::shared_ptr<Poco::Net::HTTPServerRequest> rep);
        THttpContext(const THttpContext &other) = delete;
        THttpContext& operator=(const THttpContext &other) = delete;
        virtual ~THttpContext();
    private:
        std::shared_ptr<Poco::Net::HTTPServerRequest> _req;
        std::shared_ptr<Poco::Net::HTTPServerResponse> _rep;
    };

    class BasicHandler : public Poco::Net::HTTPRequestHandler
    {
    public:
      BasicHandler();
      BasicHandler(std::function<void()> f);
      BasicHandler(const BasicHandler &other) = delete;
      BasicHandler& operator=(const BasicHandler &other) = delete;
      virtual ~BasicHandler();
      virtual void handleRequest( Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response);

    private:
      /// how to let _f accept variable parameter?
      /// maybe need create mul constructor function and define mul _f?
      std::function<void()> _f;
    };

    class PostRequestHandler : public Poco::Net::HTTPRequestHandler {
    public:
      PostRequestHandler();
      virtual ~PostRequestHandler();
      virtual void handleRequest( Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response);
    };

    class PushStreamRequestHandler : public BasicHandler {
    public:
      PushStreamRequestHandler();
      PushStreamRequestHandler(void(PushStreamRequestHandler::*f)());
      PushStreamRequestHandler(const PushStreamRequestHandler &other) = delete;
      PushStreamRequestHandler& operator=(const PushStreamRequestHandler &other) = delete;
      virtual ~PushStreamRequestHandler();

      /// see the BasicHandler base class
      //virtual void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response);

    public:
      void all();
      void add();
      void del();
    };

    class WebRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
    {
    public:
        WebRequestHandlerFactory();
        virtual ~WebRequestHandlerFactory();
        virtual Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest &request) override;

    private:
        /// register url and cb(this is a callback function that creates an HTTPRequestHandler derived class)
        void registerRouter();
        /// T is createRequestHandler() return vlaue type.
        template<typename T, typename R, typename ...Args>
        void createRequestHandler(std::string group, std::string action, R(T::*f)(Args...));

    private:
        /// map.second is function return http base pointer
        using url_newfunc_map = std::map<std::string,
        std::function< Poco::Net::HTTPRequestHandler*() >>;
        url_newfunc_map _apis_cb;
    };

}

#endif // WEB_HTTP_SERVER_H
