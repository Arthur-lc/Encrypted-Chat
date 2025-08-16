#pragma once

#include <string>
#include <vector>
#include <openssl/dh.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

class DiffieHellman {
private:
    DH* dh;
    BIGNUM* privateKey;
    BIGNUM* publicKey;
    BIGNUM* sharedSecret;
    std::vector<unsigned char> groupKey;
    bool keyEstablished;

    void generateKeys();
    void computeSharedSecret(const BIGNUM* otherPublicKey);
    void deriveGroupKey(const std::vector<unsigned char>& secretBytes);

public:
    DiffieHellman();
    ~DiffieHellman();

    std::string getPublicKeyString() const;
    void establishGroupKey(const std::vector<std::string>& otherPublicKeys);
    
    std::string encryptMessage(const std::string& message) const;
    std::string decryptMessage(const std::string& encryptedMessage) const;
    
    bool isKeyEstablished() const { return keyEstablished; }
    void resetKeys();
}; 