# Nome do compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Nome do arquivo de saída (executável)
TARGET = teste

# Nome do arquivo fonte principal
SRC = src/teste.cpp

# Alvo padrão: compila o programa
all: $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Alvo para limpar os arquivos gerados
clean:
	@echo "🧹 Limpando arquivos antigos..."
	@rm -f $(TARGET)

# Alvo para executar o programa
run:
	@echo "🚀 Executando o programa..."
	@./$(TARGET)

# O alvo que você pediu: limpa, compila e executa
teste: clean all run

# Declara alvos que não são nomes de arquivos
.PHONY: all clean run teste