//
// Created by MSI GS66 Stealth on 09.06.2022.
//

#ifndef HASHSUM_SUMCHECKER_H
#define HASHSUM_SUMCHECKER_H

#include <string>
#include <map>
#include <iostream>
#include "ini.h"

class SumChecker {
public:
    explicit SumChecker(const std::string & iniFileName);

    void AddDifference(const std::string & key, const std::string & hash);
    void OverrideDifferences();
    void PrintDifferences();

    bool isEmpty();

private:
    mINI::INIFile file;
    mINI::INIStructure ini;
    std::map<std::string, std::string> differentHashes = std::map<std::string, std::string>();
};


#endif //HASHSUM_SUMCHECKER_H
