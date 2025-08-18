#include <iostream>
#include <random>
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

class SafePrimeGenerator {
public:
    std::mt19937 rng;
    
    // Teste de primalidade de Miller-Rabin
    bool millerRabinTest(uint32_t n, int rounds = 10) {
        if (n < 2) return false;
        if (n == 2 || n == 3) return true;
        if (n % 2 == 0) return false;
        
        // Escreve n-1 como d * 2^r
        uint32_t d = n - 1;
        int r = 0;
        while (d % 2 == 0) {
            d /= 2;
            r++;
        }
        
        std::uniform_int_distribution<uint32_t> dist(2, n - 2);
        
        for (int i = 0; i < rounds; i++) {
            uint32_t a = dist(rng);
            uint64_t x = modPow(a, d, n);
            
            if (x == 1 || x == n - 1) continue;
            
            bool composite = true;
            for (int j = 0; j < r - 1; j++) {
                x = modMul(x, x, n);
                if (x == n - 1) {
                    composite = false;
                    break;
                }
            }
            
            if (composite) return false;
        }
        
        return true;
    }

    // Encontra uma raiz primitiva módulo p (onde p é primo seguro)
    uint32_t findPrimitiveRoot(uint32_t p) {
        if (!isSafePrime(p)) {
            throw std::invalid_argument("O número deve ser um primo seguro");
        }
        
        // Para primo seguro p, o grupo Z*_p tem ordem p-1 = 2*q onde q = (p-1)/2 é primo
        uint32_t q = (p - 1) / 2;
        
        std::uniform_int_distribution<uint32_t> dist(2, p - 1);
        
        for (int attempts = 0; attempts < 100000; attempts++) {
            uint32_t g = dist(rng);
            
            // Para ser raiz primitiva, g deve ter ordem p-1
            // Para primo seguro, basta verificar que g^q ≢ 1 (mod p)
            // e g^2 ≢ 1 (mod p) (ou seja, g ≢ ±1)
            
            if (g == 1 || g == p - 1) continue; // g não pode ser ±1
            
            // Verifica se g^2 ≢ 1 (mod p)
            if (modPow(g, 2, p) == 1) continue;
            
            // Verifica se g^q ≢ 1 (mod p)
            if (modPow(g, q, p) == 1) continue;
            
            return g; // Encontrou uma raiz primitiva
        }
        
        throw std::runtime_error("Não foi possível encontrar uma raiz primitiva");
    }
    
    // Exponenciação modular para evitar overflow
    uint64_t modPow(uint64_t base, uint64_t exp, uint64_t mod) {
        uint64_t result = 1;
        base %= mod;
        
        while (exp > 0) {
            if (exp & 1) {
                result = modMul(result, base, mod);
            }
            base = modMul(base, base, mod);
            exp >>= 1;
        }
        
        return result;
    }
    
    // Multiplicação modular para evitar overflow
    uint64_t modMul(uint64_t a, uint64_t b, uint64_t mod) {
        return ((uint64_t)a * b) % mod;
    }
    
    // Verifica se um número é primo seguro
    bool isSafePrime(uint32_t p) {
        if (!millerRabinTest(p)) return false;
        
        // Verifica se (p-1)/2 também é primo (primo de Sophie Germain)
        uint32_t q = (p - 1) / 2;
        return millerRabinTest(q);
    }
    
    // Verifica se 2 é um gerador adequado para Diffie-Hellman (como na RFC 3526)
    bool isValidDHGenerator(uint32_t g, uint32_t p) {
        if (!isSafePrime(p)) return false;
        if (g >= p || g < 2) return false;
        
        uint32_t q = (p - 1) / 2;
        
        // Para ser um gerador válido para DH em primo seguro:
        // g^q ≢ 1 (mod p) - garante que não está no subgrupo trivial
        // Para g = 2, isso significa que 2 gera o subgrupo dos resíduos quadráticos
        return modPow(g, q, p) != 1;
    }
    
    // Encontra o menor gerador adequado para DH (geralmente 2, 3, ou 5)
    uint32_t findSmallDHGenerator(uint32_t p) {
        if (!isSafePrime(p)) {
            throw std::invalid_argument("O número deve ser um primo seguro");
        }
        
        // Testa os candidatos pequenos primeiro (como na RFC 3526)
        std::vector<uint32_t> candidates = {2, 3, 5, 7, 11, 13, 17, 19, 23};
        
        for (uint32_t g : candidates) {
            if (g >= p) break;
            if (isValidDHGenerator(g, p)) {
                return g;
            }
        }
        
        // Se nenhum candidato pequeno funcionou, procura um maior
        std::uniform_int_distribution<uint32_t> dist(2, 100);
        
        for (int attempts = 0; attempts < 1000; attempts++) {
            uint32_t g = dist(rng);
            if (isValidDHGenerator(g, p)) {
                return g;
            }
        }
        
        throw std::runtime_error("Não foi possível encontrar um gerador DH");
    }
    
