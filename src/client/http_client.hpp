#include "utils/timer.hpp"

#include <boost/preprocessor.hpp>
#include <boost/callable_traits.hpp>
#include <boost/variant.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/log/trivial.hpp>

#include <string>
#include <unordered_map>

class HttpClient {
public:
  struct Options {
    std::string m_user_agent;
    // TODO: add more
  };

  struct Result {
      std::string errmsg;
      std::string response;

      // Returns FALSE when error
      explicit operator bool() const { return errmsg.empty(); }
  };

  class Request;

  HttpClient(boost::asio::io_context &ioctx, const Options& options) :
    m_ioctx(ioctx),
    m_options(options),
    m_ssl_ctx{boost::asio::ssl::context::sslv23_client},
    m_resolver{ioctx} {

  }
  
  ~HttpClient() {

  }

  // TODO: For now it is always https
  Request post(const std::string& host, const std::string& port, const std::string& path) {
    return Request(*this, host, port, path, boost::beast::http::verb::post, m_options.m_user_agent);
  }

  Result send(const Request& request) {
    Result res;

#define __MAKE_ERRMSG(res, msg) \
    res.errmsg = __MAKE_FILELINE; \
    res.errmsg += ": "; \
    res.errmsg += msg;

    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_stream(m_ioctx, m_ssl_ctx);

    cryptobot::Timer timer;
    timer.start();
    if( !SSL_set_tlsext_host_name(ssl_stream.native_handle(), request.m_host.c_str()) ) {
      boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
      BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

      __MAKE_ERRMSG(res, ec.message());
      return res;
    }

    boost::system::error_code ec;
    // Resolve hostname
    auto const results = m_resolver.resolve(request.m_host, request.m_port, ec);
    if ( ec ) {
      BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

      __MAKE_ERRMSG(res, ec.message());
      return res;
    }

    boost::asio::connect(ssl_stream.next_layer(), results.begin(), results.end(), ec);
    if ( ec ) {
      BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

      __MAKE_ERRMSG(res, ec.message());
      return res;
    }

    ssl_stream.handshake(boost::asio::ssl::stream_base::client, ec);
    if ( ec ) {
      BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

      __MAKE_ERRMSG(res, ec.message());
      return res;
    }
    std::cout << "Up to handshake milliseconds: " << timer.elapsedMilliseconds() << std::endl;

    boost::beast::http::request<boost::beast::http::string_body> req = request.build();
    // req.target(request.m_path);
    // req.version(11);

    // req.method(request.m_method);
    // if ( request.m_method != boost::beast::http::verb::get ) {
    //     req.body() = std::move(request.m_data);
    //     req.set(boost::beast::http::field::content_length, std::to_string(req.body().length()));
    // }

    // req.insert("X-MBX-APIKEY", m_pk);
    // req.set(boost::beast::http::field::host, request.m_host);
    // req.set(boost::beast::http::field::user_agent, m_client_api_string);
    // req.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");

    boost::beast::http::write(ssl_stream, req, ec);
    if ( ec ) {
        BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

        __MAKE_ERRMSG(res, ec.message());
        return res;
    }
    std::cout << "HTTP write milliseconds: " << timer.elapsedMilliseconds() << std::endl;

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::string_body> bres;

    boost::beast::http::read(ssl_stream, buffer, bres, ec);
    if ( ec ) {
        BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

        __MAKE_ERRMSG(res, ec.message());
        return res;
    }
    timer.stop();
    std::cout << "HTTP response read milliseconds: " << timer.elapsedMilliseconds() << std::endl;

    // TODO: perhaps add parsing in this move assignment (response class implementing move assignment with parsing)
    res.response = std::move(bres.body());
    std::cout << request.m_path << " REPLY:\n" << res.response << std::endl << std::endl;

    ssl_stream.shutdown(ec);

    return res;
  }
private:

  class Request {
  public:
    Request(HttpClient& http_client, const std::string& host, const std::string& port,
        const std::string& path, boost::beast::http::verb method, const std::string& m_user_agent)
        : m_http_client(http_client), m_host(host), m_port(port), m_path(path), m_method(method) {

    }

    Request& QueryParam(const std::string& key, const std::string& value) {
      // Update value if exists
      m_query_params[key] = value;
      return *this;
    }

    Request& Header(const std::string& name, const std::string& value) {
      // Update value if exists
      m_headers[name] = value;
      return *this;
    }

    HttpClient::Result send() {
      return m_http_client.send(*this);
    }

  private:

    // TODO: support different body types ?
    boost::beast::http::request<boost::beast::http::string_body> build() {
      boost::beast::http::request<boost::beast::http::string_body> req;

      // Add query parameters
      std::string query_string;
      for ( const auto &kv: m_query_params ) {
          if ( is_valid_value(kv.second) ) {
              if ( !data.empty() ) {
                  query_string += "&";
              }
              query_string += kv.first;
              query_string += "=";
              query_string += kv.second;
          }
      }
      req.target(m_path + query_string);

      req.version(11);

      req.method(m_method);
      if (!m_body.empty()) {
        // TODO: copying here, we could move perhaps
          req.body() = m_body;
          req.set(boost::beast::http::field::content_length, std::to_string(req.body().length()));
      }

      // Default headers
      req.set(boost::beast::http::field::host, m_host);
      req.set(boost::beast::http::field::user_agent, m_client_api_string);
      // TODO: content type should not be hardcoded
      req.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");

      // Set custom headers
      for (const auto& kv : m_headers) {
        req.insert(kv.first, kv.second);
      }

      return req;
    }

  private:
    HttpClient& m_http_client;
    std::string m_host;
    std::string m_port;
    std::string m_path;
    boost::beast::http::verb m_method;
    // TODO: support different body types ?
    std::string m_body;
    std::unordered_map<std::string, std::string> m_query_params;
    std::unordered_map<std::string, std::string> m_headers;
    // TODO: support all types of requests

    friend class HttpClient;
  };

private:
  boost::asio::io_context &m_ioctx;
  Options m_options;
  boost::asio::ssl::context m_ssl_ctx;
  boost::asio::ip::tcp::resolver m_resolver;
};