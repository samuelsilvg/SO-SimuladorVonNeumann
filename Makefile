# Compilador e flags
CXX := g++
CXXFLAGS := -Wall -Wextra -g -std=c++17 -Isrc

TARGET := teste
SRC := src/teste.cpp src/cpu/ULA.cpp
OBJ := $(SRC:.cpp=.o)

# Make clean -> make -> make run
all: clean $(TARGET) run

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "🧹 Limpando arquivos antigos..."
	@rm -f $(OBJ) $(TARGET)

run:
	@echo "🚀 Executando o programa..."
	@./$(TARGET)

.PHONY: all clean run
