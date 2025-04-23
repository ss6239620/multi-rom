CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LDFLAGS = -L/usr/lib/mysql -lmysqlclient

INC_DIR = include
SRC_DIR = .
BUILD_DIR = build
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

TARGET = $(BIN_DIR)/orm_demo

MYSQL_INCLUDE = /usr/include/mysql
INCLUDES = -I$(INC_DIR) -I$(MYSQL_INCLUDE)

all: $(TARGET)

$(TARGET): $(OBJS) build/MySQLAdapter.o build/MySQLQueryBuilder.o
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/MySQLAdapter.o: $(INC_DIR)/orm/MySQLAdapter.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/MySQLQueryBuilder.o: $(INC_DIR)/orm/MySQLQueryBuilder.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run: all
	@$(TARGET)

.PHONY: all clean run