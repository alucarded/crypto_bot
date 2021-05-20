#include "arbitrage_order_calculator.h"

ArbitrageOrderCalculator::ArbitrageOrderCalculator(const ArbitrageStrategyOptions& options) : m_opts(options) {

}

ArbitrageOrders ArbitrageOrderCalculator::Calculate(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker, double base_balance, double quote_balance) const {
  double qty = CalculateQuantity(best_bid_ticker, best_ask_ticker, base_balance, quote_balance);

  ArbitrageOrders res;
  res.sell_order = Order::CreateBuilder()
      .Symbol(best_bid_ticker.m_symbol)
      .Quantity(qty)
      .Price(best_bid_ticker.m_bid)
      .Side_(Side::SELL)
      .OrderType_(OrderType::LIMIT)
      .Build();
  res.buy_order = Order::CreateBuilder()
      .Symbol(best_ask_ticker.m_symbol)
      .Quantity(qty)
      .Price(best_ask_ticker.m_ask)
      .Side_(Side::BUY)
      .OrderType_(OrderType::LIMIT)
      .Build();
  return res;
}

double ArbitrageOrderCalculator::CalculateQuantity(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker, double base_balance, double quote_balance) const {
  SymbolPair sp = best_bid_ticker.m_symbol;
  // Find maximum volume for arbitrage
  // Set constraints on traded amount based on book volumes
  assert(best_bid_ticker.m_bid_vol.has_value());
  assert(best_ask_ticker.m_ask_vol.has_value());
  double book_bid_vol = best_bid_ticker.m_bid_vol.value();
  double book_ask_vol = best_ask_ticker.m_ask_vol.value();
  // 1. Allow maximum 20% of smaller amount from the books, but not less than default amount (if possible)
  // This is to decrease risk of not being filled immedaitely (or almost immediately)
  double base_vol = std::min(book_bid_vol, book_ask_vol);
  BOOST_LOG_TRIVIAL(debug) << "Available for arbitrage: " << base_vol << " " << sp;
  double default_vol = m_opts.m_default_amount.at(sp.GetBaseAsset());
  if (base_vol > default_vol) {
    base_vol = std::max(0.2 * base_vol, default_vol);
  }

  BOOST_LOG_TRIVIAL(debug) << best_bid_ticker.m_exchange << " - base asset account balance: " << base_balance;
  BOOST_LOG_TRIVIAL(debug) << best_ask_ticker.m_exchange << " - quote asset account balance: " << quote_balance;
  // 2. Allow maximum 90% of actual balance to be traded
  // This is to decrease risk of order failure due to insufficient funds (workaround inaccurate balance or not taking fee into account)
  double tradable_base_balance = 0.90 * base_balance;
  double tradable_quote_balance = 0.90 * quote_balance;
  // Check with available balances
  double quote_vol = std::min(base_vol * best_ask_ticker.m_ask, tradable_quote_balance);
  // Maximum volume available for arbitrage
  base_vol = std::min({base_vol, quote_vol/best_ask_ticker.m_ask, tradable_base_balance});
  BOOST_LOG_TRIVIAL(debug) << "tradable_base_balance=" << tradable_base_balance << ", tradable_quote_balance=" << tradable_quote_balance
      << ", quote_vol=" << quote_vol << ", base_vol=" << base_vol;
  
  return base_vol;
}

ArbitrageOrderCalculator::ArbitragePrices ArbitrageOrderCalculator::CalculatePrices(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) const {
  ArbitragePrices res;
  // TODO: calculate profit margin and apply partly to best ask and partly to best bid prices
  // TODO: which price gets bigger buffer?
  res.buy_price = best_ask_ticker.m_ask;
  res.sell_price = best_bid_ticker.m_bid;
  return res;
}