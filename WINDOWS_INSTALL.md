# Instalação no Windows

## 🚀 Opções de Instalação

### Opção 1: WSL (Windows Subsystem for Linux) - RECOMENDADO
```bash
# Instalar WSL
wsl --install

# Reiniciar o computador e abrir WSL
# Instalar dependências
sudo apt update
sudo apt install build-essential libssl-dev libncurses5-dev libtinfo5

# Compilar e executar
make
```

### Opção 2: MinGW + OpenSSL
1. Instalar MinGW-w64
2. Instalar OpenSSL para Windows
3. Configurar variáveis de ambiente
4. Compilar com:
```bash
g++ -o client.exe *.cpp -lssl -lcrypto -lncurses
```

### Opção 3: Visual Studio + vcpkg
```bash
# Instalar vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# Instalar dependências
./vcpkg install openssl:x64-windows
./vcpkg install ncurses:x64-windows

# Integrar com Visual Studio
./vcpkg integrate install
```

## 🔧 Configuração do Ambiente

### Variáveis de Ambiente
```
OPENSSL_ROOT_DIR=C:\OpenSSL-Win64
PATH=%PATH%;C:\OpenSSL-Win64\bin
```

### Compilação Manual
```bash
# Cliente
g++ -I. -o client.exe client/*.cpp -lssl -lcrypto -lncurses

# Servidor  
g++ -I. -o server.exe server/*.cpp -lssl -lcrypto
```

## 📝 Notas Importantes

- **ncurses**: Pode não funcionar bem no Windows nativo
- **OpenSSL**: Use versões pré-compiladas para Windows
- **WSL**: Melhor opção para desenvolvimento Linux no Windows
- **Portas**: Verifique se as portas não estão bloqueadas pelo firewall

## 🚨 Solução de Problemas

### Erro de Compilação OpenSSL
```bash
# Verificar se o OpenSSL está no PATH
openssl version

# Se não estiver, adicionar ao PATH
set PATH=%PATH%;C:\OpenSSL-Win64\bin
```

### Erro de ncurses
- Use WSL para interface ncurses
- Ou compile sem interface gráfica (modo console simples)

### Erro de Porta
- Verificar firewall do Windows
- Usar portas acima de 1024
- Executar como administrador se necessário 