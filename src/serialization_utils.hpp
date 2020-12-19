#pragma once
#include "json/json.hpp"

#include <iostream>
#include <list>
#include <string>

using json = nlohmann::json;

namespace utils {

bool check_message(json j, const std::string& field) {
    if (!j.contains(field)) {
        std::cout << "No " + field + ", skipping message" << std::endl;
        return false;
    }
    return true;
}

bool check_message(json j, const std::list<std::string>& fields) {
    for (const std::string& field : fields) {
        if (!check_message(j, field)) {
        return false;
        }
    }
    return true;
}

}