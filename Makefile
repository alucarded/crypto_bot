collector:
	g++ src/collector.cc src/ticker_client.hpp src/ticker.h src/bitstamp_ticker_client.hpp -o collector --std=c++14 -lpthread -lboost_system -lcrypto -lssl