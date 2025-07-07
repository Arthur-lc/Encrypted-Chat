CXX = g++
CXXFLAGS = -Wall -std=c++11
LDFLAGS = -Wl,-Bstatic -lncurses -ltinfo -Wl,-Bdynamic

BUILD_DIR = build

SRCS_CLIENT = mainClient.cpp client.cpp UIManager.cpp
OBJS_CLIENT = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRCS_CLIENT))

SRCS_SERVER = mainServer.cpp server.cpp UIManager.cpp
OBJS_SERVER = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRCS_SERVER))

all: build client server

build:
	@mkdir -p $(BUILD_DIR)

client: $(OBJS_CLIENT)
	$(CXX) $(OBJS_CLIENT) -o $(BUILD_DIR)/client $(LDFLAGS)

server: $(OBJS_SERVER)
	$(CXX) $(OBJS_SERVER) -o $(BUILD_DIR)/server $(LDFLAGS)

# Rule to compile source files into object files within the build directory
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: all build clean

clean:
	rm -rf $(BUILD_DIR) *.o # Removed *.o from here, as objects are now in build/