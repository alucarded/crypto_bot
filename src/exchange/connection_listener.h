#pragma once

#include <string>

class ConnectionListener {
public:
  virtual ~ConnectionListener() { }

  virtual void OnConnectionOpen(const std::string& name) = 0;

  virtual void OnConnectionClose(const std::string& name) = 0;
};