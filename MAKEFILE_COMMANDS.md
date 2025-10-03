# 📋 Comandos do Makefile - SO-SimuladorVonNeumann

## 🎯 **Comandos Disponíveis**

### **Comandos Básicos**
- `make` ou `make all` - Compila e executa o programa principal
- `make clean` - Remove arquivos gerados (.o, executáveis)
- `make run` - Executa programa principal (sem recompilar)

### **Comandos de Teste**
- `make test-hash` - Compila e testa sistema de registradores MIPS
- `make test-bank` - Compila e testa sistema de banco registradores
- `make test-all` - Executa todos os testes disponíveis
- `make check` - Verificação rápida (✅ PASSOU/❌ FALHOU)

### **Comandos de Build**
- `make teste` - Compila apenas o programa principal
- `make test_hash_register` - Compila apenas o teste do hash register
- `make debug` - Build com símbolos de debug (-DDEBUG -O0 -ggdb3)

### **Comandos de Informação**
- `make help` - Mostra todos os comandos com descrições
- `make list-files` - Lista arquivos do projeto (fontes, headers)

## � **Exemplos de Uso**

### **Desenvolvimento Diário**
```bash
make help          # Ver comandos disponíveis
make               # Compilar e testar ULA
make test-hash     # Testar registradores MIPS
make check         # Verificação rápida
```

### **Debug de Problemas**
```bash
make debug         # Build com símbolos
gdb ./teste        # Debugger
```

### **Informações do Projeto**
```bash
make list-files    # Ver estrutura
make help          # Ver todos os comandos
```

## 📊 **Tabela de Comandos**

| Comando | Função | Uso |
|---------|--------|-----|
| `make help` | Lista comandos | Primeiro uso |
| `make` | Compila e executa | Desenvolvimento |
| `make test-hash` | Testa registradores | Validar MIPS |
| `make test-bank` | Testa banco registradores | Validar MIPS |
| `make check` | Verificação rápida | Testes automáticos |
| `make debug` | Build debug | Debugging |
| `make clean` | Limpa arquivos | Rebuild |

---
**Total: 10 comandos implementados e funcionando** ✅
