#pragma once

#include "exchange/exchange_client.h"
#include "http_client.hpp"
#include "utils/string.hpp"

#include <boost/log/trivial.hpp>
#include "json/json.hpp"


#include <array>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {
  // TODO: FIXME: read it from secure config!
  std::string g_public_key = "4XUCr+2lTgf14sOhZEDDC/Z/9wKliuFORLkMh+QH4Hgy4+NAW4Rvt2Fg";
  std::string g_private_key = "EZbALHeU+EqvpJdVWtPNxBbfhoLEEPYgfrxuP5Vqy59XpddpdbvcTNKG/tki8uwERPMX8mx1Imvgzl1wdoVEHA==";

// TODO: following functions are under GPL, implement replacements
using namespace std;
  array<uint8_t, SHA256_DIGEST_LENGTH>  sha256  (string const &data)
  {
    auto  ret  =  array<uint8_t, SHA256_DIGEST_LENGTH> {};

    auto digest  =  EVP_MD_CTX_new ();
    EVP_DigestInit_ex   (digest,  EVP_sha256 (),  nullptr);
    EVP_DigestUpdate    (digest,  data.data (),  data.length ());
    EVP_DigestFinal_ex  (digest,  ret.data (),  nullptr);
    EVP_MD_CTX_free     (digest);

    return  ret;
  }



  vector<uint8_t>  hmac_sha512  (vector<uint8_t> const &data,
                                 vector<uint8_t> const &key)
  {
    auto length  =  unsigned {EVP_MAX_MD_SIZE};

    auto  ret  =  vector<uint8_t> (length);

    HMAC_CTX *const ctx  =  HMAC_CTX_new ();
    HMAC_Init_ex   (ctx, key.data (), key.size (), EVP_sha512 (), nullptr);
    HMAC_Update    (ctx, data.data (), data.size ());
    HMAC_Final     (ctx, ret.data (),  &length);
    HMAC_CTX_free  (ctx);

    ret.resize (length);

    return  ret;
  }


  vector<uint8_t>  base64_decode  (string const  &data)
  {
    auto b64  =  BIO_new (BIO_f_base64 ());
    BIO_set_flags (b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push (b64, BIO_new_mem_buf  (data.data (), data.length ()));

    auto  output  =  vector<uint8_t> {};    output.reserve (data.length ());
    auto  buffer  =  array<uint8_t, 512> {};

    for (;;)
      {
        auto const len  =  BIO_read (b64, buffer.data (), buffer.size ());
        if (len <= 0)   break;
        output.insert (end (output), begin (buffer), begin (buffer) + len);
      }

    BIO_free_all  (b64);

    return output;
  }



  string  base64_encode  (vector<uint8_t> const &data)
  {
    auto *const  b64   =   BIO_new (BIO_f_base64 ());

    BIO_set_flags  (b64,  BIO_FLAGS_BASE64_NO_NL);
    BIO_push       (b64,  BIO_new (BIO_s_mem ()));
    BIO_write      (b64,  data.data (),  data.size ());
    BIO_flush      (b64);

    BUF_MEM* bptr = nullptr;   BIO_get_mem_ptr (b64, &bptr);

    auto const ret  =  string {bptr->data, bptr->data + bptr->length};

    BIO_free_all (b64);

    return ret;
  }

}

using json = nlohmann::json;

using namespace std::placeholders; // _1, _2, etc.

class KrakenClient : public ExchangeClient {
public:
  inline static const std::string HOST = "api.kraken.com";
  inline static const std::string PORT = "443";
  inline static const std::string ADD_ORDER_PATH = "/0/private/AddOrder";
  inline static const std::string GET_ACCOUNT_BALANCE_PATH = "/0/private/Balance";
  inline static const std::string GET_OPEN_ORDERS_PATH = "/0/private/OpenOrders";
  inline static const std::string GET_WEB_SOCKETS_TOKEN_PATH = "/0/private/GetWebSocketsToken";

  static std::unordered_map<SymbolPairId, std::string> SYMBOL_MAP;
  static std::unordered_map<std::string, SymbolId> ASSET_MAP;

  // Maps Kraken asset names to our names
  static const std::unordered_map<std::string, std::string> ASSET_NAME_MAP;

  KrakenClient() : m_http_client(HttpClient::Options("cryptobot-1.0.0")) {

  }

