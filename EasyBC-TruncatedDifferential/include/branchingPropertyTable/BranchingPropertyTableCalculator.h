

#ifndef BranchingPropertyTableCalculator_H
#define BranchingPropertyTableCalculator_H

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cmath>
#include <string>
#include <iomanip>
#include "setup.h"
#include "utilities.h"
#include <bitset>

#include <filesystem>
#include <cassert>
#include <utility>
#include <cmath>
#include <sstream>
#include "SboxExtC.h"
#include "BPTProbExtC.h"
#include "truncatedDifferentialBPT/SatSearchConfig.h"

namespace fs = std::filesystem;

class BPTCalculator {

  private:
    int m;
    int t;
    std::vector<std::vector<int>> M;
    std::vector<std::vector<int>> ffmAdditionTable;
    std::vector<std::vector<int>> ffmMultiplicationTable;

    std::string BPTPath;
    std::string Path = std::string(DPATH) + "BPT/";

    std::string cnfPath;
    std::string impossPath;

    std::vector<std::vector<int>> BPT;

    std::string sageInPath_;
    std::string sageOutPath_;

    std::vector<std::vector<int>> sageIneqs_;

    std::vector<std::vector<int>> sageEqns_;

    std::vector<std::string> impossPm_;
    std::vector<std::string> possPm_;

    std::vector<std::string> possPm_cnf;
    std::vector<std::string> impossPm_cnf;

    std::vector<int> extWeighted_{};
    std::string name_;
    bool isMDS;

    int weightnum_cnf;
    std::vector<std::vector<int>> BPTCnf;

    std::vector<std::vector<double>> BPTDecimal_;

    std::vector<std::vector<double>> BPTRaw_;

    std::map<std::string, std::string> probEncodeMILP_;

    std::vector<float> extWeightedFloat_;

    int encodeWidth_ = 0;

    long long beta_ = 1;

  public:
    BPTCalculator(
        std::string name,
        const std::vector<int> &Matrix,

        const std::vector<int> &FFm

    );

    int gf_multiply(int a, int b) const;

    int nearest_power_of_2(double value);

    void sage_inGen();
    void sage_ext();

    void possBintoFile();

    void bpt_cnf_red();

    std::vector<std::vector<int>> get_BPTCnf() { return BPTCnf; }
    int get_weightnum_cnf() { return weightnum_cnf; }

    std::vector<std::vector<double>> get_BPTDecimal() const { return BPTDecimal_; }
    const std::vector<std::vector<double>> &get_BPTRaw() const { return BPTRaw_; }
    std::vector<float> ext_weighted_float() const { return extWeightedFloat_; }
    int get_encode_width() const { return encodeWidth_; }
    const std::map<std::string, std::string> &get_prob_encode_milp() const { return probEncodeMILP_; }

    std::vector<std::vector<int>> sage_ineqs() const {
        return this->sageIneqs_;
    }

    std::vector<std::vector<int>> sage_eqns() const { return sageEqns_; }

    std::vector<int> ext_weighted() const {
        return this->extWeighted_;
    }
    std::vector<std::string> get_poss() { return possPm_; }
    std::vector<std::string> get_imposs() { return impossPm_; }
    std::string get_path() { return Path; }
    std::string get_name() { return name_; }
    int get_dim() { return possPm_[0].size() + 1; }

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

    void computeBranchingTableLA(bool isInteger);

    void prepareSATEncoding(BptEncodingType encodingType, int alpha);

    void prepareMILPEncoding(BptEncodingType encodingType, int alpha);

    std::vector<std::vector<int>> convertToInts(const std::vector<std::string> &bitStrings) {
        std::vector<std::vector<int>> result;
        result.reserve(bitStrings.size());

        for (const std::string &s : bitStrings) {
            std::vector<int> row;
            row.reserve(s.size());
            for (char c : s) {

                row.push_back(c - '0');
            }
            result.push_back(row);
        }

        std::ofstream file;
        std::string file_path = Path + "imposs_cnf/";
        file_path += name_ + "_intial.txt";
        system(("touch " + file_path).c_str());
        file.open(file_path, std::ios::out);
        for (const auto &row : result) {

            for (size_t i = 0; i < row.size(); ++i) {
                file << row[i];

                if (i < row.size() - 1) {
                    file << " ";
                }
            }
            file << "\n";
        }
        file.close();

        std::cout << "数据已成功保存到 " << file_path << std::endl;
        return result;
    }

    std::map<double, std::string> maxSmtEncodeMap_;

    bool useIntegerMaxSMTMode_ = false;
    bool useOneHotEncodingMode_ = false;

    void computeMaxSMTEncoding(bool useIntegerBPT, int alpha, int maxExtSize = 0);
    long long get_beta() const { return beta_; }
};

#endif
