CPPFLAGS=-g -pthread 
LDFLAGS=-g -lpthread -lcurl

INC=-I./libwebsockets/build/lib/Headers
LIB=-L./libwebsockets/build/lib

all: SlackRtmTest

CWebSocket.o: CWebSocket.cpp CWebSocket.h
	g++ $(CPPFLAGS) $(INC) -c CWebSocket.cpp

CSlackWeb.o: CSlackWeb.cpp CSlackWeb.h
	g++ $(CPPFLAGS) $(INC) -c CSlackWeb.cpp
	
CSlackWS.o: CSlackWS.cpp CSlackWS.h
	g++ $(CPPFLAGS) $(INC) -c CSlackWS.cpp

CLogging.o: CLogging.cpp CLogging.h
	g++ $(INC) -Wall -c CLogging.cpp 

CSlackRTM.o: CSlackRTM.cpp CSlackRTM.h
	g++ $(INC) -Wall -c CSlackRTM.cpp

main.o: main.cpp
	g++ $(INC) -Wall -c main.cpp

SlackRtmTest: main.o CSlackRTM.o CWebSocket.o CSlackWS.o CSlackWeb.o CLogging.o
	g++ $(LDFLAGS) $(LIB) -lrt -lpthread -lssl -lcrypto -lz -ljson -o SlackRtmTest main.o CSlackRTM.o CSlackWeb.o CSlackWS.o CWebSocket.o CLogging.o ./libwebsockets/build/lib/libwebsockets.a


clean:
	rm -f *.o SlackRtmTest
