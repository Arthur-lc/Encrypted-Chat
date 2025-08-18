#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <random>


using ull = unsigned long long;

/**
 * @brief Fornece todas as utilidades criptográficas necessárias para o protocolo
 * de troca de chaves Diffie-Hellman em grupo (protocolo de Burmester-Desmedt).
 */
namespace CryptoUtils {

    // ========================================================================
    // 1. PUBLIC CONSTANTS AND DATA STRUCTURES
    // ========================================================================

    const int P_MODULUS = 23;
    const int G_GENERATOR = 5;

    /**
     * @brief Representa um único membro do grupo, contendo apenas informações públicas.
     */
    struct GroupMember {
        std::string id; // Unique identifier (e.g., username)
        ull publicKey;
    };

    // ========================================================================
    // 2. FUNÇÕES CRIPTOGRÁFICAS CENTRAIS (API Pública)
    // ========================================================================

    /**
     * @brief Executa exponenciação modular (base^exponent) % modulus.
     * @return O resultado de (base^exponent) % modulus.
     */
    ull modularExponent(ull base, ull exponent, int modulus) {
        ull result = 1;
        base %= modulus;
        while (exponent > 0) {
            if (exponent % 2 == 1) {
                result = (result * base) % modulus;
            }
            exponent /= 2;
            base = (base * base) % modulus;
        }
        return result;
    }

    /**
     * @brief Calcula o modular multiplicative inverse de n módulo mod.
     * Necessário para divisão modular. Assume que mod é um número primo.
     * @param n O número para encontrar o inverso.
     * @param mod O módulo.
     * @return O inverso modular.
     */
    ull modInverse(int n, int mod) {
        return modularExponent(n, mod - 2, mod);
    }
    
    /**
     * @brief Gera uma chave privada aleatória criptograficamente segura.
     * @return Uma chave privada aleatória dentro do intervalo válido [2, P_MODULUS - 1].
     */
    int generatePrivateKey() {
        // Use the C++ standard library for cryptographically secure random number generation.
        // std::random_device provides a non-deterministic source of randomness (if available).
        static std::random_device rd;
        
        // Seed the Mersenne Twister engine. Use mt19937_64 for a 64-bit generator.
        static std::mt19937_64 generator(rd());
        
        // Create a distribution that produces integers uniformly in the desired range [2, P_MODULUS - 1].
        std::uniform_int_distribution<int> distribution(2, P_MODULUS - 1);
        
        return distribution(generator);
    }


    ull generatePublicKey(int privateKey) {
        return modularExponent(G_GENERATOR, privateKey, P_MODULUS);
    }

    /**
     * @brief Calcula o valor intermediário 'X' para um cliente.
     * Este é o principal cálculo da segunda rodada do protocolo.
     * @param myPrivateKey A chave privada do cliente que realiza o cálculo.
     * @param before O membro do grupo logicamente anterior a este cliente.
     * @param after O membro do grupo logicamente posterior a este cliente.
     * @return O valor intermediário 'X' calculado.
     */
    ull calculateIntermediateValue(int myPrivateKey, const GroupMember& before, const GroupMember& after) {
        ull z_after = after.publicKey;
        ull z_before = before.publicKey;
        ull z_before_inv = modInverse(z_before, P_MODULUS);

        ull term = (z_after * z_before_inv) % P_MODULUS;
        return modularExponent(term, myPrivateKey, P_MODULUS);
    }

    /**
     * @brief Calcula o SharedSecret final 'K' para um cliente.
     * Este é o passo final do protocolo, realizado localmente por cada cliente.
     * @param myPrivateKey A chave privada do cliente que realiza o cálculo.
     * @param myIndex O índice deste cliente na lista ordenada do grupo.
     * @param orderedMembers A lista completa e ordenada de todos os membros do grupo.
     * @param intermediateValues A lista completa de todos os valores 'X' de todos os membros.
     * @return O segredo compartilhado final 'K'.
     */
    ull calculateSharedSecret(int myPrivateKey, int myIndex, const std::vector<GroupMember>& orderedMembers, const std::vector<ull>& intermediateValues) {
        const size_t N = orderedMembers.size();
        if (N == 0) return 0;

        const GroupMember& before = orderedMembers[(myIndex - 1 + N) % N];

        ull exponent_first_term = (ull)N * myPrivateKey;
        ull final_key = modularExponent(before.publicKey, exponent_first_term, P_MODULUS);

        for (size_t j = 0; j < N - 1; ++j) {
            int x_index = (myIndex + j) % N;
            ull x_value = intermediateValues[x_index];
            int exponent_for_x = N - 1 - j;
            
            ull product_term = modularExponent(x_value, exponent_for_x, P_MODULUS);

            final_key = (final_key * product_term) % P_MODULUS;
        }
        
        return final_key;
    }

} // namespace CryptoUtils


