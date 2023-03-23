#ifndef POCO_MYSQSL_H
#define POCO_MYSQSL_H

#include "Poco/Data/MySQL/MySQL.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/Session.h"

//#include "CppUnit/TestCase.h"
//#include "SQLExecutor.h"
#include "Poco/String.h"
#include "Poco/Format.h"
#include "Poco/Tuple.h"
#include "Poco/DateTime.h"
#include "Poco/Any.h"
#include "Poco/Exception.h"
#include "Poco/Data/LOB.h"
#include "Poco/Data/Date.h"
#include "Poco/Data/Time.h"
#include "Poco/Data/StatementImpl.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/Transaction.h"
#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/MySQLException.h"

#ifdef _WIN32
#include <Winsock2.h>
#endif

#include <iostream>
#include <limits>
#include <cassert>
#include <string>

using namespace Poco::Data::MySQL;
using namespace Poco::Data;
using namespace Poco;
using namespace std;

class MysqlSessionPool {
public:
    MysqlSessionPool(std::string dbinfo)
        : _db_info(dbinfo) {
        Poco::Data::MySQL::Connector::registerConnector();
        _session_pool = new Poco::Data::SessionPool(
            Poco::Data::MySQL::Connector::KEY, _db_info, _min_sessions,
            _max_sessions, _idle_time);
        assert(_session_pool != nullptr);
    }
    MysqlSessionPool(const MysqlSessionPool& other) = delete;
    MysqlSessionPool& operator=(const MysqlSessionPool& other) = delete;

    ~MysqlSessionPool() {
        if (_session_pool) {
            delete _session_pool;
            _session_pool = NULL;
        }
        Poco::Data::MySQL::Connector::unregisterConnector();
    }

    Session get() { return _session_pool->get(); }

private:
    Poco::Data::SessionPool* _session_pool = nullptr;
    const std::string _db_info;
    const int _min_sessions = 1;
    const int _max_sessions = 32;
    const int _idle_time = 128;
};

extern MysqlSessionPool g_mysql_pool;
#endif // POCO_MYSQSL_H
