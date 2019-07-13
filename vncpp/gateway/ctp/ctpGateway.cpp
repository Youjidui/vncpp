#include "ctpGateway.h"
#include <alloca.h>
#include <stdint.h>
#include <time.h>
#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <boost/locale/encoding.hpp>
#include "logging.h"
#include "vtGateway.h"
#include "vtObject.h"
#include <ThostFtdcMdApi.h>
#include <ThostFtdcTraderApi.h>

#define CTP_GATEWAY "CTP Gateway"

inline std::string tr_gbk_to_utf8(const std::string & str) {
	return boost::locale::conv::between(str, "UTF-8", "GBK");
}

inline std::string tr_utf8_to_gbk(const std::string & str) {
	return boost::locale::conv::between(str, "GBK", "UTF-8");
}

time_t datetimeFromString(const char* pdate8)
{
	struct tm dt = {0};
	sscanf(pdate8, "%04u%02u%02u", &dt.tm_year, &dt.tm_mon, &dt.tm_mday);
	dt.tm_year -= 1900;
	dt.tm_mon --;
	return mktime(&dt);
}

time_t datetimeFromString(const char* pdate8, const char* ptime8)
{
	struct tm dt = {0};
	sscanf(pdate8, "%04u%02u%02u", &dt.tm_year, &dt.tm_mon, &dt.tm_mday);
	dt.tm_year -= 1900;
	dt.tm_mon --;
	sscanf(ptime8, "%02u%*c%02u%*c%02u", &dt.tm_hour, &dt.tm_min, &dt.tm_sec);
	return mktime(&dt);
}

class MDEventHandler : public CThostFtdcMdSpi
{
	Gateway& m_base;
	CThostFtdcMdApi* m_api;
	volatile int m_requestID;
	std::string m_brokerID;
	std::string m_userID;
	std::string m_password;
	std::string m_appID;
	std::string m_authCode;

	std::vector<std::string> m_subscribedSymbols;

public:
	MDEventHandler(Gateway& base)
		: m_base(base)
		, m_api(NULL)
		, m_requestID(0)
	{
	}
	~MDEventHandler()
	{
		destroy();
	}
	bool create(const boost::property_tree::ptree* parameters)
	{
		if(parameters)
		{
			std::string connectionString;
			connectionString = parameters->get("MarketConnectionString", connectionString);
			std::string emptyString;
			m_brokerID = parameters->get("brokerID", emptyString);
			m_userID = parameters->get("userID", emptyString);
			m_password = parameters->get("password", emptyString);
			m_appID = parameters->get("AppID", emptyString);
			m_authCode = parameters->get("AuthenticationCode", emptyString);

			LOG_INFO << "CThostFtdcMdApi " << CThostFtdcMdApi::GetApiVersion();

			destroy();
			m_api =(CThostFtdcMdApi::CreateFtdcMdApi());
			if(m_api)
			{
				if(!connectionString.empty())
				{
					char* buffer = (char*)alloca(connectionString.size() + 1);
					strcpy(buffer, connectionString.c_str());
					m_api->RegisterFront(buffer);
				}
				else
					LOG_ERROR << "connectionString is empty";
				m_api->RegisterSpi(this);
				m_api->Init();
				return true;
			}
			else
			{
				LOG_ERROR << "Cannot create CThostFtdcMdApi";
			}
		}
		return false;
	}

	void destroy()
	{
		if(m_api)
		{
			m_api->Release();
			m_api = NULL;
		}
	}

	void subscribe(SubscribeRequestPtr p)
	{
		char** ppInstrumentID = new char*[1+1];
		memset(ppInstrumentID, 0, sizeof(char*) * (1 + 1));
		char* pi = new char[p->symbol.size() + 1];
		strcpy(pi, p->symbol.c_str());
		ppInstrumentID[0] = pi;
		m_api->SubscribeMarketData(ppInstrumentID, m_subscribedSymbols.size());

		m_subscribedSymbols.push_back(p->symbol);

		delete[] ppInstrumentID[0];
		delete[] ppInstrumentID;
	}

