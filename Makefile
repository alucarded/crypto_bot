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
BACKTEST_SRC=src/backtest/backtest_results_processor.cc \
			 src/db/mongo_client.hpp \
			 src/db/mongo_ticker_producer.hpp
# ARBITRAGE_FLAGS=-DWITH_MEAN_REVERSION_SIGNAL
MARKET_MAKING_SRC=src/strategy/market_making/market_making_strategy.cc \
			 src/strategy/market_making/market_making_risk_manager.cc \
       src/strategy/market_making/market_making_signal.cc \
			 src/strategy/indicator/order_book_imbalance.cc

# TODO: One configurable collector (via file specifying exchanges, assets and data to collect)
.PHONY: arbitrage_collector
arbitrage_collector:
	g++ -pipe $(COMMON_SRC) src/arbitrage_collector.cc -o arbitrage_collector $(CFLAGS) $(LDFLAGS)

.PHONY: market_making_collector
market_making_collector:
	g++ -pipe $(COMMON_SRC) src/market_making_collector.cc -o market_making_collector $(CFLAGS) $(LDFLAGS)

.PHONY: arbitrage_backtest
arbitrage_backtest:
	g++ -pipe $(COMMON_SRC) $(ARBITRAGE_SRC) $(BACKTEST_SRC) src/arbitrage_strategy_backtest.cc -o arbitrage_backtest $(CFLAGS) $(LDFLAGS)

.PHONY: unit_tests
unit_tests:
	g++ -pipe $(COMMON_SRC) $(ARBITRAGE_SRC) $(GTESTS) -o unittest $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

.PHONY: integration_tests
integration_tests:
	g++ -pipe src/http/http_client_test.cc -o http_client_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	g++ -pipe src/http/binance_client_test.cc -o binance_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)
	g++ -pipe src/http/kraken_client_test.cc -o kraken_test $(CFLAGS) $(LDFLAGS) $(TEST_FLAGS)

.PHONY: arbitrage_main
arbitrage_main:
	g++ -pipe $(COMMON_SRC) $(ARBITRAGE_SRC) src/arbitrage_strategy_main.cc -o arbitrage_main $(CFLAGS) $(LDFLAGS)

.PHONY: kraken_order_book_main
kraken_order_book_main:
	g++ -pipe $(COMMON_SRC) src/kraken_order_book_main.cc -o kraken_order_book_main $(CFLAGS) $(LDFLAGS)

.PHONY: binance_order_book_main
binance_order_book_main:
	g++ -pipe $(COMMON_SRC) src/binance_order_book_main.cc -o binance_order_book_main $(CFLAGS) $(LDFLAGS)

.PHONY: market_making_main
market_making_main:
	g++ -pipe $(COMMON_SRC) $(MARKET_MAKING_SRC) src/market_making_main.cc -o market_making_main $(CFLAGS) $(LDFLAGS)

.PHONY: indicators_generator_main
indicators_generator_main:
	g++ -pipe $(COMMON_SRC) src/strategy/indicator/order_book_imbalance.cc src/indicators_generator_main.cc -o indicators_generator_main $(CFLAGS) $(LDFLAGS)

.PHONY: market_making_backtest
market_making_backtest:
	g++ -pipe $(COMMON_SRC) $(MARKET_MAKING_SRC) $(BACKTEST_SRC) src/market_making_backtest.cc -o market_making_backtest $(CFLAGS) $(LDFLAGS)


clean:
	if [ -f collector ]; then rm collector; fi; \
	if [ -f arbitrage_backtest ]; then rm arbitrage_backtest; fi; \
	if [ -f arbitrage_main ]; then rm arbitrage_backtest; fi;