#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>
using namespace std;

#include "./lib/ThostFtdcTraderApi.h"
#include "TraderSpi.h"
#include "DataProcessor.h"
#include "TradeProcessor.h"
#include "property.h"

#pragma warning(disable : 4996)
extern list<string> tradequeue;
///日志消息队列
extern list<string> loglist;

// USER_API参数
extern CThostFtdcTraderApi* pUserApi;

// 配置参数
extern char FRONT_ADDR[];		// 前置地址
extern char BROKER_ID[];		// 经纪公司代码
extern char INVESTOR_ID[];		// 投资者代码
extern char PASSWORD[];			// 用户密码
extern char INSTRUMENT_ID[];	// 合约代码
extern TThostFtdcPriceType	LIMIT_PRICE;	// 价格
extern TThostFtdcDirectionType	DIRECTION;	// 买卖方向
extern char BROKER_ID_1[];		// 经纪公司代码
extern char INVESTOR_ID_1[];		// 投资者代码
extern char PASSWORD_1[];			// 用户密码
extern char typ;
// 请求编号
extern int iRequestID;
extern DataInitInstance* dii;
// 会话参数
TThostFtdcFrontIDType	FRONT_ID;	//前置编号
TThostFtdcSessionIDType	SESSION_ID;	//会话编号
TThostFtdcOrderRefType	ORDER_REF;	//报单引用
char tradingDay[12] = {"\0"}; 
//持仓是否已经写入定义字段
bool isPositionDefFieldReady = false;
//成交文件是否已经写入定义字段
bool isTradeDefFieldReady = false;
//用户对冲报单文件是否已经写入定义字段
bool isOrderInsertDefFieldReady = false;
//将成交信息组装成对冲报单
CThostFtdcInputOrderField assamble(CThostFtdcTradeField *pTrade);
////将投资者对冲报单信息写入文件保存
void saveInvestorOrderInsertHedge(CThostFtdcInputOrderField *order,string filepath);
//保存报单回报信息
void saveRspOrderInsertInfo(CThostFtdcInputOrderField *pInputOrder);
//提取投资者报单信息
string getInvestorOrderInsertInfo(CThostFtdcInputOrderField *order);
//以分隔符方式记录投资者报单委托信息
string getInvestorOrderInsertInfoByDelimater(CThostFtdcInputOrderField *order);
//提取投资者报单信息
string getOrderActionInfoByDelimater(CThostFtdcInputOrderActionField *order);
//提取委托回报信息
string getRtnOrderInfoByDelimater(CThostFtdcOrderField *order);
//提取成交回报信息
string getRtnTradeInfoByDelimater(CThostFtdcTradeField *order);
//将交易所报单回报响应写入文件保存
void saveRtnOrder(CThostFtdcOrderField *pOrder);
//获取交易所响应信息
string getRtnOrder(CThostFtdcOrderField *pOrder);
void initpst(CThostFtdcInvestorPositionField *pInvestorPosition);
string storeInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition);
///多个字段组合时使用的分隔符
string sep = ";";
void CTraderSpi::tmpInvestorPosition(){
    //ReqQryInvestorPosition();
}
void CTraderSpi::OnFrontConnected()
{
	cerr << "--->>> " << "OnFrontConnected" << endl;
	///用户登录请求
	ReqUserLogin();
}
void CTraderSpi::ReqUserLogin()
{
    boost::recursive_mutex::scoped_lock SLock(dii->initapi_mtx);
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
    unordered_map<string, BaseAccount*>* whichMap;
    if(dii->whichAccount=="follow"){
        whichMap=&dii->followNBAccountMap;
    }else if(dii->whichAccount=="naman"){
        whichMap=&dii->NBAccountMap;
    }else{
        LOG(ERROR)<<"wrong account map type="+dii->whichAccount;
        return;
    }
    unordered_map<string, BaseAccount*>::iterator iter = whichMap->find(dii->loginInvestorID);//合约配对信息
    if (iter == whichMap->end()) {//未建立配对关系,需新建一个vector
        string msg="loginInvestorID="+dii->loginInvestorID+",can't find user info in followNBAccountMap.";
        cerr<<msg<<endl;
        LOG(ERROR) <<msg;
    }else{
        UserAccountInfo* ba=(UserAccountInfo*)iter->second;
        cout<<"brokerID="<<dii->BROKER_ID<<endl;
        cout<<"investorID="<<ba->investorID<<endl;
        cout<<"loginInvestorID="<<dii->loginInvestorID<<endl;
        cout<<"password="<<ba->password<<endl;
        cout<<"trade_front_ip="<<dii->TRADE_FRONT_ADDR<<endl;
        //unordered_map<string, CThostFtdcTraderApi*> tradeApiMap;
        CTPInterface* interface=dii->getTradeApi(dii->loginInvestorID);
        if(interface){
            strcpy(req.BrokerID, dii->BROKER_ID.c_str());
            strcpy(req.UserID, ba->investorID.c_str());
            strcpy(req.Password, ba->password.c_str());
            int iResult = interface->pUserApi->ReqUserLogin(&req, ++iRequestID);
            cerr << "--->>> ReqUserLogin: " << ((iResult == 0) ? "success" : "failed") << endl;
            //dii->loginInvestorID="";
            dii->isUserLogin=true;
            string tmp=((iResult == 0) ? "success" : "failed");
            string msg="--->>> ReqUserLogin: " + tmp;
            LOG(INFO) <<msg;
        }else{
            string msg="ReqUserLogin:loginInvestorID="+dii->loginInvestorID+",can't find tradeApi in tradeApiMap.";
            cerr<<msg<<endl;
            LOG(ERROR) <<msg;
        }

    }

}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    //boost::recursive_mutex::scoped_lock SLock(dii->initapi_mtx);
    cerr << "--->>> " << "OnRspUserLogin,brokerid=" <<pRspUserLogin->BrokerID<<";userid="<<pRspUserLogin->UserID<< endl;
    //loglist.push_back("--->>>OnRspUserLogin\0");
	if (bIsLast && !IsErrorRspInfo(pRspInfo)){
		// 保存会话参数
        string broker_ID=boost::lexical_cast<string>(pRspUserLogin->BrokerID);
        int front_ID = pRspUserLogin->FrontID;
        int session_ID = pRspUserLogin->SessionID;
        string investorID=boost::lexical_cast<string>(pRspUserLogin->UserID);
		int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
		iNextOrderRef++;

        unordered_map<string, BaseAccount*>* whichMap;
        if(dii->whichAccount=="follow"){
            whichMap=&dii->followNBAccountMap;
        }else if(dii->whichAccount=="naman"){
            whichMap=&dii->NBAccountMap;
        }else{
            LOG(ERROR)<<"wrong account map type="+dii->whichAccount;
            return;
        }
        unordered_map<string, BaseAccount*>::iterator logIT = whichMap->find(investorID);
        if (logIT != whichMap->end()) {//未建立配对关系,需新建一个vector
            UserAccountInfo* uai = (UserAccountInfo*)logIT->second;
            uai->frontID=front_ID;
            uai->sessionID=session_ID;
            uai->orderRef=iNextOrderRef;
        }else {
            string msg="can't find investorID="+investorID+"'s initial data from map="+dii->whichAccount;
            LOG(ERROR) <<msg;
        }
		sprintf(ORDER_REF, "%d", iNextOrderRef);
		string str ="frontID=";
		char tmpfront[20];
        str.append(boost::lexical_cast<string>(FRONT_ID));str+=";";
		str.append("sessionID=");
		char tmpsession[20];
        str.append(boost::lexical_cast<string>(SESSION_ID));str+=";";
		str.append("maxOrderRef=");
		str.append(pRspUserLogin->MaxOrderRef);str+=";";
        CTPInterface* interface=dii->getTradeApi(investorID);
        if(interface){
            strcpy(tradingDay,interface->pUserApi->GetTradingDay());
        }else{
            string msg="OnRspUserLogin:investorID="+investorID+",can't find tradeApi in tradeApiMap.";
            cerr<<msg<<endl;
            LOG(ERROR) <<msg;
        }
		///获取当前交易日
		cerr << "--->>> current tradingday = " << tradingDay << endl;
		string tmpstr = "--->>> current tradingday =";
		tmpstr.append(tradingDay);
        //loglist.push_back(tmpstr);
        //tradequeue.push_back(tmpstr);
		//重新连接之后发送新的参数
		///投资者结算结果确认
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        ReqSettlementInfoConfirm(broker_ID,investorID);

    }else{
        cout<<boost::lexical_cast<string>(pRspUserLogin->UserID)+" login error,delete trade api."<<endl;
        dii->delTraderApi(boost::lexical_cast<string>(pRspUserLogin->UserID));
        dii->setLoginOk(boost::lexical_cast<string>(pRspUserLogin->UserID));
    }
}

void CTraderSpi::ReqSettlementInfoConfirm(string brokerID,string investorID){
    boost::recursive_mutex::scoped_lock SLock(dii->initapi_mtx);
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, brokerID.c_str());
    strcpy(req.InvestorID, investorID.c_str());
    CTPInterface* interface=dii->getTradeApi(investorID);
    if(interface){
        int iResult = interface->pUserApi->ReqSettlementInfoConfirm(&req, ++iRequestID);
        cerr << "--->>> ReqSettlementInfoConfirm: " << ((iResult == 0) ? "success" : "failed") << endl;
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="--->>> ReqSettlementInfoConfirm: " + tmp;
        LOG(INFO) <<msg;
    }else{
        string msg="ReqSettlementInfoConfirm:investorID="+investorID+",can't find tradeApi in tradeApiMap.";
        cerr<<msg<<endl;
        LOG(ERROR) <<msg;
    }
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    boost::recursive_mutex::scoped_lock SLock(dii->initapi_mtx);
	cerr << "--->>> " << "OnRspSettlementInfoConfirm" << endl;
    //loglist.push_back("--->>>OnRspSettlementInfoConfirm");
    //tradequeue.push_back("--->>>OnRspSettlementInfoConfirm");
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
        cout<<"OnRspSettlementInfoConfirm ok,investor="<<pSettlementInfoConfirm->InvestorID<<endl;
        if(dii->isNBMAN(boost::lexical_cast<string>(pSettlementInfoConfirm->InvestorID))){
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            dii->setLoginOk(boost::lexical_cast<string>(pSettlementInfoConfirm->InvestorID));
            return;
        }
        //loglist.push_back("投资者结算单确认完毕");
        //tradequeue.push_back("投资者结算单确认完毕");
        //直接查询资金
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        ReqQryTradingAccount(boost::lexical_cast<string>(pSettlementInfoConfirm->BrokerID),
                             boost::lexical_cast<string>(pSettlementInfoConfirm->InvestorID));
        ///请求查询投资者持仓
        //boost::this_thread::sleep(boost::posix_time::seconds(1));
        //ReqQryInvestorPosition(boost::lexical_cast<string>(pSettlementInfoConfirm->BrokerID),
        //                       boost::lexical_cast<string>(pSettlementInfoConfirm->InvestorID));
	}
}

void CTraderSpi::ReqQryInstrument()
{
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
    //strcpy(req.InstrumentID, dii->INSTRUMENT_ID);
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    CTPInterface* interface=dii->getTradeApi(dii->loginInvestorID);
    int iResult = interface->pUserApi->ReqQryInstrument(&req, ++iRequestID);
            //dii->pUserApi->ReqQryInstrument(&req, ++iRequestID);
    cerr << "--->>> ReqQryInstrument: " << ((iResult == 0) ? "success" : "failed") << endl;
}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pInstrument) {
        dii->processRspReqInstrument(pInstrument);
    }
    //cerr << "--->>> " << "OnRspQryInstrument" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        string msg="OnRspQryInstrument is done.";
        LOG(INFO)<<msg;
        cout<<msg<<endl;
        dii->isDoneSometh=true;
    }
}

