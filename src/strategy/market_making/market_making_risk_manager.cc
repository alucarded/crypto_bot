#include "strategy/market_making/market_making_risk_manager.h"

MarketMakingRiskManager::MarketMakingRiskManager(const MarketMakingRiskMangerOptions& opts, ExchangeClient* exchange_client)
    : m_options(opts), m_exchange_client(exchange_client) {

}

MarketMakingRiskManager::~MarketMakingRiskManager() {

}

void MarketMakingRiskManager::OnConnectionOpen(const std::string&) {

}

void MarketMakingRiskManager::OnConnectionClose(const std::string&) {

}

void MarketMakingRiskManager::OnAccountBalanceUpdate(const AccountBalance&) {

}

void MarketMakingRiskManager::OnOrderUpdate(const Order&) {

}

void MarketMakingRiskManager::OnPrediction(const RangePrediction&) {

}