  virtual std::string GetExchange() override {
    return "kraken";
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    const std::string& symbol_str = GetSymbolString(symbol);
    HttpClient::Result res = m_http_client.post(HOST, PORT, ADD_ORDER_PATH)
        .QueryParam("pair", symbol_str)
        .QueryParam("type", (Side::BUY == side ? "buy" : "sell"))
        .QueryParam("ordertype", "market")
        // TODO: std::to_string has default precision of 6 digits, use ostringstream
        .QueryParam("volume", std::to_string(qty))
        .Header("API-Key", g_public_key)
        .WithQueryParamSigning(std::bind(&KrakenClient::SignQueryString, this, _1, _2))
        .send();
      if (!res) {
        BOOST_LOG_TRIVIAL(error) << "Error making market order request for " << GetExchange() << ": " << res.errmsg;
        return Result<Order>(res.response, res.errmsg);
      }
      json response_json = json::parse(res.response);
      if (response_json["error"].size() > 0) {
        // TODO: propagate all errors ?
        return Result<Order>(res.response, response_json["error"][0]);
      }
      // TODO: support multiple transaction ids per order ?
      BOOST_LOG_TRIVIAL(debug) << "MarketOrder response from " << GetExchange() << ": " << std::endl << res.response << std::endl;
      auto order_id = response_json["result"]["txid"][0].get<std::string>();
      return Result<Order>(res.response, Order(order_id, order_id, symbol, side, OrderType::MARKET, qty));
  }

  // TODO: add expiration time ?
  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    const std::string& symbol_str = GetSymbolString(symbol);
    HttpClient::Result res = m_http_client.post(HOST, PORT, ADD_ORDER_PATH)
        .QueryParam("pair", symbol_str)
        .QueryParam("type", (Side::BUY == side ? "buy" : "sell"))
        .QueryParam("ordertype", "limit")
        // TODO: std::to_string has default precision of 6 digits, use ostringstream
        // FIXME: get precision
        .QueryParam("price", cryptobot::to_string(price, 8))
        .QueryParam("volume", std::to_string(qty))
        .Header("API-Key", g_public_key)
        .WithQueryParamSigning(std::bind(&KrakenClient::SignQueryString, this, _1, _2))
        .send();
      if (!res) {
        BOOST_LOG_TRIVIAL(error) << "Error making limit order request for " << GetExchange() << ": " << res.errmsg;
        return Result<Order>(res.response, res.errmsg);
      }
      json response_json = json::parse(res.response);
      if (response_json["error"].size() > 0) {
        // TODO: propagate all errors ?
        return Result<Order>(res.response, response_json["error"][0]);
      }
      // TODO: support multiple transaction ids per order ?
      BOOST_LOG_TRIVIAL(debug) << "LimitOrder response from " << GetExchange() << ": " << std::endl << res.response << std::endl;
      auto order_id = response_json["result"]["txid"][0].get<std::string>();
      return Result<Order>(res.response, Order(order_id, order_id, symbol, side, OrderType::LIMIT, qty));
  }

  virtual void CancelAllOrders() override {

  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    HttpClient::Result res = m_http_client.post(HOST, PORT, GET_ACCOUNT_BALANCE_PATH)
        .Header("API-Key", g_public_key)
        .WithQueryParamSigning(std::bind(&KrakenClient::SignQueryString, this, _1, _2))
        .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error getting account balance for " << GetExchange() << ": " << res.errmsg;
      return Result<AccountBalance>(res.response, res.errmsg);
    }
    json response_json = json::parse(res.response);
    BOOST_LOG_TRIVIAL(debug) << "GetAccountBalance() response: " << response_json;
    if (response_json["error"].size() > 0) {
      // TODO: propagate all errors ?
      return Result<AccountBalance>(res.response, response_json["error"][0]);
    }
    std::unordered_map<SymbolId, double> balances;
    // nlohmann::detail::from_json(response_json["result"], balances);
    for (const auto& el : response_json["result"].items()) {
      const auto& asset_str = el.key();
      if (ASSET_MAP.count(asset_str) == 0) {
        BOOST_LOG_TRIVIAL(warning) << "[KrakenClient::GetAccountBalance] Skipping unsupported asset: " << asset_str;
        continue;
      }
      const auto& val_str = el.value().get<std::string>();
      balances.insert(std::make_pair(ASSET_MAP[asset_str], std::stod(val_str)));
    }
    return Result<AccountBalance>(res.response, AccountBalance(std::move(balances)));
  }

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    HttpClient::Result res = m_http_client.post(HOST, PORT, GET_OPEN_ORDERS_PATH)
        .Header("API-Key", g_public_key)
        .WithQueryParamSigning(std::bind(&KrakenClient::SignQueryString, this, _1, _2))
        .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error getting open orders for " << GetExchange() << ": " << res.errmsg;
      return Result<std::vector<Order>>(res.response, res.errmsg);
    }
    json response_json = json::parse(res.response);
    if (response_json["error"].size() > 0) {
      return Result<std::vector<Order>>(res.response, response_json["error"][0]);
    }
    std::vector<Order> orders;
    for (auto& o : response_json["result"]["open"].items()) {
      std::string key_str = o.key();
      const json& val = o.value();
      const auto& descr = val["descr"];
      const auto& side_str = descr["type"].get<std::string>();
      const auto& ordertype_str = descr["ordertype"].get<std::string>();
      const auto& pair_str = descr["pair"].get<std::string>();
      const auto& vol_str = val["vol"].get<std::string>();
      const auto& price_str = descr["price"].get<std::string>();
      const auto& status_str = val["status"].get<std::string>();
      SymbolPairId sp_id = SymbolPair(pair_str);
      if (sp_id == SymbolPairId::UNKNOWN) {
        BOOST_LOG_TRIVIAL(warning) << "Unsupported pair: " << pair_str;
        // Add as unknown, so that client knows there are some unsupported orders
      }
      orders.push_back(Order(std::move(key_str), std::move(key_str), sp_id,
          (side_str == "buy" ? Side::BUY : Side::SELL),
          (ordertype_str == "market" ? OrderType::MARKET : OrderType::LIMIT),
          std::stod(vol_str),
          std::stod(price_str),
          Order::GetStatus(status_str)));
    }
    return Result<std::vector<Order>>(res.response, orders);
  }

  std::string GetWebSocketsToken() {
    HttpClient::Result res = m_http_client.post(HOST, PORT, GET_WEB_SOCKETS_TOKEN_PATH)
        .Header("API-Key", g_public_key)
        .WithQueryParamSigning(std::bind(&KrakenClient::SignQueryString, this, _1, _2))
        .send();
    if (!res) {
      BOOST_LOG_TRIVIAL(error) << "Error getting websockets token for " << GetExchange() << ": " << res.errmsg;
      return std::string();
    }
    json response_json = json::parse(res.response);
    if (response_json["error"].size() > 0) {
      BOOST_LOG_TRIVIAL(error) << "Error getting WebSocket token: " << response_json["error"][0];
      return std::string();
    }
    return response_json["result"]["token"].get<std::string>();
  }

