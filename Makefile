SOURCES=src/collector.cc \
				src/consumer/mongo_ticker_consumer.hpp \
				src/consumer/ticker_consumer.h \
				src/db/mongo_client.hpp \
				src/serialization_utils.hpp \
				src/ticker.h \
				src/websocket/binance_ticker_client.hpp \
				src/websocket/bitbay_ticker_client.hpp \
				src/websocket/bitstamp_ticker_client.hpp \
				src/websocket/coinbase_ticker_client.hpp \
				src/websocket/huobi_global_ticker_client.hpp \
				src/websocket/kraken_ticker_client.hpp \
				src/websocket/poloniex_ticker_client.hpp \
				src/websocket/ticker_client.hpp
CFLAGS=--std=c++17 -g -Wall -Wextra -Isrc/ -I. $(shell pkg-config --cflags libmongocxx)
LDFLAGS=-lpthread -latomic -lboost_system -lboost_iostreams -lcrypto -lssl -L/usr/local/lib  $(shell pkg-config --libs libmongocxx)

collector:
	g++ $(SOURCES) -o collector $(CFLAGS) $(LDFLAGS)

clean:
	if [ -f collector ]; then rm collector; fi;