#include "util.h"

string strInstrument(CThostFtdcInstrumentField *pInstrument)
{

    string tmp;
    if(pInstrument==NULL)
        return ( tmp);
    string invesinfo=
            "合约代码 InstrumentID="+lexical_cast<string>(pInstrument->InstrumentID)+
            "\n 交易所代码 ExchangeID="+pInstrument->ExchangeID+
            "\n 合约名称 InstrumentName="+ between(pInstrument->InstrumentName,"UTF-8","GBK")+
            "\n 合约在交易所的代码 ExchangeInstID="+pInstrument->ExchangeInstID+
            "\n 产品代码 ProductID="+lexical_cast<string>(pInstrument->ProductID)+
            "\n 产品类型 ProductClass="+lexical_cast<string>(pInstrument->ProductClass)+

            "\n 交割年份 DeliveryYear="+lexical_cast<string>(pInstrument->DeliveryYear)+
            "\n 交割月 DeliveryMonth="+lexical_cast<string>(pInstrument->DeliveryMonth)+
            "\n 市价单最大下单量 MaxMarketOrderVolume="+lexical_cast<string>(pInstrument->MaxMarketOrderVolume)+
            "\n 市价单最小下单量 MinMarketOrderVolume="+lexical_cast<string>(pInstrument->MinMarketOrderVolume)+
            "\n 限价单最大下单量 MaxLimitOrderVolume="+lexical_cast<string>(pInstrument->MaxLimitOrderVolume)+

            "\n 限价单最小下单量 MinLimitOrderVolume="+lexical_cast<string>(pInstrument->MinLimitOrderVolume)+
            "\n 合约数量乘数 VolumeMultiple="+lexical_cast<string>(pInstrument->VolumeMultiple)+
            "\n 最小变动价位 PriceTick="+lexical_cast<string>(pInstrument->PriceTick)+
            "\n 创建日 CreateDate="+lexical_cast<string>(pInstrument->CreateDate)+
            "\n 上市日 OpenDate="+lexical_cast<string>(pInstrument->OpenDate)+


            "\n 到期日 ExpireDate="+pInstrument->ExpireDate+
            "\n 开始交割日 StartDelivDate="+pInstrument->StartDelivDate+
            "\n 结束交割日 EndDelivDate="+pInstrument->EndDelivDate+
            "\n 合约生命周期状态 InstLifePhase="+lexical_cast<string>(pInstrument->InstLifePhase)+
            "\n 当前是否交易 IsTrading="+lexical_cast<string>(pInstrument->IsTrading)+

            "\n 持仓类型 PositionType="+pInstrument->PositionType+
            "\n 持仓日期类型 PositionDateType="+lexical_cast<string>(pInstrument->PositionDateType)+
            "\n 多头保证金率 LongMarginRatio="+lexical_cast<string>(pInstrument->LongMarginRatio)+
            "\n 空头保证金率 ShortMarginRatio="+lexical_cast<string>(pInstrument->ShortMarginRatio)+
            "\n 是否使用大额单边保证金算法 MaxMarginSideAlgorithm="+lexical_cast<string>(pInstrument->MaxMarginSideAlgorithm)+


            "\n 基础商品代码 UnderlyingInstrID="+pInstrument->UnderlyingInstrID+
            "\n 执行价 StrikePrice="+lexical_cast<string>(pInstrument->StrikePrice)+
            "\n 期权类型 OptionsType="+pInstrument->OptionsType+
            "\n 合约基础商品乘数 UnderlyingMultiple="+lexical_cast<string>(pInstrument->UnderlyingMultiple)+
            "\n 组合类型 CombinationType="+lexical_cast<string>(pInstrument->CombinationType);

return invesinfo;

}