	virtual void OnFrontConnected()
	{
		CThostFtdcReqUserLoginField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_brokerID.c_str());
		strcpy(req.UserID, m_userID.c_str());
		strcpy(req.Password, m_password.c_str());
		strcpy(req.UserProductInfo, "VNCPP");
		m_api->ReqUserLogin(&req, ++m_requestID);
	}
	virtual void OnFrontDisconnected(int nReason)
	{
		//event not ready, disconneced
		LOG_ERROR << __FUNCTION__ << ':' << nReason;
	}
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		//event ready
		char** ppInstrumentID = new char*[m_subscribedSymbols.size() + 1];
		memset(ppInstrumentID, 0, sizeof(char*) * (m_subscribedSymbols.size() + 1));
		int n = 0;
		for(auto& i : m_subscribedSymbols)
		{
			char* p = new char[i.size() + 1];
			strcpy(p, i.c_str());
			ppInstrumentID[n++] = p;
		}
		m_api->SubscribeMarketData(ppInstrumentID, m_subscribedSymbols.size());

		for(unsigned int i = 0; i < m_subscribedSymbols.size(); ++i)
			delete[] ppInstrumentID[i];
		delete[] ppInstrumentID;
	}
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pUserLogout)
		{
			LOG_DEBUG << __FUNCTION__ << ':' << pUserLogout->BrokerID << '\t' << pUserLogout->UserID; 
		}
		if(pRspInfo)
		{
			LOG_DEBUG  << __FUNCTION__ <<'[' << nRequestID << ']' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}

	}
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		//event error
		LOG_ERROR << __FUNCTION__ <<'[' << nRequestID << ']' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
	}
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pSpecificInstrument)
		{
			LOG_DEBUG << __FUNCTION__ << ':' << pSpecificInstrument->InstrumentID; 
		}
		if(pRspInfo)
		{
			LOG_DEBUG  << __FUNCTION__ <<'[' << nRequestID << ']' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
	}
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pSpecificInstrument)
		{
			LOG_DEBUG << __FUNCTION__ << ':' << pSpecificInstrument->InstrumentID; 
		}
		if(pRspInfo)
		{
			LOG_DEBUG  << __FUNCTION__ <<'[' << nRequestID << ']' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
	}
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *p)
	{
		auto t = std::make_shared<Tick>();
		t->symbol = p->InstrumentID;
		t->exchange = p->ExchangeID;
		t->lastPrice = p->LastPrice;
		t->lastVolume = p->Volume;
		t->datetime = datetimeFromString(p->TradingDay, p->UpdateTime);
		t->bidPrice[0] = p->BidPrice1;
		t->bidVolume[0] = p->BidVolume1;
		t->askPrice[0] = p->AskPrice1;
		t->askVolume[0] = p->AskVolume1;

		m_base.onTick(t);
	}

};


class TradingEventHandler : public CThostFtdcTraderSpi
{
private:
	Gateway& m_base;
	CThostFtdcTraderApi* m_api;
	volatile int m_requestID;
	std::string m_brokerID;
	std::string m_userID;
	std::string m_password;
	std::string m_appID;
	std::string m_authCode;
	unsigned m_frontID;
	unsigned m_sessionID;

	std::map<std::string, double> m_symbolSizeDict;
	std::map<std::string, PositionPtr> m_posDict;

public:
	TradingEventHandler(Gateway& base)
		: m_base(base)
		, m_api(NULL)
		, m_requestID(0)
		, m_frontID(0)
		, m_sessionID(0)
	{
	}
	~TradingEventHandler()
	{
		destroy();
	}
	bool create(const boost::property_tree::ptree* parameters)
	{
		if(parameters)
		{
			std::string connectionString;
			connectionString = parameters->get("TradingConnectionString", connectionString);
			std::string emptyString;
			m_brokerID = parameters->get("brokerID", emptyString);
			m_userID = parameters->get("userID", emptyString);
			m_password = parameters->get("password", emptyString);
			m_appID = parameters->get("AppID", emptyString);
			m_authCode = parameters->get("AuthenticationCode", emptyString);

			LOG_INFO << "CThostFtdcTraderApi" << CThostFtdcTraderApi::GetApiVersion();

			destroy();
			m_api =(CThostFtdcTraderApi::CreateFtdcTraderApi());
			if(m_api)
			{
				if(!connectionString.empty())
				{
					char* buffer = (char*)alloca(connectionString.size() + 1);
					strcpy(buffer, connectionString.c_str());
					m_api->RegisterFront(buffer);
				}
				else
					LOG_ERROR << "connectionString is empty";
				m_api->RegisterSpi(this);
				THOST_TE_RESUME_TYPE rtype = THOST_TERT_RESUME;
				m_api->SubscribePublicTopic(rtype);
				m_api->SubscribePrivateTopic(rtype);
				m_api->Init();
				return true;
			}
			else
			{
				LOG_ERROR << "Cannot create CThostFtdcMdApi";
			}
		}
		return false;
	}

