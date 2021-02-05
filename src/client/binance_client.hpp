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
  BinanceClient(const std::string& base_url, boost::asio::io_context& ioctx)
      : m_base_url(base_url), m_last_order_id(0), m_api(
        ioctx
        ,"api.binance.com"
        ,"443"
        ,g_api_key // can be empty for non USER_DATA reqs
        ,g_secret // can be empty for non USER_DATA reqs
        ,10000 // recvWindow
    ) {

  }

  virtual ~BinanceClient() {

  }

  virtual void MarketOrder(const std::string& symbol, Side side, double qty) override {
    m_api.new_order(symbol, binapi::e_side::buy, binapi::e_type::market, binapi::e_time::IOC,
        // TODO: hardcoded quantity for now
        "0.001", std::string(), std::to_string(++m_last_order_id), std::string(), std::string(),
        [] (const char *fl, int ec, std::string emsg, auto res) {
          if ( ec ) {
              BOOST_LOG_TRIVIAL(error) << "New order error: fl=" << fl << ", ec=" << ec
                  << ", emsg=" << emsg << std::endl;
              return false;
          }
          BOOST_LOG_TRIVIAL(info) << "New order: " << res << std::endl;
          //const auto& res_obj = res.get_response_full();
          return true;
        }
    );
  }

  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) override {

  }

  virtual void CancelAllOrders() override {

  }

private:
  std::string m_base_url;
  uint64_t m_last_order_id;
  binapi::rest::api m_api;
};