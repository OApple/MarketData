#include <iostream>
#include <sstream>
#include <list>
#include <string>
#include <vector>
#include<set>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <cmath>
#include <sstream>
#include <glog/logging.h>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <ThostFtdcMdApi.h>

#include "mdspi.h"
#include "property.h"
#include "util.h"

using namespace std;
using boost::lexical_cast;
using boost::locale::conv::between;
 using boost::algorithm::to_upper;
using namespace boost::gregorian;

#pragma warning(disable : 4996)

CMdSpi::CMdSpi(DataInitInstance&di):dii(di)
{
    _market_front_addr= dii._market_front_addr;
    investor_id=dii.investor_id;
    password=dii.password;
    _brokerID=dii.broker_id;
    max_instrument=50;
    instrument_start=0;
    string prefix=lexical_cast<string>(this)+"/";
    system(("mkdir  -p "+prefix).c_str());
    pUserApi = CThostFtdcMdApi::CreateFtdcMdApi(prefix.c_str());			// 创建UserApi
    pUserApi->RegisterSpi((CThostFtdcMdSpi*)this);			// 注册事件类
    pUserApi->RegisterFront((char*)(_market_front_addr.c_str()));
    pUserApi->Init();
    cout<<_market_front_addr<<"  "<<investor_id<<" "<<password<<endl;
}

void CMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
                        int nRequestID, bool bIsLast)
{
    IsErrorRspInfo(pRspInfo);
}

void CMdSpi::OnFrontDisconnected(int nReason)
{
    LOG(ERROR) << "--->>> Reason = " << nReason << endl;
}

void CMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
    LOG(ERROR) << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void CMdSpi::OnFrontConnected()
{
    LOG(ERROR) <<(lexical_cast<string>(this)+"------>>>>OnFrontConnected") ;
    ReqUserLogin();
}

void CMdSpi::ReqUserLogin()
{

    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, _brokerID.c_str());
    strcpy(req.UserID, investor_id.c_str());
    strcpy(req.Password, password.c_str());
    int iResult = pUserApi->ReqUserLogin(&req, ++iRequestID);
    LOG(INFO) << (lexical_cast<string>(this)+"<<<<----- ReqUserLogin: " + ((iResult == 0) ? "success" : "failed") );

}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG(INFO)<<(lexical_cast<string>(this)+"----->>>>OnRspUserLogin");
//    if (bIsLast && !IsErrorRspInfo(pRspInfo))
//    {
//        SubscribeMarketData();
//    }
}

//void CMdSpi::send()
//{
//    int i=0;
//    int vlen=vinstrument.size();
//    if(vlen==0)
//        return;

//    char**ppinstrument=new char*[vlen];

//    for(;i<vlen;i++)
//    {
//        ppinstrument[i]=new char[vinstrument[i]->InstrumentID.length()+1];
//        strcpy(ppinstrument[i],vinstrument[i]->InstrumentID.c_str());
//    }
//    int iResult =pUserApi->SubscribeMarketData(ppinstrument, vlen);
//    LOG(INFO) << "--->>> 发送行情订阅请求: " << ((iResult == 0) ? "成功" : "失败") << endl;

//    i=0;
//    for(;i<vlen;i++)
//    {
//        delete [] ppinstrument[i];
//    }
//    delete [] ppinstrument;
//}

//void CMdSpi::SubscribeMarketData(vector<string>ovinstrument)
//{
//    int i=0;
//    int vlen=ovinstrument.size();
//    if(vlen==0)
//        return;
//    for(;i<vlen;i++)
//    {
//        vinstrument.push_back(ovinstrument[i]);
//    }
//    SubscribeMarketData();
//}

int CMdSpi::SubscribeMarketData(const string& instrument)
{
    int vlen=1;
    char**ppinstrument=new char*[vlen];

    ppinstrument[0]=new char[instrument.size()+1];
    strcpy(ppinstrument[0],instrument.c_str());
    int iResult =pUserApi->SubscribeMarketData(ppinstrument, 1);
    LOG(ERROR)<< string(ppinstrument[0])<< "--->>> 发送行情订阅请求: " << ((iResult == 0) ? "成功" : "失败") << endl;
    delete [] ppinstrument[0];
    delete [] ppinstrument;
}


