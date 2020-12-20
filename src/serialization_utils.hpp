#pragma once
#include "json/json.hpp"

#include <iostream>
#include <list>
#include <string>
#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

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

// TODO: perhaps add more lightweight implementation later
std::string gzip_decompress(const std::string& data)
	{
		namespace bio = boost::iostreams;

		std::stringstream compressed(data);
		std::stringstream decompressed;

		bio::filtering_streambuf<bio::input> out;
		out.push(bio::gzip_decompressor());
		out.push(compressed);
		bio::copy(out, decompressed);

		return decompressed.str();
	}

}