@echo off
echo === Teste do Chat Criptografado com Diffie-Hellman ===
echo.

REM Verificar se o OpenSSL está instalado (Windows)
where openssl >nul 2>nul
if %errorlevel% neq 0 (
    echo ❌ OpenSSL não encontrado!
    echo Instale o OpenSSL para Windows ou use WSL/Linux
    echo.
    pause
    exit /b 1
)

echo ✅ OpenSSL encontrado
echo.

REM Compilar o projeto
echo 🔨 Compilando o projeto...
make clean
make

if %errorlevel% neq 0 (
    echo ❌ Erro na compilação!
    pause
    exit /b 1
)

echo ✅ Compilação concluída
echo.

REM Verificar se os executáveis foram criados
if not exist "server\server.exe" (
    echo ❌ Executável do servidor não encontrado!
    pause
    exit /b 1
)

if not exist "client\client.exe" (
    echo ❌ Executável do cliente não encontrado!
    pause
    exit /b 1
)

echo 🚀 Executáveis criados com sucesso!
echo.
echo === Como usar ===
echo.
echo 1. Iniciar o servidor:
echo    cd server ^&^& server.exe [porta]
echo.
echo 2. Conectar clientes (em terminais separados):
echo    cd client ^&^& client.exe [IP_SERVIDOR] [porta]
echo.
echo 3. Exemplo:
echo    Terminal 1: cd server ^&^& server.exe 8080
echo    Terminal 2: cd client ^&^& client.exe 127.0.0.1 8080
echo    Terminal 3: cd client ^&^& client.exe 127.0.0.1 8080
echo.
echo === Funcionamento ===
echo • Cada cliente gera chaves DH automaticamente
echo • As chaves públicas são trocadas via servidor
echo • Uma chave de grupo é estabelecida
echo • Todas as mensagens são criptografadas
echo • O servidor nunca vê o conteúdo das mensagens
echo.
echo ✅ Sistema pronto para uso!
pause 