#pragma once
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp> 
#include <iostream>
#include <memory>

class MongoClient {
public:
  static MongoClient* GetInstance () {
    static MongoClient s_instance;
    return &s_instance;
  }

  MongoClient* CreatePool(std::string mongo_uri) {
    if (m_client_pool) {
      std::cerr << "Client pool already created" << std::endl;
      return this;
    }
    m_client_pool = std::make_unique<mongocxx::pool>(mongocxx::uri {mongo_uri});
    return this;
  }

  mongocxx::pool::entry Get() {
    return m_client_pool->acquire();
  }

private:
  MongoClient() { }

  mongocxx::instance m_instance;
  std::unique_ptr<mongocxx::pool> m_client_pool;
};