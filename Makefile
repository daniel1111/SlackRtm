CPPFLAGS=-g -pthread 
LDFLAGS=-g -lpthread -lcurl

rtm_test: main.o 
	g++ $(LDFLAGS) -o rtm_test main.o

rtm_test.o: main.cpp 
	g++ $(CPPFLAGS) -c main.cpp

