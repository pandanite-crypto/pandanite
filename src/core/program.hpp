#pragma once
#include <vector>
#include "common.hpp"
#include "crypto.hpp"

using namespace std;

class Program {
    public:
        Program(vector<uint8_t>& byteCode);
        Program(json obj);
        ProgramID getId() const;
        vector<uint8_t> getByteCode() const;
        json toJson() const;
    protected:
        ProgramID id;
        vector<uint8_t> byteCode;
        friend bool operator==(const Program& a, const Program& b);
};

bool operator==(const Program& a, const Program& b);