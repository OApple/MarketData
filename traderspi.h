#pragma once
#include <list>
#include <string>
//#include <SQLiteCpp/SQLiteCpp.h>
//#include <SQLiteCpp/VariadicBind.h>
#include <ThostFtdcTraderApi.h>

#include "property.h"
#include "dataprocessor.h"
#include "mdspi.h"

using namespace std;
class CTraderSpi : public CThostFtdcTraderSpi
{
public:

    CTraderSpi( DataInitInstance&di,  bool loginOK,CThostFtdcTraderApi* pUserApi);

    CTraderSpi(DataInitInstance&di,string investorID);

    CTraderSpi(DataInitInstance&di);

    CTraderSpi();

    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    virtual void OnFrontConnected();

    ///登录请求响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,	CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);


    ///投资者结算结果确认响应
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///请求查询合约响应
        virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);


    ///错误应答
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    virtual void OnFrontDisconnected(int nReason);

    ///心跳超时警告。当长时间未收到报文时，该方法被调用。
    virtual void OnHeartBeatWarning(int nTimeLapse);

    bool GetLoginOK();

    void SetLoginOK();

    ///请求查询合约
        void ReqQryInstrument();

    string password="";
    //    string hedgeFlag="1";
    string _trade_front_addr="0";
    CThostFtdcTraderApi* pUserApi ;

    string _brokerID;
    int frontID;
    int sessionID;
    string ratio;
    int orderRef;
    int followTick=1;
    char priceType;
    CThostFtdcTraderApi* _pUserApi;
    int total_trade_num() const;
    void setTotal_trade_num(int total_trade_num);

    int profit_num() const;
    void setProfit_num(int profit_num);

    int close_num() const;
    void setClose_num(int close_num);

    double profit() const;
    void setProfit(double profit);

    double loss() const;
    void setLoss(double loss);

    int loss_num() const;
    void setLoss_num(int loss_num);

    string investorID() const;
    void setInvestorID(const string &investorID);

protected:
    string _settlemsg;
    //    boost::recursive_mutex _spi_mtx;
    bool _loginOK;
    //    CThostFtdcTraderSpi* _pUserSpi;

    int iRequestID=1;
    //    UserAccountInfo*_ba;
    DataInitInstance &dii;

    bool _positon_req_send=false;
    bool isTradeDefFieldReady = false;

    CMdSpi*vmd;

private:

    string  _investorID="";

    int _total_trade_num;

    int _profit_num;
    int _close_num;

    double _profit;
    double _loss;

    int _loss_num;


    ///用户登录请求
    void ReqUserLogin();

    ///请求查询结算信息确认
//    int ReqQrySettlementInfoConfirm();

    ///投资者结算结果确认
//    void ReqQrySettlementInfo();

    void ReqSettlementInfoConfirm();



    // 是否收到成功的响应
    bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
    // 是否我的报单回报
    bool IsMyOrder(CThostFtdcOrderField *pOrder);
    // 是否正在交易的报单
    bool IsTradingOrder(CThostFtdcOrderField *pOrder);



};
