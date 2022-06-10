#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <map>
#include <chrono>
#include "ini.h"
#include "md5.h"
#include "sha1.h"
#include "SumChecker.h"

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "accctrl.h"
#include "aclapi.h"
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

enum algorithms {
    md5_algorithm,
    sha1_algorithm
};

static std::vector<char> ReadAllBytes(const std::string& filename);
bool getAnswer();
void checkSums(const std::vector<std::string> & checkFilesNames, const algorithms & algorithm);
void demo_perms(fs::perms p);
void printOwner(const CHAR *path);

int main(int argc, char *argv[])
{
    std::vector<int> commandIndexes = std::vector<int>();
    std::vector<std::string> foundCommands = std::vector<std::string>();

    std::vector<std::string> parameters = std::vector<std::string> {
            "-c",
            "-w",
            "-a",
            "-n"
    };

    algorithms algorithm = algorithms::md5_algorithm;

    bool toFile = false;
    std::string outFileName;

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

        auto algorithm_command = std::find(foundCommands.begin(), foundCommands.end(), "-a");
        if (algorithm_command != foundCommands.end()) {
            int command_index = std::distance(foundCommands.begin(), algorithm_command);
            if (commandIndexes[command_index] + 1 >= argc) {
                std::cout << "Wrong usage" << std::endl;
                return 1;
            }
            if (strcmp(argv[commandIndexes[command_index] + 1], "md5") == 0) algorithm = algorithms::md5_algorithm;
            else if (strcmp(argv[commandIndexes[command_index] + 1], "sha1") == 0) algorithm = algorithms::sha1_algorithm;
            else {
                std::cout << "WRONG ALGORITHM" << std::endl;
                std::cout << "Chosen MD5" << std::endl;
                algorithm = algorithms::md5_algorithm;
            }

        }

        auto write_command = std::find(foundCommands.begin(), foundCommands.end(), "-w");
        if (write_command != foundCommands.end()) {
            int command_index = std::distance(foundCommands.begin(), write_command);
            if (commandIndexes[command_index] + 1 >= argc) {
                std::cout << "Wrong usage" << std::endl;
                return 1;
            }
            outFileName = argv[commandIndexes[command_index] + 1];
            toFile = true;
        }

        if (std::find(parameters.begin(), parameters.end(), argv[1]) == parameters.end()) {
            std::vector<std::string> fileNames = std::vector<std::string>();
            int limit = commandIndexes.empty() ? argc : commandIndexes[0];
            for (int i = 1; i < limit; ++i) {
                fileNames.emplace_back("./" + std::string(argv[i]));
            }
            for (const auto & file : fileNames) {
                if (!std::filesystem::exists(file)) {
                    std::cout << "No such file: " << file << std::endl;
                    continue;
                }

                switch (algorithm) {
                    case algorithms::md5_algorithm:
                        if (toFile) {
                            std::ofstream out(outFileName);
                            out << "MD5 Hash for " << file << ":" << std::endl;
                            out << md5(ReadAllBytes(file)) << std::endl;
                        }
                        else {
                            std::cout << "MD5 Hash for " << file << ":" << std::endl;
                            std::cout << md5(ReadAllBytes(file)) << std::endl;
                        }
                        break;
                    case algorithms::sha1_algorithm:
                        if (toFile) {
                            std::ofstream out(outFileName);
                            out << "SHA1 Hash for " << file << ":" << std::endl;
                            out << SHA1::from_file(file) << std::endl;
                        }
                        else {
                            std::cout << "SHA1 Hash for " << file << ":" << std::endl;
                            std::cout << SHA1::from_file(file) << std::endl;
                        }
                        break;
                }

                std::cout << std::endl;
            }
        }

        auto cOption = std::find(foundCommands.begin(), foundCommands.end(), "-c");
        if (cOption != foundCommands.end()) {
            std::vector<std::string> fileNames = std::vector<std::string>();

            int commandIndex = commandIndexes[std::distance(foundCommands.begin(), cOption)];
            int limit = std::distance(foundCommands.begin(), cOption) + 1 < commandIndexes.size() ? commandIndexes[std::distance(foundCommands.begin(), cOption) + 1] : argc;

            for (int i = commandIndex + 1; i < limit; ++i) {
                fileNames.emplace_back("./" + std::string(argv[i]));
            }

            checkSums(fileNames, algorithm);
        }

        if (std::find(foundCommands.begin(), foundCommands.end(), "-n") != foundCommands.end()) {
            for (const auto &entry: std::filesystem::directory_iterator("./")) {
                std::string pathToFile = entry.path().string();
                if (std::filesystem::is_directory(pathToFile)) continue;

                // Name
                std::string fileName = pathToFile.substr(2, pathToFile.size());
                std::cout << fileName << std::endl;

                // Size
                std::ifstream in_file(pathToFile, std::ios::binary);
                in_file.seekg(0, std::ios::end);
                auto file_size = in_file.tellg();
                std::cout << "Size: " << file_size << std::endl;

                // Permissions
                std::cout << "Permissions: ";
                demo_perms(fs::status(pathToFile).permissions());

                auto start = std::chrono::high_resolution_clock::now();
                auto md5Sum = md5(ReadAllBytes(pathToFile));
                auto sha1Sum = SHA1::from_file(pathToFile);
                auto end = std::chrono::high_resolution_clock::now();

                std::cout << "MD5 Sum: " << md5Sum << "\t" << "SHA1 Sum: " << sha1Sum << std::endl;
                std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << std::endl;

                printOwner(pathToFile.c_str());

                std::cout << std::endl;
            }
        }
    }
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
                if (pathToFile == pathToIni || pathToFile == "./HashSum.exe") continue;

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

