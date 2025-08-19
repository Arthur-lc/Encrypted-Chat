#pragma once
#include <vector>
#include <string>

struct GrupMember {
    std::string username;
    unsigned long long publicKey;
};

extern std::vector<GrupMember> groupMembers;