// ============================================================================
// INTERNAL SIMULATION CLASS (Not part of the public API)
// Represents the state that a real client application would maintain.
// ============================================================================
class Client {
private:
    int privateKey;

public:
    std::string id;
    ull publicKey;

    Client(std::string clientId) : id(clientId) {
        // Generate a private key using the utility function.
        privateKey = CryptoUtils::generatePrivateKey();
        // Generate the corresponding public key.
        publicKey = CryptoUtils::generatePublicKey(privateKey);
    }

    int getPrivateKey() const { return privateKey; }
};


// ============================================================================
// EXAMPLE USAGE
// The main function demonstrates how another team would use the CryptoUtils
// namespace to implement the key exchange flow.
// ============================================================================
int main() {
    srand(time(0));

    // --- 1. SETUP (Server-side) ---
    // The number of clients is defined for the simulation.
    const int NUM_CLIENTS = 5;
    std::cout << "Initializing Group Key Exchange for " << NUM_CLIENTS << " clients." << std::endl;
    std::cout << "Public Parameters: P=" << CryptoUtils::P_MODULUS 
              << ", G=" << CryptoUtils::G_GENERATOR << std::endl;
    
    // The server simulates clients connecting and generates their state.
    std::vector<Client> clients;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        clients.emplace_back("user" + std::to_string(i));
    }

    // The server creates the ordered list of public member data.
    // This list would be broadcast to all clients.
    std::vector<CryptoUtils::GroupMember> orderedMembers;
    for (const auto& client : clients) {
        orderedMembers.push_back({client.id, client.publicKey});
    }


    // --- 2. ROUND 2: CALCULATE INTERMEDIATE VALUES (Client-side simulation) ---
    std::cout << "\n--- Round 2: Clients calculate their intermediate 'X' values ---" << std::endl;
    std::vector<ull> intermediateValues;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        const auto& current_client = clients[i];
        const auto& before_member = orderedMembers[(i - 1 + NUM_CLIENTS) % NUM_CLIENTS];
        const auto& after_member = orderedMembers[(i + 1) % NUM_CLIENTS];

        ull x = CryptoUtils::calculateIntermediateValue(current_client.getPrivateKey(), before_member, after_member);
        intermediateValues.push_back(x);
        std::cout << "Client '" << current_client.id << "' Pk:  " << current_client.getPrivateKey() << "\t calculated X = " << x << std::endl;
    }
    // In a real app, clients would send their 'X' values to the server,
    // and the server would broadcast the complete list.


    // --- 3. FINAL STEP: CALCULATE SHARED SECRET (Client-side simulation) ---
    std::cout << "\n--- Final Step: Clients calculate the final shared secret 'K' ---" << std::endl;
    std::vector<ull> finalSecrets;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        const auto& current_client = clients[i];
        
        ull k = CryptoUtils::calculateSharedSecret(current_client.getPrivateKey(), i, orderedMembers, intermediateValues);
        finalSecrets.push_back(k);
        std::cout << "Client '" << current_client.id << "' calculated K = " << k << std::endl;
    }


    // --- 4. VERIFICATION (Server-side or for testing) ---
    std::cout << "\n--- Verification ---" << std::endl;
    bool allMatch = true;
    ull firstSecret = finalSecrets.empty() ? 0 : finalSecrets[0];
    for (size_t i = 1; i < finalSecrets.size(); ++i) {
        if (finalSecrets[i] != firstSecret) {
            allMatch = false;
            break;
        }
    }

    if (allMatch) {
        std::cout << "SUCCESS! All clients computed the same shared secret: " << firstSecret << std::endl;
    } else {
        std::cout << "FAILURE! Clients computed different shared secrets." << std::endl;
    }

    return 0;
}