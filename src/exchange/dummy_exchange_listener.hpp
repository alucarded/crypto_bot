#include "exchange_listener.h"

#include <boost/log/trivial.hpp>

#include <string>

class DummyExchangeListener : public ExchangeListener {
public:
  ~DummyExchangeListener() { }

  virtual void OnConnectionOpen(const std::string& name) override {
    BOOST_LOG_TRIVIAL(info) << "DummyExchangeListener::OnConnectionOpen, name=" << name;
  }

  virtual void OnConnectionClose(const std::string& name) override {
    BOOST_LOG_TRIVIAL(info) << "DummyExchangeListener::OnConnectionClose, name=" << name;
  }
};