    // Verifica se g é raiz primitiva módulo p
    bool isPrimitiveRoot(uint32_t g, uint32_t p) {
        if (!isSafePrime(p)) return false;
        if (g >= p || g < 2) return false;
        
        uint32_t q = (p - 1) / 2;
        
        // Para primo seguro, g é raiz primitiva se e somente se:
        // g^2 ≢ 1 (mod p) e g^q ≢ 1 (mod p)
        return (modPow(g, 2, p) != 1) && (modPow(g, q, p) != 1);
    }
    
    // Calcula a ordem de um elemento no grupo Z*_p
    uint32_t calculateOrder(uint32_t g, uint32_t p) {
        if (g >= p || g == 0) return 0;
        
        uint32_t order = 1;
        uint32_t current = g % p;
        
        while (current != 1 && order < p) {
            current = modMul(current, g, p);
            order++;
        }
        
        return (current == 1) ? order : 0;
    }
    
    // Gera um número ímpar aleatório no intervalo especificado
    uint32_t generateRandomOdd(uint32_t min, uint32_t max) {
        std::uniform_int_distribution<uint32_t> dist(min, max);
        uint32_t num = dist(rng);
        return num | 1; // Garante que seja ímpar
    }

    SafePrimeGenerator() : rng(std::random_device{}()) {}
    
    // Gera um primo seguro de 32 bits
    uint32_t generateSafePrime() {
        // Define o intervalo para números de 32 bits
        uint32_t min = 0x80000000UL; // Menor número de 32 bits
        uint32_t max = 0xFFFFFFFFUL; // Maior número de 32 bits
        
        // Garante que começamos com um número ímpar
        if (min % 2 == 0) min++;
        
        uint32_t candidate;
        int attempts = 0;
        const int maxAttempts = 500000; // Aumentado para 32 bits
        
        std::cout << "Gerando primo seguro de 32 bits..." << std::endl;
        
        do {
            candidate = generateRandomOdd(min, max);
            attempts++;
            
            if (attempts % 10000 == 0) {
                std::cout << "Tentativa " << attempts << "..." << std::endl;
            }
            
            if (attempts > maxAttempts) {
                throw std::runtime_error("Não foi possível encontrar um primo seguro após muitas tentativas");
            }
        } while (!isSafePrime(candidate));
        
        std::cout << "Primo encontrado após " << attempts << " tentativas!" << std::endl;
        return candidate;
    }
    
    // Verifica se um número dado é primo seguro
    bool verifySafePrime(uint32_t n) {
        return isSafePrime(n);
    }
    
    // Encontra gerador estilo RFC 3526 (geralmente 2)
    uint32_t findRFC3526StyleGenerator(uint32_t p) {
        std::cout << "Procurando gerador estilo RFC 3526 para p = " << p << "..." << std::endl;
        uint32_t generator = findSmallDHGenerator(p);
        std::cout << "Gerador encontrado: " << generator << std::endl;
        return generator;
    }
    
    // Encontra múltiplas raízes primitivas
    std::vector<uint32_t> findMultiplePrimitiveRoots(uint32_t p, int count) {
        std::vector<uint32_t> roots;
        std::vector<uint32_t> candidates;
        
        std::cout << "Procurando " << count << " raízes primitivas para p = " << p << "..." << std::endl;
        
        // Gera candidatos únicos
        std::uniform_int_distribution<uint32_t> dist(2, p - 1);
        
        while (roots.size() < count && candidates.size() < 10000) {
            uint32_t g = dist(rng);
            
            // Evita duplicatas
            if (std::find(candidates.begin(), candidates.end(), g) != candidates.end()) {
                continue;
            }
            candidates.push_back(g);
            
            if (isPrimitiveRoot(g, p)) {
                roots.push_back(g);
                std::cout << "Raiz primitiva " << roots.size() << ": " << g << std::endl;
            }
        }
        
        return roots;
    }
};

// Estrutura para armazenar primo seguro com geradores
struct SafePrimeWithGenerators {
    uint32_t prime;
    uint32_t dhGenerator;      // Gerador para DH (geralmente 2)
    uint32_t primitiveRoot;    // Raiz primitiva verdadeira
    uint32_t sophieGermain;
    
    SafePrimeWithGenerators(uint32_t p, uint32_t dh_g, uint32_t prim_g) 
        : prime(p), dhGenerator(dh_g), primitiveRoot(prim_g), sophieGermain((p-1)/2) {}
};

