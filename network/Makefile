CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor -O2 -pthread

# Add QNX socket library if targeting QNX Neutrino
UNAME_S := $(shell uname -s)
ifeq ($(findstring QNX,$(UNAME_S)),QNX)
    LIBS = -lsocket
else
    LIBS =
endif

LIB_SRCS = message.cpp tcp_server.cpp tcp_client.cpp
LIB_OBJS = $(LIB_SRCS:.cpp=.o)
STATIC_LIB = libaegis_qnx_network.a

ALL_BINS = example_server example_client

all: $(STATIC_LIB) $(ALL_BINS)

$(STATIC_LIB): $(LIB_OBJS)
	ar rcs $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

example_server: example_server.cpp $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) $< -L. -laegis_qnx_network $(LIBS) -o $@

example_client: example_client.cpp $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) $< -L. -laegis_qnx_network $(LIBS) -o $@

clean:
	rm -f *.o $(STATIC_LIB) $(ALL_BINS)

.PHONY: all clean

