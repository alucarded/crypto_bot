compile:
	g++ src/main.cc src/websocket_client.h src/websocket_client.cc --std=c++14 -lpthread -lboost_system -lcrypto -lssl