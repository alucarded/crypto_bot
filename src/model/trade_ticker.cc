#include "trade_ticker.h"

std::ostream &operator<<(std::ostream &os, const TradeTicker &res) {
  os << "{ \"event_time\": " << res.event_time;
  os << ", \"trade_time\": " << res.trade_time;
  os << ", \"symbol\": " << res.symbol;
  os << ", \"trade_id\": " << res.trade_id;
  os << ", \"price\": " << res.price;
  os << ", \"qty\": " << res.qty;
  os << ", \"is_market_maker\": " << res.is_market_maker;
  os << " }";
  return os;
}