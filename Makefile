SOURCES=src/collector.cc \
				src/consumer/mongo_ticker_consumer.hpp \
				src/consumer/consumer.h \
				src/db/mongo_client.hpp \
				src/serialization_utils.hpp \
				src/ticker.h \
				src/websocket/binance_ticker_client.hpp \
				src/websocket/bitbay_ticker_client.hpp \
				src/websocket/bitstamp_ticker_client.hpp \
				src/websocket/bybit_ticker_client.hpp \
				src/websocket/coinbase_ticker_client.hpp \
				src/websocket/ftx_ticker_client.hpp \
				src/websocket/huobi_global_ticker_client.hpp \
				src/websocket/kraken_ticker_client.hpp \
				src/websocket/okex_ticker_client.hpp \
				src/websocket/poloniex_ticker_client.hpp \
				src/websocket/ticker_client.hpp
CFLAGS=--std=c++17 -g -Wall -Wextra -Isrc/ -I. $(shell pkg-config --cflags libmongocxx) $(shell pkg-config --cflags zlib) -Iboost/boost_1_75_0/
LDFLAGS=-DBOOST_LOG_DYN_LINK -Lboost/boost_1_75_0/build/lib -lboost_filesystem -lboost_thread -lboost_regex -lboost_log_setup -lboost_log -lpthread -latomic -lboost_system -lboost_iostreams -lcrypto -lssl -L/usr/local/lib  $(shell pkg-config --libs libmongocxx) $(shell pkg-config --libs zlib)
TEST_FLAGS=-Ithird_party/googletest/googletest/include/ -Ithird_party/googletest/googlemock/include/ third_party/googletest/build/lib/libgtest.a third_party/googletest/build/lib/libgtest_main.a third_party/googletest/build/lib/libgmock.a -lpthread

collector:
	g++ -pipe $(SOURCES) -o collector $(CFLAGS) $(LDFLAGS)

basic_backtest:
	g++ -pipe src/basic_strategy_backtest.cc src/db/mongo_client.hpp src/producer/mongo_ticker_producer.hpp -o basic_backtest $(CFLAGS) $(LDFLAGS)

arbitrage_backtest:
	g++ -pipe src/arbitrage_strategy_backtest.cc src/db/mongo_client.hpp src/producer/mongo_ticker_producer.hpp -o arbitrage_backtest $(CFLAGS) $(LDFLAGS)

tests:
	g++ -pipe src/strategy/multi_arbitrage/arbitrage_strategy_matcher_unittest.cc -o arbitrage_strategy_matcher_unittest $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	g++ -pipe src/strategy/multi_arbitrage/arbitrage_strategy_unittest.cc -o arbitrage_strategy_unittest $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

binapi_test:
	g++ -pipe src/http/binapi_test.cc src/http/binapi/* -o binapi_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

# DBOOST_LOG_DYN_LINK 
kraken_test:
	g++ -pipe src/client/kraken_client_test.cc -o kraken_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

arbitrage_main:
	g++ -pipe src/arbitrage_strategy_main.cc src/client/binapi/* -o arbitrage_main $(CFLAGS) $(LDFLAGS)

clean:
	if [ -f collector ]; then rm collector; fi; \
	if [ -f basic_backtest ]; then rm basic_backtest; fi; \
	if [ -f arbitrage_backtest ]; then rm arbitrage_backtest; fi;