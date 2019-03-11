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
///��־��Ϣ����
extern list<string> loglist;

// USER_API����
extern CThostFtdcTraderApi* pUserApi;

// ���ò���
extern char FRONT_ADDR[];		// ǰ�õ�ַ
extern char BROKER_ID[];		// ���͹�˾����
extern char INVESTOR_ID[];		// Ͷ���ߴ���
extern char PASSWORD[];			// �û�����
extern char INSTRUMENT_ID[];	// ��Լ����
extern TThostFtdcPriceType	LIMIT_PRICE;	// �۸�
extern TThostFtdcDirectionType	DIRECTION;	// ��������
extern char BROKER_ID_1[];		// ���͹�˾����
extern char INVESTOR_ID_1[];		// Ͷ���ߴ���
extern char PASSWORD_1[];			// �û�����
extern char typ;
// ������
extern int iRequestID;
extern DataInitInstance* dii;
// �Ự����
TThostFtdcFrontIDType	FRONT_ID;	//ǰ�ñ��
TThostFtdcSessionIDType	SESSION_ID;	//�Ự���
TThostFtdcOrderRefType	ORDER_REF;	//��������
char tradingDay[12] = {"\0"}; 
//�ֲ��Ƿ��Ѿ�д�붨���ֶ�
bool isPositionDefFieldReady = false;
//�ɽ��ļ��Ƿ��Ѿ�д�붨���ֶ�
bool isTradeDefFieldReady = false;
//�û��Գ屨���ļ��Ƿ��Ѿ�д�붨���ֶ�
bool isOrderInsertDefFieldReady = false;
//���ɽ���Ϣ��װ�ɶԳ屨��
CThostFtdcInputOrderField assamble(CThostFtdcTradeField *pTrade);
////��Ͷ���߶Գ屨����Ϣд���ļ�����
void saveInvestorOrderInsertHedge(CThostFtdcInputOrderField *order,string filepath);
//���汨���ر���Ϣ
void saveRspOrderInsertInfo(CThostFtdcInputOrderField *pInputOrder);
//��ȡͶ���߱�����Ϣ
string getInvestorOrderInsertInfo(CThostFtdcInputOrderField *order);
//�Էָ�����ʽ��¼Ͷ���߱���ί����Ϣ
string getInvestorOrderInsertInfoByDelimater(CThostFtdcInputOrderField *order);
//��ȡͶ���߱�����Ϣ
string getOrderActionInfoByDelimater(CThostFtdcInputOrderActionField *order);
//��ȡί�лر���Ϣ
string getRtnOrderInfoByDelimater(CThostFtdcOrderField *order);
//��ȡ�ɽ��ر���Ϣ
string getRtnTradeInfoByDelimater(CThostFtdcTradeField *order);
//�������������ر���Ӧд���ļ�����
void saveRtnOrder(CThostFtdcOrderField *pOrder);
//��ȡ��������Ӧ��Ϣ
string getRtnOrder(CThostFtdcOrderField *pOrder);
void initpst(CThostFtdcInvestorPositionField *pInvestorPosition);
string storeInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition);
///����ֶ����ʱʹ�õķָ���
string sep = ";";
void CTraderSpi::tmpInvestorPosition(){
    //ReqQryInvestorPosition();
}
void CTraderSpi::OnFrontConnected()
{
	cerr << "--->>> " << "OnFrontConnected" << endl;
	///�û���¼����
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
    unordered_map<string, BaseAccount*>::iterator iter = whichMap->find(dii->loginInvestorID);//��Լ�����Ϣ
    if (iter == whichMap->end()) {//δ������Թ�ϵ,���½�һ��vector
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
		// ����Ự����
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
        if (logIT != whichMap->end()) {//δ������Թ�ϵ,���½�һ��vector
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
		///��ȡ��ǰ������
		cerr << "--->>> current tradingday = " << tradingDay << endl;
		string tmpstr = "--->>> current tradingday =";
		tmpstr.append(tradingDay);
        //loglist.push_back(tmpstr);
        //tradequeue.push_back(tmpstr);
		//��������֮�����µĲ���
		///Ͷ���߽�����ȷ��
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
        //loglist.push_back("Ͷ���߽��㵥ȷ�����");
        //tradequeue.push_back("Ͷ���߽��㵥ȷ�����");
        //ֱ�Ӳ�ѯ�ʽ�
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        ReqQryTradingAccount(boost::lexical_cast<string>(pSettlementInfoConfirm->BrokerID),
                             boost::lexical_cast<string>(pSettlementInfoConfirm->InvestorID));
        ///�����ѯͶ���ֲ߳�
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
	str.append("--->>> OnRspQryTradingAccount:��ѯͶ�����˻���Ϣ\n");
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
            if (logIT != dii->followNBAccountMap.end()) {//δ������Թ�ϵ,���½�һ��vector
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
            ///�����ѯͶ���ֲ߳�
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
            cout << "��ǰ�޳ֲ���Ϣ" << endl;
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
                string pst_msg = "�ֲֽṹ:" + str_instrument + ",��ͷ�ֲ���=" + string(char_tmp_pst) + ",�������=" + string(char_longtd_pst) + ",�������=" + string(char_longyd_pst) + ",��ƽ��=" + boost::lexical_cast<string>(tmppst->longAvaClosePosition) + ",�ֲ־���=" + boost::lexical_cast<string>(tmppst->longHoldAvgPrice) +
                    ";��ͷ�ֲ���=" + string(char_tmp_pst2) + ",�������=" + string(char_shorttd_pst) + ",�������=" + string(char_shortyd_pst) + ",��ƽ��=" + boost::lexical_cast<string>(tmppst->shortAvaClosePosition) + ",�ֲ־���=" + boost::lexical_cast<string>(tmppst->shortHoldAvgPrice) +
                    ";��ϳֲ���=" + boost::lexical_cast<string>(tmpArbVolume);
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
//��ʼ���ֲ���Ϣ
void initpst(CThostFtdcInvestorPositionField *pInvestorPosition)
{
    boost::recursive_mutex::scoped_lock SLock(dii->pst_mtx);
    ///��Լ����
    char	*InstrumentID = pInvestorPosition->InstrumentID;
    string str_instrumentid = string(InstrumentID);
    ///�ֲֶ�շ���
    TThostFtdcPosiDirectionType	dir = pInvestorPosition->PosiDirection;
    char PosiDirection[] = { dir,'\0' };
    ///Ͷ���ױ���־
    TThostFtdcHedgeFlagType	flag = pInvestorPosition->HedgeFlag;
    char HedgeFlag[] = { flag,'\0' };
    ///���ճֲ�
    TThostFtdcVolumeType	ydPosition = pInvestorPosition->YdPosition;
    char YdPosition[100];
    sprintf(YdPosition, "%d", ydPosition);
    ///���ճֲ�
    TThostFtdcVolumeType	position = pInvestorPosition->Position;
    char Position[100];
    sprintf(Position, "%d", position);
    string str_dir = string(PosiDirection);
    double multiplier = dii->getMultipler(str_instrumentid);
    ///�ֲ�����
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
    if (uai->positionmap.find(str_instrumentid) == uai->positionmap.end()) {//��ʱû�д�������Ҫ���Ƕ�շ���
        unordered_map<string, int> tmpmap;
        HoldPositionInfo* tmpinfo = new HoldPositionInfo();
        if ("2" == str_dir) {//��  //��ͷ
            tmpinfo->longTotalPosition = position;
            tmpinfo->longAvaClosePosition = position;
            tmpinfo->longAmount = pInvestorPosition->PositionCost;
            tmpinfo->longHoldAvgPrice = pInvestorPosition->PositionCost / (multiplier*position);
            //tmpmap["longTotalPosition"] = position;
            //��ͷ
            tmpinfo->shortTotalPosition = 0;
            //tmpmap["shortTotalPosition"] = 0;
            if ("2" == str_pst_date) {//���
                tmpinfo->longYdPosition = position;
            } else if ("1" == str_pst_date) {//���
                tmpinfo->longTdPosition = position;
            }
        } else if ("3" == str_dir) {//��
                                    //��ͷ
                                    //tmpmap["shortTotalPosition"] = position;
            tmpinfo->longTotalPosition = 0;
            tmpinfo->shortTotalPosition = position;
            tmpinfo->shortAvaClosePosition = position;
            tmpinfo->shortAmount = pInvestorPosition->PositionCost;
            tmpinfo->shortHoldAvgPrice = pInvestorPosition->PositionCost / (multiplier*position);
            //tmpmap["longTotalPosition"] = 0;
            if ("2" == str_pst_date) {//���
                tmpinfo->shortYdPosition = position;
            } else if ("1" == str_pst_date) {//���
                tmpinfo->shortTdPosition = position;
            }
        } else {
            //cout << InstrumentID << ";error:�ֲ������޷��ж�PosiDirection=" << str_dir << endl;
            LOG(ERROR) << string(InstrumentID) + ";error:�ֲ������޷��ж�PosiDirection=" + str_dir;
            return;
        }
        uai->positionmap[str_instrumentid] = tmpinfo;
    } else {
        unordered_map<string, HoldPositionInfo*>::iterator tmpmap = uai->positionmap.find(str_instrumentid);
        HoldPositionInfo* tmpinfo = tmpmap->second;
        //��Ӧ�ķ�����Ӧ���Ѿ����ڣ����������Ҫȷ��
        if ("2" == str_dir) {//��
                             //��ͷ
                             //tmpmap->second["longTotalPosition"] = position + tmpmap->second["longTotalPosition"];
            tmpinfo->longTotalPosition = position + tmpinfo->longTotalPosition;
            tmpinfo->longAvaClosePosition = position + tmpinfo->longAvaClosePosition;
            tmpinfo->longAmount = pInvestorPosition->PositionCost + tmpinfo->longAmount;
            tmpinfo->longHoldAvgPrice = tmpinfo->longAmount / (multiplier*tmpinfo->longTotalPosition);
            if ("2" == str_pst_date) {//���
                tmpinfo->longYdPosition = position + tmpinfo->longYdPosition;
            } else if ("1" == str_pst_date) {//���
                tmpinfo->longTdPosition = position + tmpinfo->longTdPosition;
            }
        } else if ("3" == str_dir) {//��
                                    //��ͷ
                                    //tmpmap->second["shortTotalPosition"] = position + tmpmap->second["shortTotalPosition"];
            tmpinfo->shortTotalPosition = position + tmpinfo->shortTotalPosition;
            tmpinfo->shortAvaClosePosition = position + tmpinfo->shortAvaClosePosition;
            tmpinfo->shortAmount = pInvestorPosition->PositionCost + tmpinfo->shortAmount;
            tmpinfo->shortHoldAvgPrice = tmpinfo->shortAmount / (multiplier*tmpinfo->shortTotalPosition);
            if ("2" == str_pst_date) {//���
                tmpinfo->shortYdPosition = position + tmpinfo->shortYdPosition;
            } else if ("1" == str_pst_date) {//���
                tmpinfo->shortTdPosition = position + tmpinfo->shortTdPosition;
            }
        } else {
            //cout << InstrumentID << ";error:�ֲ������޷��ж�PosiDirection=" << str_dir << endl;
            LOG(ERROR) << string(InstrumentID) + ";error:�ֲ������޷��ж�PosiDirection=" + str_dir;
            return;
        }
    }
    //storeInvestorPosition(pInvestorPosition);
}

void CTraderSpi::ReqOrderInsertTwo(CThostFtdcInputOrderField* req,CThostFtdcTraderApi* pUserApi){
    string investorID=boost::lexical_cast<string>(req->InvestorID);
    if(pUserApi){
        //ί���������ʹ�ÿͻ��˶���������Ÿ�ʽ
        int iResult = pUserApi->ReqOrderInsert(req,iRequestID++);
        cerr << "--->>> ReqOrderInsert:" << ((iResult == 0) ? "success" : "failed") << endl;
        //��¼����¼����Ϣ
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
    //��¼������Ϣ
    bool err = IsErrorRspInfo(pRspInfo);
    cout << "--->>> " << "OnRspOrderInsert" << "��Ӧ�����ţ�" << nRequestID << " CTP�ر�������" << pInputOrder->RequestID << endl;
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
///����¼�����ر�
void CTraderSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	//��¼������Ϣ
	bool err = IsErrorRspInfo(pRspInfo);
	string sInputOrderInfo = getInvestorOrderInsertInfoByDelimater(pInputOrder);
	if(err){
		string sResult;
		char cErrorID[100] ;

		sResult.append("--->>>����������¼�����ر�:;ErrorID=");
        sResult.append(boost::lexical_cast<string>(pRspInfo->ErrorID));
		sResult.append(";ErrorMsg=");
		sResult.append(pRspInfo->ErrorMsg).append(";");
		sInputOrderInfo.append(sResult);
	}
	//��¼����ر�������Ϣ
    //loglist.push_back(sInputOrderInfo);
	string str = "businessType=400;result=1;";
	str.append(sInputOrderInfo);
    LOG(INFO)<<(str);
	
}

void CTraderSpi::ReqOrderActionTwo(CThostFtdcInputOrderActionField *req,CThostFtdcTraderApi* pUserApi){
    string investorID=boost::lexical_cast<string>(req->InvestorID);
    if(pUserApi){
        //ί���������ʹ�ÿͻ��˶���������Ÿ�ʽ
        int iResult = pUserApi->ReqOrderAction(req,iRequestID++);
        cerr << "--->>> ReqOrderAction: " << ((iResult == 0) ? "success" : "failed") << endl;
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="--->>> ReqOrderAction: " + tmp+";";
        //��¼����¼����Ϣ
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
	//��ʱ������
// 	static bool ORDER_ACTION_SENT = false;		//�Ƿ����˱���
// 	if (ORDER_ACTION_SENT)
// 		return;

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	if(strcmp(pOrder->BrokerID,"") != 0){
		///���͹�˾����
		strcpy(req.BrokerID, pOrder->BrokerID);
	}
	if(strcmp(pOrder->InvestorID,"") != 0){
		///Ͷ���ߴ���
		strcpy(req.InvestorID, pOrder->InvestorID);
	}
	if(strcmp(pOrder->OrderRef,"") != 0){
		///��������
		strcpy(req.OrderRef, pOrder->OrderRef);
	}
	if(pOrder->RequestID != 0){
		///������
		req.RequestID = pOrder->RequestID;
	}
	if(pOrder->FrontID != 0){
		///ǰ�ñ��
		req.FrontID = pOrder->FrontID;
	}
	if(pOrder->SessionID != 0){
		///�Ự���
		req.SessionID = pOrder->SessionID;
	}
	if(strcmp(pOrder->ExchangeID,"") != 0){
		///����������
		strcpy(req.ExchangeID,pOrder->ExchangeID);
	}
	if(strcmp(pOrder->OrderSysID,"") != 0){
		///����������
		strcpy(req.OrderSysID,pOrder->OrderSysID);
	}
	if(strcmp(pOrder->InstrumentID,"") != 0){
		///����������
		strcpy(req.InstrumentID,pOrder->InstrumentID);
	}
	///������־
	req.ActionFlag = THOST_FTDC_AF_Delete;
    CTPInterface* interface=dii->getTradeApi(boost::lexical_cast<string>(pOrder->InvestorID));
    if(interface){
        int iResult = interface->pUserApi->ReqOrderAction(&req, pOrder->RequestID);
        //int iResult = interface->pUserApi->ReqQryInvestorPosition(&req, ++iRequestID);
        //int iResult = dii->pUserApi->ReqQryTradingAccount(&req, ++iRequestID);
        cerr << "--->>> ReqOrderAction: " << ((iResult == 0) ? "success" : "failed") << endl;
        string tmp=((iResult == 0) ? "success" : "failed");
        string msg="--->>> ReqOrderAction: " + tmp+";";
        //��¼����¼����Ϣ
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
// 		sResult.append("--->>>CTP�����ر���Ϣ: ErrorID=");
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
// 	sInputOrderInfo.append(";��Ӧ������:nRequestI=");
// 	sInputOrderInfo.append(cRequestid);
// 	sInputOrderInfo.append( "; CTP�ر�������:RequestID=");
// 	sInputOrderInfo.append(ctpRequestId);
    //loglist.push_back(sInputOrderInfo);
}

///����֪ͨ
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
    ///�����ύ״̬
    TThostFtdcOrderSubmitStatusType orderSubStatus = pOrder->OrderSubmitStatus;
    if (orderSubStatus == '4' || orderSubStatus == '5' || orderSubStatus == '6') {//��������
        LOG(INFO) << ("��������:" + string(pOrder->StatusMsg));
    }
    string statusMsg = boost::lexical_cast<string>(pOrder->StatusMsg);
    ///����״̬
    TThostFtdcOrderStatusType orderStatus = pOrder->OrderStatus;
    /*
    ///ȫ���ɽ�
    #define THOST_FTDC_OST_AllTraded '0'
    ///���ֳɽ����ڶ�����
    #define THOST_FTDC_OST_PartTradedQueueing '1'
    ///���ֳɽ����ڶ�����
    #define THOST_FTDC_OST_PartTradedNotQueueing '2'
    ///δ�ɽ����ڶ�����
    #define THOST_FTDC_OST_NoTradeQueueing '3'
    ///δ�ɽ����ڶ�����
    #define THOST_FTDC_OST_NoTradeNotQueueing '4'
    ///����
    #define THOST_FTDC_OST_Canceled '5'
    ///δ֪
    #define THOST_FTDC_OST_Unknown 'a'
    ///��δ����
    #define THOST_FTDC_OST_NotTouched 'b' */
    //���ֲִ���
    boost::recursive_mutex::scoped_lock SLock(dii->pst_mtx);//����
    //boost::recursive_mutex::scoped_lock SLock4(alreadyTrade_mtx);//����
    if (orderStatus == '5') {//������˵������δ�ɹ�;����Ծ��Լ��Ҫ������������Ծ��Լ��ʱ������
                             //��������������frontid+sessionid+orderrefѡ�񣻵����ֶ���������ordersysid+brokerorderseq
                             //�ж��Ƿ��ǲ���Ծ��Լ
        string msg="OnRtnOrder:process order action.";
        LOG(INFO)<<msg;
        dii->processHowManyHoldsCanBeClose(pOrder,"release");//�ͷųֲ�
        dii->processAction(pOrder);
    }else if(orderStatus == 'a') {//δ֪��״̬��������ƽ��
        string msg="OnRtnOrder:process unknown order.";
        LOG(INFO)<<msg;
        if(strlen(pOrder->OrderSysID)==0){//ctp return
            LOG(INFO)<<"order from ctp.";
            dii->updateOriOrder(pOrder,"ctp");
            dii->processHowManyHoldsCanBeClose(pOrder, "lock");//�����ֲ�
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
    }else if (orderStatus == '0') {//�ɽ�Ҳ��Ҫ�ͷ������ĳֲ�
        string msg="OnRtnOrder:process trade order.";
        LOG(INFO)<<msg;
        dii->processHowManyHoldsCanBeClose(pOrder, "release");//�ͷųֲ�.
        //return;
        ///////////////////////
        TradeInfo *pTrade=new TradeInfo();
        pTrade->investorID=boost::lexical_cast<string>(pOrder->InvestorID);
        pTrade->offsetFlag=boost::lexical_cast<string>(pOrder->CombOffsetFlag);
        pTrade->direction=boost::lexical_cast<string>(pOrder->Direction);
        pTrade->instrumentID=boost::lexical_cast<string>(pOrder->InstrumentID);

        pTrade->volume=(pOrder->VolumeTotalOriginal-pOrder->VolumeTotal);
        pTrade->tradePrice=pOrder->LimitPrice;
        dii->processtrade(pTrade);//����ֲ����
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

///�ɽ�֪ͨ
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
    //���ֲִ���
    boost::recursive_mutex::scoped_lock SLock(dii->pst_mtx);
    //dii->processHowManyHoldsCanBeClose(pOrder, "release");//�ͷųֲ�
    //����ֲ���� put modify release position in onRtnOrder.
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
//ί���д���ʱ���Ż��иñ��ģ����� pRspInfo������ǿ�ָ�롣
bool CTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// ���ErrorID != 0, ˵���յ��˴������Ӧ
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
//��ȡͶ���߱�����Ϣ
string getInvestorOrderInsertInfo(CThostFtdcInputOrderField *order)
{
	///���͹�˾����
	char	*BrokerID = order->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = order->InvestorID;
	///��Լ����
	char	*InstrumentID = order->InstrumentID;
	///��������
	char	*OrderRef = order->OrderRef;
	///�û�����
	char	*UserID = order->UserID;
	///�����۸�����
	char	OrderPriceType = order->OrderPriceType;
	///��������
	char	Direction[] = {order->Direction,'\0'};
	///��Ͽ�ƽ��־
	char	*CombOffsetFlag =order->CombOffsetFlag;
	///���Ͷ���ױ���־
	char	*CombHedgeFlag = order->CombHedgeFlag;
	///�۸�
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100];
	sprintf(LimitPrice,"%f",limitPrice);
	///����
	TThostFtdcVolumeType	volumeTotalOriginal = order->VolumeTotalOriginal;
	char VolumeTotalOriginal[100];
	sprintf(VolumeTotalOriginal,"%d",volumeTotalOriginal);
	///��Ч������
	TThostFtdcTimeConditionType	TimeCondition = order->TimeCondition;
	///GTD����
	//TThostFtdcDateType	GTDDate = order->GTDDate;
	///�ɽ�������
	TThostFtdcVolumeConditionType	VolumeCondition[] = {order->VolumeCondition,'\0'};
	///��С�ɽ���
	TThostFtdcVolumeType	MinVolume = order->MinVolume;
	///��������
	TThostFtdcContingentConditionType	ContingentCondition = order->ContingentCondition;
	///ֹ���
	TThostFtdcPriceType	StopPrice = order->StopPrice;
	///ǿƽԭ��
	TThostFtdcForceCloseReasonType	ForceCloseReason = order->ForceCloseReason;
	///�Զ������־
	TThostFtdcBoolType	IsAutoSuspend = order->IsAutoSuspend;
	///ҵ��Ԫ
	//TThostFtdcBusinessUnitType	BusinessUnit = order->BusinessUnit;
	///������
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100];
	sprintf(RequestID,"%d",requestID);
	///�û�ǿ����־
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
//��ȡͶ���߱�����Ϣ
string getInvestorOrderInsertInfoByDelimater(CThostFtdcInputOrderField *order)
{
	///���͹�˾����
	char	*BrokerID = order->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = order->InvestorID;
	///��Լ����
	char	*InstrumentID = order->InstrumentID;
	///��������
	char	*OrderRef = order->OrderRef;
	///�û�����
	char	*UserID = order->UserID;
	///�����۸�����
	char	OrderPriceType = order->OrderPriceType;
	///��������
	char	Direction[] = {order->Direction,'\0'};
	///��Ͽ�ƽ��־
	char	*CombOffsetFlag =order->CombOffsetFlag;
	///���Ͷ���ױ���־
	char	*CombHedgeFlag = order->CombHedgeFlag;
	///�۸�
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100];
	sprintf(LimitPrice,"%f",limitPrice);
	///����
	TThostFtdcVolumeType	volumeTotalOriginal = order->VolumeTotalOriginal;
	char VolumeTotalOriginal[100];
	sprintf(VolumeTotalOriginal,"%d",volumeTotalOriginal);
	///��Ч������
	TThostFtdcTimeConditionType	TimeCondition = order->TimeCondition;
	///GTD����
	//TThostFtdcDateType	GTDDate = order->GTDDate;
	///�ɽ�������
	TThostFtdcVolumeConditionType	VolumeCondition[] = {order->VolumeCondition,'\0'};
	///��С�ɽ���
	TThostFtdcVolumeType	MinVolume = order->MinVolume;
	///��������
	TThostFtdcContingentConditionType	ContingentCondition = order->ContingentCondition;
	///ֹ���
	TThostFtdcPriceType	StopPrice = order->StopPrice;
	///ǿƽԭ��
	TThostFtdcForceCloseReasonType	ForceCloseReason = order->ForceCloseReason;
	///�Զ������־
	TThostFtdcBoolType	IsAutoSuspend = order->IsAutoSuspend;
	///ҵ��Ԫ
	//TThostFtdcBusinessUnitType	BusinessUnit = order->BusinessUnit;
	///������
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100];
	sprintf(RequestID,"%d",requestID);
	///�û�ǿ����־
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
//��ȡί�лر���Ϣ
string getRtnOrderInfoByDelimater(CThostFtdcOrderField *order)
{
	///���͹�˾����
	char	*BrokerID = order->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = order->InvestorID;
	///��Լ����
	char	*InstrumentID = order->InstrumentID;
	///��������
	char	*OrderRef = order->OrderRef;
	///�û�����
	char	*UserID = order->UserID;
	///�����۸�����
	char	OrderPriceType = order->OrderPriceType;
	///��������
	char	Direction[] = {order->Direction,'\0'};
	///��Ͽ�ƽ��־
	char	*CombOffsetFlag =order->CombOffsetFlag;
	///���Ͷ���ױ���־
	char	*CombHedgeFlag = order->CombHedgeFlag;
	///�۸�
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100];
	sprintf(LimitPrice,"%f",limitPrice);
	///����
	TThostFtdcVolumeType	volumeTotalOriginal = order->VolumeTotalOriginal;
	char VolumeTotalOriginal[100];
	sprintf(VolumeTotalOriginal,"%d",volumeTotalOriginal);
	///��Ч������
	TThostFtdcTimeConditionType	TimeCondition = order->TimeCondition;
	///GTD����
	//TThostFtdcDateType	GTDDate = order->GTDDate;
	///�ɽ�������
	TThostFtdcVolumeConditionType	VolumeCondition[] = {order->VolumeCondition,'\0'};
	///��С�ɽ���
	TThostFtdcVolumeType	minVolume = order->MinVolume;
	char MinVolume[100];
	sprintf(MinVolume,"%d",minVolume);
	///��������
	TThostFtdcContingentConditionType	ContingentCondition = order->ContingentCondition;
	///ֹ���
	TThostFtdcPriceType	stopPrice = order->StopPrice;
	char StopPrice[100];
	sprintf(StopPrice,"%d",stopPrice);
	///ǿƽԭ��
	TThostFtdcForceCloseReasonType	ForceCloseReason = order->ForceCloseReason;
	///�Զ������־
	TThostFtdcBoolType	isAutoSuspend = order->IsAutoSuspend;
	char IsAutoSuspend[100];
	sprintf(IsAutoSuspend,"%d",isAutoSuspend);
	///ҵ��Ԫ
	//TThostFtdcBusinessUnitType	BusinessUnit = order->BusinessUnit;
	///������
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100];
	sprintf(RequestID,"%d",requestID);
	///���ر������
    char *OrderLocalID = order->OrderLocalID;
	///����������
	char * ExchangeID = order->ExchangeID;
	///��Ա����
	char* ParticipantID = order->ParticipantID;
	///�ͻ�����
	char*	ClientID = order->ClientID;
	///��Լ�ڽ������Ĵ���
	char*	ExchangeInstID = order->ExchangeInstID;
	///����������Ա����
	char*	TraderID = order->TraderID;
	///��װ���
	int	installID = order->InstallID;
	char InstallID[10];
	sprintf(InstallID,"%d",installID);
	///�����ύ״̬
	char	OrderSubmitStatus[] = {order->OrderSubmitStatus,'\0'};
	///������ʾ���
	int	notifySequence = order->NotifySequence;
	char NotifySequence[20];
	sprintf(NotifySequence,"%d",notifySequence);
	///������
	char*	TradingDay = order->TradingDay;
	///������
	int	settlementID = order->SettlementID;
	char SettlementID[10];
	sprintf(SettlementID,"%d",settlementID);
	///�������
	char*	OrderSysID = order->OrderSysID;
	///������Դ
	char	OrderSource[] = {order->OrderSource,'\0'};
	///����״̬
	char	OrderStatus[] = {order->OrderStatus,'\0'};
	///��������
	//TThostFtdcOrderTypeType	OrderType;
	///��ɽ�����
	int	volumeTraded = order->VolumeTraded;
	char VolumeTraded[10];
	sprintf(VolumeTraded,"%d",volumeTraded);
	///ʣ������
	int	volumeTotal = order->VolumeTotal;
	char VolumeTotal[10];
	sprintf(VolumeTotal,"%d",volumeTotal);
	///��������
	char*	InsertDate = order->InsertDate;
	///ί��ʱ��
	char*	InsertTime = order->InsertTime;
	///����ʱ��
	char*	ActiveTime = order->ActiveTime;
	///����ʱ��
	char*	SuspendTime = order->SuspendTime;
	///����޸�ʱ��
	char*	UpdateTime = order->UpdateTime;
	///����ʱ��
	char*	CancelTime = order->CancelTime;
	///����޸Ľ���������Ա����
	char*	ActiveTraderID = order->ActiveTraderID;
	///�����Ա���
	char*	ClearingPartID = order->ClearingPartID;
	///���
	int	sequenceNo = order->SequenceNo;
	char SequenceNo[20];
	sprintf(SequenceNo,"%d",sequenceNo);
	///ǰ�ñ��
	int	frontID = order->FrontID;
	char FrontID[20];
	sprintf(FrontID,"%d",frontID);
	///�Ự���
	int	sessionID = order->SessionID;
	char SessionID[20];
	sprintf(SessionID,"%d",sessionID);
	///�û��˲�Ʒ��Ϣ
	char*	UserProductInfo = order->UserProductInfo;
	///״̬��Ϣ
	char*	StatusMsg = order->StatusMsg;
	///�û�ǿ����־
	int	userForceClose = order->UserForceClose;
	char UserForceClose[20];
	sprintf(UserForceClose,"%d",userForceClose);
	///�����û�����
	char*	ActiveUserID = order->ActiveUserID;
	///���͹�˾�������
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
//��ȡ�ɽ��ر���Ϣ
string getRtnTradeInfoByDelimater(CThostFtdcTradeField *order)
{
	///���͹�˾����
	char	*BrokerID = order->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = order->InvestorID;
	///��Լ����
	char	*InstrumentID = order->InstrumentID;
	///��������
	char	*OrderRef = order->OrderRef;
	///�û�����
	char	*UserID = order->UserID;
	///����������
	char * ExchangeID = order->ExchangeID;
	///�ɽ����
	char*	TradeID = order->TradeID;
	///��������
	char	Direction[] = {order->Direction,'\0'};
	///�������
	char*	OrderSysID = order->OrderSysID;
	//��Ա����
	char*	ParticipantID = order->ParticipantID;
	///�ͻ�����
	char*	ClientID = order->ClientID;
	///��Լ�ڽ������Ĵ���
	char*	ExchangeInstID = order->ExchangeInstID;
	///��ƽ��־
	char	OffsetFlag[] = {order->OffsetFlag,'\0'};
	///Ͷ���ױ���־
	char	HedgeFlag[] = {order->HedgeFlag,'\0'};
	///�۸�
	TThostFtdcPriceType	price = order->Price;
	char Price[100];
	sprintf(Price,"%f",price);
	///����
	TThostFtdcVolumeType	volume = order->Volume;
	char Volume[100];
	sprintf(Volume,"%d",volume);
	///�ɽ�ʱ��
	char*	TradeDate = order->TradeDate;
	///�ɽ�ʱ��
	char*	TradeTime = order->TradeTime;
	///�ɽ�����
	char	TradeType[] = {order->TradeType,'\0'};
	///�ɽ�����Դ
	char	PriceSource[] = {order->PriceSource,'\0'};
	///����������Ա����
	char*	TraderID = order->TraderID;
	///���ر������
    char *OrderLocalID = order->OrderLocalID;
	///������
	int	settlementID = order->SettlementID;
	char SettlementID[10];
	sprintf(SettlementID,"%d",settlementID);
	///�����Ա���
	char*	ClearingPartID = order->ClearingPartID;
	///���
	int	sequenceNo = order->SequenceNo;
	char SequenceNo[20];
	sprintf(SequenceNo,"%d",sequenceNo);
	///������
	char*	TradingDay = order->TradingDay;
	///���͹�˾�������
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
//��ȡͶ���߱�����Ϣ
string getOrderActionInfoByDelimater(CThostFtdcInputOrderActionField *order)
{
	///���͹�˾����
	char	*BrokerID = order->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = order->InvestorID;
	///��Լ����
	char	*InstrumentID = order->InstrumentID;
	///������������
	TThostFtdcOrderActionRefType	orderActionRef = order->OrderActionRef;
	char OrderActionRef[20] = {"\0"};
	sprintf(OrderActionRef,"%d",orderActionRef);
	///��������
	char	*OrderRef = order->OrderRef;
	///�û�����
	char	*UserID = order->UserID;
	///������
	TThostFtdcRequestIDType	requestID = order->RequestID;
	char RequestID[100] = {"\0"};
	sprintf(RequestID,"%d",requestID);
	///ǰ�ñ�� int
	TThostFtdcFrontIDType	frontID = order->FrontID;
	char FrontID[7] = {"\0"};
	sprintf(FrontID,"%d",frontID);
	///�Ự��� int
	TThostFtdcSessionIDType	sessionID = order->SessionID;
	char SessionID[20] = {"\0"};
	sprintf(SessionID,"%d",sessionID);
	///����������
	char*	ExchangeID = order->ExchangeID;
	///�������
	char*	OrderSysID = order->OrderSysID;
	///������־
	char	ActionFlag[] = {order->ActionFlag,'\0'};
	///�۸� double
	TThostFtdcPriceType	limitPrice = order->LimitPrice;
	char LimitPrice[100]={"\0"};
	sprintf(LimitPrice,"%f",limitPrice);
	///�����仯int
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
//�������ر���Ӧ��Ϣд���ļ�����
void saveRspOrderInsertInfo(CThostFtdcInputOrderField *pInputOrder)
{
	saveInvestorOrderInsertHedge(pInputOrder, "d:\\test\\rsporderinsert.txt");
}
//��ȡ�������ر���Ӧ
string getRtnOrder(CThostFtdcOrderField *pOrder)
{
	///���͹�˾����
	char	*BrokerID = pOrder->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = pOrder->InvestorID;
	///��Լ����
	char	*InstrumentID = pOrder->InstrumentID;
	///��������
	char	*OrderRef = pOrder->OrderRef;
	///�û�����
	TThostFtdcUserIDType	UserID;
	///�����۸�����
	TThostFtdcOrderPriceTypeType	OrderPriceType;
	///��������
	TThostFtdcDirectionType	Direction = pOrder->Direction;
	///��Ͽ�ƽ��־
	TThostFtdcCombOffsetFlagType	CombOffsetFlag;
	///���Ͷ���ױ���־
	TThostFtdcCombHedgeFlagType	CombHedgeFlag;
	///�۸�
	TThostFtdcPriceType	LimitPrice;
	///����
	TThostFtdcVolumeType	VolumeTotalOriginal;
	///��Ч������
	TThostFtdcTimeConditionType	TimeCondition;
	///GTD����
	TThostFtdcDateType	GTDDate;
	///�ɽ�������
	TThostFtdcVolumeConditionType	VolumeCondition;
	///��С�ɽ���
	TThostFtdcVolumeType	MinVolume;
	///��������
	TThostFtdcContingentConditionType	ContingentCondition;
	///ֹ���
	TThostFtdcPriceType	StopPrice;
	///ǿƽԭ��
	TThostFtdcForceCloseReasonType	ForceCloseReason;
	///�Զ������־
	TThostFtdcBoolType	IsAutoSuspend;
	///ҵ��Ԫ
	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
	TThostFtdcRequestIDType	RequestID = pOrder->RequestID;
	char cRequestId[100];
	sprintf(cRequestId,"%d",RequestID);
	///���ر������
	char	*OrderLocalID = pOrder->OrderLocalID;
	///����������
	TThostFtdcExchangeIDType	ExchangeID;
	///��Ա����
	TThostFtdcParticipantIDType	ParticipantID;
	///�ͻ�����
	char	*ClientID = pOrder->ClientID;
	///��Լ�ڽ������Ĵ���
	TThostFtdcExchangeInstIDType	ExchangeInstID;
	///����������Ա����
	TThostFtdcTraderIDType	TraderID;
	///��װ���
	TThostFtdcInstallIDType	InstallID;
	///�����ύ״̬
	char	OrderSubmitStatus = pOrder->OrderSubmitStatus;
	char cOrderSubmitStatus[] = {OrderSubmitStatus,'\0'};
	//sprintf(cOrderSubmitStatus,"%s",OrderSubmitStatus);
	///����״̬
	TThostFtdcOrderStatusType	OrderStatus = pOrder->OrderStatus;
	char cOrderStatus[] = {OrderStatus,'\0'};
	//sprintf(cOrderStatus,"%s",OrderStatus);
	///������ʾ���
	TThostFtdcSequenceNoType	NotifySequence;
	///������
	TThostFtdcDateType	TradingDay;
	///������
	TThostFtdcSettlementIDType	SettlementID;
	///�������
	char	*OrderSysID = pOrder->OrderSysID;
	char replace[] = {'n','\0'};
	if (strlen(OrderSysID) == 0)
	{
		OrderSysID = replace;
	}
	///������Դ
	TThostFtdcOrderSourceType	OrderSource;
	///��������
	TThostFtdcOrderTypeType	OrderType;
	///��ɽ�����
	TThostFtdcVolumeType	VolumeTraded = pOrder->VolumeTraded;
	char cVolumeTraded[100];
	sprintf(cVolumeTraded,"%d",VolumeTraded);
	///ʣ������
	TThostFtdcVolumeType	VolumeTotal = pOrder->VolumeTotal;
	char iVolumeTotal[100];
	sprintf(iVolumeTotal,"%d",VolumeTotal);
	///��������
	TThostFtdcDateType	InsertDate;
	///ί��ʱ��
	TThostFtdcTimeType	InsertTime;
	///����ʱ��
	TThostFtdcTimeType	ActiveTime;
	///����ʱ��
	TThostFtdcTimeType	SuspendTime;
	///����޸�ʱ��
	TThostFtdcTimeType	UpdateTime;
	///����ʱ��
	TThostFtdcTimeType	CancelTime;
	///����޸Ľ���������Ա����
	TThostFtdcTraderIDType	ActiveTraderID;
	///�����Ա���
	TThostFtdcParticipantIDType	ClearingPartID;
	///���
	TThostFtdcSequenceNoType	SequenceNo;
	///ǰ�ñ��
	TThostFtdcFrontIDType	FrontID;
	///�Ự���
	TThostFtdcSessionIDType	SessionID;
	///�û��˲�Ʒ��Ϣ
	TThostFtdcProductInfoType	UserProductInfo;
	///״̬��Ϣ
	char	*StatusMsg = pOrder->StatusMsg;
	///�û�ǿ����־
	TThostFtdcBoolType	UserForceClose;
	///�����û�����
	TThostFtdcUserIDType	ActiveUserID;
	///���͹�˾�������
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

//�������������ر���Ӧд���ļ�����
void saveRtnOrder(CThostFtdcOrderField *pOrder)
{
	string info = getRtnOrder(pOrder);
    //loglist.push_back(info);
}
//��Ͷ���ֲ߳���Ϣд���ļ�����
string storeInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition)
{
	///��Լ����
	char	*InstrumentID = pInvestorPosition->InstrumentID;
	///���͹�˾����
	char	*BrokerID = pInvestorPosition->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = pInvestorPosition->InvestorID;
	///�ֲֶ�շ���
	TThostFtdcPosiDirectionType	dir = pInvestorPosition->PosiDirection;
	char PosiDirection[] = {dir,'\0'};
	///Ͷ���ױ���־
	TThostFtdcHedgeFlagType	flag = pInvestorPosition->HedgeFlag;
	char HedgeFlag[] = {flag,'\0'};
	///�ֲ�����
	TThostFtdcPositionDateType	positionDate = pInvestorPosition->PositionDate;
	char PositionDate[] = {positionDate,'\0'};
	///���ճֲ�
	TThostFtdcVolumeType	ydPosition = pInvestorPosition->YdPosition;
	char YdPosition[100];
	sprintf(YdPosition,"%d",ydPosition);
	///���ճֲ�
	TThostFtdcVolumeType	position = pInvestorPosition->Position;
	/*
	if (position == 0)
	{
		return 0;
	}*/
	char Position[100];
	sprintf(Position,"%d",position);
	///��ͷ����
	TThostFtdcVolumeType	LongFrozen = pInvestorPosition->LongFrozen;
	///��ͷ����
	TThostFtdcVolumeType	ShortFrozen = pInvestorPosition->ShortFrozen;
	///���ֶ�����
	TThostFtdcMoneyType	LongFrozenAmount = pInvestorPosition->LongFrozenAmount;
	///���ֶ�����
	TThostFtdcMoneyType	ShortFrozenAmount = pInvestorPosition->ShortFrozenAmount;
	///������
	TThostFtdcVolumeType	openVolume = pInvestorPosition->OpenVolume;
	char OpenVolume[100] ;
	sprintf(OpenVolume,"%d",openVolume);
	///ƽ����
	TThostFtdcVolumeType	closeVolume = pInvestorPosition->CloseVolume;
	char CloseVolume[100];
	sprintf(CloseVolume,"%d",closeVolume);
	///���ֽ��
	TThostFtdcMoneyType	OpenAmount = pInvestorPosition->OpenAmount;
	///ƽ�ֽ��
	TThostFtdcMoneyType	CloseAmount = pInvestorPosition->CloseAmount;
	///�ֲֳɱ�
	TThostFtdcMoneyType	positionCost = pInvestorPosition->PositionCost;
	char PositionCost[100];
	sprintf(PositionCost,"%f",positionCost);
	///�ϴ�ռ�õı�֤��
	TThostFtdcMoneyType	PreMargin = pInvestorPosition->PreMargin;
	///ռ�õı�֤��
	TThostFtdcMoneyType	UseMargin = pInvestorPosition->UseMargin;
	///����ı�֤��
	TThostFtdcMoneyType	FrozenMargin = pInvestorPosition->FrozenMargin;
	///������ʽ�
	TThostFtdcMoneyType	FrozenCash = pInvestorPosition->FrozenCash;
	///�����������
	TThostFtdcMoneyType	FrozenCommission = pInvestorPosition->FrozenCommission;
	///�ʽ���
	TThostFtdcMoneyType	CashIn = pInvestorPosition->CashIn;
	///������
	TThostFtdcMoneyType	Commission = pInvestorPosition->Commission;
	///ƽ��ӯ��
	TThostFtdcMoneyType	CloseProfit = pInvestorPosition->CloseProfit;
	///�ֲ�ӯ��
	TThostFtdcMoneyType	PositionProfit = pInvestorPosition->PositionProfit;
	///�ϴν����
	TThostFtdcPriceType	preSettlementPrice = pInvestorPosition->PreSettlementPrice;
	char PreSettlementPrice[100];
	sprintf(PreSettlementPrice,"%f",preSettlementPrice);
	///���ν����
	TThostFtdcPriceType	SettlementPrice = pInvestorPosition->PreSettlementPrice;
	///������
	char	*TradingDay = pInvestorPosition->TradingDay;
	///������
	TThostFtdcSettlementIDType	SettlementID;
	///���ֳɱ�
	TThostFtdcMoneyType	openCost = pInvestorPosition->OpenCost;
	char OpenCost[100] ;
	sprintf(OpenCost,"%f",openCost);
	///��������֤��
	TThostFtdcMoneyType	exchangeMargin = pInvestorPosition->ExchangeMargin;
	char ExchangeMargin[100];
	sprintf(ExchangeMargin,"%f",exchangeMargin);
	///��ϳɽ��γɵĳֲ�
	TThostFtdcVolumeType	CombPosition;
	///��϶�ͷ����
	TThostFtdcVolumeType	CombLongFrozen;
	///��Ͽ�ͷ����
	TThostFtdcVolumeType	CombShortFrozen;
	///���ն���ƽ��ӯ��
	TThostFtdcMoneyType	CloseProfitByDate = pInvestorPosition->CloseProfitByDate;
	///��ʶԳ�ƽ��ӯ��
	TThostFtdcMoneyType	CloseProfitByTrade = pInvestorPosition->CloseProfitByTrade;
	///���ճֲ�
	TThostFtdcVolumeType	todayPosition = pInvestorPosition->TodayPosition;
	char TodayPosition[100] ;
	sprintf(TodayPosition,"%d",todayPosition);
	///��֤����
	TThostFtdcRatioType	marginRateByMoney = pInvestorPosition->MarginRateByMoney;
	char MarginRateByMoney[100];
	sprintf(MarginRateByMoney,"%f",marginRateByMoney);
	///��֤����(������)
	TThostFtdcRatioType	marginRateByVolume = pInvestorPosition->MarginRateByVolume;
	char MarginRateByVolume[100];
	sprintf(MarginRateByVolume,"%f",marginRateByVolume);
	string sInvestorInfo;
	//�ļ�д���ֶζ���
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
////��Ͷ���߶Գ屨����Ϣд���ļ�����
void saveInvestorOrderInsertHedge(CThostFtdcInputOrderField *order,string filepath)
{
	string ordreInfo = getInvestorOrderInsertInfo(order);
	//cerr << "--->>> д��Գ���Ϣ��ʼ" << endl;
    //loglist.push_back(ordreInfo);
	//cerr << "--->>> д��Գ���Ϣ����" << endl;
}

//��ȡͶ���߱�����Ϣ

//��Ͷ���߳ɽ���Ϣд���ļ�����
void storeInvestorTrade(CThostFtdcTradeField *pTrade)
{
	string tradeInfo;
	///���͹�˾����
	char	*BrokerID = pTrade->BrokerID;
	///Ͷ���ߴ���
	char	*InvestorID = pTrade->InvestorID;
	///��Լ����
	char	*InstrumentID =pTrade->InstrumentID;
	///��������
	char	*OrderRef = pTrade->OrderRef;
	///�û�����
	char	*UserID = pTrade->UserID;
	///����������
	char	*ExchangeID =pTrade->ExchangeID;
	///�ɽ����
	//TThostFtdcTradeIDType	TradeID;
	///��������
	TThostFtdcDirectionType	direction = pTrade->Direction;
	char Direction[]={direction,'\0'};
	//sprintf(Direction,"%s",direction);
	///�������
	char	*OrderSysID = pTrade->OrderSysID;
	///��Ա����
	//TThostFtdcParticipantIDType	ParticipantID;
	///�ͻ�����
	char	*ClientID = pTrade->ClientID;
	///���׽�ɫ
	//TThostFtdcTradingRoleType	TradingRole;
	///��Լ�ڽ������Ĵ���
	//TThostFtdcExchangeInstIDType	ExchangeInstID;
	///��ƽ��־
	TThostFtdcOffsetFlagType	offsetFlag = pTrade->OffsetFlag;
	char OffsetFlag[]={offsetFlag,'\0'};
	//sprintf(OffsetFlag,"%s",offsetFlag);
	///Ͷ���ױ���־
	TThostFtdcHedgeFlagType	hedgeFlag = pTrade->HedgeFlag;
	char HedgeFlag[]={hedgeFlag,'\0'};
	//sprintf(HedgeFlag,"%s",hedgeFlag);
	///�۸�
	TThostFtdcPriceType	price = pTrade->Price;
	char Price[100];
	sprintf(Price,"%f",price);
	///����
	TThostFtdcVolumeType	volume = pTrade->Volume;
	char Volume[100];
	sprintf(Volume,"%d",volume);
	///�ɽ�ʱ��
	//TThostFtdcDateType	TradeDate;
	///�ɽ�ʱ��
	char	*TradeTime = pTrade->TradeTime;
	///�ɽ�����
	TThostFtdcTradeTypeType	tradeType = pTrade->TradeType;
	char TradeType[]={tradeType,'\0'};
	//sprintf(TradeType,"%s",tradeType);
	///�ɽ�����Դ
	//TThostFtdcPriceSourceType	PriceSource;
	///����������Ա����
	//TThostFtdcTraderIDType	TraderID;
	///���ر������
	char	*OrderLocalID = pTrade->OrderLocalID;
	///�����Ա���
	//TThostFtdcParticipantIDType	ClearingPartID;
	///ҵ��Ԫ
	//TThostFtdcBusinessUnitType	BusinessUnit;
	///���
	//TThostFtdcSequenceNoType	SequenceNo;
	///������
	char	*TradingDay = pTrade->TradingDay;
	///������
	//TThostFtdcSettlementIDType	SettlementID;
	///���͹�˾�������
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
//���ɽ���Ϣ��װ�ɶԳ屨��
// CThostFtdcInputOrderField assamble(CThostFtdcTradeField *pTrade)
// {
// 	CThostFtdcInputOrderField order;
// 	memset(&order,0,sizeof(order));
// 	//���ù�˾����
// 	strcpy(order.BrokerID,pTrade->BrokerID);
// 	///Ͷ���ߴ���
// 	strcpy(order.InvestorID,pTrade->InvestorID);
// 	///��Լ����
// 	strcpy(order.InstrumentID,pTrade->InstrumentID);
// 	///��������
// 	strcpy(order.OrderRef ,pTrade->OrderRef);
// 	///�����۸�����: �޼�
// 	order.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
// 	///��������: ���Ҫ�Ͷ��ַ���һ�£�������ҵĳɽ�Ϊ����ô��������
// 	TThostFtdcDirectionType	Direction = pTrade->Direction;
// 	if (Direction == '0'){
// 		order.Direction = THOST_FTDC_D_Sell;
// 	} else {
// 		order.Direction = THOST_FTDC_D_Buy;
// 	}
// 	///��Ͽ�ƽ��־: �Ͷ��ַ�һ��
// 	order.CombOffsetFlag[0] = pTrade->OffsetFlag;
// 	///���Ͷ���ױ���־
// 	order.CombHedgeFlag[0] = pTrade->HedgeFlag;
// 	///�۸�
// 	TThostFtdcPriceType price = pTrade->Price;
// 	if (order.Direction == THOST_FTDC_D_Sell){
// 		//��ԭ���ַ����ۻ����ϼ����Զ���tick
// 		order.LimitPrice = price + tickSpreadSell * tick;
// 	} else {
// 		//��ԭ���ַ����ۻ����ϼ�ȥ�Զ���tick
// 		order.LimitPrice = price - tickSpreadSell * tick;
// 	}
// 	///����: 1
// 	order.VolumeTotalOriginal = pTrade->Volume;
// 	///��Ч������: ������Ч
// 	order.TimeCondition = THOST_FTDC_TC_GFD;
// 	///�ɽ�������: �κ�����
// 	order.VolumeCondition = THOST_FTDC_VC_AV;
// 	///��С�ɽ���: 1
// 	order.MinVolume = 1;
// 	///��������: ����
// 	order.ContingentCondition = THOST_FTDC_CC_Immediately;
// 	///ǿƽԭ��: ��ǿƽ
// 	order.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
// 	///�Զ������־: ��
// 	order.IsAutoSuspend = 0;
// 	///�û�ǿ����־: ��
// 	order.UserForceClose = 0;
// 	return order;
// }
///����������װ
CThostFtdcOrderField CTraderSpi::AssambleOrderAction(list<string> orderAction){
    //loglist.push_back("��ʼ��װ�������޸ı���������Ϣ......");
	///���͹�˾����
	TThostFtdcBrokerIDType	Broker_ID;
	///Ͷ���ߴ���
	TThostFtdcInvestorIDType Investor_ID;
	///��Լ����
	char InstrumentID[31];
	///������
	int RequestID = 0;
	//�������ñ��
	char OrderRef[13];
	///ǰ�ñ��
	TThostFtdcFrontIDType FrontID = 1;
	///�Ự���
	TThostFtdcSessionIDType SessionID=0;
	///������־
	char ActionFlag[3] = "\0";
	///����������
	TThostFtdcExchangeIDType	ExchangeID="";
	///�������
	TThostFtdcOrderSysIDType OrderSysID;

	//�����ṹ��
	CThostFtdcOrderField req ;
	//CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	//cout << "~~~~~~~~~~~~~~============>����¼��"<<endl;
	
	const char * split = "="; //�ָ����
	int fieldSize = orderAction.size();
	/************************************************************************/
	/* ÿ���ֶΣ�����=�ָ������зָ�                                        */
	/************************************************************************/
	try{
		int i = 0;
		for(list<string>::iterator beg = orderAction.begin();beg != orderAction.end();beg ++){
			i++;
			string tmpstr = *beg;
			cout << tmpstr <<endl;
			//�ָ�֮����ַ�
			char * p = 0;
			//stringתchar*
			char * rawfields =new char[tmpstr.size() + 1]; 
			strcpy(rawfields,tmpstr.c_str());
			p = strtok (rawfields,split); //�ָ��ַ���
			vector<string> strlist;
			while(p!=NULL) 
			{ 
				//cout << p <<endl;
				strlist.push_back(p);
				p = strtok(NULL,split); //ָ����һ��ָ��
			}
			if(strlist.size() != 2){
				//���ֶ�Ϊ�գ�����
				string tmpstr2 = "�ֶ�ֵΪ��:";
                //loglist.push_back(tmpstr2 += tmpstr);
				string tmpstr3 = "there is field value is null!!!:";
				tmpstr3.append(tmpstr);
                //tradequeue.push_back(tmpstr3);
				continue;
			}
			/************************************************************************/
			/* ������ֵ                                                                     */
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
			//cout << "��ֵΪ:" + ttt<<endl;
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
		///���͹�˾����

		strcpy(req.BrokerID, Broker_ID);
		///Ͷ���ߴ���
		strcpy(req.InvestorID, Investor_ID);
		///��Լ����
		strcpy(req.InstrumentID, InstrumentID);
		///��������
		strcpy(req.OrderRef, OrderRef);
		req.RequestID = RequestID;
		///ǰ�ñ��
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
///����������װ2
CThostFtdcOrderField CTraderSpi::AssambleOrderActionTwo(list<string> orderAction){
    //loglist.push_back("��ʼ��װ�������޸ı���������Ϣ......");
	///���͹�˾����
	TThostFtdcBrokerIDType	Broker_ID = {"\0"};
	///Ͷ���ߴ���
	TThostFtdcInvestorIDType Investor_ID = {"\0"};
	///��Լ����
	char InstrumentID[31] = {"\0"};
	///������
	int RequestID = 0;
	//�������ñ��
	char OrderRef[13]={"\0"};
	///ǰ�ñ��
	TThostFtdcFrontIDType FrontID = 1;
	///�Ự���
	TThostFtdcSessionIDType SessionID=0;
	///������־
	char ActionFlag[3];
	///����������
	TThostFtdcExchangeIDType	ExchangeID= {"\0"};
	///�������
	TThostFtdcOrderSysIDType OrderSysID = {"\0"};

	//�����ṹ��
	CThostFtdcOrderField req ;
	//CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	//cout << "~~~~~~~~~~~~~~============>����¼��"<<endl;

	//const char * split = "="; //�ָ����
	int fieldSize = orderAction.size();
	/************************************************************************/
	/* ÿ���ֶΣ�����=�ָ������зָ�                                        */
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
		///���͹�˾����

		//strcpy(req.BrokerID, Broker_ID);
		///Ͷ���ߴ���
		//strcpy(req.InvestorID, Investor_ID);
		///��Լ����
		//strcpy(req.InstrumentID, InstrumentID);
		///��������
		//strcpy(req.OrderRef, OrderRef);
		//req.RequestID = RequestID;
		///ǰ�ñ��
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