void demo_perms(fs::perms p) {
    std::cout << ((p & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
              << ((p & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
              << ((p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-")
              << ((p & fs::perms::group_read) != fs::perms::none ? "r" : "-")
              << ((p & fs::perms::group_write) != fs::perms::none ? "w" : "-")
              << ((p & fs::perms::group_exec) != fs::perms::none ? "x" : "-")
              << ((p & fs::perms::others_read) != fs::perms::none ? "r" : "-")
              << ((p & fs::perms::others_write) != fs::perms::none ? "w" : "-")
              << ((p & fs::perms::others_exec) != fs::perms::none ? "x" : "-")
              << '\n';
}

void printOwner(const CHAR *path) {
    DWORD dwRtnCode = 0;
    PSID pSidOwner = NULL;
    BOOL bRtnBool = TRUE;
    LPTSTR AcctName = NULL;
    LPTSTR DomainName = NULL;
    DWORD dwAcctName = 1, dwDomainName = 1;
    SID_NAME_USE eUse = SidTypeUnknown;
    HANDLE hFile;
    PSECURITY_DESCRIPTOR pSD = NULL;


// Get the handle of the file object.
    hFile = CreateFile(
            TEXT(path),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

// Check GetLastError for CreateFile error code.
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD dwErrorCode = 0;

        dwErrorCode = GetLastError();
        _tprintf(TEXT("CreateFile error = %d\n"), dwErrorCode);
        return;
    }



// Get the owner SID of the file.
    dwRtnCode = GetSecurityInfo(
            hFile,
            SE_FILE_OBJECT,
            OWNER_SECURITY_INFORMATION,
            &pSidOwner,
            NULL,
            NULL,
            NULL,
            &pSD);

// Check GetLastError for GetSecurityInfo error condition.
    if (dwRtnCode != ERROR_SUCCESS) {
        DWORD dwErrorCode = 0;

        dwErrorCode = GetLastError();
        _tprintf(TEXT("GetSecurityInfo error = %d\n"), dwErrorCode);
        return;
    }

// First call to LookupAccountSid to get the buffer sizes.
    bRtnBool = LookupAccountSid(
            NULL,           // local computer
            pSidOwner,
            AcctName,
            (LPDWORD)&dwAcctName,
            DomainName,
            (LPDWORD)&dwDomainName,
            &eUse);

// Reallocate memory for the buffers.
    AcctName = (LPTSTR)GlobalAlloc(
            GMEM_FIXED,
            dwAcctName);

// Check GetLastError for GlobalAlloc error condition.
    if (AcctName == NULL) {
        DWORD dwErrorCode = 0;

        dwErrorCode = GetLastError();
        _tprintf(TEXT("GlobalAlloc error = %d\n"), dwErrorCode);
        return;
    }

    DomainName = (LPTSTR)GlobalAlloc(
            GMEM_FIXED,
            dwDomainName);

    // Check GetLastError for GlobalAlloc error condition.
    if (DomainName == NULL) {
        DWORD dwErrorCode = 0;

        dwErrorCode = GetLastError();
        _tprintf(TEXT("GlobalAlloc error = %d\n"), dwErrorCode);
        return;

    }

    // Second call to LookupAccountSid to get the account name.
    bRtnBool = LookupAccountSid(
            NULL,                   // name of local or remote computer
            pSidOwner,              // security identifier
            AcctName,               // account name buffer
            (LPDWORD)&dwAcctName,   // size of account name buffer
            DomainName,             // domain name
            (LPDWORD)&dwDomainName, // size of domain name buffer
            &eUse);                 // SID type

    // Check GetLastError for LookupAccountSid error condition.
    if (bRtnBool == FALSE) {
        DWORD dwErrorCode = 0;

        dwErrorCode = GetLastError();

        if (dwErrorCode == ERROR_NONE_MAPPED)
            _tprintf(TEXT
                     ("Account owner not found for specified SID.\n"));
        else
            _tprintf(TEXT("Error in LookupAccountSid.\n"));
        return;

    } else if (bRtnBool == TRUE)

        // Print the account name.
        _tprintf(TEXT("Account owner: %s\n"), AcctName);
}