void CTraderSpi::ReqQryTradingAccount(string brokerID,string investorID){
    boost::recursive_mutex::scoped_lock SLock(dii->initapi_mtx);
	CThostFtdcQryTradingAccountField req;
	memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, brokerID.c_str());
    strcpy(req.InvestorID, investorID.c_str());
    CTPInterface* interface=dii->getTradeApi(investorID);
    if(interface){
        int iResult = interface->pUserApi->ReqQryTradingAccount(&req, ++iRequestID);
        //int iResult = dii->pUserApi->ReqQryTradingAccount(&req, ++iRequestID);
        cerr << "--->>> ReqQryTradingAccount: " << ((iResult == 0) ? "success" : "failed") << endl;
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="--->>> ReqQryTradingAccount: " + tmp;
        LOG(INFO) <<msg;
    }else{
        string msg="ReqQryTradingAccount:investorID="+investorID+",can't find tradeApi in tradeApiMap.";
        cerr<<msg<<endl;
        LOG(ERROR) <<msg;
    }

    //loglist.push_back(tmpstr);
    //tradequeue.push_back(tmpstr);
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
    boost::recursive_mutex::scoped_lock SLock(dii->initapi_mtx);
	cerr << "--->>> " << "OnRspQryTradingAccount" << endl;
	string str;
	str.append("--->>> OnRspQryTradingAccount:查询投资者账户信息\n");
    cout<<"blast="<<boost::lexical_cast<string>(bIsLast)<<","<<endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo) ){
        cout<<"1"<<endl;
        cout<<"account="<<pTradingAccount->AccountID<<endl;
        if(pTradingAccount){
            TradingAccountField taf;
            taf.accountID=boost::lexical_cast<string>(pTradingAccount->AccountID);
            taf.available=pTradingAccount->Available;
            taf.brokerID=boost::lexical_cast<string>(pTradingAccount->BrokerID);
            taf.closeProfit=pTradingAccount->CloseProfit;
            taf.commission=pTradingAccount->Commission;
            unordered_map<string, BaseAccount*>::iterator logIT = dii->followNBAccountMap.find(boost::lexical_cast<string>(pTradingAccount->AccountID));
            if (logIT != dii->followNBAccountMap.end()) {//未建立配对关系,需新建一个vector
                UserAccountInfo* uai = (UserAccountInfo*)logIT->second;
                uai->tradingAccount=taf;
            }else {
                string msg="can't find investorID="+boost::lexical_cast<string>(pTradingAccount->AccountID)+"'s initial data from followNBAccountMap";
                LOG(ERROR) <<msg;
            }
            str.append(pTradingAccount->BrokerID);
            str.append("\t");
            str.append(pTradingAccount->AccountID);
            str.append("\t");
            char a[100],b[100];
            sprintf(a,"%f",pTradingAccount->Available);
            sprintf(b,"%f",pTradingAccount->ExchangeMargin);
            str.append(a);
            str.append("\t");
            str.append(b);
            cout<<str<<endl;
            //loglist.push_back(str);
            ///请求查询投资者持仓
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            ReqQryInvestorPosition(boost::lexical_cast<string>(pTradingAccount->BrokerID),
                                   boost::lexical_cast<string>(pTradingAccount->AccountID));
        }

	}
}

void CTraderSpi::ReqQryInvestorPosition(string brokerID,string investorID)
{
    boost::recursive_mutex::scoped_lock SLock(dii->initapi_mtx);
	CThostFtdcQryInvestorPositionField req;
	memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, brokerID.c_str());
    strcpy(req.InvestorID, investorID.c_str());
    cout<<"BrokerID="<<brokerID<<";inv="<<investorID<<endl;
	//strcpy(req.InstrumentID, INSTRUMENT_ID);
    CTPInterface* interface=dii->getTradeApi(investorID);
    if(interface){
        int iResult = interface->pUserApi->ReqQryInvestorPosition(&req, ++iRequestID);
        cerr << "--->>> ReqQryInvestorPosition: " << ((iResult == 0) ? "success" : "failed") << endl;
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="--->>> ReqQryInvestorPosition: " + tmp;
        LOG(INFO) <<msg;
        dii->setLoginOk(investorID);
    }else{
        string msg="ReqQryInvestorPosition:investorID="+investorID+",can't find tradeApi in tradeApiMap.";
        cerr<<msg<<endl;
        LOG(ERROR) <<msg;
    }
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    cerr << "--->>> " << "OnRspQryInvestorPosition" << endl;
    if (!IsErrorRspInfo(pRspInfo) && pInvestorPosition) {
        initpst(pInvestorPosition);
    }

    if (bIsLast && !IsErrorRspInfo(pRspInfo) && pInvestorPosition)
    {
        unordered_map<string, HoldPositionInfo*> positionmap=dii->getPositionMap(boost::lexical_cast<string>(pInvestorPosition->InvestorID));
        unordered_map<string, HoldPositionInfo*>::iterator tmpit = positionmap.begin();
        if (tmpit == positionmap.end()) {
            cout << "当前无持仓信息" << endl;
        } else {
            int tmpArbVolume = 0;
            for (; tmpit != positionmap.end(); tmpit++) {
                string str_instrument = tmpit->first;
                HoldPositionInfo* tmppst = tmpit->second;
                char char_tmp_pst[10] = { '\0' };
                char char_longyd_pst[10] = { '\0' };
                char char_longtd_pst[10] = { '\0' };
                //sprintf(char_tmp_pst, "%d", tmppst["longTotalPosition"]);
                //sprintf(char_longyd_pst, "%d", tmppst["longYdPosition"]);
                //sprintf(char_longtd_pst, "%d", tmppst["longTdPosition"]);
                sprintf(char_tmp_pst, "%d", tmppst->longTotalPosition);
                sprintf(char_longyd_pst, "%d", tmppst->longYdPosition);
                sprintf(char_longtd_pst, "%d", tmppst->longTdPosition);
                char char_tmp_pst2[10] = { '\0' };
                char char_shortyd_pst[10] = { '\0' };
                char char_shorttd_pst[10] = { '\0' };
                //sprintf(char_tmp_pst2, "%d", tmppst["shortTotalPosition"]);
                //sprintf(char_shortyd_pst, "%d", tmppst["shortYdPosition"]);
                //sprintf(char_shorttd_pst, "%d", tmppst["shortTdPosition"]);
                sprintf(char_tmp_pst2, "%d", tmppst->shortTotalPosition);
                sprintf(char_shortyd_pst, "%d", tmppst->shortYdPosition);
                sprintf(char_shorttd_pst, "%d", tmppst->shortTdPosition);
                int currHoldPst = 0;
                int pdHoldPst = 0;
                if (tmppst->longTotalPosition == 0) {
                    currHoldPst = tmppst->shortTotalPosition;
                } else if (tmppst->shortTotalPosition == 0) {
                    currHoldPst = tmppst->longTotalPosition;
                }else if (tmppst->longTotalPosition >= tmppst->shortTotalPosition) {
                    currHoldPst = tmppst->shortTotalPosition;
                } else {
                    currHoldPst = tmppst->longTotalPosition;
                }

                if (currHoldPst == 0) {
                    tmpArbVolume = 0;
                } else if (pdHoldPst == 0) {
                    tmpArbVolume = 0;
                } else if (currHoldPst >= pdHoldPst) {
                    tmpArbVolume = pdHoldPst;
                } else {
                    tmpArbVolume = currHoldPst;
                }
                string pst_msg = "持仓结构:" + str_instrument + ",多头持仓量=" + string(char_tmp_pst) + ",今仓数量=" + string(char_longtd_pst) + ",昨仓数量=" + string(char_longyd_pst) + ",可平量=" + boost::lexical_cast<string>(tmppst->longAvaClosePosition) + ",持仓均价=" + boost::lexical_cast<string>(tmppst->longHoldAvgPrice) +
                    ";空头持仓量=" + string(char_tmp_pst2) + ",今仓数量=" + string(char_shorttd_pst) + ",昨仓数量=" + string(char_shortyd_pst) + ",可平量=" + boost::lexical_cast<string>(tmppst->shortAvaClosePosition) + ",持仓均价=" + boost::lexical_cast<string>(tmppst->shortHoldAvgPrice) +
                    ";组合持仓量=" + boost::lexical_cast<string>(tmpArbVolume);
                char errMsg[1024];
                char tmp[1024];
                strcpy(tmp,pst_msg.c_str());
                dii->codeCC->convert(tmp,strlen(tmp),errMsg,1024);
                cout << errMsg << endl;
                LOG(INFO) << pst_msg;
                //LogMsg *logmsg = new LogMsg();
                //logmsg->setMsg(pst_msg);
                //networkTradeQueue.push(logmsg);
                //startStrategy();
            }
        }
    }
}
//初始化持仓信息
void initpst(CThostFtdcInvestorPositionField *pInvestorPosition)
{
    boost::recursive_mutex::scoped_lock SLock(dii->pst_mtx);
    ///合约代码
    char	*InstrumentID = pInvestorPosition->InstrumentID;
    string str_instrumentid = string(InstrumentID);
    ///持仓多空方向
    TThostFtdcPosiDirectionType	dir = pInvestorPosition->PosiDirection;
    char PosiDirection[] = { dir,'\0' };
    ///投机套保标志
    TThostFtdcHedgeFlagType	flag = pInvestorPosition->HedgeFlag;
    char HedgeFlag[] = { flag,'\0' };
    ///上日持仓
    TThostFtdcVolumeType	ydPosition = pInvestorPosition->YdPosition;
    char YdPosition[100];
    sprintf(YdPosition, "%d", ydPosition);
    ///今日持仓
    TThostFtdcVolumeType	position = pInvestorPosition->Position;
    char Position[100];
    sprintf(Position, "%d", position);
    string str_dir = string(PosiDirection);
    double multiplier = dii->getMultipler(str_instrumentid);
    ///持仓日期
    TThostFtdcPositionDateType	positionDate = pInvestorPosition->PositionDate;
    char PositionDate[] = { positionDate,'\0' };
    string str_pst_date = string(PositionDate);
    /*********find positionmap*********/
    string investorID=boost::lexical_cast<string>(pInvestorPosition->InvestorID);
    UserAccountInfo* uai;
    unordered_map<string, BaseAccount*>::iterator fnamIT = dii->followNBAccountMap.find(investorID);
    if(fnamIT==dii->followNBAccountMap.end()){
        string msg="initpst:investorID="+investorID+",can't find UserAccountInfo in followNBAccountMap.";
        LOG(ERROR)<<msg;
        return;
    }else{
        uai=(UserAccountInfo*)fnamIT->second;
    }
    if (uai->positionmap.find(str_instrumentid) == uai->positionmap.end()) {//暂时没有处理，不需要考虑多空方向
        unordered_map<string, int> tmpmap;
        HoldPositionInfo* tmpinfo = new HoldPositionInfo();
        if ("2" == str_dir) {//买  //多头
            tmpinfo->longTotalPosition = position;
            tmpinfo->longAvaClosePosition = position;
            tmpinfo->longAmount = pInvestorPosition->PositionCost;
            tmpinfo->longHoldAvgPrice = pInvestorPosition->PositionCost / (multiplier*position);
            //tmpmap["longTotalPosition"] = position;
            //空头
            tmpinfo->shortTotalPosition = 0;
            //tmpmap["shortTotalPosition"] = 0;
            if ("2" == str_pst_date) {//昨仓
                tmpinfo->longYdPosition = position;
            } else if ("1" == str_pst_date) {//今仓
                tmpinfo->longTdPosition = position;
            }
        } else if ("3" == str_dir) {//空
                                    //空头
                                    //tmpmap["shortTotalPosition"] = position;
            tmpinfo->longTotalPosition = 0;
            tmpinfo->shortTotalPosition = position;
            tmpinfo->shortAvaClosePosition = position;
            tmpinfo->shortAmount = pInvestorPosition->PositionCost;
            tmpinfo->shortHoldAvgPrice = pInvestorPosition->PositionCost / (multiplier*position);
            //tmpmap["longTotalPosition"] = 0;
            if ("2" == str_pst_date) {//昨仓
                tmpinfo->shortYdPosition = position;
            } else if ("1" == str_pst_date) {//今仓
                tmpinfo->shortTdPosition = position;
            }
        } else {
            //cout << InstrumentID << ";error:持仓类型无法判断PosiDirection=" << str_dir << endl;
            LOG(ERROR) << string(InstrumentID) + ";error:持仓类型无法判断PosiDirection=" + str_dir;
            return;
        }
        uai->positionmap[str_instrumentid] = tmpinfo;
    } else {
        unordered_map<string, HoldPositionInfo*>::iterator tmpmap = uai->positionmap.find(str_instrumentid);
        HoldPositionInfo* tmpinfo = tmpmap->second;
        //对应的反方向应该已经存在，这里后续需要确认
        if ("2" == str_dir) {//多
                             //多头
                             //tmpmap->second["longTotalPosition"] = position + tmpmap->second["longTotalPosition"];
            tmpinfo->longTotalPosition = position + tmpinfo->longTotalPosition;
            tmpinfo->longAvaClosePosition = position + tmpinfo->longAvaClosePosition;
            tmpinfo->longAmount = pInvestorPosition->PositionCost + tmpinfo->longAmount;
            tmpinfo->longHoldAvgPrice = tmpinfo->longAmount / (multiplier*tmpinfo->longTotalPosition);
            if ("2" == str_pst_date) {//昨仓
                tmpinfo->longYdPosition = position + tmpinfo->longYdPosition;
            } else if ("1" == str_pst_date) {//今仓
                tmpinfo->longTdPosition = position + tmpinfo->longTdPosition;
            }
        } else if ("3" == str_dir) {//空
                                    //空头
                                    //tmpmap->second["shortTotalPosition"] = position + tmpmap->second["shortTotalPosition"];
            tmpinfo->shortTotalPosition = position + tmpinfo->shortTotalPosition;
            tmpinfo->shortAvaClosePosition = position + tmpinfo->shortAvaClosePosition;
            tmpinfo->shortAmount = pInvestorPosition->PositionCost + tmpinfo->shortAmount;
            tmpinfo->shortHoldAvgPrice = tmpinfo->shortAmount / (multiplier*tmpinfo->shortTotalPosition);
            if ("2" == str_pst_date) {//昨仓
                tmpinfo->shortYdPosition = position + tmpinfo->shortYdPosition;
            } else if ("1" == str_pst_date) {//今仓
                tmpinfo->shortTdPosition = position + tmpinfo->shortTdPosition;
            }
        } else {
            //cout << InstrumentID << ";error:持仓类型无法判断PosiDirection=" << str_dir << endl;
            LOG(ERROR) << string(InstrumentID) + ";error:持仓类型无法判断PosiDirection=" + str_dir;
            return;
        }
    }
    //storeInvestorPosition(pInvestorPosition);
}

