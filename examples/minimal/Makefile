LDFLAGS=-g -lpthread -lcurl

SLACK_LIB = ../../lib/libslackrtm.a
INC = -I../../include/ 

all: SlackMinimal

main.o: main.cpp $(SLACK_LIB)
	g++ $(INC) -Wall -c main.cpp -o main.o

SlackMinimal: main.o $(SLACK_LIB)
	g++ $(LDFLAGS) -lwebsockets -lrt -lpthread -lssl -lcrypto -lz -ljson-c -o SlackMinimal main.o $(SLACK_LIB)

clean:
	rm -f *.o
	rm -f SlackMinimal