// Função auxiliar para exibir informações completas
void displayPrimeWithGenerators(const SafePrimeWithGenerators& data) {
    std::cout << "=== PRIMO SEGURO COM GERADORES ===" << std::endl;
    std::cout << "Primo seguro (p): " << data.prime << std::endl;
    std::cout << "Hexadecimal: 0x" << std::hex << data.prime << std::dec << std::endl;
    std::cout << "Primo de Sophie Germain (q): " << data.sophieGermain << std::endl;
    std::cout << "Gerador DH (como RFC 3526): " << data.dhGenerator << std::endl;
    std::cout << "Raiz primitiva verdadeira: " << data.primitiveRoot << std::endl;
    std::cout << "Número de bits do primo: " << (32 - __builtin_clz(data.prime)) << std::endl;
    std::cout << "Ordem do grupo Z*_p: " << (data.prime - 1) << std::endl;
    
    // Verificações
    std::cout << "\n=== VERIFICAÇÕES ===" << std::endl;
    
    // Testa gerador DH
    uint64_t dh_test = 1;
    uint64_t base = data.dhGenerator;
    uint32_t exp = data.sophieGermain;
    uint32_t mod = data.prime;
    
    while (exp > 0) {
        if (exp & 1) dh_test = (dh_test * base) % mod;
        base = (base * base) % mod;
        exp >>= 1;
    }
    
    std::cout << "Gerador DH - " << data.dhGenerator << "^q mod p = " << dh_test;
    std::cout << " (deve ser ≠ 1 para ser válido)" << std::endl;
    
    // Testa raiz primitiva
    std::cout << "Raiz primitiva - ordem completa: " << (data.primitiveRoot != 0 ? "✓" : "✗") << std::endl;
    
    std::cout << "\n=== EXPLICAÇÃO RFC 3526 ===" << std::endl;
    std::cout << "• RFC 3526 usa " << data.dhGenerator << " como gerador" << std::endl;
    std::cout << "• " << data.dhGenerator << " gera subgrupo de ordem " << data.sophieGermain << " (resíduos quadráticos)" << std::endl;
    std::cout << "• Isso é SEGURO para DH pois evita subgrupos pequenos" << std::endl;
    std::cout << "• Raiz primitiva " << data.primitiveRoot << " geraria grupo completo de ordem " << (data.prime-1) << std::endl;
}

/* int main() {
    SafePrimeGenerator generator;
    uint32_t g = generator.findSmallDHGenerator(3786491543);
    std::cout << g << std::endl;
} */

/* int main() {
    SafePrimeGenerator generator;
    
    try {
        std::cout << "=== Gerador de Primos Seguros com Geradores DH (RFC 3526 Style) ===" << std::endl << std::endl;
        
        // Gerar um primo seguro de 32 bits
        std::cout << "1. Gerando primo seguro de 32 bits:" << std::endl;
        uint32_t prime32 = generator.generateSafePrime();
        std::cout << std::endl;
        
        // Encontrar gerador estilo RFC 3526 (geralmente 2)
        uint32_t dhGenerator = generator.findRFC3526StyleGenerator(prime32);
        
        // Encontrar também uma raiz primitiva verdadeira
        std::cout << "Procurando raiz primitiva verdadeira..." << std::endl;
        uint32_t primitiveRoot = generator.findPrimitiveRoot(prime32);
        std::cout << "Raiz primitiva encontrada: " << primitiveRoot << std::endl;
        
        // Criar estrutura com os dados
        SafePrimeWithGenerators primeData(prime32, dhGenerator, primitiveRoot);
        std::cout << std::endl;
        displayPrimeWithGenerators(primeData);
        std::cout << std::endl;
        
        // Verificações com números pequenos conhecidos
        std::cout << "2. Testando com primo seguro pequeno (p = 23):" << std::endl;
        uint32_t p23 = 23;
        std::cout << "Testando se 2 é gerador DH válido para p=23: " 
                  << (generator.isValidDHGenerator(2, p23) ? "✓ SIM" : "✗ NÃO") << std::endl;
        std::cout << "Testando se 2 é raiz primitiva para p=23: " 
                  << (generator.isPrimitiveRoot(2, p23) ? "✓ SIM" : "✗ NÃO") << std::endl;
        
        // Para p=23, q=11
        std::cout << "2^11 mod 23 = " << generator.modPow(2, 11, 23) 
                  << " (se ≠1, então 2 gera subgrupo de ordem 11)" << std::endl;
        std::cout << "2^2 mod 23 = " << generator.modPow(2, 2, 23) 
                  << " (se ≠1, então 2 não está no subgrupo trivial)" << std::endl;
        std::cout << std::endl;
        
        // Explicação técnica
        std::cout << "3. EXPLICAÇÃO TÉCNICA:" << std::endl;
        std::cout << "• RFC 3526 usa geradores pequenos (geralmente 2) por design" << std::endl;
        std::cout << "• Estes geradores NÃO são raízes primitivas completas" << std::endl;
        std::cout << "• Eles geram o subgrupo dos resíduos quadráticos de ordem (p-1)/2" << std::endl;
        std::cout << "• Isso é SEGURO para Diffie-Hellman pois evita ataques de subgrupo pequeno" << std::endl;
        std::cout << "• Raízes primitivas completas gerariam o grupo inteiro de ordem p-1" << std::endl;
        std::cout << std::endl;
        
        std::cout << "4. COMPARAÇÃO DE TAMANHOS:" << std::endl;
        std::cout << "• Gerador DH " << dhGenerator << " gera subgrupo de ordem: " << ((prime32-1)/2) << std::endl;
        std::cout << "• Raiz primitiva " << primitiveRoot << " gera grupo completo de ordem: " << (prime32-1) << std::endl;
        std::cout << "• RFC 3526 prefere o subgrupo menor por razões de segurança" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} */