private:
  void SignQueryString(HttpClient::Request& request, std::string& query_string) {
    struct timeval sys_time;
    gettimeofday(&sys_time, nullptr);

    auto nonce = ostringstream{};
    nonce << (uint64_t) ((sys_time.tv_sec * 1000000) + sys_time.tv_usec);

    if (!query_string.empty()) {
      query_string += '&';
    }
    query_string += "nonce=" + nonce.str();
    auto const D = sha256(nonce.str() + query_string);
    auto const digest = request.GetPath() + std::string{std::begin(D), std::end(D)};
    auto private_key = base64_decode(g_private_key); //std::vector<uint8_t>(std::begin(g_private_key), std::end(g_private_key));
    auto const hmac = base64_encode(
        hmac_sha512(vector<uint8_t>{std::begin(digest), std::end(digest)}, private_key));
    request.Header("API-Sign", hmac);
  }

  const std::string& GetSymbolString(SymbolPairId symbol) const {
    if (SYMBOL_MAP.count(symbol) == 0) {
      BOOST_LOG_TRIVIAL(error) << "Unknown symbol. Exiting.";
      std::exit(1);
    }
    return SYMBOL_MAP.at(symbol);
  }

private:
  HttpClient m_http_client;
};

const std::unordered_map<std::string, std::string> KrakenClient::ASSET_NAME_MAP = {
  {"XXBT", "BTC"},
  {"XBT", "BTC"}
};

std::unordered_map<SymbolPairId, std::string> KrakenClient::SYMBOL_MAP = {
  {SymbolPairId::ADA_USDT, "ADAUSDT"},
  {SymbolPairId::BTC_USDT, "BTCUSDT"},
  {SymbolPairId::ETH_USDT, "ETHUSDT"},
  {SymbolPairId::EOS_USDT, "EOSUSDT"},
  {SymbolPairId::ADA_BTC, "ADABTC"},
  {SymbolPairId::ETH_BTC, "ETHBTC"},
  {SymbolPairId::EOS_BTC, "EOSBTC"},
  {SymbolPairId::EOS_ETH, "EOSETH"}
};

std::unordered_map<std::string, SymbolId> KrakenClient::ASSET_MAP = {
  {"ADA", SymbolId::ADA},
  {"XBT", SymbolId::BTC},
  {"XXBT", SymbolId::BTC},
  {"ETH", SymbolId::ETH},
  {"XETH", SymbolId::ETH},
  {"EOS", SymbolId::EOS},
  {"USDT", SymbolId::USDT}
};