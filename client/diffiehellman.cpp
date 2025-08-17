#include <string>
#include <cstdlib>
#include <cmath>
#include <time.h>

namespace DiffieHellman {
    using namespace std;

    /**
     * P e G vão ser numeros aleatorios que todos devem saber
     * podemos:
     *  - o primeito client decide e manda pro servidor
     *  - o servidor msm decide e manda pros clientes
     *  
     * Eles não podem ser qualquer número, tem que ver a regra direito antes de implementar
     */
    const int P = 23;
    const int G = 5;

    /**
     * (base^exp) % mod.
     */
    int modularExponent(int base, int exponent, int modulus) {
        int result = 1;
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

    int generatePrivateKey() {
        return rand();
    }

    int generatePublicKey(int privateKey){ // (int p, int g) {
        return modularExponent(G, privateKey, P);
    }

    int generateSharedSecret(int publicKey, int privateKey) {
        return modularExponent(publicKey, privateKey, P);
    }
}

// ============================================================================================================ 
//                                            So pra testar
// ============================================================================================================
#include <iostream>

using namespace DiffieHellman;

struct Client
{
    int publicKey;
    int privateKey;
    int sharedSecret;

    Client() {
        privateKey = generatePrivateKey();
        publicKey = generatePublicKey(privateKey);
    }
};


int main() {
    Client a,b;

    srand(time(0));

    a.sharedSecret = generateSharedSecret(b.publicKey, a.privateKey);
    b.sharedSecret = generateSharedSecret(a.publicKey, b.privateKey);

    cout << "a: " << endl;
    cout << "\tshared secret: " << a.sharedSecret << endl;
    cout << "\tprivate: " << a.privateKey << endl;
    cout << "\tpublic: " << a.publicKey << endl;

    cout << "b: " << endl;
    cout << "\tshared secret: " << b.sharedSecret << endl;
    cout << "\tprivate: " << b.privateKey << endl;
    cout << "\tpublic: " << b.publicKey << endl;
}


