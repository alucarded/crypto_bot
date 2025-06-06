#include "arbitrage_order_calculator.h"

ArbitrageOrderCalculator::ArbitrageOrderCalculator(const ArbitrageStrategyOptions& options) : m_opts(options) {

}

ArbitrageOrders ArbitrageOrderCalculator::Calculate(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker, double base_balance, double quote_balance) const {
  double qty = CalculateQuantity(best_bid_ticker, best_ask_ticker, base_balance, quote_balance);
  ArbitragePrices prices = CalculatePrices(best_bid_ticker, best_ask_ticker);

  ArbitrageOrders res;
  res.sell_order = Order::CreateBuilder()
      .Symbol(best_bid_ticker.symbol)
      .Quantity(qty)
      .Price(prices.sell_price)
      .Side_(Side::SELL)
      .OrderType_(OrderType::LIMIT)
      .Build();
  res.buy_order = Order::CreateBuilder()
      .Symbol(best_ask_ticker.symbol)
      .Quantity(qty)
      .Price(prices.buy_price)
      .Side_(Side::BUY)
      .OrderType_(OrderType::LIMIT)
      .Build();
  return res;
}

double ArbitrageOrderCalculator::CalculateQuantity(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker, double base_balance, double quote_balance) const {
  SymbolPair sp = best_bid_ticker.symbol;
  // Find maximum volume for arbitrage
  // Set constraints on traded amount based on book volumes
  assert(best_bid_ticker.bid_vol.has_value());
  assert(best_ask_ticker.ask_vol.has_value());
  double book_bid_vol = best_bid_ticker.bid_vol.value();
  double book_ask_vol = best_ask_ticker.ask_vol.value();
  double base_vol = std::min(book_bid_vol, book_ask_vol);
  BOOST_LOG_TRIVIAL(debug) << "Available for arbitrage: " << base_vol << " " << sp;
  // Allow maximum 30% of smaller amount from the books
  // This is to decrease risk of not being filled immedaitely (or almost immediately)
  base_vol = 0.3 * base_vol;

  BOOST_LOG_TRIVIAL(debug) << best_bid_ticker.exchange << " - base asset account balance: " << base_balance;
  BOOST_LOG_TRIVIAL(debug) << best_ask_ticker.exchange << " - quote asset account balance: " << quote_balance;
  // 2. Allow maximum 90% of actual balance to be traded
  // This is to decrease risk of order failure due to insufficient funds (workaround inaccurate balance or not taking fee into account)
  double tradable_base_balance = 0.90 * base_balance;
  double tradable_quote_balance = 0.90 * quote_balance;
  // Check with available balances
  double quote_vol = std::min(base_vol * best_ask_ticker.ask, tradable_quote_balance);
  // Maximum volume available for arbitrage
  base_vol = std::min({base_vol, quote_vol/best_ask_ticker.ask, tradable_base_balance});
  BOOST_LOG_TRIVIAL(info) << "tradable_base_balance=" << tradable_base_balance << ", tradable_quote_balance=" << tradable_quote_balance
      << ", quote_vol=" << quote_vol << ", base_vol=" << base_vol;
  
  return base_vol;
}

ArbitrageOrderCalculator::ArbitragePrices ArbitrageOrderCalculator::CalculatePrices(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) const {
  ArbitragePrices res;
  const auto& best_bid_exchange_params = m_opts.exchange_params.at(best_bid_ticker.exchange);
  const auto& best_ask_exchange_params = m_opts.exchange_params.at(best_ask_ticker.exchange);
  double best_bid_exchange_fee = best_bid_exchange_params.fee;
  double best_ask_exchange_fee = best_ask_exchange_params.fee;
  // Part of quote currency sum received when selling
  double sell_cost_coeff = 1.0 - best_bid_exchange_fee;
  // Part of quote currency sum paid when buying
  double buy_cost_coeff = 1.0 + best_ask_exchange_fee;
  double max_buy_price = (sell_cost_coeff / buy_cost_coeff) * best_bid_ticker.bid;
  double min_sell_price = (buy_cost_coeff / sell_cost_coeff) * best_ask_ticker.ask;
  // Here we use existing profit margin of arbitrage opportunity to reduce risk of not filling order immediately
  // We give more space for filling order on the side of less liquid exchange
  // The amount of margin added to current book price is inversely proportional to daily volume of an exchange
  // This is due to the fact that arbitrage opportunities arise because of large orders pushing the market on more liquid exchange
  // Worst case is 0 profit
  // TODO: next step - use trade ticker to better calculate price
  // Also, take into consideration volumes for best ask and bid prices in the book
  double book_bid_vol = best_bid_ticker.bid_vol.value();
  double book_ask_vol = best_ask_ticker.ask_vol.value();
  double vol_sum = book_bid_vol + book_ask_vol;
  double best_bid_exchange_daily_vol = best_bid_exchange_params.daily_volume;
  double best_ask_exchange_daily_vol = best_ask_exchange_params.daily_volume;
  double daily_vol_sum = best_ask_exchange_daily_vol + best_bid_exchange_daily_vol;
  double ask_coeff = (best_ask_exchange_daily_vol / daily_vol_sum + book_ask_vol / vol_sum) / 2;
  double bid_coeff = (best_bid_exchange_daily_vol / daily_vol_sum + book_bid_vol / vol_sum) / 2;
  // TODO: take latencies into consideration
  BOOST_LOG_TRIVIAL(debug) << "Ask coeff: " << ask_coeff << ", bid coeff: " << bid_coeff;
  res.buy_price = best_ask_ticker.ask * ask_coeff + max_buy_price * bid_coeff;
  res.sell_price = best_bid_ticker.bid * bid_coeff + min_sell_price * ask_coeff;
  return res;
}