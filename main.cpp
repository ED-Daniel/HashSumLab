#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <map>
#include "ini.h"
#include "md5.h"
#include "sha1.h"
#include "SumChecker.h"

enum algorithms {
    md5_algorithm,
    sha1_algorithm
};

char * readBytes(const std::string & path);
bool getAnswer();
void checkSums(const std::vector<std::string> & checkFilesNames, const algorithms & algorithm);

int main(int argc, char *argv[])
{
    std::map<std::string, std::string> newFiles = std::map<std::string, std::string>();
    std::map<std::string, std::string> differentHashes = std::map<std::string, std::string>();

    std::vector<int> commandIndexes = std::vector<int>();
    std::vector<std::string> foundCommands = std::vector<std::string>();

    std::vector<std::string> parameters = std::vector<std::string> {
            "-c",
            "-w",
            "-a",
            "-n"
    };

    std::string algorithm = "md5";

    // Calling without arguments
    if (argc == 1) {
        if (std::filesystem::exists("./cksum.ini")) {
            mINI::INIFile file("./cksum.ini");
            mINI::INIStructure ini;
            file.read(ini);

            for (const auto &entry: std::filesystem::directory_iterator("./")) {
                std::string pathToFile = entry.path().string();
                if (std::filesystem::is_directory(pathToFile)) continue;
                if (pathToFile == "./cksum.ini") continue;

                std::string lower_case_path = pathToFile;
                std::for_each(lower_case_path.begin(), lower_case_path.end(), [](char &c) {
                    c = std::tolower(c);
                });

                bool foundFile = false;
                for (auto const &it: ini) {
                    if (foundFile) break;
                    auto const &section = it.first;
                    auto const &collection = it.second;
                    for (auto const &it2: collection) {
                        auto const &key = it2.first;
                        auto const &value = it2.second;

                        if (lower_case_path == "./" + key) {
                            foundFile = true;
                            std::string newVal = md5(readBytes(pathToFile));
                            if (value == newVal) std::cout << "OKAY: " << pathToFile << std::endl;
                            else {
                                differentHashes[pathToFile.substr(2, pathToFile.size())] = newVal;
                                std::cout << "DIFFERENT MD5 HASH: " << pathToFile << std::endl;
                                std::cout << "NEW HASH: " << newVal << std::endl;
                                std::cout << std::endl;
                            }
                            break;
                        }
                    }
                }

                if (!foundFile) newFiles[pathToFile.substr(2, pathToFile.size())] = md5(readBytes(pathToFile));

            }

            if (newFiles.empty() && differentHashes.empty()) {
                std::cout << "All files succeeded check!" << std::endl;
                return 0;
            }

            if (!newFiles.empty()) {
                std::cout << "============================================================================="
                          << std::endl;
                std::cout << std::endl;

                std::cout << "NEW FILES:" << std::endl;
                for (auto const&[key, val]: newFiles) {
                    std::cout << key        // string (key)
                              << ':'
                              << val        // string's value
                              << std::endl;
                }

                std::cout << "Want to add them to .ini file?" << std::endl;
                std::cout << std::endl;

                if (getAnswer()) {
                    for (auto const&[key, val]: newFiles) {
                        ini["HASHES"][key] = val;
                    }
                    file.write(ini);
                    std::cout << "All new files were added to .ini" << std::endl;
                }
                else
                    std::cout << "New files were ignored" << std::endl;
            }

            if (!differentHashes.empty()) {
                std::cout << std::endl;
                std::cout << "============================================================================="
                          << std::endl;
                std::cout << std::endl;

                std::cout << "DIFF HASHES:" << std::endl;
                for (auto const&[key, val]: differentHashes) {
                    std::cout << key        // string (key)
                              << ':'
                              << val        // string's value
                              << std::endl;
                }

                std::cout << "Want to update .ini file?" << std::endl;
                std::cout << std::endl;

                if (getAnswer()) {
                    for (auto const&[key, val]: differentHashes) {
                        ini["HASHES"][key] = val;
                    }
                    file.write(ini);
                    std::cout << ".ini file was updated successfully" << std::endl;
                }
                else
                    std::cout << "Errors were ignored" << std::endl;
            }
        }
        else {
            std::cout << "Usage" << std::endl;
        }
    }
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            if (std::find(parameters.begin(), parameters.end(), argv[i]) != parameters.end()) {
                foundCommands.emplace_back(argv[i]);
                commandIndexes.push_back(i);
            }
        }
        if (std::find(parameters.begin(), parameters.end(), argv[1]) == parameters.end()) {
            std::vector<std::string> fileNames = std::vector<std::string>();
            int limit = commandIndexes.empty() ? argc : commandIndexes[0];
            for (int i = 1; i < limit; ++i) {
                fileNames.emplace_back("./" + std::string(argv[i]));
            }
            for (const auto & file : fileNames) {
                std::cout << "MD5 Hash for " << file << ":" << std::endl;
                std::cout << md5(readBytes(file)) << std::endl;
                std::cout << std::endl;
            }
        }
    }

//  cout << SHA1::from_file(R"(C:\Users\MSI GS66 Stealth\CLionProjects\HashSum\CMakeLists.txt)") << endl;
    return 0;
}

