LDFLAGS=-g -lwebsockets -lpthread -lcurl -lrt -lssl -lcrypto -lz -ljson-c -lmosquitto -lmosquittopp 

SLACK_LIB = ../../lib/libslackrtm.a
INC = -I../../include/

all: SlackMqtt

main.o: main.cpp $(SLACK_LIB)
	g++ $(INC) -Wall -c main.cpp -o main.o

SlackMqtt: main.o $(SLACK_LIB)
	g++ $(LDFLAGS) -o SlackMqtt main.o $(SLACK_LIB)

clean:
	rm -f *.o
	rm -f SlackMqtt
