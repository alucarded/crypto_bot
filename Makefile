SOURCES=src/collector.cc \
				src/consumer/mongo_ticker_consumer.hpp \
				src/consumer/ticker_consumer.h \
				src/db/mongo_client.hpp \
				src/serialization_utils.hpp \
				src/ticker.h \
				src/websocket/bitstamp_ticker_client.hpp \
				src/websocket/kraken_ticker_client.hpp \
				src/websocket/ticker_client.hpp
INCLUDES=-Isrc/ -I.
CFLAGS=--std=c++17 -g -Wall -Wextra $(shell pkg-config --cflags libmongocxx)
LDFLAGS=-lpthread -latomic -lboost_system -lcrypto -lssl -L/usr/local/lib  $(shell pkg-config --libs libmongocxx)

collector:
	g++ $(SOURCES) -o collector $(INCLUDES) $(CFLAGS) $(LDFLAGS)