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

static std::vector<char> ReadAllBytes(const std::string& filename);
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
        checkSums(std::vector<std::string> {"./cksum.ini"}, algorithms::md5_algorithm);
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
                std::cout << md5(ReadAllBytes(file)) << std::endl;
                std::cout << std::endl;
            }
        }

        auto cOption = std::find(foundCommands.begin(), foundCommands.end(), "-c");
        if (cOption != foundCommands.end()) {
            std::vector<std::string> fileNames = std::vector<std::string>();

            int commandIndex = commandIndexes[std::distance(foundCommands.begin(), cOption)];
            int limit = commandIndex + 1 < commandIndexes.size() ? commandIndexes[commandIndex + 1] : argc;

            for (int i = commandIndex + 1; i < limit; ++i) {
                fileNames.emplace_back("./" + std::string(argv[i]));
            }

            checkSums(fileNames, algorithms::md5_algorithm);
        }
    }

//    std::cout << md5(std::vector<char> {'h', 'e', 'l', 'l'}) << std::endl;
//  cout << SHA1::from_file(R"(C:\Users\MSI GS66 Stealth\CLionProjects\HashSum\CMakeLists.txt)") << endl;
    return 0;
}

static std::vector<char> ReadAllBytes(const std::string& filename)
{
    std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    if (pos == 0) {
        return std::vector<char>{};
    }

    std::vector<char>  result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    return result;
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
                                    newVal = md5(ReadAllBytes(pathToFile));
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
                        newVal = md5(ReadAllBytes(pathToFile));
                        break;
                    case algorithms::sha1_algorithm:
                        newVal = SHA1::from_file(pathToFile);
                        break;
                    default:
                        break;
                }

                if (!foundFile) newFiles[lower_case_path.substr(2, lower_case_path.size())] = newVal;

            }

            sumCheckers.push_back(checker);
        }
        else {
            std::cout << "No .ini file found with such name: " << pathToIni << std::endl;
        }
    }
    if (checkFilesNames.size() > 1) {
        for (const auto & pathToIni : checkFilesNames) {
            if (std::filesystem::exists(pathToIni)) {
                mINI::INIFile file(pathToIni);
                mINI::INIStructure ini;
                file.read(ini);

                std::vector<std::string> to_erase = std::vector<std::string>();

                for (auto const &it: ini) {
                    auto const &section = it.first;
                    auto const &collection = it.second;
                    for (auto const &it2: collection) {
                        auto const &key = it2.first;
                        auto const &value = it2.second;

                        for (auto const&[new_file_name, val]: newFiles) {
                            if (new_file_name == key) to_erase.push_back(key);
                        }
                    }
                }

                for (const auto & erase_key : to_erase) newFiles.erase(erase_key);
            }
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
                std::cin.clear();
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