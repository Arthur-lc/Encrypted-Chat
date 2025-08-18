#pragma once

#include <string>
#include <vector>

using ull = unsigned long long;

namespace CryptoUtils {

    const ull P_MODULUS = 3786491543;
    const int G_GENERATOR = 5;

    struct GroupMember {
        std::string id;
        ull publicKey;
    };

    ull modularExponent(ull base, ull exponent, ull modulus);
    ull modInverse(ull n, ull mod);
    ull generatePrivateKey();
    ull generatePublicKey(ull privateKey);
    ull calculateIntermediateValue(ull myPrivateKey, const GroupMember& before, const GroupMember& after);
    ull calculateSharedSecret(ull myPrivateKey, int myIndex, const std::vector<GroupMember>& orderedMembers, const std::vector<ull>& intermediateValues);

}