void CTraderSpi::ReqOrderInsertTwo(CThostFtdcInputOrderField* req,CThostFtdcTraderApi* pUserApi){
    string investorID=boost::lexical_cast<string>(req->InvestorID);
    if(pUserApi){
        //委托类操作，使用客户端定义的请求编号格式
        int iResult = pUserApi->ReqOrderInsert(req,iRequestID++);
        cerr << "--->>> ReqOrderInsert:" << ((iResult == 0) ? "success" : "failed") << endl;
        //记录报单录入信息
        string orderinsertstr = getInvestorOrderInsertInfoByDelimater(req);
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="ReqOrderInsert:--->>> ReqOrderInsert: " + tmp+";"+orderinsertstr;
        LOG(INFO) <<msg;
    }else{
        string msg="ReqOrderInsert:investorID="+investorID+",can't find tradeApi in tradeApiMap.";
        cerr<<msg<<endl;
        LOG(ERROR) <<msg;
    }
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    //记录错误信息
    bool err = IsErrorRspInfo(pRspInfo);
    cout << "--->>> " << "OnRspOrderInsert" << "响应请求编号：" << nRequestID << " CTP回报请求编号" << pInputOrder->RequestID << endl;
    char tt[20];
    sprintf(tt, "%d", pRspInfo->ErrorID);
    string sInputOrderInfo = getInvestorOrderInsertInfoByDelimater(pInputOrder);
    string tmpstr2;
    tmpstr2.append("businessType=100").append(sep).append("result=1").append(sep).append(sInputOrderInfo).append("ErrorID=").append(tt);
    tmpstr2.append(sep).append("ErrorMsg=").append(pRspInfo->ErrorMsg);
    LOG(INFO)<<(tmpstr2);
    string sResult = "";
    if(err){
        string orderRef = boost::lexical_cast<string>(pInputOrder->OrderRef);
        string instrumentID = boost::lexical_cast<string>(pInputOrder->InstrumentID);
        string investorID = boost::lexical_cast<string>(pInputOrder->InvestorID);
        LOG(INFO) << "order insert error:delete order insert lists.";

        for(list<OrderInfo*>::iterator it = dii->bidList.begin();it != dii->bidList.end();){
            OrderInfo* orderInfo = *it;
            if(investorID==orderInfo->investorID&&orderInfo->orderRef == orderRef && instrumentID == boost::lexical_cast<string>(pInputOrder->InstrumentID)){
                LOG(INFO) << "find order insert ,and delete:" + dii->getOrderInfo(orderInfo);
                it = dii->bidList.erase(it);
                break;
            }else{
                it++;
            }
        }
        for(list<OrderInfo*>::iterator it = dii->askList.begin();it != dii->askList.end();){
            OrderInfo* orderInfo = *it;
            if(investorID==orderInfo->investorID&&orderInfo->orderRef == orderRef && instrumentID == boost::lexical_cast<string>(pInputOrder->InstrumentID)){
                LOG(INFO) << "find order insert ,and delete:" + dii->getOrderInfo(orderInfo);
                it = dii->askList.erase(it);
                break;
            }else{
                it++;
            }
        }
    }
}
///报单录入错误回报
void CTraderSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	//记录错误信息
	bool err = IsErrorRspInfo(pRspInfo);
	string sInputOrderInfo = getInvestorOrderInsertInfoByDelimater(pInputOrder);
	if(err){
		string sResult;
		char cErrorID[100] ;

		sResult.append("--->>>交易所报单录入错误回报:;ErrorID=");
        sResult.append(boost::lexical_cast<string>(pRspInfo->ErrorID));
		sResult.append(";ErrorMsg=");
		sResult.append(pRspInfo->ErrorMsg).append(";");
		sInputOrderInfo.append(sResult);
	}
	//记录错误回报报单信息
    //loglist.push_back(sInputOrderInfo);
	string str = "businessType=400;result=1;";
	str.append(sInputOrderInfo);
    LOG(INFO)<<(str);
	
}

void CTraderSpi::ReqOrderActionTwo(CThostFtdcInputOrderActionField *req,CThostFtdcTraderApi* pUserApi){
    string investorID=boost::lexical_cast<string>(req->InvestorID);
    if(pUserApi){
        //委托类操作，使用客户端定义的请求编号格式
        int iResult = pUserApi->ReqOrderAction(req,iRequestID++);
        cerr << "--->>> ReqOrderAction: " << ((iResult == 0) ? "success" : "failed") << endl;
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="--->>> ReqOrderAction: " + tmp+";";
        //记录报单录入信息
        string orderinsertstr = getOrderActionInfoByDelimater(req);
        msg+=orderinsertstr;
        LOG(INFO) <<msg;
    }else{
        string msg="ReqOrderAction:investorID="+investorID+",can't find tradeApi in tradeApiMap.";
        cerr<<msg<<endl;
        LOG(ERROR) <<msg;
    }
}
/*
void CTraderSpi::ReqOrderAction(CThostFtdcOrderField *pOrder)
{
	//暂时不适用
// 	static bool ORDER_ACTION_SENT = false;		//是否发送了报单
// 	if (ORDER_ACTION_SENT)
// 		return;

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	if(strcmp(pOrder->BrokerID,"") != 0){
		///经纪公司代码
		strcpy(req.BrokerID, pOrder->BrokerID);
	}
	if(strcmp(pOrder->InvestorID,"") != 0){
		///投资者代码
		strcpy(req.InvestorID, pOrder->InvestorID);
	}
	if(strcmp(pOrder->OrderRef,"") != 0){
		///报单引用
		strcpy(req.OrderRef, pOrder->OrderRef);
	}
	if(pOrder->RequestID != 0){
		///请求编号
		req.RequestID = pOrder->RequestID;
	}
	if(pOrder->FrontID != 0){
		///前置编号
		req.FrontID = pOrder->FrontID;
	}
	if(pOrder->SessionID != 0){
		///会话编号
		req.SessionID = pOrder->SessionID;
	}
	if(strcmp(pOrder->ExchangeID,"") != 0){
		///交易所代码
		strcpy(req.ExchangeID,pOrder->ExchangeID);
	}
	if(strcmp(pOrder->OrderSysID,"") != 0){
		///交易所代码
		strcpy(req.OrderSysID,pOrder->OrderSysID);
	}
	if(strcmp(pOrder->InstrumentID,"") != 0){
		///交易所代码
		strcpy(req.InstrumentID,pOrder->InstrumentID);
	}
	///操作标志
	req.ActionFlag = THOST_FTDC_AF_Delete;
    CTPInterface* interface=dii->getTradeApi(boost::lexical_cast<string>(pOrder->InvestorID));
    if(interface){
        int iResult = interface->pUserApi->ReqOrderAction(&req, pOrder->RequestID);
        //int iResult = interface->pUserApi->ReqQryInvestorPosition(&req, ++iRequestID);
        //int iResult = dii->pUserApi->ReqQryTradingAccount(&req, ++iRequestID);
        cerr << "--->>> ReqOrderAction: " << ((iResult == 0) ? "success" : "failed") << endl;
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="--->>> ReqOrderAction: " + tmp+";";
        //记录报单录入信息
        string orderinsertstr = getOrderActionInfoByDelimater(&req);
        msg+=orderinsertstr;
        LOG(INFO) <<msg;
    }else{
        string msg="ReqOrderAction:investorID="+boost::lexical_cast<string>(pOrder->InvestorID)+",can't find tradeApi in tradeApiMap.";
        cerr<<msg<<endl;
        LOG(ERROR) <<msg;
    }


}
*/
void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspOrderAction" << endl;
	bool err = IsErrorRspInfo(pRspInfo);
	string sInputOrderInfo = getOrderActionInfoByDelimater(pInputOrderAction);
    LOG(INFO)<<"--->>> OnRspOrderAction"+sInputOrderInfo;
    //tradequeue.push_back("OnRspOrderAction:" + sInputOrderInfo);
	string sResult = "";
// 	if(err){
// 		char cErrorID[100] ;
// 		itoa(pRspInfo->ErrorID,cErrorID,10);
// 		char cRequestid[100];
// 		sprintf(cRequestid,"%d",nRequestID);
// 		char ctpRequestId[100];
// 		sprintf(ctpRequestId,"%d",pInputOrderAction->RequestID);
// 		sResult.append("--->>>CTP报单回报信息: ErrorID=");
// 		sResult.append(cErrorID);
// 		sResult.append(", ErrorMsg=");
// 		sResult.append(pRspInfo->ErrorMsg);
// 	}
// 
// 
// 	//tradequeue.push_back(sResult);
// 
// 	sInputOrderInfo.append("\t");
// 	sInputOrderInfo.append(sResult);
// 	sInputOrderInfo.append(";响应请求编号:nRequestI=");
// 	sInputOrderInfo.append(cRequestid);
// 	sInputOrderInfo.append( "; CTP回报请求编号:RequestID=");
// 	sInputOrderInfo.append(ctpRequestId);
    //loglist.push_back(sInputOrderInfo);
}

///报单通知
void CTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    if(dii->start_process == 0){
        return;
    }
    cerr << "--->>> " << "OnRtnOrder" << endl;
    string tmpstr = getRtnOrderInfoByDelimater(pOrder);
    char	status[] = { pOrder->OrderSubmitStatus,'\0' };
    string tmpstr2;
    tmpstr2.append("businessType=300").append(sep).append("result=").append(status).append(sep).append(tmpstr);
    LOG(INFO) << ("--->>>OnRtnOrder:" + tmpstr);
    /**********we will follow naman's order here!!*******/
    if(dii->isNBMANOrder(pOrder)){
        LOG(INFO) << "this is an nbman's order,may not be processed.";
        return ;
    }
    //tradequeue.push_back(tmpstr2);
    ///报单提交状态
    TThostFtdcOrderSubmitStatusType orderSubStatus = pOrder->OrderSubmitStatus;
    if (orderSubStatus == '4' || orderSubStatus == '5' || orderSubStatus == '6') {//报单错误
        LOG(INFO) << ("报单错误:" + string(pOrder->StatusMsg));
    }
    string statusMsg = boost::lexical_cast<string>(pOrder->StatusMsg);
    ///报单状态
    TThostFtdcOrderStatusType orderStatus = pOrder->OrderStatus;
    /*
    ///全部成交
    #define THOST_FTDC_OST_AllTraded '0'
    ///部分成交还在队列中
    #define THOST_FTDC_OST_PartTradedQueueing '1'
    ///部分成交不在队列中
    #define THOST_FTDC_OST_PartTradedNotQueueing '2'
    ///未成交还在队列中
    #define THOST_FTDC_OST_NoTradeQueueing '3'
    ///未成交不在队列中
    #define THOST_FTDC_OST_NoTradeNotQueueing '4'
    ///撤单
    #define THOST_FTDC_OST_Canceled '5'
    ///未知
    #define THOST_FTDC_OST_Unknown 'a'
    ///尚未触发
    #define THOST_FTDC_OST_NotTouched 'b' */
    //锁持仓处理
    boost::recursive_mutex::scoped_lock SLock(dii->pst_mtx);//锁定
    //boost::recursive_mutex::scoped_lock SLock4(alreadyTrade_mtx);//锁定
    if (orderStatus == '5') {//撤单，说明报单未成功;不活跃合约需要持续报单，活跃合约暂时不处理
                             //正常撤单都是由frontid+sessionid+orderref选择；但是手动撤单采用ordersysid+brokerorderseq
                             //判断是否是不活跃合约
        string msg="OnRtnOrder:process order action.";
        LOG(INFO)<<msg;
        dii->processHowManyHoldsCanBeClose(pOrder,"release");//释放持仓
        dii->processAction(pOrder);
    }else if(orderStatus == 'a') {//未知单状态，锁定可平量
        string msg="OnRtnOrder:process unknown order.";
        LOG(INFO)<<msg;
        if(strlen(pOrder->OrderSysID)==0){//ctp return
            LOG(INFO)<<"order from ctp.";
            dii->updateOriOrder(pOrder,"ctp");
            dii->processHowManyHoldsCanBeClose(pOrder, "lock");//锁定持仓
        }else if(strlen(pOrder->OrderSysID)!=0){//exchange return
            LOG(INFO)<<"order from exchange.";
            dii->updateOriOrder(pOrder,"exg");
        }
    }else if(orderStatus == '4'||orderStatus == '3'||orderStatus == '2'||orderStatus == '1'){
        string msg="OnRtnOrder:process unTrade order.";
        LOG(INFO)<<msg;
        if(strlen(pOrder->OrderSysID)!=0){//exchange return
            dii->updateOriOrder(pOrder,"exg");
        }
    }else if (orderStatus == '0') {//成交也需要释放锁定的持仓
        string msg="OnRtnOrder:process trade order.";
        LOG(INFO)<<msg;
        dii->processHowManyHoldsCanBeClose(pOrder, "release");//释放持仓.
        //return;
        ///////////////////////
        TradeInfo *pTrade=new TradeInfo();
        pTrade->investorID=boost::lexical_cast<string>(pOrder->InvestorID);
        pTrade->offsetFlag=boost::lexical_cast<string>(pOrder->CombOffsetFlag);
        pTrade->direction=boost::lexical_cast<string>(pOrder->Direction);
        pTrade->instrumentID=boost::lexical_cast<string>(pOrder->InstrumentID);

        pTrade->volume=(pOrder->VolumeTotalOriginal-pOrder->VolumeTotal);
        pTrade->tradePrice=pOrder->LimitPrice;
        dii->processtrade(pTrade);//处理持仓情况
        /////////////////////
    }else{
        string msg="OnRtnOrder:this kind order type is not defined.";
        LOG(INFO)<<msg;
    }
    /*
    string rst = processArbRtnOrder(pOrder);
    string msg = "";
    if (rst == "1") {
        msg = "process instrumentID=" + string(pOrder->InstrumentID) + " rtnOrder success!";
    } else {
        msg = "process instrumentID=" + string(pOrder->InstrumentID) + " rtnOrder failed!";
    }
    LOG(INFO) << (msg);*/
}

