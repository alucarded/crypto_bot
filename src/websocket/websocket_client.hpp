#pragma once
#include "model/ticker.h"

#include <boost/log/trivial.hpp>

#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <optional>

using namespace std::chrono_literals;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
//typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

class WebsocketClient {
public:
    void start(std::promise<void>&& promise) {
        m_start_promise = std::move(promise);
        start(m_uri);
    }

    // Do not call from m_thread!
    void close() {
        try {
            m_endpoint->pause_reading(m_con);
            m_endpoint->close(m_con, websocketpp::close::status::normal, "");
        } catch (const websocketpp::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Exception when closing websocketpp endpoint: " << e.what();
        }
    }
protected:
    WebsocketClient(const std::string& uri, const std::string& name) :  m_uri(uri), m_name(name), m_endpoint(new client()),
            m_do_reconnect(true), m_reconnect_delay(3000ms) {
        init_endpoint();
    }

    void init_endpoint() {
        m_endpoint->set_access_channels(websocketpp::log::alevel::all);
        m_endpoint->set_error_channels(websocketpp::log::elevel::all);

        m_endpoint->init_asio();
        m_endpoint->start_perpetual();

        //m_endpoint.set_socket_init_handler(bind(&WebsocketClient::on_socket_init,this,::_1));
        m_endpoint->set_tls_init_handler(bind(&WebsocketClient::on_tls_init,this,::_1));
        m_endpoint->set_message_handler(bind(&WebsocketClient::on_message,this,::_1,::_2));
        m_endpoint->set_open_handler(bind(&WebsocketClient::on_open,this,::_1));
        m_endpoint->set_close_handler(bind(&WebsocketClient::on_close,this,::_1));
        m_endpoint->set_fail_handler(bind(&WebsocketClient::on_fail,this,::_1));
        m_endpoint->set_ping_handler(bind(&WebsocketClient::on_ping,this,::_1,::_2));
    }

    void start(const std::string& uri) {
        connect(uri);
        m_thread = std::make_shared<std::thread>(&client::run, m_endpoint.get());
        m_thread->detach();
    }

    void connect(const std::string& uri) {
        websocketpp::lib::error_code ec;
        m_con = m_endpoint->get_connection(uri, ec);

        if (ec) {
            m_endpoint->get_alog().write(websocketpp::log::alevel::app, ec.message());
            return;
        }

        m_endpoint->connect(m_con);
    }

    virtual void send(const std::string& message) {
        BOOST_LOG_TRIVIAL(debug) << m_name + ": Sending" << std::endl;
        websocketpp::lib::error_code ec;
        m_endpoint->send(m_con->get_handle(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            m_endpoint->get_alog().write(websocketpp::log::alevel::app, ec.message());
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
            BOOST_LOG_TRIVIAL(error) << "Got exception" << std::endl;
            BOOST_LOG_TRIVIAL(error) << e.what() << std::endl;
        }
        return ctx;
    }

    virtual void on_fail(websocketpp::connection_hdl hdl) {
        client::connection_ptr con = m_endpoint->get_con_from_hdl(hdl);
        
        std::cout << m_name + ": Fail handler" << std::endl;
        std::cout << con->get_state() << std::endl;
        std::cout << con->get_local_close_code() << std::endl;
        std::cout << con->get_local_close_reason() << std::endl;
        std::cout << con->get_remote_close_code() << std::endl;
        std::cout << con->get_remote_close_reason() << std::endl;
        std::cout << con->get_ec() << " - " << con->get_ec().message() << std::endl;

        reconnect(hdl);
    }

    virtual void on_open(websocketpp::connection_hdl conn) {
        BOOST_LOG_TRIVIAL(debug) << m_name + ": Connection opened" << std::endl;
        m_start_promise.set_value();
        OnOpen(conn);
    }

    virtual void on_close(websocketpp::connection_hdl conn) {
        BOOST_LOG_TRIVIAL(debug) << m_name + ": Connection closed";
        OnClose(conn);
        reconnect(conn);
    }

    void reconnect(websocketpp::connection_hdl conn) {
        // Reconnect
        if (m_do_reconnect) {
            m_endpoint->stop();
            m_endpoint->reset();
            // m_endpoint.reset(new client());
            // init_endpoint();
            // TODO: do we need to delay here ?
            std::this_thread::sleep_for(m_reconnect_delay);
            start(std::move(std::promise<void>()));
        }
    }

    virtual void on_message(websocketpp::connection_hdl conn, client::message_ptr msg) {
        // For testing reconnecting:
        // close();
        try {
            OnMessage(conn, msg);
        } catch (std::exception const & e) {
            BOOST_LOG_TRIVIAL(error) << "WebsocketClient::OnMessage exception for connection " << m_name << ": " << e.what() << std::endl;
            BOOST_LOG_TRIVIAL(error) << "Message: " << msg->get_payload() << std::endl;
        }
    }

    virtual bool on_ping(websocketpp::connection_hdl conn, std::string payload) {
        BOOST_LOG_TRIVIAL(debug) << m_name + " ping control frame payload: " + payload;
        return OnPing(conn, payload);
    }

    virtual void OnOpen(websocketpp::connection_hdl conn) = 0;
    virtual void OnClose(websocketpp::connection_hdl conn) = 0;
    virtual void OnMessage(websocketpp::connection_hdl conn, client::message_ptr msg) = 0;
    virtual bool OnPing(websocketpp::connection_hdl conn, std::string payload) {}

protected:
    std::string m_uri;
    std::string m_name;
    std::shared_ptr<client> m_endpoint;
    client::connection_ptr m_con;
    std::shared_ptr<std::thread> m_thread;
    bool m_do_reconnect;
    std::chrono::duration<int64_t, std::milli> m_reconnect_delay;
    std::promise<void> m_start_promise;
};