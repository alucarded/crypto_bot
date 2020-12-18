SOURCES=src/collector.cc src/mongo_ticker_collector.hpp src/ticker_client.hpp src/ticker.h src/bitstamp_ticker_client.hpp
CFLAGS=--std=c++14 -g -Wall -Wextra $(shell pkg-config --cflags libmongocxx)
LDFLAGS=-lpthread -latomic -lboost_system -lcrypto -lssl  $(shell pkg-config --libs libmongocxx)

collector:
	g++ $(SOURCES) -o collector $(CFLAGS) $(LDFLAGS)