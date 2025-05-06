CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LDFLAGS = -L/usr/lib/mysql -lmysqlclient -lcrypto

INC_DIR = include
SRC_DIR = .
BUILD_DIR = build
BIN_DIR = bin

MYSQL_INCLUDE = /usr/include/mysql
ORM_DIR = $(INC_DIR)/orm
MYSQL_SRC_DIR = $(ORM_DIR)/MY_SQL
UTILS_DIR = $(INC_DIR)/orm/utils
MIGRATION_DIR = $(INC_DIR)/orm/Migration
SERIALIZER_SRC_DIR = $(INC_DIR)/serializer

UTILS_SRCS = $(wildcard $(UTILS_DIR)/*.cpp)
UTILS_OBJS = $(patsubst $(UTILS_DIR)/%.cpp,$(BUILD_DIR)/utils/%.o,$(UTILS_SRCS))

MIGRATION_SRCS = $(wildcard $(MIGRATION_DIR)/*.cpp)
MIGRATION_OBJS = $(patsubst $(MIGRATION_DIR)/%.cpp,$(BUILD_DIR)/Migrations/%.o,$(MIGRATION_SRCS))

INCLUDES = -I$(INC_DIR) -I$(MYSQL_INCLUDE) -I$(ORM_DIR) -I$(MIGRATION_DIR) -I$(MYSQL_SRC_DIR) -I$(UTILS_DIR) -I$(SERIALIZER_SRC_DIR)

# Source files
MAIN_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
MYSQL_SRCS = $(wildcard $(MYSQL_SRC_DIR)/*.cpp)
SERIALIZER_SRCS = $(wildcard $(SERIALIZER_SRC_DIR)/*.cpp)

# Object files
MAIN_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(MAIN_SRCS))
MYSQL_OBJS = $(patsubst $(MYSQL_SRC_DIR)/%.cpp,$(BUILD_DIR)/orm/MY_SQL/%.o,$(MYSQL_SRCS))
SERIALIZER_OBJS = $(patsubst $(SERIALIZER_SRC_DIR)/%.cpp,$(BUILD_DIR)/serializer/%.o,$(SERIALIZER_SRCS))

OBJS = $(MAIN_OBJS) $(MYSQL_OBJS) $(SERIALIZER_OBJS) $(UTILS_OBJS) $(MIGRATION_OBJS)

TARGET = $(BIN_DIR)/orm_demo

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Compile main source
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile MySQL ORM source
$(BUILD_DIR)/orm/MY_SQL/%.o: $(MYSQL_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile serializer source
$(BUILD_DIR)/serializer/%.o: $(SERIALIZER_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile utils source
$(BUILD_DIR)/utils/%.o: $(UTILS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile migrations source
$(BUILD_DIR)/Migrations/%.o: $(MIGRATION_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run: all
	@$(TARGET)

.PHONY: all clean run
