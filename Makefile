CFLAGS=--std=c++17 -g -O3 -Wall -Wextra -Isrc/ -I. $(shell pkg-config --cflags libmongocxx) $(shell pkg-config --cflags zlib) -Iboost/boost_1_75_0/
LDFLAGS=-DBOOST_LOG_DYN_LINK -Lboost/boost_1_75_0/build/lib -lboost_filesystem -lboost_thread -lboost_regex -lboost_log_setup -lboost_log -lpthread -latomic -lboost_system -lboost_iostreams -lcrypto -lssl -L/usr/local/lib  $(shell pkg-config --libs libmongocxx) $(shell pkg-config --libs zlib)
TEST_FLAGS=-Ithird_party/googletest/googletest/include/ -Ithird_party/googletest/googlemock/include/ third_party/googletest/build/lib/libgtest.a third_party/googletest/build/lib/libgtest_main.a third_party/googletest/build/lib/libgmock.a -lpthread

collector:
	g++ -pipe src/collector.cc -o collector $(CFLAGS) $(LDFLAGS)

basic_backtest:
	g++ -pipe src/basic_strategy_backtest.cc src/db/mongo_client.hpp src/producer/mongo_ticker_producer.hpp -o basic_backtest $(CFLAGS) $(LDFLAGS)

arbitrage_backtest:
	g++ -pipe src/arbitrage_strategy_backtest.cc src/db/mongo_client.hpp src/producer/mongo_ticker_producer.hpp -o arbitrage_backtest $(CFLAGS) $(LDFLAGS)

unit_tests:
	g++ -pipe src/strategy/multi_arbitrage/arbitrage_strategy_matcher_unittest.cc -o arbitrage_strategy_matcher_unittest $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	#g++ -pipe src/strategy/multi_arbitrage/arbitrage_strategy_unittest.cc -o arbitrage_strategy_unittest $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	g++ -pipe src/order_book_unittest.cc -o order_book_unittest $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

integration_tests:
	g++ -pipe src/http/binance_client_test.cc -o binance_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	g++ -pipe src/http/kraken_client_test.cc -o kraken_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

arbitrage_main:
	g++ -pipe src/arbitrage_strategy_main.cc -o arbitrage_main $(CFLAGS) $(LDFLAGS)

arbitrage_finder:
	g++ -pipe src/arbitrage_finder_main.cc -o arbitrage_finder_main $(CFLAGS) $(LDFLAGS)

clean:
	if [ -f collector ]; then rm collector; fi; \
	if [ -f basic_backtest ]; then rm basic_backtest; fi; \
	if [ -f arbitrage_backtest ]; then rm arbitrage_backtest; fi;