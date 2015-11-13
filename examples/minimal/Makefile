LDFLAGS=-g -lpthread -lcurl

SLACK_LIB = ../../lib/libslackrtm.a
INC = -I../../include/ -I../../libwebsockets/build/lib/Headers -I../../libwebsockets/lib -I../../libwebsockets/build

all: SlackMinimal

main.o: main.cpp $(SLACK_LIB)
	g++ $(INC) -Wall -c main.cpp -o main.o

SlackMinimal: main.o $(SLACK_LIB)
	g++ $(LDFLAGS) -lrt -lpthread -lssl -lcrypto -lz -ljson -o SlackMinimal main.o $(SLACK_LIB)

clean:
	rm -f *.o
	rm -f SlackMinimal