///成交通知
void CTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    if(dii->start_process == 0){
        return;
    }
    cerr << "--->>> " << "OnRtnTrade" << endl;
    if(dii->isNBMANTrade(pTrade)){
        LOG(INFO) << "this is an nbman's trade,may not be processed.";
        return ;
    }
    //锁持仓处理
    boost::recursive_mutex::scoped_lock SLock(dii->pst_mtx);
    //dii->processHowManyHoldsCanBeClose(pOrder, "release");//释放持仓
    //处理持仓情况 put modify release position in onRtnOrder.
    /*
    TradeInfo *trade=new TradeInfo();
    trade->investorID=boost::lexical_cast<string>(pTrade->InvestorID);
    trade->offsetFlag=boost::lexical_cast<string>(pTrade->OffsetFlag);
    trade->direction=boost::lexical_cast<string>(pTrade->Direction);
    trade->instrumentID=boost::lexical_cast<string>(pTrade->InstrumentID);

    trade->volume=pTrade->Volume;
    trade->tradePrice=pTrade->Price;
    dii->processtrade(trade);*/
    string tmpstr = getRtnTradeInfoByDelimater(pTrade);
    LOG(INFO) << ("--->>>OnRtnTrade:" + tmpstr);
    dii->deleteOriOrder(boost::lexical_cast<string>(pTrade->OrderSysID));

}

void CTraderSpi:: OnFrontDisconnected(int nReason)
{
	cerr << "--->>> " << "OnFrontDisconnected" << endl;
	cerr << "--->>> Reason = " << nReason << endl;
	string tmpstr = "error:OnFrontDisconnected:reason=";
	tmpstr.append(boost::lexical_cast<string>(nReason));
    //tradequeue.push_back(tmpstr);
}
		
void CTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
	cerr << "--->>> " << "OnHeartBeatWarning" << endl;
	cerr << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "------------------>>> " << "OnRspError" << endl;
	IsErrorRspInfo(pRspInfo);
}
//委托有错误时，才会有该报文；否则 pRspInfo本身就是空指针。
bool CTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// 如果ErrorID != 0, 说明收到了错误的响应
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult){
		string tmpstr = "--->>> ErrorID=";
        char tmpchar[1024];
        tmpstr.append(boost::lexical_cast<string>(pRspInfo->ErrorID));
		tmpstr.append(", ErrorMsg=");
		tmpstr.append(pRspInfo->ErrorMsg);
        strcpy(tmpchar,tmpstr.c_str());
        char errMsg[1024];
        dii->codeCC->convert(tmpchar,strlen(tmpchar),errMsg,1024);
        //cerr << "--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << errMsg << endl;
        LOG(ERROR)<<errMsg;
        //loglist.push_back(tmpstr);
        //tradequeue.push_back(tmpstr);
	}
	return bResult;
}

bool CTraderSpi::IsMyOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->FrontID == FRONT_ID) &&
			(pOrder->SessionID == SESSION_ID) &&
			(strcmp(pOrder->OrderRef, ORDER_REF) == 0));
}