string strDepthMarketData(CThostFtdcDepthMarketDataField *pmd)
{

    string tmp;
    if(pmd==NULL)
        return ( tmp);
    string invesinfo=
            "交易日 TradingDay="+lexical_cast<string>(pmd->TradingDay)+
            "\n 合约代码 InstrumentID="+pmd->InstrumentID+
            "\n 交易所代码 ExchangeID="+pmd->ExchangeID+
            "\n 合约在交易所的代码 ExchangeInstID="+pmd->ExchangeInstID+
            "\n 最新价 LastPrice="+lexical_cast<string>(pmd->LastPrice)+
            "\n 上次结算价 PreSettlementPrice="+lexical_cast<string>(pmd->PreSettlementPrice)+
            "\n 昨收盘 PreClosePrice="+lexical_cast<string>(pmd->PreClosePrice)+
            "\n 昨持仓量 PreOpenInterest="+lexical_cast<string>(pmd->PreOpenInterest)+
            "\n 今开盘 OpenPrice="+lexical_cast<string>(pmd->OpenPrice)+
            "\n 最高价 HighestPrice="+lexical_cast<string>(pmd->HighestPrice)+
            "\n 最低价 LowestPrice="+lexical_cast<string>(pmd->LowestPrice)+

            "\n 数量 Volume="+lexical_cast<string>(pmd->Volume)+
            "\n 成交金额 Turnover="+lexical_cast<string>(pmd->Turnover)+
            "\n 持仓量 OpenInterest="+lexical_cast<string>(pmd->OpenInterest)+
            "\n 今收盘 ClosePrice="+lexical_cast<string>(pmd->ClosePrice)+
            "\n 本次结算价 SettlementPrice="+lexical_cast<string>(pmd->SettlementPrice)+
            "\n 涨停板价 UpperLimitPrice="+lexical_cast<string>(pmd->UpperLimitPrice)+
            "\n 跌停板价 LowerLimitPrice="+lexical_cast<string>(pmd->LowerLimitPrice)+
            "\n 昨虚实度 PreDelta="+lexical_cast<string>(pmd->PreDelta)+
            "\n 今虚实度 CurrDelta="+lexical_cast<string>(pmd->CurrDelta)+
            "\n 最后修改时间 UpdateTime="+lexical_cast<string>(pmd->UpdateTime)+
            "\n 最后修改毫秒 UpdateMillisec="+lexical_cast<string>(pmd->UpdateMillisec)+
            "\n 申买价一 BidPrice1="+lexical_cast<string>(pmd->BidPrice1)+
            "\n 申买量一 BidVolume1="+lexical_cast<string>(pmd->BidVolume1)+
            "\n 申卖价一 AskPrice1="+lexical_cast<string>(pmd->AskPrice1)+
            "\n 申卖量一 AskVolume1="+lexical_cast<string>(pmd->AskVolume1)+
            "\n 申买价二 BidPrice2="+lexical_cast<string>(pmd->BidPrice2)+
            "\n 申买量二 BidVolume2="+lexical_cast<string>(pmd->BidVolume2)+
            "\n 申卖价二 AskPrice2="+lexical_cast<string>(pmd->AskPrice2)+

            "\n 申卖量二 AskVolume2="+lexical_cast<string>(pmd->AskVolume2)+
            "\n 申买价三 BidPrice3="+lexical_cast<string>(pmd->BidPrice3)+
            "\n 申买量三 BidVolume3="+lexical_cast<string>(pmd->BidVolume3)+
            "\n 申卖价三 AskPrice3="+lexical_cast<string>(pmd->AskPrice3)+
            "\n 申卖量三 AskVolume3="+lexical_cast<string>(pmd->AskVolume3)+
            "\n 申买价四 BidPrice4="+lexical_cast<string>(pmd->BidPrice4)+
            "\n 申买量四 BidVolume4="+lexical_cast<string>(pmd->BidVolume4)+
            "\n 申卖价四 AskPrice4="+lexical_cast<string>(pmd->AskPrice4)+
            "\n 申卖量四 AskVolume4="+lexical_cast<string>(pmd->AskVolume4)+
            "\n 申买价五 BidPrice5="+lexical_cast<string>(pmd->BidPrice5)+
            "\n 申买量五 BidVolume5="+lexical_cast<string>(pmd->BidVolume5)+
            "\n 申卖价五 AskPrice5="+lexical_cast<string>(pmd->AskPrice5)+
            "\n 申卖量五 AskVolume5="+lexical_cast<string>(pmd->AskVolume5)+
            "\n 当日均价 AveragePrice="+lexical_cast<string>(pmd->AveragePrice)+
             "\n 业务日期 ActionDay="+lexical_cast<string>(pmd->ActionDay);

    return invesinfo;
}
