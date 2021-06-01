#include "backtest_results_processor.h"

#include "utils/string.h"

BacktestResultsProcessor::BacktestResultsProcessor(const std::string& results_file) : m_results_file(results_file, std::ios::out | std::ios::app | std::ios::binary) {
}

BacktestResultsProcessor::~BacktestResultsProcessor() {
  m_results_file.close();
}

std::unordered_map<SymbolId, double> BacktestResultsProcessor::GetCumulativeBalances() {
  std::unordered_map<SymbolId, double> res;
  for (const auto& p : m_exchange_balances) {
    const auto& balance_map = p.second;
    for (const auto& balance : balance_map) {
      if (res.find(balance.first) == res.end()) {
        res.emplace(balance.first, 0.0d);
      }
      res[balance.first] += balance.second;
    }
  }
  return res;
}

// double BacktestResultsProcessor::CalculateAssetsValue(const std::unordered_map<SymbolPairId, double>& prices, SymbolId quote_asset) {
//   double account_value = 0;
//   for (auto* client : m_backtest_clients) {
//     auto balance_map = GetBalanceMap(client);
//     for (const auto& p : balance_map) {
//       if (p.first == quote_asset) {
//         account_value += p.second;
//         continue;
//       }
//       SymbolPair sp(p.first, quote_asset);
//       SymbolPairId spid = SymbolPairId(sp);
//       assert(prices.find(spid) != prices.end());
//       double price = prices.at(SymbolPairId(sp));
//       account_value += p.second * price;
//     }
//   }
//   return account_value;
// }

  // void PrintBalances() {
  //   const size_t sz = m_balances.size();
  //   for (auto it = m_balances.begin(); it != m_balances.end(); ++it) {
  //     const SymbolId& spid = it->first;
  //     // TODO: FIXME: set correct precision!!
  //     std::string b = std::to_string(m_balances.at(spid));
  //     m_results_file << b;
  //     if (std::next(it) != m_balances.end()) {
  //       m_results_file << ",";
  //     } else {
  //       // TODO: initial tickers can vary depending on exchanges, you want some fixed initial tickers probably...
  //       double account_value = CalculateAccountValue(SymbolId::USDT, m_initial_tickers);
  //       m_results_file << "," << std::to_string(account_value) << "\n";
  //     }
  //   }
  //   m_results_file.flush();
  // }

void BacktestResultsProcessor::OnBalanceUpdate(const AccountBalance& account_balance) {
  // Write to file
  // TODO:
  // Update
  m_exchange_balances[account_balance.GetExchange()] = account_balance.GetBalanceMap();
}

// void BacktestResultsProcessor::WriteBalanceCsvHeaders() {
//   std::ostringstream oss;
//   for (auto it = m_balances.begin(); it != m_balances.end(); ++it) {
//     const SymbolId& spid = it->first;
//     if (std::next(it) != m_balances.end()) {
//       oss << spid << ",";
//     } else {
//       oss << spid << "," << "Account Value" << "\n";
//     }
//   }
//   m_results_file << oss.str();
// }

std::ostream& operator<<(std::ostream& os, const std::unordered_map<SymbolId, double>& sdmap) {
  for (const auto& p : sdmap) {
    os << p.first << ": " << cryptobot::to_string(p.second, 10) << std::endl;
  }
  return os;
}