#include "exchange/exchange_client.h"
#include "http_client.hpp"
#include "binapi/api.hpp"

#include "json/json.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>

#include <openssl/hmac.h>
#include <openssl/sha.h>

using json = nlohmann::json;

using namespace std::placeholders; // _1, _2, etc.

namespace {

// TODO: Get from param store
const std::string g_api_key = "ruYwDHUo7PB4WW3MEha9j5gy8F9f7Pq0NtOdLZHyCkhQ5BZv39QC0PjHsdBx8RJz";
const std::string g_secret = "x11P9YF6n3ZMgrU1C1kFjh9TxEiGKYq9EwOm6ebFcodCNTa4EuBv9VLO1yOcyFca";

std::uint64_t get_current_ms_epoch() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
}

std::string b2a_hex(const std::uint8_t *p, std::size_t n) {
    static const char hex[] = "0123456789abcdef";
    std::string res;
    res.reserve(n * 2);

    for ( auto end = p + n; p != end; ++p ) {
        const std::uint8_t v = (*p);
        res += hex[(v >> 4) & 0x0F];
        res += hex[v & 0x0F];
    }

    return res;
}

std::string hmac_sha256(const char *key, std::size_t klen, const char *data, std::size_t dlen) {
    std::uint8_t digest[EVP_MAX_MD_SIZE];
    std::uint32_t dilen{};

    auto p = ::HMAC(
         ::EVP_sha256()
        ,key
        ,klen
        ,(std::uint8_t *)data
        ,dlen
        ,digest
        ,&dilen
    );
    assert(p);

    return b2a_hex(digest, dilen);
}

}

class BinanceClient : public ExchangeClient {
public:
  inline static const std::string HOST = "api.binance.com";
  inline static const std::string PORT = "443";
  inline static const std::string ADD_ORDER_PATH = "/api/v3/order";
  inline static const std::string GET_ACCOUNT_BALANCE_PATH = "/api/v3/account";
  inline static const std::string GET_OPEN_ORDERS_PATH = "/api/v3/openOrders";

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
        ),
        m_http_client(HttpClient::Options("cryptobot-1.0.0")),
        m_timeout(1000) {
  }

  virtual ~BinanceClient() {

  }

  virtual std::string GetExchange() override {
    return "binance";
  }

  virtual Result<Order> MarketOrder(const std::string& symbol, Side side, double qty) override {
    binapi::rest::api::result<binapi::rest::new_order_resp_type> res = m_api.new_order(symbol, (Side::BID == side ? binapi::e_side::buy : binapi::e_side::sell),
        binapi::e_type::market, binapi::e_time::GTC,
        std::to_string(qty), std::string(), std::to_string(++m_last_order_id), std::string(), std::string());
    if ( !res ) {
        BOOST_LOG_TRIVIAL(error) << "Binance MarketOrder error: " << res.errmsg << std::endl;
        return Result<Order>(res.reply, res.errmsg);
    }
    return Result<Order>(res.reply, Order(std::to_string(m_last_order_id)));
  }

  virtual Result<Order> LimitOrder(const std::string& symbol, Side side, double qty, double price) override {
    binapi::rest::api::result<binapi::rest::new_order_resp_type> res = m_api.new_order(symbol, (Side::BID == side ? binapi::e_side::buy : binapi::e_side::sell),
        binapi::e_type::limit, binapi::e_time::GTC,
        std::to_string(qty), std::to_string(price), std::to_string(++m_last_order_id), std::string(), std::string());
    if ( !res ) {
        BOOST_LOG_TRIVIAL(error) << "Binance LimitOrder error: " << res.errmsg << std::endl;
        return Result<Order>(res.reply, res.errmsg);
    }
    return Result<Order>(res.reply, Order(std::to_string(m_last_order_id)));
  }

  virtual void CancelAllOrders() override {

  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    // binapi::rest::api::result<binapi::rest::account_info_t> res = m_api.account_info();
    // //auto account_info = res.response;
    // json response_json = json::parse(res.reply);
    // json balances_json = response_json["balances"];
    // std::unordered_map<std::string, double> balances;
    // for (const auto& b : balances_json) {
    //   balances.insert(std::make_pair(b["asset"], b["free"].get<double>()));
    // }
    // return AccountBalance(std::move(balances));
    HttpClient::Result res = m_http_client.get(HOST, PORT, GET_ACCOUNT_BALANCE_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
        .send();
    json response_json = json::parse(res.response);
    if (response_json.contains("code")) {
      return Result<AccountBalance>(res.response, response_json["msg"].get<std::string>());
    }
    std::unordered_map<std::string, std::string> balances;
    for (const auto& b : response_json["balances"]) {
      balances.insert(std::make_pair(b["asset"], b["free"]));
    }
    
    return Result<AccountBalance>(res.response, AccountBalance(balances));
  }

  virtual Result<std::vector<Order>> GetOpenOrders(const std::string& symbol) override {
    HttpClient::Result res = m_http_client.get(HOST, PORT, GET_OPEN_ORDERS_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .QueryParam("symbol", symbol)
        .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
        .send();
    json response_json = json::parse(res.response);
    if (response_json.contains("code")) {
      return Result<std::vector<Order>>(res.response, response_json["msg"].get<std::string>());
    }
    std::vector<Order> orders;
    for (const auto& order : response_json) {
      orders.push_back(Order(order["clientOrderId"].get<std::string>()));
    }
    return Result<std::vector<Order>>(res.response, orders);
  }
private:

  void SignData(HttpClient::Request& request, std::string& data) {
    (void) request; // unused
    assert(!g_api_key.empty() && !g_secret.empty());

    if ( !data.empty() ) {
        data += "&";
    }
    data += "timestamp=";
    std::uint64_t timestamp = get_current_ms_epoch();
    data += std::to_string(timestamp);

    data += "&recvWindow=";
    data += std::to_string(m_timeout);

    std::string signature = hmac_sha256(
        g_secret.c_str()
        ,g_secret.length()
        ,data.c_str()
        ,data.length()
    );

    data += "&signature=";
    data += signature;
  }

private:
  uint64_t m_last_order_id;
  boost::asio::io_context m_ioctx;
  binapi::rest::api m_api;
  HttpClient m_http_client;
  int m_timeout;
};