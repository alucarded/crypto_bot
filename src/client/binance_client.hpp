#include "exchange_client.h"
#include "binapi/api.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>

namespace {

// TODO: Get from param store
const std::string g_api_key = "ruYwDHUo7PB4WW3MEha9j5gy8F9f7Pq0NtOdLZHyCkhQ5BZv39QC0PjHsdBx8RJz";
const std::string g_secret = "x11P9YF6n3ZMgrU1C1kFjh9TxEiGKYq9EwOm6ebFcodCNTa4EuBv9VLO1yOcyFca";

}

class BinanceClient : public ExchangeClient {
public:
  BinanceClient()
      : m_last_order_id(0), m_ioctx(), m_api(
        m_ioctx
        ,"api.binance.com"
        ,"443"
        ,g_api_key // can be empty for non USER_DATA reqs
        ,g_secret // can be empty for non USER_DATA reqs
        // From docs: "An additional parameter, recvWindow, may be sent
        // to specify the number of milliseconds after timestamp the request is valid for."
        ,1000 // recvWindow
        ,"cryptobot-1.0.0"
    ) {

  }

  virtual ~BinanceClient() {

  }

  virtual NewOrderResult MarketOrder(const std::string& symbol, Side side, double qty) override {
    binapi::rest::api::result<binapi::rest::new_order_resp_type> res = m_api.new_order(symbol, (Side::BID == side ? binapi::e_side::buy : binapi::e_side::sell),
        binapi::e_type::market, binapi::e_time::GTC,
        std::to_string(qty), std::string(), std::to_string(++m_last_order_id), std::string(), std::string());
    if ( !res ) {
        BOOST_LOG_TRIVIAL(error) << "Binance MarketOrder error: " << res.errmsg << std::endl;
    }
    // TODO: return res.v
    return res.reply;
  }

  virtual NewOrderResult LimitOrder(const std::string& symbol, Side side, double qty, double price) override {
    binapi::rest::api::result<binapi::rest::new_order_resp_type> res = m_api.new_order(symbol, (Side::BID == side ? binapi::e_side::buy : binapi::e_side::sell),
        binapi::e_type::limit, binapi::e_time::GTC,
        std::to_string(qty), std::to_string(price), std::to_string(++m_last_order_id), std::string(), std::string());
    if ( !res ) {
        BOOST_LOG_TRIVIAL(error) << "Binance LimitOrder error: " << res.errmsg << std::endl;
    }
    // TODO: return res.v
    return res.reply;
  }

  virtual void CancelAllOrders() override {

  }

private:
  uint64_t m_last_order_id;
  boost::asio::io_context m_ioctx;
  binapi::rest::api m_api;
};