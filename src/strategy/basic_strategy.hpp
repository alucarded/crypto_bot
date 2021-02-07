#include "ticker_transformer.hpp"
#include "trading_strategy.h"

#include <string>
#include <map>
#include <memory>

struct BasicStrategyOptions : StrategyOptions {
  double m_buy_profit_margin = 100.0;
  double m_sell_profit_margin = 120.0;
  double m_min_qty = 0.001;
};

std::map<std::string, double> ESTIMATED_TRADING_VOLUME =
{
  std::pair<std::string, double> ("binance", 12704429036),
  std::pair<std::string, double> ("coinbase", 2694078691),
  std::pair<std::string, double> ("kraken", 1316166815),
  std::pair<std::string, double> ("ftx", 284008679),
  std::pair<std::string, double> ("bitstamp", 904068482),
  std::pair<std::string, double> ("bitbay", 85674398),
  std::pair<std::string, double> ("huobiglobal", 4986875662),
  std::pair<std::string, double> ("poloniex", 152434675),
};

class BasicStrategy : public TradingStrategy, public Consumer<RawTicker> {
public:
  // TODO: move more generic stuff to TradingStrategy class
  BasicStrategy(const BasicStrategyOptions& opts, ExchangeClient* exchange_client)
      : m_opts(opts), m_exchange_client(exchange_client) {
    // m_volume_sum = 0;
    // std::for_each(ESTIMATED_TRADING_VOLUME.begin(), ESTIMATED_TRADING_VOLUME.end(),
    //     [&](const auto& p) { m_volume_sum += p.second; });
    m_transformers["coinbase"] = std::make_unique<CoinbaseTickerTransformer>();
  }

  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) override {
    if (!tickers.count(m_opts.m_trading_exchange)) {
      return;
    }
    //m_exchange_client->OnTicker(tickers[m_opts.m_trading_exchange]);
    if (tickers.size() < m_opts.m_required_exchanges) {
      std::cout << "Not enough exchanges. Got " << std::to_string(tickers.size()) << ", required " << std::to_string(m_opts.m_required_exchanges) << std::endl;
      return;
    }
    //std::cout << "Executing strategy" << std::endl;
    double avg_bid = 0;
    //double bid_vol_sum = 0;
    double avg_ask = 0;
    //double ask_vol_sum = 0;
    double volume_sum = 0;
    for (const std::pair<std::string, Ticker>& p : tickers) {
      avg_bid += p.second.m_bid*ESTIMATED_TRADING_VOLUME[p.first];
      avg_ask += p.second.m_ask*ESTIMATED_TRADING_VOLUME[p.first];
      volume_sum += ESTIMATED_TRADING_VOLUME[p.first];
    }
    avg_bid /= volume_sum;
    avg_ask /= volume_sum;
    const Ticker& ex_ticker = tickers.at(m_opts.m_trading_exchange);
    double bid_margin = ex_ticker.m_bid - avg_bid;
    double ask_margin = avg_ask - ex_ticker.m_ask;
    m_max_margin = std::max(m_max_margin, bid_margin);
    m_max_margin = std::max(m_max_margin, ask_margin);
    std::cout << "bid_margin = " << std::to_string(bid_margin) << ", ask_margin = " << std::to_string(ask_margin) << std::endl;
    if (bid_margin > m_opts.m_sell_profit_margin
        && ex_ticker.m_bid_vol >= m_opts.m_min_qty) {
      // Sell
      PrintTickers();
      std::cout << "Bid margin: " << bid_margin << std::endl;
      m_exchange_client->MarketOrder("BTCUSD", Side::ASK, 0.001);
    } else if (ask_margin > m_opts.m_buy_profit_margin
        && ex_ticker.m_ask_vol >= m_opts.m_min_qty) {
      // Buy
      PrintTickers();
      std::cout << "Ask margin: " << ask_margin << std::endl;
      m_exchange_client->MarketOrder("BTCUSD", Side::BID, 0.001);
    } else {
      std::cout << "No good trade to make" << std::endl;
    }
  }

  virtual void Consume(const RawTicker& raw_ticker) override {
    // std::cout << "Consumed:" << std::endl;
    // std::cout << raw_ticker << std::endl;
    // Any empty ticker from an exchange discards validity of its data
    if (raw_ticker.m_bid.empty() || raw_ticker.m_ask.empty()) {
      m_tickers.erase(raw_ticker.m_exchange);
      std::cout << "Empty ticker: " << raw_ticker << std::endl;
      if (m_opts.m_trading_exchange == raw_ticker.m_exchange) {
        m_exchange_client->OnDisconnected();
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
      ticker.m_symbol = raw_ticker.m_symbol;
      if (m_tickers.count(raw_ticker.m_exchange)) {
        assert(ticker.m_arrived_ts > m_tickers[raw_ticker.m_exchange].m_arrived_ts);
      }
      if (!ProcessTicker(raw_ticker.m_exchange, ticker)) {
        return;
      }
      m_tickers[raw_ticker.m_exchange] = ticker;
      if (m_opts.m_trading_exchange == raw_ticker.m_exchange) {
        m_exchange_client->OnTicker(ticker);
      }
    }
    execute(m_tickers);
  }

  void PrintStats() {
    std::cout << "Max margin: " << m_max_margin << std::endl;
  }

  virtual void OnTicker(const Ticker& ticker) override {

  }

  virtual void OnDisconnected(const std::string& exchange_name) override {

  }

private:
  bool ProcessTicker(const std::string& exchange, Ticker& ticker) {
    bool ticker_accepted = true;
    if (m_transformers.count(exchange)) {
      ticker_accepted = m_transformers[exchange]->Transform(ticker);
    }
    return ticker_accepted;
  }
  void PrintTickers() {
    for (const std::pair<std::string, Ticker>& p : m_tickers) {
      std::cout << p.first << " bid: " << p.second.m_bid << std::endl;
      std::cout << p.first << " ask: " << p.second.m_ask << std::endl;
    }
  }

  BasicStrategyOptions m_opts;
  ExchangeClient* m_exchange_client;
  std::map<std::string, Ticker> m_tickers;
  std::map<std::string, std::unique_ptr<TickerTransformer>> m_transformers;

  // double m_volume_sum;
  double m_max_margin;
};