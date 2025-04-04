#include "trade_ticker.h"

std::ostream &operator<<(std::ostream &os, const TradeTicker &res) {
  os << "{ \"exchange\": " << res.exchange;
  os << ", \"event_time\": " << res.event_time;
  os << ", \"trade_time\": " << res.trade_time;
  os << ", \"symbol\": " << res.symbol;
  os << ", \"exchange\": " << res.exchange;
  os << ", \"trade_id\": " << res.trade_id;
  os << ", \"price\": " << res.price;
  os << ", \"qty\": " << res.qty;
  os << ", \"is_market_maker\": " << res.is_market_maker;
  os << ", \"arrived_ts\": " << res.arrived_ts;
  os << " }";
  return os;
}