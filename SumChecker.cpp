//
// Created by MSI GS66 Stealth on 09.06.2022.
//

#include "SumChecker.h"

SumChecker::SumChecker(const std::string &iniFileName) : file(iniFileName) {
    file.read(ini);
}

void SumChecker::AddDifference(const std::string &key, const std::string &hash) {
    differentHashes[key] = hash;
}

void SumChecker::OverrideDifferences() {
    file.read(ini);
    for (const auto & [key, val]: differentHashes) ini["HASHES"][key] = val;
    file.write(ini);
}

void SumChecker::PrintDifferences() {
    std::cout << "DIFF HASHES:" << std::endl;
    for (auto const&[key, val]: differentHashes) {
        std::cout << key        // string (key)
                  << ':'
                  << val        // string's value
                  << std::endl;
    }
}

bool SumChecker::isEmpty() {
    return differentHashes.empty();
}
