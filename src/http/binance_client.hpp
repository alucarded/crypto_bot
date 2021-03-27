#pragma once
#include "exchange/exchange_client.h"
#include "http_client.hpp"
#include "binapi/api.hpp"
#include "model/binance.h"

#include "json/json.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <cstdlib>
#include <optional>

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

// TODO: This should be rather named BinanceRestClient or BinanceHttpClient
class BinanceClient : public ExchangeClient {
public:
  inline static const std::string HOST = "api.binance.com";
  inline static const std::string PORT = "443";
  inline static const std::string ADD_ORDER_PATH = "/api/v3/order";
  inline static const std::string GET_ACCOUNT_BALANCE_PATH = "/api/v3/account";
  inline static const std::string GET_OPEN_ORDERS_PATH = "/api/v3/openOrders";
  inline static const std::string LISTEN_KEY_PATH = "/api/v3/userDataStream";

  static std::unordered_map<SymbolPairId, std::string> SYMBOL_MAP;

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

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    const std::string& symbol_str = GetSymbolString(symbol);
    HttpClient::Result res = m_http_client.get(HOST, PORT, ADD_ORDER_PATH)
      .Header("X-MBX-APIKEY", g_api_key)
      .QueryParam("symbol", symbol_str)
      .QueryParam("side", (side == Side::BUY ? "BUY" : "SELL"))
      .QueryParam("type", "MARKET")
      // TODO: make sure precision is always fine
      .QueryParam("quantity", std::to_string(qty))
      .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
      .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error making market order request for " << GetExchange() << ": " << res.errmsg;
      return Result<Order>(res.response, res.errmsg);
    }
    json res_json = json::parse(res.response);
    const auto& order_id = res_json["orderId"].get<int64_t>();
    const auto& client_order_id = res_json["clientOrderId"].get<std::string>();
    return Result<Order>(res.response, Order(std::to_string(order_id), client_order_id, symbol, side, OrderType::MARKET, qty));
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    const std::string& symbol_str = GetSymbolString(symbol);
    HttpClient::Result res = m_http_client.get(HOST, PORT, ADD_ORDER_PATH)
      .Header("X-MBX-APIKEY", g_api_key)
      .QueryParam("symbol", symbol_str)
      .QueryParam("side", (side == Side::BUY ? "BUY" : "SELL"))
      .QueryParam("type", "LIMIT")
      // TODO: make sure precision is always fine
      .QueryParam("quantity", std::to_string(qty))
      .QueryParam("price", std::to_string(price))
      // TODO: for now always GTC
      .QueryParam("timeInForce", "GTC")
      .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
      .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error making limit order request for " << GetExchange() << ": " << res.errmsg;
      return Result<Order>(res.response, res.errmsg);
    }
    json res_json = json::parse(res.response);
    const auto& order_id = res_json["orderId"].get<int64_t>();
    const auto& client_order_id = res_json["clientOrderId"].get<std::string>();
    return Result<Order>(res.response, Order(std::to_string(order_id), client_order_id, symbol, side, OrderType::LIMIT, qty));
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
    BOOST_LOG_TRIVIAL(debug) << "[BinanceClient::GetAccountBalance] " << res.response << std::endl;
    if (response_json.contains("code")) {
      return Result<AccountBalance>(res.response, response_json["msg"].get<std::string>());
    }
    std::unordered_map<SymbolId, std::string> balances;
    for (const auto& b : response_json["balances"]) {
      auto asset_str = b["asset"];
      if (BINANCE_ASSET_MAP.count(asset_str) == 0) {
        BOOST_LOG_TRIVIAL(warning) << "[BinanceClient::GetAccountBalance] Skipping unsupported asset: " << asset_str;
        continue;
      }
      balances.insert(std::make_pair(BINANCE_ASSET_MAP.at(asset_str), b["free"]));
    }
    
    return Result<AccountBalance>(res.response, AccountBalance(balances));
  }

  virtual Result<std::vector<Order>> GetOpenOrders(SymbolPairId symbol) override {
    const std::string& symbol_str = GetSymbolString(symbol);
    HttpClient::Result res = m_http_client.get(HOST, PORT, GET_OPEN_ORDERS_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .QueryParam("symbol", symbol_str)
        .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
        .send();
    json response_json = json::parse(res.response);
    BOOST_LOG_TRIVIAL(debug) << "[BinanceClient::GetOpenOrders] " << response_json;
    if (response_json.contains("code")) {
      return Result<std::vector<Order>>(res.response, response_json["msg"].get<std::string>());
    }
    std::vector<Order> orders;
    for (const auto& order : response_json) {
      const auto& id_str = std::to_string(order["orderId"].get<std::int64_t>());
      const auto& client_id_str = order["clientOrderId"].get<std::string>();
      const auto& symbol_str = order["symbol"].get<std::string>();
      const auto& side_str = order["side"].get<std::string>();
      const auto& order_type_str = order["type"].get<std::string>();
      const auto& orig_qty_str = order["origQty"].get<std::string>();
      orders.push_back(Order(id_str, client_id_str,
        SymbolPair(order["symbol"].get<std::string>()),
        (side_str == "BUY" ? Side::BUY : Side::SELL),
        // TODO: support more order types
        (order_type_str == "MARKET" ? OrderType::MARKET : OrderType::LIMIT),
        std::stod(orig_qty_str)));
    }
    return Result<std::vector<Order>>(res.response, orders);
  }

  // BinanceClient specific API
  std::optional<std::string> StartUserDataStream() {
    HttpClient::Result res = m_http_client.post(HOST, PORT, LISTEN_KEY_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .send();

    json response_json = json::parse(res.response);
    if (!response_json.contains("listenKey")) {
      return std::nullopt;
    }
    auto ret = response_json["listenKey"].get<std::string>();
    return std::optional<std::string>(ret);
  }

  bool KeepaliveUserDataStream(const std::string& listen_key) {
    HttpClient::Result res = m_http_client.put(HOST, PORT, LISTEN_KEY_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .QueryParam("listenKey", listen_key)
        .send();
    return bool(res);
  }

  bool CloseUserDataStream(const std::string& listen_key) {
    HttpClient::Result res = m_http_client.delete_(HOST, PORT, LISTEN_KEY_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .QueryParam("listenKey", listen_key)
        .send();
    return bool(res);
  }

private:

  const std::string& GetSymbolString(SymbolPairId symbol) const {
    if (SYMBOL_MAP.count(symbol) == 0) {
      BOOST_LOG_TRIVIAL(error) << "Unknown symbol. Exiting.";
      std::exit(1);
    }
    return SYMBOL_MAP.at(symbol);
  }

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

std::unordered_map<SymbolPairId, std::string> BinanceClient::SYMBOL_MAP = {
  {SymbolPairId::ADA_USDT, "ADAUSDT"},
  {SymbolPairId::BTC_USDT, "BTCUSDT"},
  {SymbolPairId::ETH_USDT, "ETHUSDT"},
  {SymbolPairId::EOS_USDT, "EOSUSDT"},
  {SymbolPairId::ADA_BTC, "ADABTC"},
  {SymbolPairId::ETH_BTC, "ETHBTC"},
  {SymbolPairId::EOS_BTC, "EOSBTC"},
  {SymbolPairId::EOS_ETH, "EOSETH"}
};