char * readBytes(const std::string & path) {
    std::ifstream file (R"(C:\Users\MSI GS66 Stealth\CLionProjects\HashSum\CMakeLists.txt)", std::ifstream::binary);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        auto fileSize = file.tellg();
        auto memBlock = new char[fileSize];
        file.seekg(0, std::ios::beg);
        file.read(memBlock, fileSize);
        file.close();
        return memBlock;
    }
    else {
        return nullptr;
    }
}

bool getAnswer() {
    std::cout << "Y/y (yes) | N/n (no)" << std::endl;

    std::string answer;
    std::cin >> answer;

    while (!(answer == "Y" || answer == "y" || answer == "N" || answer == "n")) {
        std::cout << "WRONG INPUT" << std::endl;
        std::cin >> answer;
    }
    return (answer == "Y" || answer == "y");
}

void checkSums(const std::vector<std::string> & checkFilesNames, const algorithms & algorithm) {
    std::map<std::string, std::string> newFiles = std::map<std::string, std::string>();

    std::vector<SumChecker> sumCheckers = std::vector<SumChecker>();

    for (const auto & pathToIni : checkFilesNames) {
        if (std::filesystem::exists(pathToIni)) {
            mINI::INIFile file(pathToIni);
            mINI::INIStructure ini;
            file.read(ini);

            SumChecker checker = SumChecker(pathToIni);

            for (const auto &entry: std::filesystem::directory_iterator("./")) {
                std::string pathToFile = entry.path().string();
                if (std::filesystem::is_directory(pathToFile)) continue;
                if (pathToFile == pathToIni) continue;

                std::string lower_case_path = pathToFile;
                std::for_each(lower_case_path.begin(), lower_case_path.end(), [](char &c) {
                    c = std::tolower(c);
                });

                bool foundFile = false;
                for (auto const &it: ini) {
                    if (foundFile) break;
                    auto const &section = it.first;
                    auto const &collection = it.second;
                    for (auto const &it2: collection) {
                        auto const &key = it2.first;
                        auto const &value = it2.second;

                        if (lower_case_path == "./" + key) {
                            foundFile = true;
                            newFiles.erase(key);

                            std::string newVal;
                            switch (algorithm) {
                                case algorithms::md5_algorithm:
                                    newVal = md5(readBytes(pathToFile));
                                    break;
                                case algorithms::sha1_algorithm:
                                    newVal = SHA1::from_file(pathToFile);
                                    break;
                                default:
                                    break;
                            }

                            if (value == newVal) std::cout << "OKAY: " << pathToFile << std::endl;
                            else checker.AddDifference(pathToFile.substr(2, pathToFile.size()), newVal);
                            break;
                        }
                    }
                }

                std::string newVal;
                switch (algorithm) {
                    case algorithms::md5_algorithm:
                        newVal = md5(readBytes(pathToFile));
                        break;
                    case algorithms::sha1_algorithm:
                        newVal = SHA1::from_file(pathToFile);
                        break;
                    default:
                        break;
                }

                if (!foundFile) newFiles[pathToFile.substr(2, pathToFile.size())] = newVal;

            }

            sumCheckers.push_back(checker);
        }
        else {
            std::cout << "No .ini file found with such name: " << pathToIni << std::endl;
        }
    }
    if (newFiles.empty() && std::all_of(sumCheckers.begin(), sumCheckers.end(), [](SumChecker c){ return c.isEmpty(); })) {
        std::cout << "All files succeeded check!" << std::endl;
        return;
    }

    if (!newFiles.empty()) {
        std::cout << "============================================================================="
                  << std::endl;
        std::cout << std::endl;

        std::cout << "NEW FILES:" << std::endl;
        for (auto const&[key, val]: newFiles) {
            std::cout << key        // string (key)
                      << ':'
                      << val        // string's value
                      << std::endl;
        }

        std::cout << "Want to add them to .ini file?" << std::endl;
        std::cout << std::endl;

        if (getAnswer()) {
            std::cout << "Choose file: " << std::endl;

            for (int i = 0; i < checkFilesNames.size(); i++) std::cout << i + 1 << ". " << checkFilesNames[i] << std::endl;

            int num;
            std::cin >> num;
            while (!std::cin.good() || num > checkFilesNames.size() || num <= 0) {
                std::cout << "Wrong number" << std::endl;
                std::cout << "Try again: " << std::endl;
                std::cin >> num;
            }

            auto fileName = checkFilesNames[num - 1];

            mINI::INIFile file(fileName);
            mINI::INIStructure ini;
            file.read(ini);

            for (auto const&[key, val]: newFiles) {
                ini["HASHES"][key] = val;
            }
            file.write(ini);
            std::cout << "All new files were added to .ini" << std::endl;
        } else
            std::cout << "New files were ignored" << std::endl;
    }

    for (auto checker : sumCheckers) {
        if (!checker.isEmpty()) {
            std::cout << std::endl;
            std::cout << "============================================================================="
                      << std::endl;
            std::cout << std::endl;

            checker.PrintDifferences();

            std::cout << "Want to update .ini file?" << std::endl;
            std::cout << std::endl;

            if (getAnswer()) {
                checker.OverrideDifferences();
                std::cout << ".ini file was updated successfully" << std::endl;
            } else
                std::cout << "Errors were ignored" << std::endl;
        }
    }
}