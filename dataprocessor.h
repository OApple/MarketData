#pragma once
#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H
#include <string>
#include <list>
#include <iostream>
#include <mutex>
#include <time.h>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <sqlite3.h>
#include <ThostFtdcTraderApi.h>
#include <ThostFtdcUserApiDataType.h>


#include "mysqlconnectpool.h"
#include "property.h"

using namespace std;

class DataInitInstance
{
public:

    DataInitInstance(void);
    virtual ~DataInitInstance(void);
    /************************************************************************/
    /* 初始化参数列表                                                                     */
    /************************************************************************/
    void initTradeApi();

    void GetConfigFromFile();
    void DataInit();
    string getTime();


    ///save orderField to DB

    vector<string> getInstrumentIDZH(string InstrumentID);
    string getHeyueName(string str);
    //UserAccount* getTradeAccount(string);
    //void setUserAccount(UserAccount* ua);



    //property config
    string environment="1";//test environment
    string marketServerIP = "";
    int  marketServerPort = 0;
    string tradeServerIP = "";
    int tradeServerPort = 0;
    int queryServerPort = 0;
    string queryServerIP = "";
    string	broker_id = "0077";				// 经纪公司代码
    string tradeday;
    int remoteTradeServerPort = 0;//交易端口
    int mkdatasrvport = 0;
    string investor_id;
    string  password = "0";			// 用户密码
    string login_id = "";

    string _market_front_addr;
    string _trade_front_addr;
    int followTimes=1;
    string db_host;
    string db_user;
    string db_pwd;
    string db_name;
    int     db_maxConnSize=10;
    string db_charset="utf8";
    int     db_port=3306;

    string redis_host="127.0.0.1";
    string redis_pwd;
    string redis_port="6379";
    // UserApi对象
    //    CMdSpi*pspi;
    string instrumentlist;
    int instrumentcnt;
    char*ppinstrument[1024];
    MysqlConnectPool *mysql_pool ;

};

#endif // DATAPROCESSOR_H
