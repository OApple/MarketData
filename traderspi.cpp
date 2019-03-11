#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <ThostFtdcTraderApi.h>
#include "traderspi.h"
#include "dataprocessor.h"
#include "property.h"
#include "util.h"
#include "mdspi.h"

using namespace std;
using boost::locale::conv::between;
using boost::lexical_cast;
using boost::split;
using boost::algorithm::trim_copy;
using boost::is_any_of;
using namespace boost::gregorian;


#pragma warning(disable : 4996)


CTraderSpi::CTraderSpi(DataInitInstance &di, string investorID):dii(di)
{
    _trade_front_addr= dii._trade_front_addr;
    _investorID=investorID;
    string prefix=_investorID+"/"+dii.getTime()+"/";
    system(("mkdir  -p "+prefix).c_str());
    CThostFtdcTraderApi* pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(prefix.c_str());			// 创建UserApi
    pUserApi->RegisterSpi((CThostFtdcTraderSpi*)this);			// 注册事件类
    pUserApi->SubscribePublicTopic(THOST_TERT_RESUME);					// 注册公有流
    pUserApi->SubscribePrivateTopic(THOST_TERT_RESUME);					// 注册私有流
    pUserApi->RegisterFront((char*)(_trade_front_addr.c_str()));
    _pUserApi=pUserApi;
    vmd=new CMdSpi(dii);

}

CTraderSpi::CTraderSpi( DataInitInstance &di,
                        bool loginOK,
                        CThostFtdcTraderApi* pUserApi):dii(di),_loginOK(loginOK),_pUserApi(pUserApi)
{}

CTraderSpi::CTraderSpi(DataInitInstance &di):dii(di)
{
    _trade_front_addr= dii._trade_front_addr;
    _investorID=dii.investor_id;
    password=dii.password;
    string prefix=_investorID+"/"+dii.getTime()+"/";
    system(("mkdir  -p "+prefix).c_str());
    CThostFtdcTraderApi* pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(prefix.c_str());			// 创建UserApi
    pUserApi->RegisterSpi((CThostFtdcTraderSpi*)this);			// 注册事件类
    pUserApi->SubscribePublicTopic(THOST_TERT_RESUME);					// 注册公有流
    pUserApi->SubscribePrivateTopic(THOST_TERT_RESUME);					// 注册私有流
    pUserApi->RegisterFront((char*)(_trade_front_addr.c_str()));
    _pUserApi=pUserApi;
     _pUserApi->Init();

}

// CTraderSpi:: CTraderSpi()
//  {

//  }



void CTraderSpi::OnFrontConnected()
{
    LOG(INFO) <<(lexical_cast<string>(this)+"------>>>>OnFrontConnected") ;
    ReqUserLogin();
}
void CTraderSpi::ReqUserLogin()
{
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));

    strcpy(req.BrokerID, dii.broker_id.c_str());
    strcpy(req.UserID, _investorID.c_str());
    strcpy(req.Password, password.c_str());
    int iResult = _pUserApi->ReqUserLogin(&req, ++iRequestID);
//    string strreq=strReqUserLoginField(&req);
    LOG(ERROR) << (lexical_cast<string>(this)+"<<<<----- ReqUserLogin: " + ((iResult == 0) ? "success" : "failed") );

}




void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
//        str=strRspUserLoginField(pRspUserLogin);
        LOG(ERROR)<<(lexical_cast<string>(this)+"----->>>>OnRspUserLogin");
        frontID = pRspUserLogin->FrontID;
        sessionID= pRspUserLogin->SessionID;
        orderRef= lexical_cast<int>(pRspUserLogin->MaxOrderRef)+1;
        _brokerID=pRspUserLogin->BrokerID;

        date tradingday=from_undelimited_string(string(pRspUserLogin->TradingDay));
        dii.tradeday=to_iso_extended_string(tradingday);
        vmd=new CMdSpi(dii);
//        char tradingDay[12] = {"\0"};
//        strcpy(tradingDay,_pUserApi->GetTradingDay());
//        LOG(INFO) << lexical_cast<string>(this) << "<<<------>>> current tradingday = " << tradingDay << endl;

        ReqSettlementInfoConfirm();
    }
    else
    {
//        str=strRspInfoField(pRspInfo);
        LOG(ERROR) << lexical_cast<string>(this)<<lexical_cast<string>(pRspUserLogin->UserID)+" login errori."<<endl;
    }
}