	void destroy()
	{
		if(m_api)
		{
			m_api->Release();
			m_api = NULL;
		}
	}

public:
	virtual void OnFrontConnected()
	{
		if(!m_appID.empty())
		{
		CThostFtdcReqAuthenticateField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_brokerID.c_str());
		strcpy(req.UserID, m_userID.c_str());
#if 0
		strcpy(req.UserProductInfo, "VNCPP");
		strcpy(req.AppID, m_appID.c_str());
#else
		strcpy(req.UserProductInfo, m_appID.c_str());
#endif
		strcpy(req.AuthCode, m_authCode.c_str());

		m_api->ReqAuthenticate(&req, ++m_requestID);
		}
		else
		{
		CThostFtdcReqUserLoginField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_brokerID.c_str());
		strcpy(req.UserID, m_userID.c_str());
		strcpy(req.Password, m_password.c_str());
		strcpy(req.UserProductInfo, "VNCPP");
		m_api->ReqUserLogin(&req, ++m_requestID);
		}
	}
	virtual void OnFrontDisconnected(int nReason)
	{
		LOG_ERROR << __FUNCTION__ << ':' << nReason;
	}
		
	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse)
	{
		LOG_ERROR << __FUNCTION__ << ':' << nTimeLapse;
	}
	
	///客户端认证响应
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pRspInfo && pRspInfo->ErrorID != 0)
		{
			LOG_ERROR << __FUNCTION__ << ':' << pRspAuthenticateField->UserProductInfo << '\t' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
		else 
		{
		CThostFtdcReqUserLoginField req;
		memset(&req, 0, sizeof(req));
		strcpy(req.BrokerID, m_brokerID.c_str());
		strcpy(req.UserID, m_userID.c_str());
		strcpy(req.Password, m_password.c_str());
		strcpy(req.UserProductInfo, "VNCPP");
		m_api->ReqUserLogin(&req, ++m_requestID);
		}
	}
	

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *p, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pRspInfo && pRspInfo->ErrorID != 0)
		{
			LOG_ERROR << __FUNCTION__ << ':' << p->UserID << '\t' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
		else 
		{
			LOG_INFO << __FUNCTION__ << ':' << p->TradingDay << ',' << p->LoginTime << ',' 
				<< p->BrokerID << ',' << p->UserID << ',' << p->SystemName << ',' 
				<< p->FrontID << ',' << p->SessionID << ',' << p->MaxOrderRef << ',' 
				<< p->SHFETime << ',' << p->DCETime << ',' << p->CZCETime << ','
				<< p->FFEXTime << ',' << p->INETime;
			m_frontID = p->FrontID;
			m_sessionID = p->SessionID;

			//CThostFtdcQryExchangeField req;
			//memset(&req, 0, sizeof(req));
			//m_api->ReqQryExchange(&req, ++m_requestID);
			
			CThostFtdcQrySettlementInfoConfirmField req;
			strcpy(req.BrokerID, m_brokerID.c_str());
			strcpy(req.InvestorID, m_userID.c_str());
			m_api->ReqQrySettlementInfoConfirm(&req, ++m_requestID);
		}
	}

	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
	}

	///用户口令更新请求响应
	virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///资金账户口令更新请求响应
	virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///报单录入请求响应
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///预埋单录入请求响应
	virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///预埋撤单录入请求响应
	virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///报单操作请求响应
	virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///查询最大报单数量响应
	virtual void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pRspInfo)
		{
			LOG_WARNING << __FUNCTION__ << ':' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
		if(bIsLast)
		{
			CThostFtdcQryInstrumentField req;
			memset(&req, 0, sizeof(req));
			m_api->ReqQryInstrument(&req, ++m_requestID);
		}
	}

	///删除预埋单响应
	virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///删除预埋撤单响应
	virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询报单响应
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询成交响应
	virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询投资者持仓响应
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pRspInfo)
		{
			LOG_WARNING << __FUNCTION__ << ':' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
		if(pInvestorPosition)
		{
			std::string posName = pInvestorPosition->InstrumentID;
			posName += (DIRECTION_LONG == pInvestorPosition->PosiDirection ? 'B' : 'S');
			PositionPtr p;
			auto i = m_posDict.find(posName);
			if(i != m_posDict.end())
				p = i->second;
			else
			{
				p = std::make_shared<Position>();
				p->gatewayName = CTP_GATEWAY;
				p->symbol = pInvestorPosition->InstrumentID;
				p->vtSymbol = p->make_vtSymbol(pInvestorPosition->InstrumentID, pInvestorPosition->ExchangeID);
				p->direction = pInvestorPosition->PosiDirection;
				p->vtPositionName = posName;
				p->avgPrice = 0;
				p->position = 0;
				p->profit = 0;
				p->forzen = 0;
				m_posDict.insert(std::make_pair(posName, p));
			}

			if(strcmp(pInvestorPosition->ExchangeID, "SHFE") == 0)
			{
				if(pInvestorPosition->YdPosition != 0 && pInvestorPosition->TodayPosition == 0)
					p->yesterdayPosition = pInvestorPosition->Position;
			}
			else
			{
				p->yesterdayPosition = pInvestorPosition->Position - pInvestorPosition->TodayPosition;
			}

			double size = 1;
			auto it = m_symbolSizeDict.find(p->symbol);
			if(it != m_symbolSizeDict.end())
			{
				size = it->second;
			}
			auto cost = p->avgPrice * p->position * size;

			p->position += pInvestorPosition->Position;
			p->profit += pInvestorPosition->PositionProfit;
			
			if(p->position != 0)
			{
				p->avgPrice = (cost + pInvestorPosition->PositionCost) / (p->position * size);
			}

			p->forzen += ((p->direction == DIRECTION_LONG) ? pInvestorPosition->LongFrozen : pInvestorPosition->ShortFrozen);

			if(bIsLast)
			{
				for(auto i : m_posDict)
				{
					m_base.onPosition(i.second);
				}
				m_posDict.clear();
			}
		}
	}

	///请求查询资金账户响应
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		auto p = std::make_shared<Account>();
		//p->gatewayName = CTP_GATEWAY;
		p->account = pTradingAccount->AccountID;
		p->vtAccount = std::string(CTP_GATEWAY) + '/' + p->account;
		p->previousBalance = pTradingAccount->PreBalance;
		p->available = pTradingAccount->Available;
		p->commission = pTradingAccount->Commission;
		p->margin = pTradingAccount->CurrMargin;
		p->closeProfit = pTradingAccount->CloseProfit;
		p->positionProfit = pTradingAccount->PositionProfit;
		p->balance = pTradingAccount->PreBalance - pTradingAccount->PreCredit - pTradingAccount->PreMortgage
			+ pTradingAccount->Mortgage - pTradingAccount->Withdraw + pTradingAccount->Deposit
			+ pTradingAccount->CloseProfit + pTradingAccount->PositionProfit + pTradingAccount->CashIn
			- pTradingAccount->Commission;

		m_base.onAccount(p);
	}

	///请求查询投资者响应
	virtual void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询交易编码响应
	virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询合约保证金率响应
	virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询合约手续费率响应
	virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询交易所响应
	virtual void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pRspInfo)
		{
			LOG_WARNING << __FUNCTION__ << ':' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
		if(pExchange)
		{
			LOG_INFO << pExchange->ExchangeID << '\t' << pExchange->ExchangeName << '\t' << pExchange->ExchangeProperty; 
			CThostFtdcQryProductField req;
			memset(&req, 0, sizeof(req));
			strcpy(req.ExchangeID, pExchange->ExchangeID);
			m_api->ReqQryProduct(&req, ++m_requestID);
		}
		if(bIsLast)
		{
			LOG_DEBUG << __FUNCTION__ << " complete";
		}
	}

	///请求查询产品响应
	virtual void OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pRspInfo)
		{
			LOG_WARNING << __FUNCTION__ << ':' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
		if(pProduct)
		{
			LOG_INFO << pProduct->ExchangeID << '\t' << pProduct->ProductID << '\t' << pProduct->ProductName << '\t' << pProduct->ProductClass
				<< '\t' << pProduct->VolumeMultiple << '\t' << pProduct->PriceTick << '\t' << pProduct->TradeCurrencyID
				<< '\t' << pProduct->ExchangeProductID << '\t' << pProduct->UnderlyingMultiple;

			CThostFtdcQryInstrumentField req;
			memset(&req, 0, sizeof(req));
			strcpy(req.ExchangeID, pProduct->ExchangeID);
			strcpy(req.ProductID, pProduct->ProductID);
			m_api->ReqQryInstrument(&req, ++m_requestID);
		}
		if(bIsLast)
		{
			LOG_DEBUG << __FUNCTION__ << '\t' << (pProduct  ? pProduct->ExchangeID : "")  << " complete";
		}
	}

	///请求查询合约响应
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if(pRspInfo)
		{
			LOG_WARNING << __FUNCTION__ << ':' << pRspInfo->ErrorID << '\t' << pRspInfo->ErrorMsg;
		}
		if(pInstrument)
		{
			auto p = std::make_shared<Contract>();
			p->symbol = pInstrument->InstrumentID;
			p->exchange = pInstrument->ExchangeID;
			p->vtSymbol = _Contract::make_vtSymbol(p->symbol, p->exchange);
			p->gatewayName = CTP_GATEWAY;
			//p->name = pInstrument->InstrumentName;
			p->name = tr_gbk_to_utf8(pInstrument->InstrumentName);
			p->contractSize = pInstrument->VolumeMultiple;
			p->priceTick = pInstrument->PriceTick;
			p->strikePrice = pInstrument->StrikePrice;
			p->productClass = pInstrument->StrikePrice;
			p->expiryDate = datetimeFromString(pInstrument->ExpireDate);
			p->underlyingSymbol = pInstrument->UnderlyingInstrID;
			p->optionType = pInstrument->OptionsType;

			m_symbolSizeDict[p->symbol] = p->contractSize;
			m_base.onContract(p);
		}
		if(bIsLast)
		{
			LOG_DEBUG << __FUNCTION__ << '\t' << (pInstrument  ? pInstrument->ExchangeID : "") 
				<< '\t' << (pInstrument  ? pInstrument->ProductID : "")  << " complete";
		}
	}

	///请求查询行情响应
	virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询投资者结算结果响应
	virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询客户通知响应
	virtual void OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询结算信息确认响应
	virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询投资者持仓明细响应
	virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///查询保证金监管系统经纪公司资金账户密钥响应
	virtual void OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询投资者品种/跨品种保证金响应
	virtual void OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询交易所保证金率响应
	virtual void OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询交易所调整保证金率响应
	virtual void OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询产品组
	virtual void OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询期权交易成本响应
	virtual void OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///请求查询期权合约手续费响应
	virtual void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}


	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

	///报单通知
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder) {}

	///成交通知
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade) {}

	///报单录入错误回报
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) {}

	///报单操作错误回报
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {}

	///合约交易状态通知
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) {}
	
};