bool CTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}
//提取投资者报单信息
string getInvestorOrderInsertInfo(CThostFtdcInputOrderField *order)
{
	///经纪公司代码
	char	*BrokerID = order->BrokerID;
	///投资者代码
	char	*InvestorID = order->InvestorID;
	///合约代码
	char	*InstrumentID = order->InstrumentID;
	///报单引用
	char	*OrderRef = order->OrderRef;
	///用户代码
	char	*UserID = order->UserID;
	///报单价格条件
	char	OrderPriceType = order->OrderPriceType;
	///买卖方向
	char	Direction[] = {order->Direction,'\0'};
	///组合开平标志
	char	*CombOffsetFlag =order->CombOffsetFlag;
	///组合投机套保标志
	char	*CombHedgeFlag = order->CombHedgeFlag;
	///价格
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100];
	sprintf(LimitPrice,"%f",limitPrice);
	///数量
	TThostFtdcVolumeType	volumeTotalOriginal = order->VolumeTotalOriginal;
	char VolumeTotalOriginal[100];
	sprintf(VolumeTotalOriginal,"%d",volumeTotalOriginal);
	///有效期类型
	TThostFtdcTimeConditionType	TimeCondition = order->TimeCondition;
	///GTD日期
	//TThostFtdcDateType	GTDDate = order->GTDDate;
	///成交量类型
	TThostFtdcVolumeConditionType	VolumeCondition[] = {order->VolumeCondition,'\0'};
	///最小成交量
	TThostFtdcVolumeType	MinVolume = order->MinVolume;
	///触发条件
	TThostFtdcContingentConditionType	ContingentCondition = order->ContingentCondition;
	///止损价
	TThostFtdcPriceType	StopPrice = order->StopPrice;
	///强平原因
	TThostFtdcForceCloseReasonType	ForceCloseReason = order->ForceCloseReason;
	///自动挂起标志
	TThostFtdcBoolType	IsAutoSuspend = order->IsAutoSuspend;
	///业务单元
	//TThostFtdcBusinessUnitType	BusinessUnit = order->BusinessUnit;
	///请求编号
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100];
	sprintf(RequestID,"%d",requestID);
	///用户强评标志
	TThostFtdcBoolType	UserForceClose = order->UserForceClose;

	string ordreInfo;
	if (!isOrderInsertDefFieldReady)
	{
		ordreInfo.append("BrokerID\t");
		ordreInfo.append("InvestorID\t");
		ordreInfo.append("InstrumentID\t");
		ordreInfo.append("OrderRef\t");
		ordreInfo.append("UserID\t");
		ordreInfo.append("Direction\t");
		ordreInfo.append("CombOffsetFlag\t");
		ordreInfo.append("CombHedgeFlag\t");
		ordreInfo.append("LimitPrice\t");
		ordreInfo.append("VolumeTotalOriginal\t");
		ordreInfo.append("VolumeCondition\t");
		ordreInfo.append("RequestID\n");
		isOrderInsertDefFieldReady = true;
	}
	ordreInfo.append(BrokerID);ordreInfo.append("\t");
	ordreInfo.append(InvestorID);ordreInfo.append("\t");
	ordreInfo.append(InstrumentID);ordreInfo.append("\t");
	ordreInfo.append(OrderRef);ordreInfo.append("\t");
	ordreInfo.append(UserID);ordreInfo.append("\t");
	ordreInfo.append(Direction);ordreInfo.append("\t");
	ordreInfo.append(CombOffsetFlag);ordreInfo.append("\t");
	ordreInfo.append(CombHedgeFlag);ordreInfo.append("\t");
	ordreInfo.append(LimitPrice);ordreInfo.append("\t");
	ordreInfo.append(VolumeTotalOriginal);ordreInfo.append("\t");
	ordreInfo.append(VolumeCondition);ordreInfo.append("\t");
	ordreInfo.append(RequestID);ordreInfo.append("\t");
	cout << ordreInfo <<endl;
    //loglist.push_back(ordreInfo);
	return ordreInfo;
}
//提取投资者报单信息
string getInvestorOrderInsertInfoByDelimater(CThostFtdcInputOrderField *order)
{
	///经纪公司代码
	char	*BrokerID = order->BrokerID;
	///投资者代码
	char	*InvestorID = order->InvestorID;
	///合约代码
	char	*InstrumentID = order->InstrumentID;
	///报单引用
	char	*OrderRef = order->OrderRef;
	///用户代码
	char	*UserID = order->UserID;
	///报单价格条件
	char	OrderPriceType = order->OrderPriceType;
	///买卖方向
	char	Direction[] = {order->Direction,'\0'};
	///组合开平标志
	char	*CombOffsetFlag =order->CombOffsetFlag;
	///组合投机套保标志
	char	*CombHedgeFlag = order->CombHedgeFlag;
	///价格
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100];
	sprintf(LimitPrice,"%f",limitPrice);
	///数量
	TThostFtdcVolumeType	volumeTotalOriginal = order->VolumeTotalOriginal;
	char VolumeTotalOriginal[100];
	sprintf(VolumeTotalOriginal,"%d",volumeTotalOriginal);
	///有效期类型
	TThostFtdcTimeConditionType	TimeCondition = order->TimeCondition;
	///GTD日期
	//TThostFtdcDateType	GTDDate = order->GTDDate;
	///成交量类型
	TThostFtdcVolumeConditionType	VolumeCondition[] = {order->VolumeCondition,'\0'};
	///最小成交量
	TThostFtdcVolumeType	MinVolume = order->MinVolume;
	///触发条件
	TThostFtdcContingentConditionType	ContingentCondition = order->ContingentCondition;
	///止损价
	TThostFtdcPriceType	StopPrice = order->StopPrice;
	///强平原因
	TThostFtdcForceCloseReasonType	ForceCloseReason = order->ForceCloseReason;
	///自动挂起标志
	TThostFtdcBoolType	IsAutoSuspend = order->IsAutoSuspend;
	///业务单元
	//TThostFtdcBusinessUnitType	BusinessUnit = order->BusinessUnit;
	///请求编号
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100];
	sprintf(RequestID,"%d",requestID);
	///用户强评标志
	TThostFtdcBoolType	UserForceClose = order->UserForceClose;

	string ordreInfo;
	if (!isOrderInsertDefFieldReady)
	{
// 		ordreInfo.append("BrokerID\t");
// 		ordreInfo.append("InvestorID\t");
// 		ordreInfo.append("InstrumentID\t");
// 		ordreInfo.append("OrderRef\t");
// 		ordreInfo.append("UserID\t");
// 		ordreInfo.append("Direction\t");
// 		ordreInfo.append("CombOffsetFlag\t");
// 		ordreInfo.append("CombHedgeFlag\t");
// 		ordreInfo.append("LimitPrice\t");
// 		ordreInfo.append("VolumeTotalOriginal\t");
// 		ordreInfo.append("VolumeCondition\t");
// 		ordreInfo.append("RequestID\n");
		isOrderInsertDefFieldReady = true;
	}
	ordreInfo.append("BrokerID=");ordreInfo.append(BrokerID);ordreInfo.append(sep);
	ordreInfo.append("InvestorID=");ordreInfo.append(InvestorID);ordreInfo.append(sep);
	ordreInfo.append("InstrumentID=");ordreInfo.append(InstrumentID);ordreInfo.append(sep);
	ordreInfo.append("OrderRef=");ordreInfo.append(OrderRef);ordreInfo.append(sep);
	ordreInfo.append("UserID=");ordreInfo.append(UserID);ordreInfo.append(sep);
	ordreInfo.append("Direction=");ordreInfo.append(Direction);ordreInfo.append(sep);
	ordreInfo.append("CombOffsetFlag=");ordreInfo.append(CombOffsetFlag);ordreInfo.append(sep);
	ordreInfo.append("CombHedgeFlag=");ordreInfo.append(CombHedgeFlag);ordreInfo.append(sep);
	ordreInfo.append("LimitPrice=");ordreInfo.append(LimitPrice);ordreInfo.append(sep);
	ordreInfo.append("VolumeTotalOriginal=");ordreInfo.append(VolumeTotalOriginal);ordreInfo.append(sep);
	ordreInfo.append("VolumeCondition=");ordreInfo.append(VolumeCondition);ordreInfo.append(sep);
	ordreInfo.append("RequestID=");ordreInfo.append(RequestID);ordreInfo.append(sep);
	cout << ordreInfo <<endl;
    ////loglist.push_back(ordreInfo);
	return ordreInfo;
}
//提取委托回报信息
string getRtnOrderInfoByDelimater(CThostFtdcOrderField *order)
{
	///经纪公司代码
	char	*BrokerID = order->BrokerID;
	///投资者代码
	char	*InvestorID = order->InvestorID;
	///合约代码
	char	*InstrumentID = order->InstrumentID;
	///报单引用
	char	*OrderRef = order->OrderRef;
	///用户代码
	char	*UserID = order->UserID;
	///报单价格条件
	char	OrderPriceType = order->OrderPriceType;
	///买卖方向
	char	Direction[] = {order->Direction,'\0'};
	///组合开平标志
	char	*CombOffsetFlag =order->CombOffsetFlag;
	///组合投机套保标志
	char	*CombHedgeFlag = order->CombHedgeFlag;
	///价格
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100];
	sprintf(LimitPrice,"%f",limitPrice);
	///数量
	TThostFtdcVolumeType	volumeTotalOriginal = order->VolumeTotalOriginal;
	char VolumeTotalOriginal[100];
	sprintf(VolumeTotalOriginal,"%d",volumeTotalOriginal);
	///有效期类型
	TThostFtdcTimeConditionType	TimeCondition = order->TimeCondition;
	///GTD日期
	//TThostFtdcDateType	GTDDate = order->GTDDate;
	///成交量类型
	TThostFtdcVolumeConditionType	VolumeCondition[] = {order->VolumeCondition,'\0'};
	///最小成交量
	TThostFtdcVolumeType	minVolume = order->MinVolume;
	char MinVolume[100];
	sprintf(MinVolume,"%d",minVolume);
	///触发条件
	TThostFtdcContingentConditionType	ContingentCondition = order->ContingentCondition;
	///止损价
	TThostFtdcPriceType	stopPrice = order->StopPrice;
	char StopPrice[100];
	sprintf(StopPrice,"%d",stopPrice);
	///强平原因
	TThostFtdcForceCloseReasonType	ForceCloseReason = order->ForceCloseReason;
	///自动挂起标志
	TThostFtdcBoolType	isAutoSuspend = order->IsAutoSuspend;
	char IsAutoSuspend[100];
	sprintf(IsAutoSuspend,"%d",isAutoSuspend);
	///业务单元
	//TThostFtdcBusinessUnitType	BusinessUnit = order->BusinessUnit;
	///请求编号
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100];
	sprintf(RequestID,"%d",requestID);
	///本地报单编号
    char *OrderLocalID = order->OrderLocalID;
	///交易所代码
	char * ExchangeID = order->ExchangeID;
	///会员代码
	char* ParticipantID = order->ParticipantID;
	///客户代码
	char*	ClientID = order->ClientID;
	///合约在交易所的代码
	char*	ExchangeInstID = order->ExchangeInstID;
	///交易所交易员代码
	char*	TraderID = order->TraderID;
	///安装编号
	int	installID = order->InstallID;
	char InstallID[10];
	sprintf(InstallID,"%d",installID);
	///报单提交状态
	char	OrderSubmitStatus[] = {order->OrderSubmitStatus,'\0'};
	///报单提示序号
	int	notifySequence = order->NotifySequence;
	char NotifySequence[20];
	sprintf(NotifySequence,"%d",notifySequence);
	///交易日
	char*	TradingDay = order->TradingDay;
	///结算编号
	int	settlementID = order->SettlementID;
	char SettlementID[10];
	sprintf(SettlementID,"%d",settlementID);
	///报单编号
	char*	OrderSysID = order->OrderSysID;
	///报单来源
	char	OrderSource[] = {order->OrderSource,'\0'};
	///报单状态
	char	OrderStatus[] = {order->OrderStatus,'\0'};
	///报单类型
	//TThostFtdcOrderTypeType	OrderType;
	///今成交数量
	int	volumeTraded = order->VolumeTraded;
	char VolumeTraded[10];
	sprintf(VolumeTraded,"%d",volumeTraded);
	///剩余数量
	int	volumeTotal = order->VolumeTotal;
	char VolumeTotal[10];
	sprintf(VolumeTotal,"%d",volumeTotal);
	///报单日期
	char*	InsertDate = order->InsertDate;
	///委托时间
	char*	InsertTime = order->InsertTime;
	///激活时间
	char*	ActiveTime = order->ActiveTime;
	///挂起时间
	char*	SuspendTime = order->SuspendTime;
	///最后修改时间
	char*	UpdateTime = order->UpdateTime;
	///撤销时间
	char*	CancelTime = order->CancelTime;
	///最后修改交易所交易员代码
	char*	ActiveTraderID = order->ActiveTraderID;
	///结算会员编号
	char*	ClearingPartID = order->ClearingPartID;
	///序号
	int	sequenceNo = order->SequenceNo;
	char SequenceNo[20];
	sprintf(SequenceNo,"%d",sequenceNo);
	///前置编号
	int	frontID = order->FrontID;
	char FrontID[20];
	sprintf(FrontID,"%d",frontID);
	///会话编号
	int	sessionID = order->SessionID;
	char SessionID[20];
	sprintf(SessionID,"%d",sessionID);
	///用户端产品信息
	char*	UserProductInfo = order->UserProductInfo;
	///状态信息
	char*	StatusMsg = order->StatusMsg;
	///用户强评标志
	int	userForceClose = order->UserForceClose;
	char UserForceClose[20];
	sprintf(UserForceClose,"%d",userForceClose);
	///操作用户代码
	char*	ActiveUserID = order->ActiveUserID;
	///经纪公司报单编号
	int	brokerOrderSeq = order->BrokerOrderSeq;
	char BrokerOrderSeq[20];
	sprintf(BrokerOrderSeq,"%d",brokerOrderSeq);


	string ordreInfo;
	ordreInfo.append("BrokerID=");ordreInfo.append(BrokerID);ordreInfo.append(sep);
	ordreInfo.append("InvestorID=");ordreInfo.append(InvestorID);ordreInfo.append(sep);
    ordreInfo.append("frontID=");ordreInfo.append(boost::lexical_cast<string>(order->FrontID));ordreInfo.append(sep);
    ordreInfo.append("sessionID=");ordreInfo.append(boost::lexical_cast<string>(order->SessionID));ordreInfo.append(sep);
	ordreInfo.append("InstrumentID=");ordreInfo.append(InstrumentID);ordreInfo.append(sep);
	ordreInfo.append("OrderRef=");ordreInfo.append(OrderRef);ordreInfo.append(sep);
	ordreInfo.append("UserID=");ordreInfo.append(UserID);ordreInfo.append(sep);
	//ordreInfo.append("OrderPriceType=");ordreInfo.append(OrderPriceType);ordreInfo.append(sep);
	ordreInfo.append("Direction=");ordreInfo.append(Direction);ordreInfo.append(sep);
	ordreInfo.append("CombOffsetFlag=");ordreInfo.append(CombOffsetFlag);ordreInfo.append(sep);
	ordreInfo.append("CombHedgeFlag=");ordreInfo.append(CombHedgeFlag);ordreInfo.append(sep);
	ordreInfo.append("LimitPrice=");ordreInfo.append(LimitPrice);ordreInfo.append(sep);
	ordreInfo.append("VolumeTotalOriginal=");ordreInfo.append(VolumeTotalOriginal);ordreInfo.append(sep);
	//ordreInfo.append("TimeCondition=");ordreInfo.append(TimeCondition);ordreInfo.append(sep);
	ordreInfo.append("MinVolume=");ordreInfo.append(MinVolume);ordreInfo.append(sep);
	ordreInfo.append("VolumeCondition=");ordreInfo.append(VolumeCondition);ordreInfo.append(sep);
	//ordreInfo.append("ContingentCondition=");ordreInfo.append(ContingentCondition);ordreInfo.append(sep);
	ordreInfo.append("StopPrice=");ordreInfo.append(StopPrice);ordreInfo.append(sep);
	//ordreInfo.append("ForceCloseReason=");ordreInfo.append(ForceCloseReason);ordreInfo.append(sep);
	ordreInfo.append("IsAutoSuspend=");ordreInfo.append(IsAutoSuspend);ordreInfo.append(sep);
	ordreInfo.append("RequestID=");ordreInfo.append(RequestID);ordreInfo.append(sep);

	ordreInfo.append("OrderLocalID=");ordreInfo.append(OrderLocalID);ordreInfo.append(sep);
	ordreInfo.append("ExchangeID=");ordreInfo.append(ExchangeID);ordreInfo.append(sep);
	ordreInfo.append("ParticipantID=");ordreInfo.append(ParticipantID);ordreInfo.append(sep);
	ordreInfo.append("ClientID=");ordreInfo.append(ClientID);ordreInfo.append(sep);
	ordreInfo.append("ExchangeInstID=");ordreInfo.append(ExchangeInstID);ordreInfo.append(sep);
	ordreInfo.append("TraderID=");ordreInfo.append(TraderID);ordreInfo.append(sep);
	ordreInfo.append("InstallID=");ordreInfo.append(InstallID);ordreInfo.append(sep);
	ordreInfo.append("OrderSubmitStatus=");ordreInfo.append(OrderSubmitStatus);ordreInfo.append(sep);
	ordreInfo.append("NotifySequence=");ordreInfo.append(NotifySequence);ordreInfo.append(sep);
	ordreInfo.append("TradingDay=");ordreInfo.append(TradingDay);ordreInfo.append(sep);
	ordreInfo.append("SettlementID=");ordreInfo.append(SettlementID);ordreInfo.append(sep);
	ordreInfo.append("OrderSysID=");ordreInfo.append(OrderSysID);ordreInfo.append(sep);
	ordreInfo.append("OrderSource=");ordreInfo.append(OrderSource);ordreInfo.append(sep);
	ordreInfo.append("OrderStatus=");ordreInfo.append(OrderStatus);ordreInfo.append(sep);
	ordreInfo.append("VolumeTraded=");ordreInfo.append(VolumeTraded);ordreInfo.append(sep);
	ordreInfo.append("VolumeTotal=");ordreInfo.append(VolumeTotal);ordreInfo.append(sep);
	ordreInfo.append("InsertDate=");ordreInfo.append(InsertDate);ordreInfo.append(sep);
	ordreInfo.append("InsertTime=");ordreInfo.append(InsertTime);ordreInfo.append(sep);
	ordreInfo.append("ActiveTime=");ordreInfo.append(ActiveTime);ordreInfo.append(sep);
	ordreInfo.append("SuspendTime=");ordreInfo.append(SuspendTime);ordreInfo.append(sep);
	ordreInfo.append("UpdateTime=");ordreInfo.append(UpdateTime);ordreInfo.append(sep);
	ordreInfo.append("CancelTime=");ordreInfo.append(CancelTime);ordreInfo.append(sep);
	ordreInfo.append("ActiveTraderID=");ordreInfo.append(ActiveTraderID);ordreInfo.append(sep);
	ordreInfo.append("ClearingPartID=");ordreInfo.append(ClearingPartID);ordreInfo.append(sep);
	ordreInfo.append("SequenceNo=");ordreInfo.append(SequenceNo);ordreInfo.append(sep);
	ordreInfo.append("FrontID=");ordreInfo.append(FrontID);ordreInfo.append(sep);
	ordreInfo.append("SessionID=");ordreInfo.append(SessionID);ordreInfo.append(sep);
	ordreInfo.append("UserProductInfo=");ordreInfo.append(UserProductInfo);ordreInfo.append(sep);
    char errMsg[1024];
    dii->codeCC->convert(StatusMsg,strlen(StatusMsg),errMsg,1024);
    ordreInfo.append("StatusMsg=");ordreInfo.append(errMsg);ordreInfo.append(sep);
	ordreInfo.append("UserForceClose=");ordreInfo.append(UserForceClose);ordreInfo.append(sep);
	ordreInfo.append("ActiveUserID=");ordreInfo.append(ActiveUserID);ordreInfo.append(sep);
	ordreInfo.append("BrokerOrderSeq=");ordreInfo.append(BrokerOrderSeq);ordreInfo.append(sep);

	cout << ordreInfo <<endl;
    ////loglist.push_back(ordreInfo);
	return ordreInfo;
}
//提取成交回报信息
string getRtnTradeInfoByDelimater(CThostFtdcTradeField *order)
{
	///经纪公司代码
	char	*BrokerID = order->BrokerID;
	///投资者代码
	char	*InvestorID = order->InvestorID;
	///合约代码
	char	*InstrumentID = order->InstrumentID;
	///报单引用
	char	*OrderRef = order->OrderRef;
	///用户代码
	char	*UserID = order->UserID;
	///交易所代码
	char * ExchangeID = order->ExchangeID;
	///成交编号
	char*	TradeID = order->TradeID;
	///买卖方向
	char	Direction[] = {order->Direction,'\0'};
	///报单编号
	char*	OrderSysID = order->OrderSysID;
	//会员代码
	char*	ParticipantID = order->ParticipantID;
	///客户代码
	char*	ClientID = order->ClientID;
	///合约在交易所的代码
	char*	ExchangeInstID = order->ExchangeInstID;
	///开平标志
	char	OffsetFlag[] = {order->OffsetFlag,'\0'};
	///投机套保标志
	char	HedgeFlag[] = {order->HedgeFlag,'\0'};
	///价格
	TThostFtdcPriceType	price = order->Price;
	char Price[100];
	sprintf(Price,"%f",price);
	///数量
	TThostFtdcVolumeType	volume = order->Volume;
	char Volume[100];
	sprintf(Volume,"%d",volume);
	///成交时期
	char*	TradeDate = order->TradeDate;
	///成交时间
	char*	TradeTime = order->TradeTime;
	///成交类型
	char	TradeType[] = {order->TradeType,'\0'};
	///成交价来源
	char	PriceSource[] = {order->PriceSource,'\0'};
	///交易所交易员代码
	char*	TraderID = order->TraderID;
	///本地报单编号
    char *OrderLocalID = order->OrderLocalID;
	///结算编号
	int	settlementID = order->SettlementID;
	char SettlementID[10];
	sprintf(SettlementID,"%d",settlementID);
	///结算会员编号
	char*	ClearingPartID = order->ClearingPartID;
	///序号
	int	sequenceNo = order->SequenceNo;
	char SequenceNo[20];
	sprintf(SequenceNo,"%d",sequenceNo);
	///交易日
	char*	TradingDay = order->TradingDay;
	///经纪公司报单编号
	int	brokerOrderSeq = order->BrokerOrderSeq;
	char BrokerOrderSeq[20];
	sprintf(BrokerOrderSeq,"%d",brokerOrderSeq);


	string ordreInfo;
	ordreInfo.append("BrokerID=");ordreInfo.append(BrokerID);ordreInfo.append(sep);
	ordreInfo.append("InvestorID=");ordreInfo.append(InvestorID);ordreInfo.append(sep);
	ordreInfo.append("InstrumentID=");ordreInfo.append(InstrumentID);ordreInfo.append(sep);
	ordreInfo.append("OrderRef=");ordreInfo.append(OrderRef);ordreInfo.append(sep);
	ordreInfo.append("UserID=");ordreInfo.append(UserID);ordreInfo.append(sep);
	ordreInfo.append("ExchangeID=");ordreInfo.append(ExchangeID);ordreInfo.append(sep);
	ordreInfo.append("TradeID=");ordreInfo.append(TradeID);ordreInfo.append(sep);
	ordreInfo.append("Direction=");ordreInfo.append(Direction);ordreInfo.append(sep);
	ordreInfo.append("OrderSysID=");ordreInfo.append(OrderSysID);ordreInfo.append(sep);
	ordreInfo.append("ParticipantID=");ordreInfo.append(ParticipantID);ordreInfo.append(sep);
	ordreInfo.append("ClientID=");ordreInfo.append(ClientID);ordreInfo.append(sep);
	ordreInfo.append("ExchangeInstID=");ordreInfo.append(ExchangeInstID);ordreInfo.append(sep);
	ordreInfo.append("OffsetFlag=");ordreInfo.append(OffsetFlag);ordreInfo.append(sep);
	ordreInfo.append("HedgeFlag=");ordreInfo.append(HedgeFlag);ordreInfo.append(sep);
	ordreInfo.append("Price=");ordreInfo.append(Price);ordreInfo.append(sep);
	ordreInfo.append("Volume=");ordreInfo.append(Volume);ordreInfo.append(sep);
	ordreInfo.append("TradeDate=");ordreInfo.append(TradeDate);ordreInfo.append(sep);
	ordreInfo.append("TradeTime=");ordreInfo.append(TradeTime);ordreInfo.append(sep);
	ordreInfo.append("TradeType=");ordreInfo.append(TradeType);ordreInfo.append(sep);
	ordreInfo.append("PriceSource=");ordreInfo.append(PriceSource);ordreInfo.append(sep);

	ordreInfo.append("TraderID=");ordreInfo.append(TraderID);ordreInfo.append(sep);
	ordreInfo.append("OrderLocalID=");ordreInfo.append(OrderLocalID);ordreInfo.append(sep);
	ordreInfo.append("ClearingPartID=");ordreInfo.append(ClearingPartID);ordreInfo.append(sep);
	//ordreInfo.append("BusinessUnit=");ordreInfo.append(BusinessUnit);ordreInfo.append(sep);
	//ordreInfo.append("TimeCondition=");ordreInfo.append(TimeCondition);ordreInfo.append(sep);
	ordreInfo.append("SequenceNo=");ordreInfo.append(SequenceNo);ordreInfo.append(sep);
	ordreInfo.append("TradingDay=");ordreInfo.append(TradingDay);ordreInfo.append(sep);
	//ordreInfo.append("ContingentCondition=");ordreInfo.append(ContingentCondition);ordreInfo.append(sep);
	ordreInfo.append("SettlementID=");ordreInfo.append(SettlementID);ordreInfo.append(sep);
	//ordreInfo.append("ForceCloseReason=");ordreInfo.append(ForceCloseReason);ordreInfo.append(sep);
	ordreInfo.append("BrokerOrderSeq=");ordreInfo.append(BrokerOrderSeq);ordreInfo.append(sep);

	cout << ordreInfo <<endl;
    ////loglist.push_back(ordreInfo);
	return ordreInfo;
}
//提取投资者报单信息
string getOrderActionInfoByDelimater(CThostFtdcInputOrderActionField *order)
{
	///经纪公司代码
	char	*BrokerID = order->BrokerID;
	///投资者代码
	char	*InvestorID = order->InvestorID;
	///合约代码
	char	*InstrumentID = order->InstrumentID;
	///报单操作引用
	TThostFtdcOrderActionRefType	orderActionRef = order->OrderActionRef;
	char OrderActionRef[20] = {"\0"};
	sprintf(OrderActionRef,"%d",orderActionRef);
	///报单引用
	char	*OrderRef = order->OrderRef;
	///用户代码
	char	*UserID = order->UserID;
	///请求编号
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100] = {"\0"};
	sprintf(RequestID,"%d",requestID);
	///前置编号 int
	TThostFtdcFrontIDType	frontID = order->FrontID;
	char FrontID[7] = {"\0"};
	sprintf(FrontID,"%d",frontID);
	///会话编号 int
	TThostFtdcSessionIDType	sessionID = order->SessionID;
	char SessionID[20] = {"\0"};
	sprintf(SessionID,"%d",sessionID);
	///交易所代码
	char*	ExchangeID = order->ExchangeID;
	///报单编号
	char*	OrderSysID = order->OrderSysID;
	///操作标志
	char	ActionFlag[] = {order->ActionFlag,'\0'};
	///价格 double
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100]={"\0"};
	sprintf(LimitPrice,"%f",limitPrice);
	///数量变化int
	TThostFtdcVolumeType	volumeChange = order->VolumeChange;
	cout<<order->VolumeChange<<endl;
	cout<<volumeChange<<endl;
	char VolumeChange[100]={"\0"};
	sprintf(VolumeChange,"%d",volumeChange);
	cout<<VolumeChange<<endl;

	string ordreInfo="\0";
	ordreInfo.append("BrokerID=");ordreInfo.append(BrokerID);ordreInfo.append(sep);
	ordreInfo.append("InvestorID=");ordreInfo.append(InvestorID);ordreInfo.append(sep);
	ordreInfo.append("InstrumentID=");ordreInfo.append(InstrumentID);ordreInfo.append(sep);
	ordreInfo.append("OrderActionRef=");ordreInfo.append(OrderActionRef);ordreInfo.append(sep);
	ordreInfo.append("OrderRef=");ordreInfo.append(OrderRef);ordreInfo.append(sep);
	ordreInfo.append("UserID=");ordreInfo.append(UserID);ordreInfo.append(sep);
	ordreInfo.append("RequestID=");ordreInfo.append(RequestID);ordreInfo.append(sep);
	ordreInfo.append("FrontID=");ordreInfo.append(FrontID);ordreInfo.append(sep);
	ordreInfo.append("SessionID=");ordreInfo.append(SessionID);ordreInfo.append(sep);
	ordreInfo.append("ExchangeID=");ordreInfo.append(ExchangeID);ordreInfo.append(sep);
	ordreInfo.append("OrderSysID=");ordreInfo.append(OrderSysID);ordreInfo.append(sep);
	ordreInfo.append("ActionFlag=");ordreInfo.append(ActionFlag);ordreInfo.append(sep);
	ordreInfo.append("LimitPrice=");ordreInfo.append(LimitPrice);ordreInfo.append(sep);
	ordreInfo.append("VolumeChange=");ordreInfo.append(VolumeChange);ordreInfo.append(sep);
	cout << ordreInfo <<endl;
    ////loglist.push_back(ordreInfo);
	return ordreInfo;
}
//将报单回报响应信息写入文件保存
void saveRspOrderInsertInfo(CThostFtdcInputOrderField *pInputOrder)
{
	saveInvestorOrderInsertHedge(pInputOrder, "d:\\test\\rsporderinsert.txt");
}
//获取交易所回报响应
string getRtnOrder(CThostFtdcOrderField *pOrder)
{
	///经纪公司代码
	char	*BrokerID = pOrder->BrokerID;
	///投资者代码
	char	*InvestorID = pOrder->InvestorID;
	///合约代码
	char	*InstrumentID = pOrder->InstrumentID;
	///报单引用
	char	*OrderRef = pOrder->OrderRef;
	///用户代码
	TThostFtdcUserIDType	UserID;
	///报单价格条件
	TThostFtdcOrderPriceTypeType	OrderPriceType;
	///买卖方向
	TThostFtdcDirectionType	Direction = pOrder->Direction;
	///组合开平标志
	TThostFtdcCombOffsetFlagType	CombOffsetFlag;
	///组合投机套保标志
	TThostFtdcCombHedgeFlagType	CombHedgeFlag;
	///价格
	TThostFtdcPriceType	LimitPrice;
	///数量
	TThostFtdcVolumeType	VolumeTotalOriginal;
	///有效期类型
	TThostFtdcTimeConditionType	TimeCondition;
	///GTD日期
	TThostFtdcDateType	GTDDate;
	///成交量类型
	TThostFtdcVolumeConditionType	VolumeCondition;
	///最小成交量
	TThostFtdcVolumeType	MinVolume;
	///触发条件
	TThostFtdcContingentConditionType	ContingentCondition;
	///止损价
	TThostFtdcPriceType	StopPrice;
	///强平原因
	TThostFtdcForceCloseReasonType	ForceCloseReason;
	///自动挂起标志
	TThostFtdcBoolType	IsAutoSuspend;
	///业务单元
	TThostFtdcBusinessUnitType	BusinessUnit;
	///请求编号
	TThostFtdcRequestIDType	RequestID = pOrder->RequestID;
	char cRequestId[100];
	sprintf(cRequestId,"%d",RequestID);
	///本地报单编号
	char	*OrderLocalID = pOrder->OrderLocalID;
	///交易所代码
	TThostFtdcExchangeIDType	ExchangeID;
	///会员代码
	TThostFtdcParticipantIDType	ParticipantID;
	///客户代码
	char	*ClientID = pOrder->ClientID;
	///合约在交易所的代码
	TThostFtdcExchangeInstIDType	ExchangeInstID;
	///交易所交易员代码
	TThostFtdcTraderIDType	TraderID;
	///安装编号
	TThostFtdcInstallIDType	InstallID;
	///报单提交状态
	char	OrderSubmitStatus = pOrder->OrderSubmitStatus;
	char cOrderSubmitStatus[] = {OrderSubmitStatus,'\0'};
	//sprintf(cOrderSubmitStatus,"%s",OrderSubmitStatus);
	///报单状态
	TThostFtdcOrderStatusType	OrderStatus = pOrder->OrderStatus;
	char cOrderStatus[] = {OrderStatus,'\0'};
	//sprintf(cOrderStatus,"%s",OrderStatus);
	///报单提示序号
	TThostFtdcSequenceNoType	NotifySequence;
	///交易日
	TThostFtdcDateType	TradingDay;
	///结算编号
	TThostFtdcSettlementIDType	SettlementID;
	///报单编号
	char	*OrderSysID = pOrder->OrderSysID;
	char replace[] = {'n','\0'};
	if (strlen(OrderSysID) == 0)
	{
		OrderSysID = replace;
	}
	///报单来源
	TThostFtdcOrderSourceType	OrderSource;
	///报单类型
	TThostFtdcOrderTypeType	OrderType;
	///今成交数量
	TThostFtdcVolumeType	VolumeTraded = pOrder->VolumeTraded;
	char cVolumeTraded[100];
	sprintf(cVolumeTraded,"%d",VolumeTraded);
	///剩余数量
	TThostFtdcVolumeType	VolumeTotal = pOrder->VolumeTotal;
	char iVolumeTotal[100];
	sprintf(iVolumeTotal,"%d",VolumeTotal);
	///报单日期
	TThostFtdcDateType	InsertDate;
	///委托时间
	TThostFtdcTimeType	InsertTime;
	///激活时间
	TThostFtdcTimeType	ActiveTime;
	///挂起时间
	TThostFtdcTimeType	SuspendTime;
	///最后修改时间
	TThostFtdcTimeType	UpdateTime;
	///撤销时间
	TThostFtdcTimeType	CancelTime;
	///最后修改交易所交易员代码
	TThostFtdcTraderIDType	ActiveTraderID;
	///结算会员编号
	TThostFtdcParticipantIDType	ClearingPartID;
	///序号
	TThostFtdcSequenceNoType	SequenceNo;
	///前置编号
	TThostFtdcFrontIDType	FrontID;
	///会话编号
	TThostFtdcSessionIDType	SessionID;
	///用户端产品信息
	TThostFtdcProductInfoType	UserProductInfo;
	///状态信息
	char	*StatusMsg = pOrder->StatusMsg;
	///用户强评标志
	TThostFtdcBoolType	UserForceClose;
	///操作用户代码
	TThostFtdcUserIDType	ActiveUserID;
	///经纪公司报单编号
	TThostFtdcSequenceNoType	BrokerOrderSeq;
	string info;
	info.append(BrokerID);info.append("\t");
	info.append(InvestorID);info.append("\t");
	info.append(InstrumentID);info.append("\t");
	info.append(OrderRef);info.append("\t");
	info.append(cRequestId);info.append("\t");
	info.append(ClientID);info.append("\t");
	info.append(OrderSysID);info.append("\t");
	info.append(cVolumeTraded);info.append("\t");
	info.append(iVolumeTotal);info.append("\t");
	info.append(StatusMsg);info.append("\t");
	info.append(cOrderSubmitStatus);info.append("\t");
	info.append(cOrderStatus);info.append("\t");
	return info;
}

