#include "DiffieHellman.h"
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <cstring>

DiffieHellman::DiffieHellman() : dh(nullptr), privateKey(nullptr), publicKey(nullptr), 
                                  sharedSecret(nullptr), keyEstablished(false) {
    generateKeys();
}

DiffieHellman::~DiffieHellman() {
    if (dh) DH_free(dh);
    if (privateKey) BN_free(privateKey);
    if (publicKey) BN_free(publicKey);
    if (sharedSecret) BN_free(sharedSecret);
}

void DiffieHellman::generateKeys() {
    // Gerar parâmetros DH (p, g)
    dh = DH_new();
    if (!dh) return;

    // Usar parâmetros padrão seguros
    if (DH_generate_parameters_ex(dh, 2048, DH_GENERATOR_2, nullptr) != 1) {
        DH_free(dh);
        dh = nullptr;
        return;
    }

    // Gerar par de chaves
    if (DH_generate_key(dh) != 1) {
        DH_free(dh);
        dh = nullptr;
        return;
    }

    privateKey = BN_dup(DH_get0_priv_key(dh));
    publicKey = BN_dup(DH_get0_pub_key(dh));
}

std::string DiffieHellman::getPublicKeyString() const {
    if (!publicKey) return "";
    
    char* hex = BN_bn2hex(publicKey);
    if (!hex) return "";
    
    std::string result(hex);
    OPENSSL_free(hex);
    return result;
}

void DiffieHellman::establishGroupKey(const std::vector<std::string>& otherPublicKeys) {
    if (otherPublicKeys.empty()) return;
    
    // Para simplificar, vamos usar a primeira chave pública para estabelecer a chave de grupo
    // Em uma implementação mais robusta, você poderia combinar múltiplas chaves
    const std::string& firstKey = otherPublicKeys[0];
    
    BIGNUM* otherPub = BN_new();
    if (!otherPub) return;
    
    if (BN_hex2bn(&otherPub, firstKey.c_str()) != 1) {
        BN_free(otherPub);
        return;
    }
    
    computeSharedSecret(otherPub);
    BN_free(otherPub);
}

void DiffieHellman::computeSharedSecret(const BIGNUM* otherPublicKey) {
    if (!dh || !otherPublicKey) return;
    
    // Calcular segredo compartilhado
    sharedSecret = BN_new();
    if (!sharedSecret) return;
    
    if (DH_compute_key(sharedSecret, otherPublicKey, dh) == -1) {
        BN_free(sharedSecret);
        sharedSecret = nullptr;
        return;
    }
    
    deriveGroupKey();
}

void DiffieHellman::deriveGroupKey() {
    if (!sharedSecret) return;
    
    // Converter o segredo compartilhado para bytes
    int secretLen = BN_num_bytes(sharedSecret);
    std::vector<unsigned char> secretBytes(secretLen);
    BN_bn2bin(sharedSecret, secretBytes.data());
    
    // Derivar chave de grupo usando SHA-256
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return;
    
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return;
    }
    
    if (EVP_DigestUpdate(ctx, secretBytes.data(), secretBytes.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        return;
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return;
    }
    
    EVP_MD_CTX_free(ctx);
    
    groupKey.assign(hash, hash + hashLen);
    keyEstablished = true;
}

std::string DiffieHellman::encryptMessage(const std::string& message) const {
    if (!keyEstablished || groupKey.empty()) return message;
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return message;
    
    // Usar AES-256-GCM para criptografia
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, groupKey.data(), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return message;
    }
    
    std::vector<unsigned char> encrypted(message.length() + EVP_MAX_BLOCK_LENGTH);
    int len;
    
    if (EVP_EncryptUpdate(ctx, encrypted.data(), &len, 
                          (const unsigned char*)message.c_str(), message.length()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return message;
    }
    
    int finalLen;
    if (EVP_EncryptFinal_ex(ctx, encrypted.data() + len, &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return message;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Converter para base64 para transmissão segura
    std::string result;
    for (int i = 0; i < len + finalLen; i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", encrypted[i]);
        result += hex;
    }
    
    return result;
}

std::string DiffieHellman::decryptMessage(const std::string& encryptedMessage) const {
    if (!keyEstablished || groupKey.empty()) return encryptedMessage;
    
    // Converter de hex para bytes
    std::vector<unsigned char> encrypted;
    for (size_t i = 0; i < encryptedMessage.length(); i += 2) {
        if (i + 1 < encryptedMessage.length()) {
            std::string byteString = encryptedMessage.substr(i, 2);
            unsigned char byte = (unsigned char)strtol(byteString.c_str(), nullptr, 16);
            encrypted.push_back(byte);
        }
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return encryptedMessage;
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, groupKey.data(), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return encryptedMessage;
    }
    
    std::vector<unsigned char> decrypted(encrypted.size());
    int len;
    
    if (EVP_DecryptUpdate(ctx, decrypted.data(), &len, encrypted.data(), encrypted.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return encryptedMessage;
    }
    
    int finalLen;
    if (EVP_DecryptFinal_ex(ctx, decrypted.data() + len, &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return encryptedMessage;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    
    return std::string((char*)decrypted.data(), len + finalLen);
}

void DiffieHellman::resetKeys() {
    keyEstablished = false;
    groupKey.clear();
    if (sharedSecret) {
        BN_free(sharedSecret);
        sharedSecret = nullptr;
    }
} 