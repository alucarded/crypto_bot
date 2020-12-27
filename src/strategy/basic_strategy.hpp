#include "trading_strategy.h"

#include <string>

struct BasicStrategyOptions : StrategyOptions {
  double m_profit_threshold = 50.0;
};

class BasicStrategy : public TradingStrategy, public Consumer<RawTicker> {
public:
  // TODO: move more generic stuff to TradingStrategy class
  BasicStrategy(const BasicStrategyOptions& opts, ExchangeAccount* exchange_account)
      : m_opts(opts), m_exchange_account(exchange_account) {

  }

  virtual void execute(const std::map<std::string, Ticker>& tickers) override {
    if (!tickers.count(m_opts.m_trading_exchange)) {
      return;
    }
    //m_exchange_account->OnTicker(tickers[m_opts.m_trading_exchange]);
    if (tickers.size() < m_opts.m_required_exchanges) {
      std::cout << "Not enough exchanges" << std::endl;
      return;
    }
    //std::cout << "Executing strategy" << std::endl;
    double avg_bid = 0;
    //double bid_vol_sum = 0;
    double avg_ask = 0;
    //double ask_vol_sum = 0;
    for (const std::pair<std::string, Ticker>& p : tickers) {
      avg_bid += p.second.m_bid;
      avg_ask += p.second.m_ask;
    }
    avg_bid /= tickers.size();
    avg_ask /= tickers.size();
    const Ticker& ex_ticker = tickers.at(m_opts.m_trading_exchange);
    if (ex_ticker.m_bid - m_opts.m_profit_threshold > avg_bid) {
      // Sell
      m_exchange_account->MarketOrder("BTCUSD", Side::ASK, 0.0001);
    } else if (ex_ticker.m_ask + m_opts.m_profit_threshold < avg_ask) {
      // Buy
      m_exchange_account->MarketOrder("BTCUSD", Side::BID, 0.0001);
    }
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