//将交易所报单回报响应写入文件保存
void saveRtnOrder(CThostFtdcOrderField *pOrder)
{
	string info = getRtnOrder(pOrder);
    //loglist.push_back(info);
}
//将投资者持仓信息写入文件保存
string storeInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition)
{
	///合约代码
	char	*InstrumentID = pInvestorPosition->InstrumentID;
	///经纪公司代码
	char	*BrokerID = pInvestorPosition->BrokerID;
	///投资者代码
	char	*InvestorID = pInvestorPosition->InvestorID;
	///持仓多空方向
	TThostFtdcPosiDirectionType	dir = pInvestorPosition->PosiDirection;
	char PosiDirection[] = {dir,'\0'};
	///投机套保标志
	TThostFtdcHedgeFlagType	flag = pInvestorPosition->HedgeFlag;
	char HedgeFlag[] = {flag,'\0'};
	///持仓日期
	TThostFtdcPositionDateType	positionDate = pInvestorPosition->PositionDate;
	char PositionDate[] = {positionDate,'\0'};
	///上日持仓
	TThostFtdcVolumeType	ydPosition = pInvestorPosition->YdPosition;
	char YdPosition[100];
	sprintf(YdPosition,"%d",ydPosition);
	///今日持仓
	TThostFtdcVolumeType	position = pInvestorPosition->Position;
	/*
	if (position == 0)
	{
		return 0;
	}*/
	char Position[100];
	sprintf(Position,"%d",position);
	///多头冻结
	TThostFtdcVolumeType	LongFrozen = pInvestorPosition->LongFrozen;
	///空头冻结
	TThostFtdcVolumeType	ShortFrozen = pInvestorPosition->ShortFrozen;
	///开仓冻结金额
	TThostFtdcMoneyType	LongFrozenAmount = pInvestorPosition->LongFrozenAmount;
	///开仓冻结金额
	TThostFtdcMoneyType	ShortFrozenAmount = pInvestorPosition->ShortFrozenAmount;
	///开仓量
	TThostFtdcVolumeType	openVolume = pInvestorPosition->OpenVolume;
	char OpenVolume[100] ;
	sprintf(OpenVolume,"%d",openVolume);
	///平仓量
	TThostFtdcVolumeType	closeVolume = pInvestorPosition->CloseVolume;
	char CloseVolume[100];
	sprintf(CloseVolume,"%d",closeVolume);
	///开仓金额
	TThostFtdcMoneyType	OpenAmount = pInvestorPosition->OpenAmount;
	///平仓金额
	TThostFtdcMoneyType	CloseAmount = pInvestorPosition->CloseAmount;
	///持仓成本
	TThostFtdcMoneyType	positionCost = pInvestorPosition->PositionCost;
	char PositionCost[100];
	sprintf(PositionCost,"%f",positionCost);
	///上次占用的保证金
	TThostFtdcMoneyType	PreMargin = pInvestorPosition->PreMargin;
	///占用的保证金
	TThostFtdcMoneyType	UseMargin = pInvestorPosition->UseMargin;
	///冻结的保证金
	TThostFtdcMoneyType	FrozenMargin = pInvestorPosition->FrozenMargin;
	///冻结的资金
	TThostFtdcMoneyType	FrozenCash = pInvestorPosition->FrozenCash;
	///冻结的手续费
	TThostFtdcMoneyType	FrozenCommission = pInvestorPosition->FrozenCommission;
	///资金差额
	TThostFtdcMoneyType	CashIn = pInvestorPosition->CashIn;
	///手续费
	TThostFtdcMoneyType	Commission = pInvestorPosition->Commission;
	///平仓盈亏
	TThostFtdcMoneyType	CloseProfit = pInvestorPosition->CloseProfit;
	///持仓盈亏
	TThostFtdcMoneyType	PositionProfit = pInvestorPosition->PositionProfit;
	///上次结算价
	TThostFtdcPriceType	preSettlementPrice = pInvestorPosition->PreSettlementPrice;
	char PreSettlementPrice[100];
	sprintf(PreSettlementPrice,"%f",preSettlementPrice);
	///本次结算价
	TThostFtdcPriceType	SettlementPrice = pInvestorPosition->PreSettlementPrice;
	///交易日
	char	*TradingDay = pInvestorPosition->TradingDay;
	///结算编号
	TThostFtdcSettlementIDType	SettlementID;
	///开仓成本
	TThostFtdcMoneyType	openCost = pInvestorPosition->OpenCost;
	char OpenCost[100] ;
	sprintf(OpenCost,"%f",openCost);
	///交易所保证金
	TThostFtdcMoneyType	exchangeMargin = pInvestorPosition->ExchangeMargin;
	char ExchangeMargin[100];
	sprintf(ExchangeMargin,"%f",exchangeMargin);
	///组合成交形成的持仓
	TThostFtdcVolumeType	CombPosition;
	///组合多头冻结
	TThostFtdcVolumeType	CombLongFrozen;
	///组合空头冻结
	TThostFtdcVolumeType	CombShortFrozen;
	///逐日盯市平仓盈亏
	TThostFtdcMoneyType	CloseProfitByDate = pInvestorPosition->CloseProfitByDate;
	///逐笔对冲平仓盈亏
	TThostFtdcMoneyType	CloseProfitByTrade = pInvestorPosition->CloseProfitByTrade;
	///今日持仓
	TThostFtdcVolumeType	todayPosition = pInvestorPosition->TodayPosition;
	char TodayPosition[100] ;
	sprintf(TodayPosition,"%d",todayPosition);
	///保证金率
	TThostFtdcRatioType	marginRateByMoney = pInvestorPosition->MarginRateByMoney;
	char MarginRateByMoney[100];
	sprintf(MarginRateByMoney,"%f",marginRateByMoney);
	///保证金率(按手数)
	TThostFtdcRatioType	marginRateByVolume = pInvestorPosition->MarginRateByVolume;
	char MarginRateByVolume[100];
	sprintf(MarginRateByVolume,"%f",marginRateByVolume);
	string sInvestorInfo;
	//文件写入字段定义
	if(!isPositionDefFieldReady)
	{
		isPositionDefFieldReady = true;
		sInvestorInfo.append("InstrumentID\t");
		sInvestorInfo.append("BrokerID\t");
		sInvestorInfo.append("InvestorID\t");
		sInvestorInfo.append("PosiDirection\t");
		sInvestorInfo.append("HedgeFlag\t");
		sInvestorInfo.append("PositionDate\t");
		sInvestorInfo.append("YdPosition\t");
		sInvestorInfo.append("Position\t");
		sInvestorInfo.append("OpenVolume\t");
		sInvestorInfo.append("CloseVolume\t");
		sInvestorInfo.append("PositionCost\t");
		sInvestorInfo.append("PreSettlementPrice\t");
		sInvestorInfo.append("TradingDay\t");
		sInvestorInfo.append("OpenCost\t");
		sInvestorInfo.append("ExchangeMargin\t");
		sInvestorInfo.append("TodayPosition\t");

		sInvestorInfo.append("MarginRateByMoney\t");
		sInvestorInfo.append("MarginRateByVolume\t");
        //loglist.push_back(sInvestorInfo);
	}
	sInvestorInfo.clear();
	
	sInvestorInfo.append("InstrumentID=");sInvestorInfo.append(InstrumentID);sInvestorInfo.append("\t");
	sInvestorInfo.append(BrokerID);sInvestorInfo.append("\t");
	sInvestorInfo.append(InvestorID);sInvestorInfo.append("\t");
	sInvestorInfo.append(PosiDirection);sInvestorInfo.append("\t");
	sInvestorInfo.append(HedgeFlag);sInvestorInfo.append("\t");
	sInvestorInfo.append(PositionDate);sInvestorInfo.append("\t");
	sInvestorInfo.append(YdPosition);sInvestorInfo.append("\t");
	sInvestorInfo.append(Position);sInvestorInfo.append("\t");
	sInvestorInfo.append(OpenVolume);sInvestorInfo.append("\t");
	sInvestorInfo.append(CloseVolume);sInvestorInfo.append("\t");
	sInvestorInfo.append(PositionCost);sInvestorInfo.append("\t");
	sInvestorInfo.append(PreSettlementPrice);sInvestorInfo.append("\t");

	sInvestorInfo.append(TradingDay);sInvestorInfo.append("\t");
	sInvestorInfo.append(OpenCost);sInvestorInfo.append("\t");
	sInvestorInfo.append(ExchangeMargin);sInvestorInfo.append("\t");
	sInvestorInfo.append(TodayPosition);sInvestorInfo.append("\t");
	sInvestorInfo.append(MarginRateByMoney);sInvestorInfo.append("\t");
	sInvestorInfo.append(MarginRateByVolume);sInvestorInfo.append("\t");
    //loglist.push_back(sInvestorInfo);
	///////////////////////////////////////////////////////////////////////////////////////////////
	sInvestorInfo.clear();
	sInvestorInfo.append("InstrumentID=");sInvestorInfo.append(InstrumentID);sInvestorInfo.append(sep);
	sInvestorInfo.append("BrokerID=");sInvestorInfo.append(BrokerID);sInvestorInfo.append(sep);
	sInvestorInfo.append("InvestorID=");sInvestorInfo.append(InvestorID);sInvestorInfo.append(sep);
	sInvestorInfo.append("PosiDirection=");sInvestorInfo.append(PosiDirection);sInvestorInfo.append(sep);
	sInvestorInfo.append("HedgeFlag=");sInvestorInfo.append(HedgeFlag);sInvestorInfo.append(sep);
	sInvestorInfo.append("PositionDate=");sInvestorInfo.append(PositionDate);sInvestorInfo.append(sep);
	sInvestorInfo.append("YdPosition=");sInvestorInfo.append(YdPosition);sInvestorInfo.append(sep);
	sInvestorInfo.append("Position=");sInvestorInfo.append(Position);sInvestorInfo.append(sep);
	sInvestorInfo.append("OpenVolume=");sInvestorInfo.append(OpenVolume);sInvestorInfo.append(sep);
	sInvestorInfo.append("CloseVolume=");sInvestorInfo.append(CloseVolume);sInvestorInfo.append(sep);
	sInvestorInfo.append("PositionCost=");sInvestorInfo.append(PositionCost);sInvestorInfo.append(sep);
	sInvestorInfo.append("PreSettlementPrice=");sInvestorInfo.append(PreSettlementPrice);sInvestorInfo.append(sep);

	sInvestorInfo.append("TradingDay=");sInvestorInfo.append(TradingDay);sInvestorInfo.append(sep);
	sInvestorInfo.append("OpenCost=");sInvestorInfo.append(OpenCost);sInvestorInfo.append(sep);
	sInvestorInfo.append("ExchangeMargin=");sInvestorInfo.append(ExchangeMargin);sInvestorInfo.append(sep);
	sInvestorInfo.append("TodayPosition=");sInvestorInfo.append(TodayPosition);sInvestorInfo.append(sep);
	sInvestorInfo.append("MarginRateByMoney=");sInvestorInfo.append(MarginRateByMoney);sInvestorInfo.append(sep);
	sInvestorInfo.append("MarginRateByVolume=");sInvestorInfo.append(MarginRateByVolume);sInvestorInfo.append(sep);
    //loglist.push_back(sInvestorInfo);
	return sInvestorInfo;
}
////将投资者对冲报单信息写入文件保存
void saveInvestorOrderInsertHedge(CThostFtdcInputOrderField *order,string filepath)
{
	string ordreInfo = getInvestorOrderInsertInfo(order);
	//cerr << "--->>> 写入对冲信息开始" << endl;
    //loglist.push_back(ordreInfo);
	//cerr << "--->>> 写入对冲信息结束" << endl;
}

