#pragma once
#ifdef WIN32
#include <Windows.h>
typedef HMODULE		T_DLL_HANDLE;
#else
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>
#include <mysql++.h>
#include<ssqls.h>
#include <sqlite3.h>

#include "property.h"
//#include "mysqlconnectpool.h"
//#include"DBtable.h"
#include "dataprocessor.h"
using boost::locale::conv::between;
using boost::lexical_cast;
using boost::split;
using boost::is_any_of;
using boost::algorithm::trim_copy;

using namespace std;
// 请求编号

DataInitInstance::DataInitInstance(void)
{
}
DataInitInstance::~DataInitInstance(void)
{
}


//user=126373/123456/1:1/1/1,122467/lhh520/1:5/2/2

void DataInitInstance::GetConfigFromFile(){
    std::ifstream myfile("config/global.properties");
    if (!myfile) {
        LOG(ERROR) << "读取global.properties文件失败" << endl;
    }
    string str;
    while (getline(myfile, str)) {
        int pos = str.find("#");
        if (pos == 0) {
            //cout << "注释:" << str << endl;
            continue;
        }
        else {
            LOG(ERROR)  <<str<<endl;
            vector<string> name_value;
            split(name_value,str, is_any_of("="));
            if(name_value.size()!=2){
                continue;
            } else if ("investorid" == name_value[0]) {
                investor_id = name_value[1];
            }
            else if ("password" == name_value[0]) {
                password = name_value[1];
            }
            else if ("brokerid" == name_value[0]) {
                broker_id = name_value[1];
            }
            else if ("mdFrontAddr" == name_value[0]) {
                _market_front_addr= name_value[1];
            }
            else if ("tradeFrontAddr" == name_value[0]) {
                _trade_front_addr=name_value[1];
            }
            else if("DBHost"== name_value[0]){
                db_host=name_value[1];
            }else if("DBUser"==name_value[0]){
                db_user=name_value[1];
            }else if("DBPWD"==name_value[0]){
                db_pwd=name_value[1];
            }else if("DBMaxConnSize"==name_value[0]){
                db_maxConnSize=boost::lexical_cast<int>(name_value[1]);
            }else if("DBNAME"==name_value[0]){
                db_name = name_value[1];
            }else if("DBPORT"==name_value[0]){
                db_port=boost::lexical_cast<int>(name_value[1]);
            }else if("DBCharSet"==name_value[0]){
                db_charset=name_value[1];
            }else if("redis_host"==name_value[0]){
                redis_host=name_value[1];
            }
            else if("redis_port"==name_value[0]){
                redis_port=name_value[1];
            }
            else if("redis_pwd"==name_value[0]){
                redis_pwd=name_value[1];
            }
            else if("instrumentlist"==name_value[0]){
                instrumentlist=name_value[1];
                vector<string> tmp;
               split(tmp,instrumentlist,boost::is_any_of(","));
                for (unsigned int i = 0; i < tmp.size();i++)
                {
                    ppinstrument[i]=new char[tmp[i].length()+1];
                    strcpy(ppinstrument[i],tmp[i].c_str());
                    cout<<   ppinstrument[i]<<endl;
                }
                instrumentcnt=tmp.size();
            }
        }
    }
}



void DataInitInstance::DataInit()
{
    mysql_pool = new MysqlConnectPool(db_name, db_host, db_user, db_pwd, db_port, db_charset, db_maxConnSize);
}










vector<string> DataInitInstance::getInstrumentIDZH(string InstrumentID)
{
    vector<string> res ;
    string variety_ch="";
    string instrumentid_ch="";
    boost::algorithm::to_upper(InstrumentID);
    for (int i = 0; i < InstrumentID.length(); i++)
    {
        char s = InstrumentID[i] ;
        if(s >='0' && s<='9')
            instrumentid_ch.push_back(s);
        else
            variety_ch.push_back(s);
    }

    variety_ch = getHeyueName(variety_ch);

    instrumentid_ch = variety_ch+instrumentid_ch;

    res.push_back(instrumentid_ch);
    res.push_back(variety_ch);
    return res;
}

string DataInitInstance::getHeyueName(string str)
{
    if("A"==str) return "豆一";
    else if("B"==str) return "豆二";
    else if("BB"==str) return "胶板";
    else if("C"==str) return "玉米";
    else if("CS"==str) return "淀粉";
    else if("FB"==str) return "纤板";
    else if("I"==str) return "铁矿";
    else if("J"==str) return "焦炭";
    else if("JD"==str) return "鸡蛋";
    else if("JM"==str) return "焦煤";
    else if("L"==str) return "塑料";
    else if("M"==str) return "豆粕";
    else if("P"==str) return "棕榈";
    else if("PP"==str) return "PP";
    else if("V"==str) return "PVC";
    else if("Y"==str) return "豆油";
    else if("AG"==str) return "白银";
    else if("AL"==str) return "铝";
    else if("AU"==str) return "黄金";
    else if("BU"==str) return "沥青";
    else if("CU"==str) return "铜";
    else if("FU"==str) return "燃油";
    else if("HC"==str) return "热卷";
    else if("NI"==str) return "镍";
    else if("PB"==str) return "铅";
    else if("RB"==str) return "螺纹";
    else if("RU"==str) return "橡胶";
    else if("SN"==str) return "锡";
    else if("WR"==str) return "线材";
    else if("ZN"==str) return "锌";
    else if("CF"==str) return "棉花";
    else if("FG"==str) return "玻璃";
    else if("JR"==str) return "粳稻";
    else if("LR"==str) return "晚稻";
    else if("MA"==str) return "甲醇";
    else if("OI"==str) return "菜油";
    else if("PM"==str) return "普麦";
    else if("RI"==str) return "早稻";
    else if("RM"==str) return "菜粕";
    else if("RS"==str) return "油籽";
    else if("SF"==str) return "硅铁";
    else if("SM"==str) return "锰硅";
    else if("SR"==str) return "白糖";
    else if("TA"==str) return "PTA";
    else if("WH"==str) return "强麦";
    else if("ZC"==str) return "动煤";
    else if("IC"==str) return "中证500股指";
    else if("IF"==str) return "IF";
    else if("IH"==str) return "IH";
    else if("T"==str) return "T";
    else if("TF"==str) return "TF";
    else if("AP"==str) return "鲜苹果";
    else return "";

}


void DataInitInstance::initTradeApi()
{

}

string DataInitInstance::getTime(){
    time_t timep;
    time (&timep);
    char tmp[64];
    //strftime(tmp, sizeof(tmp), "%Y-%m-%d~%H:%M:%S",localtime(&timep) );
    strftime(tmp, sizeof(tmp), "%Y-%m-%d",localtime(&timep) );
    return tmp;
}

#endif
