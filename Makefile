collector:
	g++ src/collector.cc src/websocket_client.hpp -o collector --std=c++14 -lpthread -lboost_system -lcrypto -lssl