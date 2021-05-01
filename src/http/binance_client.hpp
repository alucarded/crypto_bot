#pragma once
#include "exchange/binance_settings.hpp"
#include "exchange/exchange_client.h"
#include "http_client.hpp"
#include "model/symbol.h"
#include "utils/string.hpp"

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
  inline static const std::string EXCHANGE_INFO_PATH = "/api/v3/exchangeInfo";
  inline static const std::string GET_ACCOUNT_BALANCE_PATH = "/api/v3/account";
  inline static const std::string GET_OPEN_ORDERS_PATH = "/api/v3/openOrders";
  inline static const std::string LISTEN_KEY_PATH = "/api/v3/userDataStream";

  BinanceClient()
      : m_last_order_id(0),
        m_http_client(HttpClient::Options("cryptobot-1.0.0", 1)),
        m_timeout(1000),
        m_binance_settings(GetExchangeInfo()) {
  }

  virtual ~BinanceClient() {

  }

  virtual std::string GetExchange() override {
    return "binance";
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    const std::string& symbol_str = GetSymbolString(symbol);
    auto pair_settings = m_binance_settings.GetPairSettings(symbol);
    HttpClient::Result res = m_http_client.post(HOST, PORT, ADD_ORDER_PATH)
      .Header("X-MBX-APIKEY", g_api_key)
      .QueryParam("symbol", symbol_str)
      .QueryParam("side", (side == Side::BUY ? "BUY" : "SELL"))
      .QueryParam("type", "MARKET")
      .QueryParam("quantity", cryptobot::to_string(qty, pair_settings.order_precision))
      .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
      .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error making market order request for " << GetExchange() << ": " << res.errmsg;
      return Result<Order>(res.response, res.errmsg);
    }
    json res_json = json::parse(res.response);
    if (!res_json.contains("orderId")) {
      const auto& errmsg = res_json["msg"].get<std::string>();
      BOOST_LOG_TRIVIAL(error) << "Error response making market order request for "
          << GetExchange() << ": " << errmsg;
      return Result<Order>(res.response, errmsg);
    }
    const auto& order_id = res_json["orderId"].get<int64_t>();
    const auto& client_order_id = res_json["clientOrderId"].get<std::string>();
    double cummulative_quote_qty = std::stod(res_json["cummulativeQuoteQty"].get<std::string>());
    // Price is required to calculate locked balance
    // TODO: maybe pass quote quantity directly to avoid dividing and then multiplying again
    double price = cummulative_quote_qty / qty;
    const auto& status = res_json["status"].get<std::string>();
    return Result<Order>(res.response, Order(std::to_string(order_id), client_order_id, symbol, side, OrderType::MARKET, qty, price, Order::GetStatus(status)));
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    const std::string& symbol_str = GetSymbolString(symbol);
    auto pair_settings = m_binance_settings.GetPairSettings(symbol);
    HttpClient::Result res = m_http_client.post(HOST, PORT, ADD_ORDER_PATH)
      .Header("X-MBX-APIKEY", g_api_key)
      .QueryParam("symbol", symbol_str)
      .QueryParam("side", (side == Side::BUY ? "BUY" : "SELL"))
      .QueryParam("type", "LIMIT")
      .QueryParam("quantity", cryptobot::to_string(qty, pair_settings.order_precision))
      .QueryParam("price", cryptobot::to_string(price, pair_settings.quote_precision))
      // TODO: for now always GTC
      .QueryParam("timeInForce", "GTC")
      .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
      .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error making limit order request for " << GetExchange() << ": " << res.errmsg;
      return Result<Order>(res.response, res.errmsg);
    }
    json res_json = json::parse(res.response);
    if (!res_json.contains("orderId")) {
      const auto& errmsg = res_json["msg"].get<std::string>();
      BOOST_LOG_TRIVIAL(error) << "Error response making limit order request for "
          << GetExchange() << ": " << errmsg;
      return Result<Order>(res.response, errmsg);
    }
    const auto& order_id = res_json["orderId"].get<int64_t>();
    const auto& client_order_id = res_json["clientOrderId"].get<std::string>();
    const auto& status = res_json["status"].get<std::string>();
    return Result<Order>(res.response, Order(std::to_string(order_id), client_order_id, symbol, side, OrderType::LIMIT, qty, price, Order::GetStatus(status)));
  }

  virtual void CancelAllOrders() override {

  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    HttpClient::Result res = m_http_client.get(HOST, PORT, GET_ACCOUNT_BALANCE_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
        .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error getting account balance for " << GetExchange() << ": " << res.errmsg;
      return Result<AccountBalance>(res.response, res.errmsg);
    }
    json response_json = json::parse(res.response);
    BOOST_LOG_TRIVIAL(debug) << "[BinanceClient::GetAccountBalance] " << res.response << std::endl;
    if (response_json.contains("code")) {
      return Result<AccountBalance>(res.response, response_json["msg"].get<std::string>());
    }
    std::unordered_map<SymbolId, double> balances;
    for (const auto& b : response_json["balances"]) {
      auto asset_str = b["asset"];
      if (BINANCE_ASSET_MAP.count(asset_str) == 0) {
        BOOST_LOG_TRIVIAL(warning) << "[BinanceClient::GetAccountBalance] Skipping unsupported asset: " << asset_str;
        continue;
      }
      const auto& free_str  = b["free"].get<std::string>();
      const auto& locked_str = b["locked"].get<std::string>();
      balances.insert(std::make_pair(BINANCE_ASSET_MAP.at(asset_str), std::stod(free_str) + std::stod(locked_str)));
    }

    return Result<AccountBalance>(res.response, AccountBalance(std::move(balances)));
  }

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    HttpClient::Result res = m_http_client.get(HOST, PORT, GET_OPEN_ORDERS_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .WithQueryParamSigning(std::bind(&BinanceClient::SignData, this, _1, _2))
        .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error getting open orders for " << GetExchange() << ": " << res.errmsg;
      return Result<std::vector<Order>>(res.response, res.errmsg);
    }
    json response_json = json::parse(res.response);
    BOOST_LOG_TRIVIAL(debug) << "[BinanceClient::GetOpenOrders] " << response_json;
    if (response_json.contains("code")) {
      return Result<std::vector<Order>>(res.response, response_json["msg"].get<std::string>());
    }
    std::vector<Order> orders;
    for (const auto& order : response_json) {
      auto id_str = std::to_string(order["orderId"].get<std::int64_t>());
      auto client_id_str = order["clientOrderId"].get<std::string>();
      const auto& symbol_str = order["symbol"].get<std::string>();
      const auto& side_str = order["side"].get<std::string>();
      const auto& order_type_str = order["type"].get<std::string>();
      const auto& orig_qty_str = order["origQty"].get<std::string>();
      const auto& price = order["price"].get<std::string>();
      const auto& status = order["status"].get<std::string>();
      SymbolPairId symbol_id = SymbolPair::FromBinanceString(symbol_str);
      orders.push_back(Order(std::move(id_str), std::move(client_id_str),
        symbol_id,
        (side_str == "BUY" ? Side::BUY : Side::SELL),
        // TODO: support more order types
        (order_type_str == "MARKET" ? OrderType::MARKET : OrderType::LIMIT),
        std::stod(orig_qty_str),
        std::stod(price),
        Order::GetStatus(status)));
    }
    return Result<std::vector<Order>>(res.response, orders);
  }

  // BinanceClient specific API
  std::optional<std::string> StartUserDataStream() {
    HttpClient::Result res = m_http_client.post(HOST, PORT, LISTEN_KEY_PATH)
        .Header("X-MBX-APIKEY", g_api_key)
        .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error starting user data stream for " << GetExchange() << ": " << res.errmsg;
      return std::nullopt;
    }
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

  BinanceSettings GetBinanceSettings() const {
    return m_binance_settings;
  }
private:

  const std::string& GetSymbolString(SymbolPairId symbol) const {
    if (BINANCE_SYMBOL_TO_STRING_MAP.count(symbol) == 0) {
      BOOST_LOG_TRIVIAL(error) << "Unknown symbol. Exiting.";
      std::exit(1);
    }
    return BINANCE_SYMBOL_TO_STRING_MAP.at(symbol);
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

  json GetExchangeInfo() {
    BOOST_LOG_TRIVIAL(debug) << "Getting Binance exchange info";
    HttpClient::Result res = m_http_client.get(HOST, PORT, EXCHANGE_INFO_PATH)
        .send();
    if (!res) {
      throw std::runtime_error("Error getting Binance exchange info: " + res.errmsg);
    }
    return json::parse(res.response);
  }

private:
  uint64_t m_last_order_id;
  HttpClient m_http_client;
  int m_timeout;
  BinanceSettings m_binance_settings;
};