CPPFLAGS=-g -pthread 
LDFLAGS=-g -lpthread -lcurl

INC=-I./libwebsockets/build/lib/Headers
LIB=-L./libwebsockets/build/lib



ws_test: ws_test.o
	g++ $(LDFLAGS) $(LIB) -lrt -lpthread -lssl -lcrypto -lz -o ws_test ws_test.o ./libwebsockets/build/lib/libwebsockets.a

ws_test.o: ws_test.cpp 
	g++ $(CPPFLAGS) $(INC) -c ws_test.cpp

CWebSocket: CWebSocket.o CLogging.o
	g++ $(LDFLAGS) $(LIB) -lrt -lpthread -lssl -lcrypto -lz -o CWebSocket CWebSocket.o CLogging.o ./libwebsockets/build/lib/libwebsockets.a

CWebSocket.o: CWebSocket.cpp CWebSocket.h
	g++ $(CPPFLAGS) $(INC) -c CWebSocket.cpp

CSlackWeb.o: CSlackWeb.cpp CSlackWeb.h
	g++ $(CPPFLAGS) $(INC) -c CSlackWeb.cpp


CLogging.o: CLogging.cpp CLogging.h
	g++ $(INC) -Wall -c CLogging.cpp 

rtm_test: main.o 
	g++ $(LDFLAGS) -o rtm_test main.o

rtm_test.o: main.cpp 
	g++ $(CPPFLAGS) -c main.cpp

	
SlackRTM: CWebSocket.o CSlackWeb.o CLogging.o
	g++ $(LDFLAGS) $(LIB) -lrt -lpthread -lssl -lcrypto -lz -o SlackRTM CSlackWeb.o CWebSocket.o CLogging.o ./libwebsockets/build/lib/libwebsockets.a
	

clean:
	rm -f *.o rtm_test ws_test
	