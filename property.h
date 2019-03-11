#ifndef PROPERTY_H
#define PROPERTY_H
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include<iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif  // _WIND32
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/thread.hpp>
#include <boost/atomic/atomic.hpp>
#include <chrono>
#include <iconv.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>
#include <glog/log_severity.h>
#include <boost/locale.hpp>
#include "./lib/ThostFtdcTraderApi.h"
#include "./lib/ThostFtdcUserApiDataType.h"
//#include "DataProcessor.h"
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
using namespace std;
inline unsigned int PthreadSelf()
{
#ifdef WIN32
    return::GetCurrentThreadId();
#else
    return pthread_self();
#endif
}
class CChineseCode{
public:
    static void UTF_8ToGB2312(string &pOut, char *pText, int pLen);//UTF-8 转为 GB2312
};
//日志信息类
class LogMsg {
public:
    LogMsg() {}
    string& getMsg() {
        return strmsg;
    }
    void setMsg(string msg) {
        strmsg = msg;
    }
    int& GetData(){
        return m_iData;
    }
    int& GetID() {
        return ID;
    }
    void setID(int msgid) {
        ID = msgid;
    }
    ~LogMsg() {}
private:
    int ID = 0;
    int m_iData;
    string m_szDataString;
    string strmsg;
    //char m_szDataString[MAX_DATA_SIZE];
};
class UniverseTools{
public:
    static vector<string> split(string str, string pattern);
};
// 代码转换操作类

class CodeConverter {
private:
    iconv_t cd;
public:
    // 构造
    CodeConverter(const char *from_charset,const char *to_charset) {
        cd = iconv_open(to_charset,from_charset);
    }
// 析构
    ~CodeConverter() {
    iconv_close(cd);
}
    // 转换输出
    int convert(char *inbuf,int inlen,char *outbuf,int outlen) {
        char **pin = &inbuf;
        char **pout = &outbuf;
        memset(outbuf,0,outlen);
        return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
    }
};
/*持仓信息情况*/
class HoldPositionInfo {
public:
    int shortTotalPosition;//空头总持仓
    int longTotalPosition;//多头总持仓
    int shortYdPosition;//空头昨持仓
    int longYdPosition;//多头昨持仓
    int shortTdPosition;//空头今持仓
    int longTdPosition;//多头今持仓
    int shortAvaClosePosition;//空头可平量
    int longAvaClosePosition;//多头可平量
    double longAmount;//多头持仓交易金额
    double shortAmount;//空头持仓交易金额
    double shortHoldAvgPrice;//空头持仓均价
    double longHoldAvgPrice;//多头持仓均价

};
class InstrumentInfo {
public:
    TThostFtdcExchangeIDType ExchangeID;
    ///合约数量乘数
    TThostFtdcVolumeMultipleType	VolumeMultiple;
    ///最小变动价位
    double	PriceTick;
    ///多头保证金率
    TThostFtdcRatioType	LongMarginRatio;
    ///空头保证金率
    TThostFtdcRatioType	ShortMarginRatio;
    string instrumentID;
    ///涨停板价
    TThostFtdcPriceType	UpperLimitPrice =0;
    ///跌停板价
    TThostFtdcPriceType	LowerLimitPrice = 0;
    bool isPriceInit = false;
};
///资金账户
class TradingAccountField{
public:
    ///经纪公司代码
    string	brokerID;
    ///投资者帐号
    string	accountID;
    ///手续费
    double	commission;
    ///平仓盈亏
    double	closeProfit;
    ///持仓盈亏
    double	positionProfit;
    ///可用资金
    double	available;
};
class BaseAccount{
public:
    string  investorID="";						///< 用户代码
    string password="";
    string hedgeFlag="1";
};
/*user account info*/
class UserAccountInfo:public BaseAccount{
public:
    vector<string> nbman;
    int frontID;
    int sessionID;
    int orderRef;
    int followTick=1;
    unordered_map<string, HoldPositionInfo*> positionmap;//hold info
    TradingAccountField tradingAccount;
    unordered_map<string, bool> holdPstIsLocked;			//保存已经锁仓的报单，防止多次未知单回报导致的多次锁仓情况。
};
/*order fields for assembling a orderinsert object*/
class UserOrderField{
public:
    int frontID;
    int sessionID;
    unsigned int orderRef;
    int requestID;
    string direction;
    string instrumentID;
    string offsetFlag;
    string hedgeFlag;
    double orderInsertPrice;
    int volume;
    string mkType;
    //string orderType;//addition info
    unsigned int clientOrderToken;
    string function;//addition info
    string timeFlag="0";
    string investorID="";
    string brokerID="";
    int followCount=0;// follow order times
    char orderPriceType[2]="";
/////限价define THOST_FTDC_OPT_LimitPrice '2'
///最优价
//#define THOST_FTDC_OPT_BestPrice '3'
///最新价
//#define THOST_FTDC_OPT_LastPrice '4'
};
/*报单信息*/
class OrderInfo {
public:
    string tradingDay = "";
    string tradeTime = "";
    boost::posix_time::ptime orderInsertTime;
    boost::atomic_int32_t orderSeq;//组合报单序号
    string systemID = "";//系统编号，每个产品一个编号
    string investorID;
    string brokerID;
    string direction;
    string offsetFlag;
    string hedgeFlag;
    string orderPriceType;
    string orderType;//"0":buy open;"1":sell open;"01":buy close;"11":sell close;"2":stop profit;"3":stop loss
    string mkType;//"agg":aggressive mm;"pas":passive mm;"0":
    int sessionID;
    int frontID;
    string orderRef;
    string orderLocalID;
    string orderSysID;
    string brokerOrderSeq;
    double price;
    int volume;
    string instrumentID;
    int begin_up_cul = 0;
    int begin_down_cul = 0;
    int userID;//equal investorID
    int followCount=0;// follow order times
    unsigned int clientOrderToken;
    unsigned char m_Side;
    unsigned char m_SecType;
    string orderStatus="-1";
    string status= "0";//0:normal;1:now action(10:begin action;11:);a:unknown
    string function = "0";//0:normal order;100:stop loss order;200:stop profit order
};
/*CTP interface info*/
class CTPInterface{
public:
    string investorID;
    CThostFtdcTraderSpi* pUserSpi;
    CThostFtdcTraderApi* pUserApi;			// 创建UserApi

};
/*组合报单信息*/
class TradeInfo {
public:
    string tradingDay = "";
    string tradeTime = "";
    boost::atomic_int32_t orderSeq;//组合报单序号
    string systemID = "";//系统编号，每个产品一个编号
    string activeOrderActionStatus = "100";//活跃合约撤单状态：0,表示成交；100，初始,发送活跃合约报单，还未收到回报；110，撤单中；120，已撤单，追单中;其他，表示收到报单的回报状态
    int sessionID;
    int frontID;
    string instrumentID;
    string direction;
    string investorID;
    string offsetFlag;
    double tradePrice = 0;
    int volume;
    double realOpenPrice = 0;//不活跃合约实际开仓成交价格
    double realClosePrice = 0;//不活跃合约实际平仓成交价格

};
#endif // PROPERTY_H
