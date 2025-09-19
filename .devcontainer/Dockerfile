# Utiliza a imagem base da Microsoft para desenvolvimento C++ no Ubuntu 22.04
# Esta imagem já vem com g++, gdb, CMake, Git e outras ferramentas essenciais.
FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-22.04

# Atualiza a lista de pacotes e instala as ferramentas de build e os fontes do GoogleTest/GoogleMock.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends build-essential libgtest-dev libgmock-dev

# O pacote libgtest-dev do Ubuntu/Debian instala apenas os fontes em /usr/src/googletest.
# É necessário compilá-los manualmente. A abordagem correta é compilar
# e depois copiar as bibliotecas para um local padrão do sistema, pois
# o alvo 'install' não é fornecido pelos arquivos CMake do pacote Debian.
WORKDIR /tmp/build
RUN cmake /usr/src/googletest && \
    make && \
    cp lib/*.a /usr/lib/

# Limpa o diretório de build temporário para manter a imagem limpa
RUN rm -rf /tmp/build

# Define o diretório de trabalho padrão para o seu projeto dentro do container
WORKDIR /workspaces