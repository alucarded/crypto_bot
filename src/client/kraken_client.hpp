#include "exchange_client.h"
#include "http_client.hpp"

#include <array>
#include <sstream>
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

class KrakenClient : public ExchangeClient {
public:
  inline static const std::string HOST = "api.kraken.com";

  KrakenClient() : m_http_client(HttpClient::Options("cryptobot-1.0.0")) {

  }

  virtual MarketOrderResult MarketOrder(const std::string& symbol, Side side, double qty) override {
    HttpClient::Result res = m_http_client.post("api.kraken.com", "443", "/0/private/AddOrder")
        .QueryParam("pair", symbol)
        .QueryParam("type", (Side::BID == side ? "buy" : "sell"))
        .QueryParam("ordertype", "market")
        // TODO: std::to_string has default precision of 6 digits, use ostringstream
        .QueryParam("volume", std::to_string(qty))
        //.QueryParam("trading_agreement", "agree")
        .Header("API-Key", g_public_key)
        .WithQueryParamSigning([g_private_key](HttpClient::Request& request, std::string& query_string) {
          struct timeval sys_time;   gettimeofday  (&sys_time, nullptr);

          auto nonce  =  ostringstream {};
    
          nonce << (uint64_t) ((sys_time.tv_sec * 1000000) + sys_time.tv_usec);

          if (!query_string.empty()) {
            query_string += '&';
          }
          query_string += "nonce=" + nonce.str();
          auto const  D       =  sha256 (nonce.str ()  +  query_string);
          auto const  digest  =  "/0/private/AddOrder"  +  std::string {std::begin (D), std::end (D)};
          auto private_key = base64_decode (g_private_key); //std::vector<uint8_t>(std::begin(g_private_key), std::end(g_private_key));
          auto const  hmac
              =  base64_encode  
                 (hmac_sha512  (vector<uint8_t> {std::begin (digest), std::end (digest)},
                                private_key));
          request.Header("API-Sign", hmac);
        })
        .send();
      return res.response;
  }

  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) override {

  }

  virtual void CancelAllOrders() override {

  }
private:
  HttpClient m_http_client;
};