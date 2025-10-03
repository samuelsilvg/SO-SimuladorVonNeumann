# Define o compilador a ser usado
CXX = g++

# Define as flags de compilação. -std=c++17 para recursos modernos, -Wall para avisos, -pthread para threads
CXXFLAGS = -std=c++17 -Wall -pthread

# Define o nome do executável final
TARGET = io

# Define os arquivos de código-fonte (.cpp)
SOURCES = main.cpp IOManager.cpp

# Define os arquivos objeto (.o) que serão gerados a partir dos fontes
OBJECTS = $(SOURCES:.cpp=.o)

# Regra principal: o que fazer quando você digita 'make'
all: $(TARGET)

# Regra para criar o executável final a partir dos arquivos objeto
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Regra genérica para criar um arquivo .o a partir de um arquivo .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regra para limpar os arquivos gerados (executável e objetos)
clean:
	rm -f $(TARGET) $(OBJECTS)