string strRspInfoField(CThostFtdcRspInfoField *pRspInfo)
{
    string tmp;
    if(pRspInfo==NULL)
        return ( tmp);
    string tmpstr;
    tmpstr =
            "错误代码 ErrorID="+ lexical_cast<string>(pRspInfo->ErrorID)+
            "\n 错误信息 ErrorMsg="+between(string(pRspInfo->ErrorMsg),"UTF-8","GBK");

    return tmpstr;
}

void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    string str=strRspInfoField(pRspInfo);
//    cout<<str<<endl;
    LOG(INFO)<<str;

}

void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG(INFO) << __FUNCTION__ << endl;
}


static string getProductID( const string&instrument)
{
    string productid;

    if(instrument.size()==0)
        return productid;
    else
    {
        for(int i=0;i<instrument.size();i++)
        {
            if((instrument[i]>='A')&&(instrument[i]<='Z'))
                productid.push_back(instrument[i]);
             if((instrument[i]>='a')&&(instrument[i]<='z'))
                productid.push_back(instrument[i]);
        }
    }
    to_upper(productid);
     return productid;
}

static string toNewInstrumentID(string _strExchangeID, string _InstrumentID, string _strCommodityNo)
{
 to_upper(_strExchangeID);
 to_upper(_InstrumentID);
 to_upper(_strCommodityNo);
    string newInstrumentID = _strExchangeID + "_F_" + _strCommodityNo + "_" + _InstrumentID.replace(_InstrumentID.find_first_of(_strCommodityNo),_strCommodityNo.length(), "") + "_N___N_";
    return newInstrumentID;
}
static string GetExchangeID(string _CommodityNo)
       {

           set<string> strArr_CFFEX = { "IC", "IF", "IH", "T", "TF", "TS"};
          set<string> strArr_DCE =  { "A", "B", "BB", "C", "CS", "FB", "I", "J", "JD", "JM", "L", "M", "P", "PP", "V", "Y", "EG"};
         set<string> strArr_SHFE =  { "AG", "AL", "AU", "BU", "CU", "FU", "HC", "NI", "PB", "RB", "RU", "SN", "WR", "ZN", "SP"};
         set<string> strArr_ZCE = { "CF", "FG", "JR", "LR", "MA", "OI", "PM", "RI", "RM", "RS", "SF", "SM", "SR", "TA", "WH", "ZC","AP","CY" };
            to_upper(_CommodityNo);
           if (strArr_CFFEX.find(_CommodityNo)!=strArr_CFFEX.end()) return "CFFEX";
           if (strArr_DCE.find(_CommodityNo)!=strArr_DCE.end()) return "DCE";
           if (strArr_SHFE.find(_CommodityNo)!=strArr_SHFE.end()) return "SHFE";
           if (strArr_ZCE.find(_CommodityNo)!=strArr_ZCE.end()) return "ZCE";
           return string();
       }
void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    string datastr=strDepthMarketData(pDepthMarketData);
    if(pDepthMarketData==nullptr)
        return;

