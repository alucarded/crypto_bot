SOURCES=src/collector.cc src/serialization_utils.hpp src/ticker_consumer.h src/mongo_ticker_consumer.hpp src/ticker_client.hpp src/ticker.h src/bitstamp_ticker_client.hpp
CFLAGS=--std=c++17 -g -Wall -Wextra $(shell pkg-config --cflags libmongocxx)
LDFLAGS=-lpthread -latomic -lboost_system -lcrypto -lssl -L/usr/local/lib  $(shell pkg-config --libs libmongocxx)

collector:
	g++ $(SOURCES) -o collector $(CFLAGS) $(LDFLAGS)