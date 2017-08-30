CPPFLAGS=-g -pthread 
LDFLAGS=-g -lpthread -lcurl

INC=-I./include
LIB=

BUILD_DIR = build/
SRC_DIR = cpp/
HEAD_DIR = include/
LIB_DIR = lib/
CC_OUT = -o $(BUILD_DIR)$(notdir $@)

OBJS = $(BUILD_DIR)CSlackRTM.o $(BUILD_DIR)CWebSocket.o $(BUILD_DIR)CSlackWS.o $(BUILD_DIR)CSlackWeb.o

.PHONY: clean examples

all: lib examples

lib: $(LIB_DIR)libslackrtm.a

$(BUILD_DIR)CWebSocket.o: $(SRC_DIR)CWebSocket.cpp $(HEAD_DIR)CWebSocket.h 
	g++ $(CPPFLAGS) $(INC) -c $(SRC_DIR)CWebSocket.cpp  $(CC_OUT)

$(BUILD_DIR)CSlackWeb.o: $(SRC_DIR)CSlackWeb.cpp $(HEAD_DIR)CSlackWeb.h
	g++ $(CPPFLAGS) $(INC) -c $(SRC_DIR)CSlackWeb.cpp  $(CC_OUT)
	
$(BUILD_DIR)CSlackWS.o: $(SRC_DIR)CSlackWS.cpp $(HEAD_DIR)CSlackWS.h 
	g++ $(CPPFLAGS) $(INC) -c $(SRC_DIR)CSlackWS.cpp  $(CC_OUT)

$(BUILD_DIR)CSlackRTM.o: $(SRC_DIR)CSlackRTM.cpp $(HEAD_DIR)CSlackRTM.h
	g++ $(INC) -Wall -c $(SRC_DIR)CSlackRTM.cpp $(CC_OUT)

$(LIB_DIR)libslackrtm.a: $(OBJS)
	ar -cq $(LIB_DIR)libslackrtm.a $(OBJS)

	
# $(LIB_DIR)libslackrtm.a: $(LIB_DIR)slackrtm.a 
#	cp $(LIB_DIR)slackrtm.a  $(LIB_DIR)libslackrtm.a

examples: $(LIB_DIR)libslackrtm.a
	cd examples/minimal ; $(MAKE)
	cd examples/mqtt ; $(MAKE)

install: examples
	cp examples/mqtt/SlackMqtt $(DESTDIR)/usr/bin

clean:
	rm -f build/*.o
	rm -f SlackRtmTest
	rm -f lib/*.a
	cd examples/minimal ; $(MAKE) clean
	cd examples/mqtt ; $(MAKE) clean
