#pragma once
#include "json/json.hpp"

#include <iostream>
#include <list>
#include <stdio.h>
#include <string>
#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <zlib.h>

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

void *myalloc(void *q, unsigned n, unsigned m) {
    (void)q;
    return calloc(n, m);
}

void myfree(void *q, void *p) {
    (void)q;
    free(p);
}

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        return err; \
    } \
}

int zlib_inflate(Byte *compr, Byte *uncompr,
    uLong comprLen, uLong uncomprLen)
{
    int err;
    z_stream d_stream; /* decompression stream */

    //strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = myalloc;
    d_stream.zfree = myfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 0;
    d_stream.next_out = uncompr;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "inflate");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    // if (strcmp((char*)uncompr, hello)) {
    //     fprintf(stderr, "bad inflate\n");
    //     exit(1);
    // } else {
    //     printf("inflate(): %s\n", (char *)uncompr);
    // }
    return Z_OK;
}

std::string zlib_inflate(const std::string& compressed) {
    Byte uncomp[2048];
    zlib_inflate((Byte*) compressed.data(), uncomp, compressed.size(), 2048);
    return std::string((char*)uncomp);
}

template <typename T>
std::string to_string(const T val, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << val;
    return out.str();
}

}