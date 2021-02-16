#include "json/json.hpp"

#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>

namespace cryptobot {

using json = nlohmann::json;

json GetConfigJson(const std::string& config_path) {
  std::ifstream config_fs(config_path, std::ios::in | std::ios::binary);
  if (config_fs)
  {
    std::ostringstream contents;
    contents << config_fs.rdbuf();
    config_fs.close();
    return json::parse(contents.str());
  }
  throw(errno);
}

} // namespace cryptobot