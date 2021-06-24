#include "exchange/exchange_client.h"
#include "db/mongo_client.hpp"
#include "db/mongo_ticker_producer.hpp"
#include "strategy/indicator/indicators_generator.hpp"
#include "utils/config.hpp"
#include "utils/string.h"

#include "json/json.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <cerrno>
#include <iostream>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Please provide configuration file" << std::endl;
    return -1;
  }

  logging::core::get()->set_filter(
      logging::trivial::severity >= logging::trivial::debug);

  std::string config_path(argv[1]);
  auto config_json = cryptobot::GetConfigJson(config_path);
  // TODO: take credentials from parameter store
  std::string pass("DRt99xd4o7PMfygqotE8");
  MongoClient* mongo_client =
          MongoClient::GetInstance()->CreatePool("mongodb://" + config_json["user"].get<std::string>() + ":" + pass
          + "@" + config_json["host"].get<std::string>() + ":" + config_json["port"].get<std::string>()
          + "/?authSource=" + config_json["authSource"].get<std::string>());
  std::cout << "Created Mongo client pool" << std::endl;
  pass.clear();

  IndicatorsGenerator indicators_generator;

  MongoTickerProducer mongo_producer(mongo_client, config_json["db"].get<std::string>(), config_json["collection"].get<std::string>());
  // First register clients, so that execution price is same as seen price
  // mongo_producer.Register(&binance_backtest_client);
  // mongo_producer.Register(&kraken_backtest_client);
  mongo_producer.Register(&indicators_generator);

  int64_t count = mongo_producer.Produce();
  std::cout << "Produced " + std::to_string(count) + " events" << std::endl;

  return 0;
}