//            cout<<datastr<<endl;
//        LOG(ERROR)<<datastr;
    //    unsigned char buf[20]={'\0'};
    //    memset(buf,0,20);
    //    if (abs(pDepthMarketData->AskPrice1) > 10000000000 || abs(pDepthMarketData->LastPrice) > 10000000000 )
    //    {
    //        LOG(ERROR) << (boost::lexical_cast<string>(pDepthMarketData->InstrumentID) + "初始化行情出现问题;");
    //         LOG(ERROR)<<datastr;
    //        return;
    //    }
    //    if (pDepthMarketData->InstrumentID == NULL||pDepthMarketData->BidPrice1 == NULL)
    //    {
    //        LOG(ERROR) << (boost::lexical_cast<string>(pDepthMarketData->InstrumentID) +"行情数据为空！！！！！");
    //         LOG(ERROR)<<datastr;
    //        return;
    //    }



    string strCommodityNo =getProductID(string(pDepthMarketData->InstrumentID));
    string strExchangeID = GetExchangeID(strCommodityNo);//由品种获得其对于的交易所ID

    string newInstrumentIDstr = toNewInstrumentID(strExchangeID, pDepthMarketData->InstrumentID, strCommodityNo);

    string strtimestamp = to_iso_extended_string(day_clock::local_day()) + " " + pDepthMarketData->UpdateTime + "." + lexical_cast<string>(pDepthMarketData->UpdateMillisec);//获得时间戳
    //                     strtimestamp = ToRight_strtimestamp(strtimestamp);

    string json;
    json.append("{");

    //001 品种
    json.append("\"DataSouce\":{\"CommodityNo\":\"" + strCommodityNo + "\",");



    //002合约ID  guiQuote.ContractId--pDepthMarketData.InstrumentID合约代码
    json.append("\"ContractID\":\"" + newInstrumentIDstr + "\",");



    //003时间戳   guiQuote.DateTimeStamp--pDepthMarketData.UpdateTime+pDepthMarketData.UpdateMillisec  最后修改时间+最后修改毫秒
    json.append("\"DateTimeStamp\":\"" + strtimestamp + "\",");


    ostringstream   tout;
    tout.flags(std::ios::fixed);
    tout.precision(2);//覆盖默认精度

    //004卖价  00001卖价guiQuote.QAskPrice--pDepthMarketData.AskPrice1申卖价一
    double AskPrice1=0;
    if(pDepthMarketData->AskPrice1!=numeric_limits<double>::max())
    {
        AskPrice1=pDepthMarketData->AskPrice1;
    }
    tout<<AskPrice1;
    json.append("\"QAskPrice\":\"" + tout.str() + "\",");

    //005卖量    guiQuote.QAskQty--pDepthMarketData.AskVolume1申卖量一
    json.append("\"QAskQty\":\"" + lexical_cast<string>(pDepthMarketData->AskVolume1)  + "\",");

    //006均价guiQuote.QAveragePrice--pDepthMarketData.AveragePrice当日均价
    double AveragePrice=0;
    if(pDepthMarketData->AveragePrice!=numeric_limits<double>::max())
    {
        AveragePrice=pDepthMarketData->AveragePrice;
    }
    tout.seekp(0);
    tout.str("");
    tout<<AveragePrice;
    json.append("\"QAveragePrice\":\"" + tout.str() + "\",");

    //007买价00002买价guiQuote.QBidPrice--pDepthMarketData.BidPrice1申买价一
    double BidPrice1=0;
    if(pDepthMarketData->BidPrice1!=numeric_limits<double>::max())
    {
        BidPrice1=pDepthMarketData->BidPrice1;
    }
    tout.seekp(0);
    tout.str("");
    tout<<BidPrice1;
    json.append("\"QBidPrice\":\"" +tout.str()+ "\",");

    //008买量guiQuote.QBidQty--pDepthMarketData.BidVolume1申买量一
    json.append("\"QBidQty\":\"" + lexical_cast<string>(pDepthMarketData->BidVolume1) + "\",");

    //009收盘价
    double _ClosingPrice = 0;
    if (pDepthMarketData->ClosePrice != numeric_limits<double>::max())
    {
        _ClosingPrice = (pDepthMarketData->ClosePrice);
    }
    tout.seekp(0);
    tout.str("");
    tout<<_ClosingPrice;
    json.append("\"QClosingPrice\":\"" + tout.str() + "\",");

    //010最高价00004最高价guiQuote.QHighPrice--pDepthMarketData.HighestPrice最高价
    double _highPrice = 0;
    if (pDepthMarketData->HighestPrice != numeric_limits<double>::max())
    {
        _highPrice = (pDepthMarketData->HighestPrice);
    }
    tout.seekp(0);
    tout.str("");
    tout<<_highPrice;
    json.append("\"QHighPrice\":\"" +tout.str()+ "\",");

    //--- 011最新价-是否为空和取小数位数6位后的转化
    //011最新价guiQuote.QLastPrice--pDepthMarketData.LastPrice最新价
    double LastPrice=0;
    if(pDepthMarketData->LastPrice!=numeric_limits<double>::max())
    {
        LastPrice=pDepthMarketData->LastPrice;
    }
    tout.seekp(0);
    tout.str("");
    tout<<LastPrice;
    json.append("\"QLastPrice\":\"" + tout.str() + "\",");

    // 012跌停价guiQuote.QLimitDownPrice--pDepthMarketData.LowerLimitPrice跌停板价
    double LowerLimitPrice=0;
    if(pDepthMarketData->LowerLimitPrice!=numeric_limits<double>::max())
    {
        LowerLimitPrice=pDepthMarketData->LowerLimitPrice;
    }
    tout.seekp(0);
    tout.str("");
    tout<<LowerLimitPrice;
    json.append("\"QLimitDownPrice\":\"" + tout.str() + "\",");

    //013最低价00005最低价guiQuote.QLowPrice--pDepthMarketData.LowestPrice最低价
    double _lowPrice = 0;
    if (pDepthMarketData->LowestPrice !=  numeric_limits<double>::max())
    {
        _lowPrice = round(pDepthMarketData->LowestPrice);
    }
    tout.seekp(0);
    tout.str("");
    tout<<_lowPrice;
    json.append("\"QLowPrice\":\"" + tout.str() + "\",");

    //014涨停价guiQuote.QLimitUpPrice--pDepthMarketData.UpperLimitPrice涨停板价
    double UpperLimitPrice=0;
    if(pDepthMarketData->UpperLimitPrice!=numeric_limits<double>::max())
    {
        UpperLimitPrice=pDepthMarketData->UpperLimitPrice;
    }
    tout.seekp(0);
    tout.str("");
    tout<<UpperLimitPrice;
    json.append("\"QLimitUpPrice\":\"" + tout.str() + "\",");

    //015开盘价00006开盘价guiQuote.QOpeningPrice--pDepthMarketData.OpenPrice今开盘
    double _openingPrice = 0;
    if (pDepthMarketData->OpenPrice != numeric_limits<double>::max())
    {
        _openingPrice = round(pDepthMarketData->OpenPrice);
    }
    tout.seekp(0);
    tout.str("");
    tout<<_openingPrice;
    json.append("\"QOpeningPrice\":\"" + tout.str() + "\",");

    //016当日总成交量guiQuote.QTotalQty--pDepthMarketData.Volume数量
    json.append("\"QTotalQty\":\"" + lexical_cast<string>(pDepthMarketData->Volume) + "\",");

    //017昨结算价guiQuote.QPreSettlePrice--pDepthMarketData.PreSettlementPrice上次结算价
    double PreSettlementPrice=0;
    if(pDepthMarketData->PreSettlementPrice!=numeric_limits<double>::max())
    {
        PreSettlementPrice=pDepthMarketData->PreSettlementPrice;
    }
    tout.seekp(0);
    tout.str("");
    tout<<PreSettlementPrice;
    json.append("\"QPreSettlePrice\":\"" + tout.str()+ "\",");

    //018涨幅guiQuote.QChangeRate--(pDepthMarketData.LastPrice-pDepthMarketData.PreSettlementPrice)/pDepthMarketData.PreSettlementPrice  （最新价-昨日结算价）/昨日结算价
    double douChangeRate;
    if (pDepthMarketData->PreSettlementPrice == 0)
    {
        douChangeRate = 0;
    }
    else
    {
        douChangeRate = (pDepthMarketData->LastPrice-pDepthMarketData->PreSettlementPrice)/pDepthMarketData->PreSettlementPrice;
    }
    tout.precision(4);
    tout.seekp(0);
    tout.str("");
    tout<<douChangeRate;
    json.append("\"QChangeRate\":\"" +tout.str() + "\",");

    //019涨跌guiQuote.QChangeValue--  pDepthMarketData.LastPrice-pDepthMarketData.PreSettlementPrice  最新价-昨日结算价
    double douChangeValue=pDepthMarketData->LastPrice-pDepthMarketData->PreSettlementPrice;
    tout.precision(2);
    tout.seekp(0);
    tout.str("");
    tout<<douChangeValue;
    json.append("\"QChangeValue\":\"" + tout.str() + "\",");

    //020持仓量guiQuote.QPositionQty--pDepthMarketData.OpenInterest持仓量
    json.append("\"QPositionQty\":\"" + lexical_cast<string>(pDepthMarketData->OpenInterest) + "\",");

    // 021当日成交金额guiQuote.QTotalTurnover--pDepthMarketData.Turnover成交金额
    double Turnover=0;
    if(pDepthMarketData->Turnover!=numeric_limits<double>::max())
    {
        Turnover=pDepthMarketData->Turnover;
    }
    tout.seekp(0);
    tout.str("");
    tout<<Turnover;
    json.append("\"QTotalTurnover\":\"" + tout.str() + "\",");

    //022昨收盘价guiQuote.QPreClosingPrice--pDepthMarketData.PreClosePrice昨收盘
    tout.seekp(0);
    tout.str("");
    tout<<pDepthMarketData->PreClosePrice;
    json.append("\"QPreClosingPrice\":\"" + tout.str() + "\",");

    //023最新成交量guiQuote.QLastQty--若dicInstr[pDepthMarketData.InstrumentID]为0，则为0；
    //若dicInstr[pDepthMarketData.InstrumentID]不为0，则为(这次-上次)pDepthMarketData.Volume数量-lastVol
    int lastVol = 0;
    if (instrument_map[pDepthMarketData->InstrumentID] != 0)
        lastVol = (pDepthMarketData->Volume - instrument_map[pDepthMarketData->InstrumentID]);
    json.append("\"QLastQty\":\"" + lexical_cast<string>(lastVol) + "\",");
    //把当前累计的总成交量记录到lastVol中，以便下次计算最新成交量
    instrument_map[pDepthMarketData->InstrumentID] = pDepthMarketData->Volume;

    //024结算价结算价00007
    double _SettlePrice = 0;
    if (pDepthMarketData->SettlementPrice != numeric_limits<double>::max())
    {
        _SettlePrice = round(pDepthMarketData->SettlementPrice);
    }
    tout.seekp(0);
    tout.str("");
    tout<<_SettlePrice;
    json.append("\"QSettlePrice\":\"" + tout.str() + "\",");

    //025昨持仓量guiQuote.QPrePositionQty--pDepthMarketData.PreOpenInterest昨持仓量
    json.append("\"QPrePositionQty\":\"" +lexical_cast<string>(pDepthMarketData->PreOpenInterest) + "\"}");
    json.append("}\r\n");
//        LOG(ERROR)<<datastr<<json;
    boost::filesystem::path file_path("./data/"+strExchangeID+"/"+strCommodityNo+"/"+dii.tradeday);   //初始化
//    LOG(ERROR)<<("./data/"+strExchangeID+"/"+strCommodityNo+"/"+to_iso_extended_string(tradeday));
    if (!boost::filesystem::exists(file_path))
    {
        boost::filesystem::create_directories(file_path);  //文件夹不存在。创建
    }
    else
    {
        boost::filesystem::fstream fstream(file_path/(newInstrumentIDstr+".dat"), std::ios_base::app);
        //write data to file
        fstream << json;
        fstream.close();
    }


}


bool CMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    // 如果ErrorID != 0, 说明收到了错误的响应
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
        LOG(INFO) << "--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
    return bResult;
}



