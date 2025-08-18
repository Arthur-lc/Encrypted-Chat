# Protocolo

## Servidor -> Cliente (S2C)
### S2C_INITIATE_KEY_EXCHANGE
Cenário: O servidor detecta uma mudança no grupo e comanda a todos que iniciem uma nova troca de chaves.

```json
{
  "type": "S2C_INITIATE_KEY_EXCHANGE",
  "payload": {
    "groupSize": 3,
    "orderedMembers": [
      {
        "username": "Carol",
        "publicKey": 19
      },
      {
        "username": "Alice",
        "publicKey": 17
      },
      {
        "username": "Bob",
        "publicKey": 10
      }
    ]
  }
}
```

### S2C_DISTRIBUTE_INTERMEDIATE_VALUES
Cenário: O servidor já recebeu os valores X de todos os 3 clientes para a sessão e agora os distribui para o grupo.
```json
{
  "type": "S2C_DISTRIBUTE_INTERMEDIATE_VALUES",
  "payload": {
    "intermediateValues": [
      {
        "username": "Carol",
        "intermediateX": 15
      },
      {
        "username": "Alice",
        "intermediateX": 8
      },
      {
        "username": "Bob",
        "intermediateX": 22
      }
    ]
  }
}
```

### S2C_BROADCAST_GROUP_MESSAGE
Cenário: Bob enviou uma mensagem. O servidor a repassa para Alice e Carol.

```json
{
  "type": "S2C_BROADCAST_GROUP_MESSAGE",
  "payload": {
    "sender": "Bob",
    "ciphertext": "RXN0YSBlIHVtYSBtZW5zYWdlbSBzZWNyZXRhLiBFc3Blcm8gcXVlIGZ1bmNpb25lIQ=="
  }
}
```

### S2C_USER_NOTIFICATION
Cenário: Um novo usuário, "David", acabou de entrar no grupo.
"USER_JOINED" ou "USER_DISCONNECTED"
```json
{
  "type": "S2C_USER_NOTIFICATION",
  "payload": {
    "event": "USER_JOINED",
    "username": "David"
  }
}
```


## Cliente -> Servidor (C2S)

### C2S_AUTHENTICATE_AND_JOIN
Cenário: Alice se conecta ao servidor pela primeira vez.
```json
{
  "type": "C2S_AUTHENTICATE_AND_JOIN",
  "payload": {
    "username": "Alice",
    "publicKey": 17
  }
}
```

### C2S_SEND_INTERMEDIATE_VALUE
Cenário: Alice responde à solicitação de troca de chaves com seu valor X calculado.

```json
{
  "type": "C2S_SEND_INTERMEDIATE_VALUE",
  "payload": {
    "intermediateX": 8
  }
}
```

### C2S_SEND_GROUP_MESSAGE
Cenário: Carol, após a troca de chaves ser concluída, envia uma mensagem para o grupo.

```json
{
  "type": "C2S_SEND_GROUP_MESSAGE",
  "payload": {
    "ciphertext": "T2laLCBwZXNzb2FsISBUZXN0YW5kbyBhIG5vdmEgY2hhdmUu"
  }
}
```