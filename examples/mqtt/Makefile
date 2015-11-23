LDFLAGS=-g -lpthread -lcurl -lrt -lssl -lcrypto -lz -ljson -lmosquitto -lmosquittopp 

SLACK_LIB = ../../lib/libslackrtm.a
INC = -I../../include/ -I../../libwebsockets/build/lib/Headers -I../../libwebsockets/lib -I../../libwebsockets/build

all: SlackMqtt

main.o: main.cpp $(SLACK_LIB)
	g++ $(INC) -Wall -c main.cpp -o main.o

SlackMqtt: main.o $(SLACK_LIB)
	g++ $(LDFLAGS) -o SlackMqtt main.o $(SLACK_LIB)

clean:
	rm -f *.o
	rm -f SlackMqtt
