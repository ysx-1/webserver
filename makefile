CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cpp 
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread
#-lpthread -lmysqlclient

clean:
	rm  -r server