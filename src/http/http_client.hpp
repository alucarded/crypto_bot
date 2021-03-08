#pragma once
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
    Options(const std::string& user_agent) : m_user_agent(user_agent) {}
    std::string m_user_agent;
    // TODO: add more
  };

  struct Result {
      std::string errmsg;
      std::string response;

      // Returns FALSE when error
      explicit operator bool() const { return errmsg.empty(); }
  };

  class Request {
  public:
    Request(HttpClient& http_client, const std::string& host, const std::string& port,
        const std::string& path, boost::beast::http::verb method, const std::string& user_agent)
        : m_http_client(http_client), m_host(host), m_port(port), m_path(path), m_method(method), m_user_agent(user_agent) {

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

    Request& WithQueryParamSigning(const std::function<void(Request&, std::string&)>& signing_function) {
      m_signing_function = signing_function;
      return *this;
    }

    HttpClient::Result send() {
      return m_http_client.send(*this);
    }

    // Getters
    inline
    const std::string& GetPath() const {
      return m_path;
    }
  private:

    // TODO: support different body types ?
    boost::beast::http::request<boost::beast::http::string_body> build() {
      boost::beast::http::request<boost::beast::http::string_body> req;

      // Add query parameters
      std::string query_string;
      for ( const auto &kv: m_query_params ) {
        if ( !query_string.empty() ) {
            query_string += "&";
        }
        query_string += kv.first;
        query_string += "=";
        query_string += kv.second;
      }
      if (m_signing_function) {
        m_signing_function(*this, query_string);
      }

      if ( m_method != boost::beast::http::verb::get ) {
        req.target(m_path);
        req.body() = std::move(query_string);
        req.set(boost::beast::http::field::content_length, std::to_string(req.body().length()));
      } else {
        req.target(m_path + '?' + query_string);
      }
      req.version(11);

      req.method(m_method);
      if (!m_body.empty()) {
        // TODO: copying here, we could move perhaps
          req.body() = m_body;
          req.set(boost::beast::http::field::content_length, std::to_string(req.body().length()));
      }

      // Default headers
      req.set(boost::beast::http::field::host, m_host);
      //req.set(boost::beast::http::field::user_agent, m_user_agent);
      // TODO: content type should not be hardcoded
      //req.set(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");

      // Set custom headers
      for (const auto& kv : m_headers) {
        req.insert(kv.first, kv.second);
      }

      BOOST_LOG_TRIVIAL(debug) << req << std::endl;
      return req;
    }

  private:
    HttpClient& m_http_client;
    std::string m_host;
    std::string m_port;
    std::string m_path;
    boost::beast::http::verb m_method;
    std::string m_user_agent;
    // TODO: support different body types ?
    std::string m_body;
    std::unordered_map<std::string, std::string> m_query_params;
    std::unordered_map<std::string, std::string> m_headers;
    // TODO: support all types of requests

    std::function<void(Request&, std::string&)> m_signing_function;

    friend class HttpClient;
  };

public:

  HttpClient(const Options& options) :
    m_ioctx(),
    m_options(options),
    m_ssl_ctx{boost::asio::ssl::context::sslv23_client},
    m_resolver{m_ioctx} {

  }
  
  ~HttpClient() {

  }

  // TODO: For now it is always https
  HttpClient::Request get(const std::string& host, const std::string& port, const std::string& path) {
    return Request(*this, host, port, path, boost::beast::http::verb::get, m_options.m_user_agent);
  }

  HttpClient::Request post(const std::string& host, const std::string& port, const std::string& path) {
    return Request(*this, host, port, path, boost::beast::http::verb::post, m_options.m_user_agent);
  }

  Result send(Request& request) {
    Result res;

#define __STRINGIZE_I(x) #x
#define __STRINGIZE(x) __STRINGIZE_I(x)

#define __MAKE_FILELINE \
    __FILE__ "(" __STRINGIZE(__LINE__) ")"

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

    // TODO: support keep-alive
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
    BOOST_LOG_TRIVIAL(debug) << "Up to handshake milliseconds: " << timer.elapsedMilliseconds() << std::endl;

    boost::beast::http::request<boost::beast::http::string_body> req = request.build();

    boost::beast::http::write(ssl_stream, req, ec);
    if ( ec ) {
        BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

        __MAKE_ERRMSG(res, ec.message());
        return res;
    }
    BOOST_LOG_TRIVIAL(debug) << "HTTP write milliseconds: " << timer.elapsedMilliseconds() << std::endl;

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::string_body> bres;

    boost::beast::http::read(ssl_stream, buffer, bres, ec);
    if ( ec ) {
        BOOST_LOG_TRIVIAL(error) << "msg=" << ec.message() << std::endl;

        __MAKE_ERRMSG(res, ec.message());
        return res;
    }
    timer.stop();
    BOOST_LOG_TRIVIAL(debug) << "HTTP response read milliseconds: " << timer.elapsedMilliseconds() << std::endl;

    // TODO: perhaps add parsing in this move assignment (response class implementing move assignment with parsing)
    res.response = std::move(bres.body());
    BOOST_LOG_TRIVIAL(debug) << request.m_path << " REPLY:\n" << res.response << std::endl << std::endl;

    ssl_stream.shutdown(ec);

    return res;
  }

private:
  boost::asio::io_context m_ioctx;
  Options m_options;
  boost::asio::ssl::context m_ssl_ctx;
  boost::asio::ip::tcp::resolver m_resolver;
};