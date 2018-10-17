SRC_DIR := ./src
OBJ_DIR := ./obj
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
LDFLAGS := 
CXXFLAGS := -Wall

quokka: $(OBJ_FILES)
	   g++ -O1 -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	   g++ -O1 $(CXXFLAGS) -c $< -o $@

clean:
	rm -f obj/*.o