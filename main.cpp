#include <iostream>
#include <glog/logging.h>
#include "dataprocessor.h"
#include "traderspi.h"
#include "mdspi.h"
using namespace std;
DataInitInstance dii;
// UserApi对象
int main()
{
    google::InitGoogleLogging("");
    system("mkdir -p log");
    google::SetLogDestination(google::GLOG_INFO, "./log/info_");
    google::SetLogDestination(google::GLOG_ERROR, "./log/error_");
    dii.GetConfigFromFile();
     CTraderSpi*cspi=new CTraderSpi(dii);
//    CMdSpi*pspi=new CMdSpi(dii);

    while(1){
        boost::this_thread::sleep(boost::posix_time::seconds(1000));    //microsecond,millisecn
    }
    return 0;
}

