CFLAGS=--std=c++17 -g -Wall -Wextra -Isrc/ -I. $(shell pkg-config --cflags libmongocxx) $(shell pkg-config --cflags zlib) -Iboost/boost_1_75_0/
LDFLAGS=-Lboost/boost_1_75_0/build/lib boost/boost_1_75_0/build/lib/libboost_log.a -lboost_filesystem -lboost_thread -lboost_regex -lboost_log_setup -lpthread -latomic -lboost_system -lboost_iostreams -lcrypto -lssl -L/usr/local/lib  $(shell pkg-config --libs libmongocxx) $(shell pkg-config --libs zlib)
TEST_FLAGS=-Ithird_party/googletest/googletest/include/ -Ithird_party/googletest/googlemock/include/ third_party/googletest/build/lib/libgtest.a third_party/googletest/build/lib/libgtest_main.a third_party/googletest/build/lib/libgmock.a -lpthread

GTESTS=src/backtest/backtest_exchange_client_unittest.cc \
       src/model/order_book_unittest.cc \
       src/exchange/account_manager_unittest.cc \
       src/strategy/arbitrage/arbitrage_strategy_matcher_unittest.cc \
       src/strategy/arbitrage/arbitrage_order_calculator_unittest.cc \
       src/strategy/indicator/simple_moving_average_unittest.cc \
			 src/utils/string_unittest.cc

COMMON_SRC=src/model/order.cc \
       src/model/order_book.cc \
			 src/model/symbol.cc \
			 src/model/ticker.cc \
			 src/model/trade_ticker.cc \
			 src/model/account_balance.cc \
			 src/exchange/account_manager_impl.cc \
			 src/exchange/account_refresher.cc \
			 src/utils/math.cc \
			 src/utils/string.cc
ARBITRAGE_SRC=src/strategy/arbitrage/arbitrage_order_calculator.cc \
			 src/strategy/arbitrage/arbitrage_strategy_matcher.cc
ARBITRAGE_BACKTEST_SRC=src/backtest/backtest_results_processor.cc \
			 src/db/mongo_client.hpp \
			 src/db/mongo_ticker_producer.hpp \
       src/arbitrage_strategy_backtest.cc
# ARBITRAGE_FLAGS=-DWITH_MEAN_REVERSION_SIGNAL

collector:
	g++ -pipe $(COMMON_SRC) src/collector.cc -o collector $(CFLAGS) $(LDFLAGS)

.PHONY: arbitrage_backtest
arbitrage_backtest:
	g++ -pipe $(COMMON_SRC) $(ARBITRAGE_SRC) $(ARBITRAGE_BACKTEST_SRC) -o arbitrage_backtest $(CFLAGS) $(LDFLAGS)

unit_tests:
	g++ -pipe $(COMMON_SRC) $(ARBITRAGE_SRC) $(GTESTS) -o unittest $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

integration_tests:
	g++ -pipe src/http/http_client_test.cc -o http_client_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	g++ -pipe src/http/binance_client_test.cc -o binance_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	g++ -pipe src/http/kraken_client_test.cc -o kraken_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

.PHONY: arbitrage_main
arbitrage_main:
	g++ -pipe $(COMMON_SRC) $(ARBITRAGE_SRC) src/arbitrage_strategy_main.cc -o arbitrage_main $(CFLAGS) $(LDFLAGS)

kraken_order_book_main:
	g++ -pipe src/kraken_order_book_main.cc -o kraken_order_book_main $(CFLAGS) $(LDFLAGS)

.PHONY: market_making_main
market_making_main:
	g++ -pipe $(COMMON_SRC) src/market_making_main.cc -o market_making_main $(CFLAGS) $(LDFLAGS)

clean:
	if [ -f collector ]; then rm collector; fi; \
	if [ -f arbitrage_backtest ]; then rm arbitrage_backtest; fi; \
	if [ -f arbitrage_main ]; then rm arbitrage_backtest; fi;