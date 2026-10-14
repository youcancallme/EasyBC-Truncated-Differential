

#ifndef EASYBC_SBOXMODEL_H
#define EASYBC_SBOXMODEL_H

#include <cassert>
#include <utility>
#include <cmath>
#include <bitset>
#include "SboxExtC.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

class SboxM {

  private:
    std::string name_;
    std::vector<int> sbox_;
    std::string mode_;

    int ddtInputSize_;
    int ddtOutputSize_;
    int sboxInputSize_;
    int sboxOutputSize_;

    int ddtProbNum_{};
    std::vector<int> ddtProb_{};
    std::vector<float> extWeighted_{};
    std::unordered_map<std::string, std::string> ddtProbEncode_;

    std::string path_ = std::string(DPATH) + "sbox/";
    std::string ddtPath_{}, latPath_{}, arxPath_{};

    std::string cnfPath;
    std::string impossPath;
    std::vector<std::vector<int>> sboxCnf;

    std::vector<std::string> impossPm_;
    std::vector<std::string> possPm_;

    std::vector<std::string> possPm_cnf;
    std::vector<std::string> impossPm_cnf;

    int weightnum_cnf;

    std::string sageInPath_;
    std::string sageOutPath_;
    std::vector<std::vector<int>> sageIneqs_;

    int sboxMDP;

  public:
    SboxM(std::string name, std::vector<int> sbox, std::string mode);
    SboxM(std::string name, std::string mode);

    void ddt_gen();
    void lat_gen();
    static int binCounter1(int input);

    void get_ext();
    void pattern_ext();
    void sage_ext();

    void arxDiff();
    void arxLinear();
    void arxSageGen();

    void trueTableGen();

    void possBintoHex();

    void sbox_cnf_red();

    void sbox_cnf_red_2();

    void set_name(std::string name) { this->name_ = name; }
    std::string get_name() { return name_; }
    std::string get_ddt() { return ddtPath_; }
    std::string get_lat() { return latPath_; }
    std::string get_path() { return path_; }
    std::string get_mode() { return mode_; }
    int get_sbox_len() { return int(log2(sbox_.size())); }
    std::vector<std::string> get_poss() { return possPm_; }
    std::vector<std::string> get_imposs() { return impossPm_; }
    std::vector<std::vector<int>> get_sage_ineqs() { return sageIneqs_; }
    int get_dim() { return possPm_[0].size() + 1; }
    int get_ext_len() { return ddtProbNum_; }

    std::vector<std::vector<int>> get_sboxCnf() { return sboxCnf; }
    int get_weightnum_cnf() { return weightnum_cnf; }

    float getSboxMDP() {
        std::sort(ddtProb_.rbegin(), ddtProb_.rend());
        int maxProb = ddtProb_[1];
        float rtn = -log2(maxProb / pow(2, sqrt(sbox_.size())));
        return rtn;
    }

    bool isAllZeros(const std::string &str) {

        if (str.empty()) {
            return false;
        }

        for (char ch : str) {
            if (ch != '0') {
                return false;
            }
        }

        return true;
    }

    std::vector<int> get_extWeighted() {
        std::vector<int> rtn;
        std::vector<float> temp = extWeighted_;
        while (true) {
            int times = 10, counter = 0;
            bool allInt = false;
            for (auto &weight : temp) {
                if (weight - int(weight) == 0) {
                    counter++;
                }
            }
            if (counter == extWeighted_.size())
                break;
        }
        for (auto item : temp) {
            rtn.push_back(int(item));
        }
        return rtn;
    }

    std::string find_matching_file(const std::string &directory) {
        int min_number = INT_MAX;
        std::string result;

        for (const auto &entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();

                if (filename.find("cnf.") == 0 &&
                    filename.size() >= 5 &&
                    (filename.size() < 8 || filename.substr(filename.size() - 7) != ".source")) {

                    std::string number_part = filename.substr(4);
                    size_t dot_pos = number_part.find('.');
                    if (dot_pos != std::string::npos) {
                        number_part = number_part.substr(0, dot_pos);
                    }

                    try {
                        int current_number = std::stoi(number_part);

                        if (current_number < min_number) {
                            min_number = current_number;
                            result = entry.path().string();
                        }
                    } catch (const std::invalid_argument &) {

                        continue;
                    }
                }
            }
        }

        return result;
    }

    std::vector<std::vector<int>> read_and_generate_red_cnf(const std::string &filename, int size) {
        std::ifstream inputFile(filename);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return {};
        }

        int outer_size;
        inputFile >> outer_size;

        std::vector<std::vector<int>> red_cnf(outer_size, std::vector<int>(size, 2));

        std::string line;
        std::getline(inputFile, line);

        int row = 0;
        while (std::getline(inputFile, line)) {
            if (line.empty())
                continue;

            std::istringstream iss(line);
            int value;
            while (iss >> value) {
                int index = abs(value) - 1;
                if (index >= 0 && index < size) {
                    red_cnf[row][index] = (value > 0) ? 1 : 0;
                } else {
                    std::cerr << "Invalid index: " << index << " (value: " << value << ")" << std::endl;
                }
            }
            row++;
        }

        inputFile.close();
        return red_cnf;
    }

    std::string binaryToCNF(const std::string &binaryLine) {
        std::stringstream cnfClause;
        for (size_t i = 0; i < binaryLine.length(); ++i) {
            char bit = binaryLine[i];
            int variable = i + 1;
            if (bit == '1') {
                cnfClause << "-" << variable << " ";
            } else if (bit == '0') {
                cnfClause << variable << " ";
            } else {
                std::cerr << "Error: Invalid character in binary line: " << bit << std::endl;
                exit(1);
            }
        }
        cnfClause << "0";
        return cnfClause.str();
    }

    std::vector<std::vector<int>> read_and_generate_red_cnf_2(const std::string &filename, int size) {
        std::ifstream inputFile(filename);

        std::string line;
        while (getline(inputFile, line)) {
            if (line.empty() || line[0] == 'c') {
                continue;
            }
            if (line[0] == 'p') {
                break;
            }
        }

        std::istringstream headerStream(line);
        std::string temp;
        int numVariables, numClauses;
        headerStream >> temp >> temp >> numVariables >> numClauses;

        std::vector<std::vector<int>> red_cnf(numClauses, std::vector<int>(size, 2));

        int row = 0;
        while (getline(inputFile, line)) {
            if (line.empty() || line[0] == 'c') {
                continue;
            }

            std::istringstream iss(line);
            int value;
            while (iss >> value) {
                if (value == 0) {
                    break;
                }
                int index = abs(value) - 1;
                if (index >= 0 && index < size) {
                    red_cnf[row][index] = (value > 0) ? 1 : 0;
                } else {
                    std::cerr << "Invalid index: " << index << " (value: " << value << ")" << std::endl;
                }
            }
            row++;
        }

        inputFile.close();
        return red_cnf;
    }
};

#endif
