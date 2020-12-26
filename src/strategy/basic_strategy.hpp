#include "trading_strategy.h"

#include <string>

struct BasicStrategyOptions : StrategyOptions {

};

class BasicStrategy : public TradingStrategy, public Consumer<RawTicker> {
public:
  // TODO: move more generic stuff to TradingStrategy class
  BasicStrategy(const BasicStrategyOptions& opts, ExchangeAccount* exchange_account)
      : m_opts(opts), m_exchange_account(exchange_account) {

  }

  virtual void execute(const std::map<std::string, Ticker>& tickers) override {
    //m_exchange_account->OnTicker(tickers[m_opts.m_trading_exchange]);
    if (tickers.size() < m_opts.m_required_exchanges) {
      std::cout << "Not enough exchanges" << std::endl;
      return;
    }
    std::cout << "Executing strategy" << std::endl;
  }

  virtual void Consume(const RawTicker& raw_ticker) override {
    // std::cout << "Consumed:" << std::endl;
    // std::cout << raw_ticker << std::endl;
    if (raw_ticker.m_bid.empty() || raw_ticker.m_ask.empty()) {
      m_tickers.erase(raw_ticker.m_exchange);
      std::cout << "Empty ticker: " << raw_ticker << std::endl;
      if (m_opts.m_trading_exchange == raw_ticker.m_exchange) {
        m_exchange_account->OnDisconnected();
        return;
      }
    } else {
      Ticker ticker;
      ticker.m_bid = std::stod(raw_ticker.m_bid);
      ticker.m_bid_vol = raw_ticker.m_bid_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_bid_vol));
      ticker.m_ask = std::stod(raw_ticker.m_ask);
      ticker.m_ask_vol = raw_ticker.m_ask_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_ask_vol));
      ticker.m_source_ts = raw_ticker.m_source_ts ? std::optional<int64_t>(raw_ticker.m_source_ts) : std::nullopt;
      ticker.m_arrived_ts = raw_ticker.m_arrived_ts;
      if (m_tickers.count(raw_ticker.m_exchange)) {
        assert(ticker.m_arrived_ts > m_tickers[raw_ticker.m_exchange].m_arrived_ts);
      }
      m_tickers[raw_ticker.m_exchange] = ticker;
      if (m_opts.m_trading_exchange == raw_ticker.m_exchange) {
        m_exchange_account->OnTicker(ticker);
      }
    }
    execute(m_tickers);
  }

private:
  BasicStrategyOptions m_opts;
  ExchangeAccount* m_exchange_account;
  std::map<std::string, Ticker> m_tickers;
};