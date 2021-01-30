#pragma once
#include "ticker.h"
#include "consumer/consumer.h"

#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <optional>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
//typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

class TickerClient {
public:
    virtual const std::string GetUrl() const = 0;
    // TODO: Use Exchange enum
    virtual const std::string GetExchangeName() const = 0;

    void start() {
        start(GetUrl());
    }
protected:
    TickerClient(Consumer<RawTicker>* ticker_consumer) : m_ticker_consumer(ticker_consumer), m_do_reconnect(3), m_tries(0) {
        m_endpoint.set_access_channels(websocketpp::log::alevel::connect);
        m_endpoint.set_error_channels(websocketpp::log::elevel::warn);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        //m_endpoint.set_socket_init_handler(bind(&TickerClient::on_socket_init,this,::_1));
        m_endpoint.set_tls_init_handler(bind(&TickerClient::on_tls_init,this,::_1));
        m_endpoint.set_message_handler(bind(&TickerClient::on_message,this,::_1,::_2));
        m_endpoint.set_open_handler(bind(&TickerClient::on_open,this,::_1));
        m_endpoint.set_close_handler(bind(&TickerClient::on_close,this,::_1));
        m_endpoint.set_fail_handler(bind(&TickerClient::on_fail,this,::_1));
    }

    void start(const std::string& uri) {
        connect(uri);
        m_thread = std::make_shared<std::thread>(&client::run, &m_endpoint);
    }

    void connect(const std::string& uri) {
        websocketpp::lib::error_code ec;
        m_con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            m_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
            return;
        }

        m_endpoint.connect(m_con);
    }

    virtual void send(const std::string& message) {
        std::cout << GetExchangeName() + ": Sending" << std::endl;
        websocketpp::lib::error_code ec;
        m_endpoint.send(m_con->get_handle(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            m_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
        }
    }

    virtual context_ptr on_tls_init(websocketpp::connection_hdl) {
        context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::single_dh_use);
            //ctx->set_verify_mode(boost::asio::ssl::verify_none);
        } catch (std::exception& e) {
            std::cout << "Got exception" << std::endl;
            std::cout << e.what() << std::endl;
        }
        return ctx;
    }

    virtual void on_fail(websocketpp::connection_hdl hdl) {
        client::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
        
        std::cout << GetExchangeName() + ": Fail handler" << std::endl;
        std::cout << con->get_state() << std::endl;
        std::cout << con->get_local_close_code() << std::endl;
        std::cout << con->get_local_close_reason() << std::endl;
        std::cout << con->get_remote_close_code() << std::endl;
        std::cout << con->get_remote_close_reason() << std::endl;
        std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
    }

    virtual void on_open(websocketpp::connection_hdl) {
        std::cout << GetExchangeName() + ": Connection opened" << std::endl;
        // Send empty ticker
        if (m_ticker_consumer) {
            m_ticker_consumer->Consume(RawTicker::Empty(GetExchangeName()));
        }
        request_ticker();
    }

    virtual void on_close(websocketpp::connection_hdl) {
        std::cout << GetExchangeName() + ": Connection closed" << std::endl;
        // Send empty ticker
        if (m_ticker_consumer) {
            m_ticker_consumer->Consume(RawTicker::Empty(GetExchangeName()));
        }
        if (m_tries < m_do_reconnect) {
            ++m_tries;
            connect(GetUrl());
        }
    }

    virtual void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
        m_tries = 0;
        std::optional<RawTicker> ticker_opt = extract_ticker(msg);
        if (!ticker_opt.has_value()) {
            return;
        }
        if (m_ticker_consumer) {
            m_ticker_consumer->Consume(ticker_opt.value());
        }
    }

    virtual void request_ticker() = 0;
    virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) = 0;

protected:
    client m_endpoint;
    client::connection_ptr m_con;
    std::shared_ptr<std::thread> m_thread;
    Consumer<RawTicker>* m_ticker_consumer;
    int m_do_reconnect;
    int m_tries;
};