//提取投资者报单信息

//将投资者成交信息写入文件保存
void storeInvestorTrade(CThostFtdcTradeField *pTrade)
{
	string tradeInfo;
	///经纪公司代码
	char	*BrokerID = pTrade->BrokerID;
	///投资者代码
	char	*InvestorID = pTrade->InvestorID;
	///合约代码
	char	*InstrumentID =pTrade->InstrumentID;
	///报单引用
	char	*OrderRef = pTrade->OrderRef;
	///用户代码
	char	*UserID = pTrade->UserID;
	///交易所代码
	char	*ExchangeID =pTrade->ExchangeID;
	///成交编号
	//TThostFtdcTradeIDType	TradeID;
	///买卖方向
	TThostFtdcDirectionType	direction = pTrade->Direction;
	char Direction[]={direction,'\0'};
	//sprintf(Direction,"%s",direction);
	///报单编号
	char	*OrderSysID = pTrade->OrderSysID;
	///会员代码
	//TThostFtdcParticipantIDType	ParticipantID;
	///客户代码
	char	*ClientID = pTrade->ClientID;
	///交易角色
	//TThostFtdcTradingRoleType	TradingRole;
	///合约在交易所的代码
	//TThostFtdcExchangeInstIDType	ExchangeInstID;
	///开平标志
	TThostFtdcOffsetFlagType	offsetFlag = pTrade->OffsetFlag;
	char OffsetFlag[]={offsetFlag,'\0'};
	//sprintf(OffsetFlag,"%s",offsetFlag);
	///投机套保标志
	TThostFtdcHedgeFlagType	hedgeFlag = pTrade->HedgeFlag;
	char HedgeFlag[]={hedgeFlag,'\0'};
	//sprintf(HedgeFlag,"%s",hedgeFlag);
	///价格
	TThostFtdcPriceType	price = pTrade->Price;
	char Price[100];
	sprintf(Price,"%f",price);
	///数量
	TThostFtdcVolumeType	volume = pTrade->Volume;
	char Volume[100];
	sprintf(Volume,"%d",volume);
	///成交时期
	//TThostFtdcDateType	TradeDate;
	///成交时间
	char	*TradeTime = pTrade->TradeTime;
	///成交类型
	TThostFtdcTradeTypeType	tradeType = pTrade->TradeType;
	char TradeType[]={tradeType,'\0'};
	//sprintf(TradeType,"%s",tradeType);
	///成交价来源
	//TThostFtdcPriceSourceType	PriceSource;
	///交易所交易员代码
	//TThostFtdcTraderIDType	TraderID;
	///本地报单编号
	char	*OrderLocalID = pTrade->OrderLocalID;
	///结算会员编号
	//TThostFtdcParticipantIDType	ClearingPartID;
	///业务单元
	//TThostFtdcBusinessUnitType	BusinessUnit;
	///序号
	//TThostFtdcSequenceNoType	SequenceNo;
	///交易日
	char	*TradingDay = pTrade->TradingDay;
	///结算编号
	//TThostFtdcSettlementIDType	SettlementID;
	///经纪公司报单编号
	//TThostFtdcSequenceNoType	BrokerOrderSeq;
	if (!isTradeDefFieldReady)
	{
		isTradeDefFieldReady = true;
		tradeInfo.append("BrokerID\t");
		tradeInfo.append("InvestorID\t");
		tradeInfo.append("InstrumentID\t");
		tradeInfo.append("OrderRef\t");
		tradeInfo.append("UserID\t");
		tradeInfo.append("ExchangeID\t");
		tradeInfo.append("Direction\t");
		tradeInfo.append("ClientID\t");
		tradeInfo.append("OffsetFlag\t");
		tradeInfo.append("HedgeFlag\t");
		tradeInfo.append("Price\t");
		tradeInfo.append("Volume\t");
		tradeInfo.append("TradeTime\t");
		tradeInfo.append("TradeType\t");
		tradeInfo.append("OrderLocalID\t");
		tradeInfo.append("TradingDay\t");
		tradeInfo.append("ordersysid\t");
        //loglist.push_back(tradeInfo);
	}
	tradeInfo.clear();
	tradeInfo.append(BrokerID);tradeInfo.append("\t");
	tradeInfo.append(InvestorID);tradeInfo.append("\t");
	tradeInfo.append(InstrumentID);tradeInfo.append("\t");
	tradeInfo.append(OrderRef);tradeInfo.append("\t");
	tradeInfo.append(UserID);tradeInfo.append("\t");
	tradeInfo.append(ExchangeID);tradeInfo.append("\t");
	tradeInfo.append(Direction);tradeInfo.append("\t");
	tradeInfo.append(ClientID);tradeInfo.append("\t");
	tradeInfo.append(OffsetFlag);tradeInfo.append("\t");
	tradeInfo.append(HedgeFlag);tradeInfo.append("\t");
	tradeInfo.append(Price);tradeInfo.append("\t");
	tradeInfo.append(Volume);tradeInfo.append("\t");
	tradeInfo.append(TradeTime);tradeInfo.append("\t");
	tradeInfo.append(TradeType);tradeInfo.append("\t");
	tradeInfo.append(OrderLocalID);tradeInfo.append("\t");
	tradeInfo.append(TradingDay);tradeInfo.append("\t");
	tradeInfo.append(OrderSysID);tradeInfo.append("\t");
    //loglist.push_back(tradeInfo);
}
//将成交信息组装成对冲报单
// CThostFtdcInputOrderField assamble(CThostFtdcTradeField *pTrade)
// {
// 	CThostFtdcInputOrderField order;
// 	memset(&order,0,sizeof(order));
// 	//经济公司代码
// 	strcpy(order.BrokerID,pTrade->BrokerID);
// 	///投资者代码
// 	strcpy(order.InvestorID,pTrade->InvestorID);
// 	///合约代码
// 	strcpy(order.InstrumentID,pTrade->InstrumentID);
// 	///报单引用
// 	strcpy(order.OrderRef ,pTrade->OrderRef);
// 	///报单价格条件: 限价
// 	order.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
// 	///买卖方向: 这个要和对手方的一致，即如果我的成交为买，那么这里变成卖
// 	TThostFtdcDirectionType	Direction = pTrade->Direction;
// 	if (Direction == '0'){
// 		order.Direction = THOST_FTDC_D_Sell;
// 	} else {
// 		order.Direction = THOST_FTDC_D_Buy;
// 	}
// 	///组合开平标志: 和对手方一致
// 	order.CombOffsetFlag[0] = pTrade->OffsetFlag;
// 	///组合投机套保标志
// 	order.CombHedgeFlag[0] = pTrade->HedgeFlag;
// 	///价格
// 	TThostFtdcPriceType price = pTrade->Price;
// 	if (order.Direction == THOST_FTDC_D_Sell){
// 		//在原对手方报价基础上加上自定义tick
// 		order.LimitPrice = price + tickSpreadSell * tick;
// 	} else {
// 		//在原对手方报价基础上减去自定义tick
// 		order.LimitPrice = price - tickSpreadSell * tick;
// 	}
// 	///数量: 1
// 	order.VolumeTotalOriginal = pTrade->Volume;
// 	///有效期类型: 当日有效
// 	order.TimeCondition = THOST_FTDC_TC_GFD;
// 	///成交量类型: 任何数量
// 	order.VolumeCondition = THOST_FTDC_VC_AV;
// 	///最小成交量: 1
// 	order.MinVolume = 1;
// 	///触发条件: 立即
// 	order.ContingentCondition = THOST_FTDC_CC_Immediately;
// 	///强平原因: 非强平
// 	order.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
// 	///自动挂起标志: 否
// 	order.IsAutoSuspend = 0;
// 	///用户强评标志: 否
// 	order.UserForceClose = 0;
// 	return order;
// }
///撤单报单组装
CThostFtdcOrderField CTraderSpi::AssambleOrderAction(list<string> orderAction){
    //loglist.push_back("开始组装撤单、修改报单请求信息......");
	///经纪公司代码
	TThostFtdcBrokerIDType	Broker_ID;
	///投资者代码
	TThostFtdcInvestorIDType Investor_ID;
	///合约代码
	char InstrumentID[31];
	///请求编号
	int RequestID = 0;
	//报单引用编号
	char OrderRef[13];
	///前置编号
	TThostFtdcFrontIDType FrontID = 1;
	///会话编号
	TThostFtdcSessionIDType SessionID=0;
	///操作标志
	char ActionFlag[3] = "\0";
	///交易所代码
	TThostFtdcExchangeIDType	ExchangeID="";
	///报单编号
	TThostFtdcOrderSysIDType OrderSysID;

	//报单结构体
	CThostFtdcOrderField req ;
	//CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	//cout << "~~~~~~~~~~~~~~============>报单录入"<<endl;
	
	const char * split = "="; //分割符号
	int fieldSize = orderAction.size();
	/************************************************************************/
	/* 每个字段，按照=分隔符进行分割                                        */
	/************************************************************************/
	try{
		int i = 0;
		for(list<string>::iterator beg = orderAction.begin();beg != orderAction.end();beg ++){
			i++;
			string tmpstr = *beg;
			cout << tmpstr <<endl;
			//分割之后的字符
			char * p = 0;
			//string转char*
			char * rawfields =new char[tmpstr.size() + 1]; 
			strcpy(rawfields,tmpstr.c_str());
			p = strtok (rawfields,split); //分割字符串
			vector<string> strlist;
			while(p!=NULL) 
			{ 
				//cout << p <<endl;
				strlist.push_back(p);
				p = strtok(NULL,split); //指向下一个指针
			}
			if(strlist.size() != 2){
				//有字段为空，不填
				string tmpstr2 = "字段值为空:";
                //loglist.push_back(tmpstr2 += tmpstr);
				string tmpstr3 = "there is field value is null!!!:";
				tmpstr3.append(tmpstr);
                //tradequeue.push_back(tmpstr3);
				continue;
			}
			/************************************************************************/
			/* 变量赋值                                                                     */
			/*Broker_ID			1
			/*Investor_ID		2
			/*InstrumentID		3
			/*RequestID			4
			/*OrderRef			5
			/*FrontID			6
			/*SessionID			7
			/*ExchangeID		8
			/*OrderSysID		9
			/************************************************************************/
			string ttt = strlist.at(1);
			//cout << "赋值为:" + ttt<<endl;
			if(i == 1){
				strcpy(Broker_ID,ttt.c_str());
			}else if(i == 2){
				strcpy(Investor_ID,ttt.c_str());
			}else if(i == 3){
				strcpy(InstrumentID,ttt.c_str());
			}else if(i == 4){
				RequestID = atoi(ttt.c_str());
			}else if(i == 5){
				strcpy(OrderRef,ttt.c_str());
			}else if(i == 6){
				FrontID = atoi(ttt.c_str());
			}else if(i == 7){
				SessionID = atol(ttt.c_str());
			}else if(i == 8){
				strcpy(ExchangeID,ttt.c_str());
			}else if(i == 9){
				strcpy(OrderSysID,ttt.c_str());
			}
		}
		///经纪公司代码

		strcpy(req.BrokerID, Broker_ID);
		///投资者代码
		strcpy(req.InvestorID, Investor_ID);
		///合约代码
		strcpy(req.InstrumentID, InstrumentID);
		///报单引用
		strcpy(req.OrderRef, OrderRef);
		req.RequestID = RequestID;
		///前置编号
		req.FrontID = FrontID;
		req.SessionID = SessionID;
		strcpy(req.ExchangeID , ExchangeID);
		strcpy(req.OrderSysID , OrderSysID);
	}catch(const runtime_error &re){
		cerr << re.what()<<endl;
	}catch (exception* e)
	{
		cerr << e->what()<<endl;
        //loglist.push_back(e->what());
	}
	return req;
}
///撤单报单组装2
CThostFtdcOrderField CTraderSpi::AssambleOrderActionTwo(list<string> orderAction){
    //loglist.push_back("开始组装撤单、修改报单请求信息......");
	///经纪公司代码
	TThostFtdcBrokerIDType	Broker_ID = {"\0"};
	///投资者代码
	TThostFtdcInvestorIDType Investor_ID = {"\0"};
	///合约代码
	char InstrumentID[31] = {"\0"};
	///请求编号
	int RequestID = 0;
	//报单引用编号
	char OrderRef[13]={"\0"};
	///前置编号
	TThostFtdcFrontIDType FrontID = 1;
	///会话编号
	TThostFtdcSessionIDType SessionID=0;
	///操作标志
	char ActionFlag[3];
	///交易所代码
	TThostFtdcExchangeIDType	ExchangeID= {"\0"};
	///报单编号
	TThostFtdcOrderSysIDType OrderSysID = {"\0"};

	//报单结构体
	CThostFtdcOrderField req ;
	//CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	//cout << "~~~~~~~~~~~~~~============>报单录入"<<endl;

	//const char * split = "="; //分割符号
	int fieldSize = orderAction.size();
	/************************************************************************/
	/* 每个字段，按照=分隔符进行分割                                        */
	/************************************************************************/
	try{
		for(list<string>::iterator beg = orderAction.begin();beg != orderAction.end();beg ++){
			string tmpstr = *beg;

            vector<string> vec = UniverseTools::split(tmpstr,"=");
			if("FrontID"==vec[0]){
				FrontID = boost::lexical_cast<int>(vec[1]);
				req.FrontID = FrontID;
			}
			if("SessionID"==vec[0]){
				SessionID = boost::lexical_cast<int>(vec[1]);
				req.SessionID = SessionID;
			}
			if("OrderRef"==vec[0]){
				strcpy(OrderRef,vec[1].c_str());
				strcpy(req.OrderRef, OrderRef);
			}
			if("InstrumentID"==vec[0]){
				strcpy(InstrumentID,vec[1].c_str());
				strcpy(req.InstrumentID, InstrumentID);
			}
			if("OrderSysID"==vec[0]){
				strcpy(OrderSysID,vec[1].c_str());
				strcpy(req.OrderSysID, OrderSysID);
			}
			if("ExchangeID"==vec[0]){
				strcpy(ExchangeID,vec[1].c_str());
				strcpy(req.ExchangeID, ExchangeID);
			}
		}
		///经纪公司代码

		//strcpy(req.BrokerID, Broker_ID);
		///投资者代码
		//strcpy(req.InvestorID, Investor_ID);
		///合约代码
		//strcpy(req.InstrumentID, InstrumentID);
		///报单引用
		//strcpy(req.OrderRef, OrderRef);
		//req.RequestID = RequestID;
		///前置编号
		//req.FrontID = FrontID;
		//req.SessionID = SessionID;
		//strcpy(req.ExchangeID , ExchangeID);
		//strcpy(req.OrderSysID , OrderSysID);
	}catch(const runtime_error &re){
		cerr << re.what()<<endl;
	}catch (exception* e)
	{
		cerr << e->what()<<endl;
        //loglist.push_back(e->what());
	}
	return req;
}
