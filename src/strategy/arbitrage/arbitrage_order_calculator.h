#include "arbitrage_strategy_options.h"
#include "model/order.h"
#include "model/ticker.h"

struct ArbitrageOrders {
  Order sell_order; // best bid side
  Order buy_order; // best ask side
};

// Price should be calculate so that order can be executed at a worse prices then the current ones,
// but still with worse case 0+ profit.
class ArbitrageOrderCalculator {
public:
  ArbitrageOrderCalculator(const ArbitrageStrategyOptions& options);

  ArbitrageOrders Calculate(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker, double base_balance, double quote_balance) const;

private:
  struct ArbitragePrices {
    double sell_price;
    double buy_price;
  };

  double CalculateQuantity(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker, double base_balance, double quote_balance) const;
  ArbitragePrices CalculatePrices(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) const;

private:
  ArbitrageStrategyOptions m_opts;
};