#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <ThostFtdcMdApi.h>

#include "dataprocessor.h"
#include "property.h"


using namespace std;
using boost::locale::conv::between;
using boost::lexical_cast;
using boost::split;
using boost::is_any_of;
string strDepthMarketData(CThostFtdcDepthMarketDataField *pmd);
string strInstrument(CThostFtdcInstrumentField *pInstrument);
#endif // UTIL_H
