# Chat Criptografado

A comunicação é feita utilizando jsons.  
Leia [**protocolo.md**](https://github.com/Arthur-lc/Encrypted-Chat/blob/cripto-bros/protocol.md) para entender como funciona o protocolo de comunicação.

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