class CtpGateway : public Gateway
{
	const boost::property_tree::ptree* m_parameters;
	MDEventHandler m_mdHandler;
	TradingEventHandler m_tdHandler;
	bool m_mdReady;
	bool m_tdReady;

	public:
	virtual const std::string getClassName()
	{
		return CTP_GATEWAY;
	}

	CtpGateway(const std::string& aInstanceName, EventEngine& ee)
		: Gateway(aInstanceName, ee)
		  , m_parameters(NULL)
		  , m_mdHandler(*this)
		  , m_tdHandler(*this)
		  , m_mdReady(false)
		  , m_tdReady(false)
	{
	}

	void setParameter(const boost::property_tree::ptree* parameters)
	{
		m_parameters = parameters;
	}

	virtual void connect()
	{
		m_mdReady = m_mdHandler.create(parameters);
		if(!m_mdReady)
		{
			LOG_ERROR << "Create CTP API instance failed";
		}
	}
	virtual void close()
	{
		m_mdReady = false;
		m_mdHandler.destroy();
	}

	virtual void subscribe(SubscribeRequestPtr p)
	{
		if(m_mdReady)
		{
			m_mdHandler.subscribe(p);
		}
	}
	virtual OrderID sendOrder(OrderRequestPtr p)
	{
		return OrderID();
	}
	virtual void cancelOrder(CancelOrderRequestPtr p)
	{}

	virtual AccountPtr queryAccount()
	{
		return AccountPtr();
	}
	virtual std::vector<PositionPtr> queryPosition()
	{
		return std::vector<PositionPtr>();
	}
	//virtual void queryHistory(QueryHistoryRequestPtr p) = 0;
};

GatewayPtr ctpGateway(const std::string& aInstanceName, EventEnginePtr e)
{
	LOG_DEBUG << __FUNCTION__;
	//return GatewayPtr(new CtpGateway(aInstanceName, e), GatewayDeleter(&staticLibraryDestroyGatewayInstance, NULL));
	return std::make_shared<CtpGateway>(aInstanceName, *e);
}

