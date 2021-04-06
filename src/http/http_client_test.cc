#include "http_client.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono;

void SendRequest(HttpClient& http_client) {
  HttpClient::Result res = http_client.get("api.binance.com", std::to_string(443), "/api/v3/time")
      .send();
  if (!res) {
    BOOST_LOG_TRIVIAL(error) << "Error sending request: " << res.errmsg;
  } else {
    BOOST_LOG_TRIVIAL(info) << "Response: " << res.response;
  }
}

int main() {
  HttpClient::Options opts{"test_agent"};
  HttpClient http_client{opts};
  SendRequest(http_client);
  std::this_thread::sleep_for(1s);
  SendRequest(http_client);
  std::this_thread::sleep_for(10s);
  SendRequest(http_client);
  std::this_thread::sleep_for(60s);
  SendRequest(http_client);
  std::this_thread::sleep_for(300s);
  SendRequest(http_client);
  return 0;
}