#include "backtest_exchange_client.hpp"
#include "exchange/account_balance_listener.h"
#include "model/symbol.h"
#include "model/ticker.h"

#include <initializer_list>
#include <iostream>
#include <memory>
#include <unordered_map>

class BacktestResultsProcessor : public AccountBalanceListener {
public:
  BacktestResultsProcessor(const std::string& results_file);
  ~BacktestResultsProcessor();

  std::unordered_map<SymbolId, double> GetCumulativeBalances();
  // double CalculateAssetsValue(const std::unordered_map<SymbolPairId, double>& prices, SymbolId quote_asset);

  virtual void OnAccountBalanceUpdate(const AccountBalance& account_balance) override;
// private:
//   void WriteBalanceCsvHeaders();
private:
  std::unordered_map<std::string, std::unordered_map<SymbolId, double>> m_exchange_balances;
  std::ofstream m_results_file;
};

std::ostream& operator<<(std::ostream& os, const std::unordered_map<SymbolId, double>& sdmap);