void CTraderSpi::ReqSettlementInfoConfirm(){
    CThostFtdcSettlementInfoConfirmField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, _brokerID.c_str());
    strcpy(req.InvestorID, _investorID.c_str());
    int iResult = _pUserApi->ReqSettlementInfoConfirm(&req, ++iRequestID);
    LOG(INFO) << lexical_cast<string>(this) << "<<<----- ReqSettlementInfoConfirm: investorID=" <<_investorID<< ((iResult == 0) ? "success" : "failed") << endl;
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG(INFO) << lexical_cast<string>(this) << "------->>>OnRspSettlementInfoConfirm" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo)&&pSettlementInfoConfirm)
    {
        LOG(INFO) << lexical_cast<string>(this) << "----->>> OnRspSettlementInfoConfirm: investorID=" <<pSettlementInfoConfirm->InvestorID<< endl;
        ReqQryInstrument();
    }
}

void CTraderSpi::ReqQryInstrument()
{
    CThostFtdcQryInstrumentField req;
    memset(&req, 0, sizeof(req));
    int iResult = _pUserApi->ReqQryInstrument(&req, ++iRequestID);
    cerr << "--->>> ReqQryInstrument: " << ((iResult == 0) ? "success" : "failed") << endl;
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
//     cerr << "-------->>>OnRspQryInstrument" << endl;
     string str=strInstrument(pInstrument);
//     cout<<str<<endl;
     LOG(INFO)<<str;
//     return ;
      int i=0;
      if (pInstrument)
      {
//          Instrument ins;
//          ins.ExchangeID=pInstrument->ExchangeID;
//          ins.InstrumentID=pInstrument->InstrumentID;
//          ins.ProductID=pInstrument->ProductID;

         vmd->SubscribeMarketData(string(pInstrument->InstrumentID));
      }
//      if(bIsLast)
//      {
//           for(i=0;i<vmd.size();i++)
//           {
//             vmd->send();
//           }
//      }
}

void CTraderSpi::OnFrontDisconnected(int nReason)
{
    LOG(INFO) << "--->>> Reason = " << nReason << endl;
}

void CTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
    LOG(INFO) << lexical_cast<string>(this) << "--->>>OnHeartBeatWarning   nTimerLapse = " << nTimeLapse << endl;
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG(INFO) << lexical_cast<string>(this)  << "------>>>OnRspError" << endl;
    IsErrorRspInfo(pRspInfo);
}
//委托有错误时，才会有该报文；否则 pRspInfo本身就是空指针。
bool CTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
        if (bResult){
            string tmpstr =
           "ErrorID="+ lexical_cast<string>(pRspInfo->ErrorID)+
            ", ErrorMsg="+string(pRspInfo->ErrorMsg);

           LOG(ERROR)<<between(tmpstr,"UTF-8","GBK");
        }
    return bResult;
}

bool CTraderSpi::IsMyOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->FrontID == frontID) &&
            (pOrder->SessionID == sessionID) &&
            (lexical_cast<int>(pOrder->OrderRef)==orderRef) );
}

bool CTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
            (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
            (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}




//提取投资者报单信息


int CTraderSpi::total_trade_num() const
{
    return _total_trade_num;
}

void CTraderSpi::setTotal_trade_num(int total_trade_num)
{
    _total_trade_num = total_trade_num;
}
int CTraderSpi::profit_num() const
{
    return _profit_num;
}

void CTraderSpi::setProfit_num(int profit_num)
{
    _profit_num = profit_num;
}
int CTraderSpi::close_num() const
{
    return _close_num;
}

void CTraderSpi::setClose_num(int close_num)
{
    _close_num = close_num;
}
double CTraderSpi::profit() const
{
    return _profit;
}

void CTraderSpi::setProfit(double profit)
{
    _profit = profit;
}
double CTraderSpi::loss() const
{
    return _loss;
}

void CTraderSpi::setLoss(double loss)
{
    _loss = loss;
}
int CTraderSpi::loss_num() const
{
    return _loss_num;
}

void CTraderSpi::setLoss_num(int loss_num)
{
    _loss_num = loss_num;
}
string CTraderSpi::investorID() const
{
    return _investorID;
}

void CTraderSpi::setInvestorID(const string &investorID)
{
    _investorID = investorID;
}







