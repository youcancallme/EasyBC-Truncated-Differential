

#ifndef EASYBC_BPTPROBEXTC_H
#define EASYBC_BPTPROBEXTC_H

#include <z3++.h>
#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include "utilities.h"
#include "setup.h"

class BPTProbExtC {

  private:
    z3::context context_;
    z3::optimize optimize_;

    std::string name_;

    int maxExtSize_;

    int timer_ = 3600 * 12;

    int precisionChars_ = 12;

    std::vector<double> pLog2_;

    std::vector<std::string> pKeys_;

    std::vector<std::string> c_;
    std::vector<std::vector<std::string>> p_;

    std::string smtLib_;

    std::string basePath_;
    std::string smtModelPath_;
    std::string resultPath_;

    std::vector<float> weighted_{};
    std::vector<int> weightedIdx_{};
    bool resultsParsed_ = false;

    static std::string probToKey(double v, int prec = 6) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(prec) << v;
        return oss.str();
    }

    std::string toSmtReal(double v) const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precisionChars_) << std::fabs(v);
        std::string s = oss.str();

        if (s.find('.') != std::string::npos) {
            size_t last = s.find_last_not_of('0');
            if (last != std::string::npos && s[last] == '.')
                ++last;
            s = s.substr(0, last + 1);
        }
        return (v < 0.0) ? ("(- " + s + ")") : s;
    }

    void buildSmtLib() {

        for (const auto &cj : c_) {
            smtLib_ += "(declare-const " + cj + " Real)\n";
        }

        for (const auto &pi : p_) {
            for (const auto &pij : pi) {
                smtLib_ += "(declare-const " + pij + " Int)\n";
            }
        }

        for (const auto &pi : p_) {
            for (const auto &pij : pi) {
                smtLib_ += "(assert (>= " + pij + " 0))\n";
                smtLib_ += "(assert (<= " + pij + " 1))\n";
            }
        }

        for (int i = 0; i < (int)pLog2_.size(); ++i) {
            std::string sum;
            for (int j = 0; j < maxExtSize_; ++j) {
                sum += "(* " + c_[j] + " " + p_[i][j] + ")";
            }
            smtLib_ += "(assert (= " + toSmtReal(pLog2_[i]) + " (+ " + sum + ")))\n";
        }

        for (const auto &cj : c_) {
            smtLib_ += "(assert-soft (= " + cj + " 0) :weight 1)\n";
        }
    }

  public:
    BPTProbExtC(const std::string &name,
                const std::vector<double> &probs,
                int maxExtSize = 0)
        : optimize_(context_), name_(name) {
        maxExtSize_ = (maxExtSize > 0) ? maxExtSize : (int)probs.size();
        if (maxExtSize_ < 1)
            maxExtSize_ = 1;

        for (double p : probs) {
            pLog2_.push_back(p);
            pKeys_.push_back(probToKey(p));
        }

        for (int j = 0; j < maxExtSize_; ++j) {
            c_.push_back("c" + std::to_string(j));
        }

        for (int i = 0; i < (int)pLog2_.size(); ++i) {
            std::vector<std::string> pi;
            for (int j = 0; j < maxExtSize_; ++j) {
                pi.push_back("p" + std::to_string(i) + "_" + std::to_string(j));
            }
            p_.push_back(pi);
        }

        basePath_ = std::string(DPATH) + "BPT/" + name + "/prob_extc/";
        smtModelPath_ = basePath_ + name + "_smtLib.txt";
        resultPath_ = basePath_ + name + "_results.txt";

        system(("mkdir -p " + basePath_).c_str());

        buildSmtLib();
        solve();
    }

    void solve() {

        std::ofstream smtW(smtModelPath_, std::ios::trunc);
        if (!smtW) {
            std::cerr << "[BPTProbExtC] Cannot write SMT model: "
                      << smtModelPath_ << "\n";
        } else {
            smtW << smtLib_;
        }

        optimize_.push();
        optimize_.from_string(smtLib_.c_str());

        z3::params zp(context_);
        zp.set(":timeout", (unsigned)(timer_ * 1000u));
        optimize_.set(zp);

        clock_t t0 = clock();
        time_t ts = time(nullptr);
        optimize_.check();
        double clockSec = (double)(clock() - t0) / CLOCKS_PER_SEC;
        double wallSec = difftime(time(nullptr), ts);

        std::ofstream rf(resultPath_, std::ios::trunc);
        if (!rf) {
            std::cerr << "[BPTProbExtC] Cannot write results: "
                      << resultPath_ << "\n";
            return;
        }
        rf << "clock time : " << clockSec << "s\n";
        rf << "wall time : " << wallSec << "s\n";

        auto model = optimize_.get_model();
        for (int i = 0; i < (int)model.size(); ++i) {
            auto decl = model.get_const_decl(i);
            auto val = model.eval(model.get_const_interp(decl));
            std::cout << decl.name() << " : " << val << "\n";
            rf << decl.name() << " : " << val << "\n";
        }
        rf.close();

        std::cout << "[BPTProbExtC] Done. Results -> " << resultPath_ << "\n";
    }

    std::vector<float> getWeighted() {
        if (resultsParsed_)
            return weighted_;

        weighted_.clear();
        weightedIdx_.clear();

        std::ifstream f(resultPath_);
        if (!f.is_open()) {
            std::cerr << "[BPTProbExtC] Cannot read results: "
                      << resultPath_ << "\n";
            return weighted_;
        }

        std::string line;
        while (std::getline(f, line)) {
            auto parts = utilities::split(line, " ");
            if (parts.size() < 3)
                continue;

            for (int j = 0; j < (int)c_.size(); ++j) {
                if (parts[0] == c_[j]) {
                    const std::string &valStr = parts[2];

                    if (valStr == "0" || valStr == "0.0")
                        break;
                    try {
                        float w = std::stof(valStr);
                        if (std::fabs(w) > 1e-9f) {
                            weighted_.push_back(w);
                            weightedIdx_.push_back(j);
                        }
                    } catch (...) {
                    }
                    break;
                }
            }
        }
        return weighted_;
    }

    std::vector<std::string> getEncodes() {

        std::unordered_map<std::string, std::string> varVals;
        std::ifstream f(resultPath_);
        if (!f.is_open()) {
            std::cerr << "[BPTProbExtC] Cannot read results: "
                      << resultPath_ << "\n";
            return {};
        }
        std::string line;
        while (std::getline(f, line)) {
            auto parts = utilities::split(line, " ");
            if (parts.size() >= 3 && !parts[0].empty() && parts[0][0] == 'p') {
                varVals[parts[0]] = parts[2];
            }
        }

        std::vector<std::string> encodes;
        encodes.reserve(pLog2_.size());
        for (int i = 0; i < (int)pLog2_.size(); ++i) {
            std::string bits;
            bits.reserve(maxExtSize_);
            for (int j = 0; j < maxExtSize_; ++j) {
                std::string key = "p" + std::to_string(i) + "_" + std::to_string(j);
                auto it = varVals.find(key);
                if (it != varVals.end() && it->second == "1") {
                    bits += '1';
                } else {
                    bits += '0';
                }
            }
            encodes.push_back(bits);
        }
        return encodes;
    }

    std::map<std::string, std::string> getEncodeMap() {
        std::map<std::string, std::string> result;
        auto encodes = getEncodes();
        for (int i = 0; i < (int)pKeys_.size() && i < (int)encodes.size(); ++i) {
            result[pKeys_[i]] = encodes[i];
        }
        return result;
    }

    int getMaxExtSize() const { return maxExtSize_; }
    const std::vector<double> &getPLog2() const { return pLog2_; }
    const std::vector<std::string> &getPKeys() const { return pKeys_; }

    static std::string keyOf(double v, int prec = 6) {
        return probToKey(v, prec);
    }
};

#endif
