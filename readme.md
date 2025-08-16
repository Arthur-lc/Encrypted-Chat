# Chat Criptografado


## Pré-requisitos

Antes de compilar o projeto, certifique-se de ter as seguintes dependências instaladas:

```sh
sudo apt-get update
sudo apt-get install build-essential libncurses5-dev libssl-dev
```

## Como Compilar

Para compilar o projeto, basta executar o comando `make` na raiz do diretório:

```sh
make
```

Isso irá compilar tanto o cliente quanto o servidor e criar os executáveis nos diretórios `client/build/` e `server/build/`.

Ambos podem ser compilados independentemente com:
```sh
cd client && make
```
ou
```sh
cd server && make
```

## Como Executar

### Servidor

Para iniciar o servidor, execute o seguinte comando:

```sh
./server/server
```

O servidor começará a escutar por conexões na porta 8080.

### Cliente

Para iniciar o cliente, execute o seguinte comando:

```sh
./client/client
```

## Tecnologias Utilizadas

*   **Linguagem:** C++
*   **Interface do Cliente:** Biblioteca `ncurses`
*   **Comunicação:** Sockets TCP
*   **Build:** Make