#pragma once
#include "ticker.h"
#include "consumer/ticker_consumer.h"

#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
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
    virtual const std::string GetExchangeName() const = 0;
protected:
    TickerClient(TickerConsumer* ticker_consumer) : m_ticker_consumer(ticker_consumer) {
        m_endpoint.set_access_channels(websocketpp::log::alevel::all);
        m_endpoint.set_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        //m_endpoint.set_socket_init_handler(bind(&TickerClient::on_socket_init,this,::_1));
        m_endpoint.set_tls_init_handler(bind(&TickerClient::on_tls_init,this,::_1));
        m_endpoint.set_message_handler(bind(&TickerClient::on_message,this,::_1,::_2));
        m_endpoint.set_open_handler(bind(&TickerClient::on_open,this,::_1));
        //m_endpoint.set_close_handler(bind(&TickerClient::on_close,this,::_1));
        m_endpoint.set_fail_handler(bind(&TickerClient::on_fail,this,::_1));
    }

    virtual void start(std::string uri) {
        websocketpp::lib::error_code ec;
        m_con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            m_endpoint.get_alog().write(websocketpp::log::alevel::app,ec.message());
            return;
        }

        m_endpoint.connect(m_con);
        m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
    }

    virtual void send(const std::string& message) {
        std::cout << "Sending" << std::endl;
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
        
        std::cout << "Fail handler" << std::endl;
        std::cout << con->get_state() << std::endl;
        std::cout << con->get_local_close_code() << std::endl;
        std::cout << con->get_local_close_reason() << std::endl;
        std::cout << con->get_remote_close_code() << std::endl;
        std::cout << con->get_remote_close_reason() << std::endl;
        std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;
    }

    virtual void on_open(websocketpp::connection_hdl) {
        std::cout << "Connection opened" << std::endl;
    }

    virtual void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
        std::optional<RawTicker> ticker_opt = extract_ticker(msg);
        if (!ticker_opt.has_value()) {
            return;
        }
        if (m_ticker_consumer) {
            m_ticker_consumer->Consume(ticker_opt.value());
        }
    }

    virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) = 0;

protected:
    client m_endpoint;
    client::connection_ptr m_con;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
    TickerConsumer* m_ticker_consumer;
};