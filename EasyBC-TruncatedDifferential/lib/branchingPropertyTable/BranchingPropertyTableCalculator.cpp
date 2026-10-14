

#include "branchingPropertyTable/BranchingPropertyTableCalculator.h"

#include <z3++.h>

#include <numeric>
#include <set>
#include <cmath>
#include <iostream>
#include <iomanip>

extern std::map<std::string, std::vector<int>> pboxM;
extern std::map<std::string, int> pboxMSize;
extern std::map<std::string, std::vector<int>> Ffm;
extern std::string cipherName;

BPTCalculator::BPTCalculator(
    std::string name,

    const std::vector<int> &Matrix,

    const std::vector<int> &FFm) : name_(std::move(name)) {

    int rowSizeM = int(sqrt(Matrix.size())), rowSizeF = int(sqrt(FFm.size())), rowCounter = 0;

    std::vector<int> tempRow;

    this->t = rowSizeM;
    this->m = log2(rowSizeF);

    for (int i : Matrix) {
        tempRow.push_back(i);
        rowCounter++;
        if (rowCounter == rowSizeM) {
            this->M.push_back(tempRow);
            tempRow.clear();
            rowCounter = 0;
        }
    }

    for (int i : FFm) {
        tempRow.push_back(i);
        rowCounter++;
        if (rowCounter == rowSizeF) {
            this->ffmMultiplicationTable.push_back(tempRow);
            tempRow.clear();
            rowCounter = 0;
        }
    }

    this->Path = this->Path + cipherName + "/";

    system(("mkdir -p ../data/BPT/" + cipherName).c_str());

    this->BPTPath = this->Path + name_ + "_BPT.txt";

    system(("touch " + this->BPTPath).c_str());
}

int BPTCalculator::gf_multiply(int a, int b) const {
    if (m == 1)
        return b;
    else
        return this->ffmMultiplicationTable[a][b];
}

int BPTCalculator::nearest_power_of_2(double value) {

    if (value <= 1) {
        return 1;
    }

    int counter = 0;
    while (true) {
        int power = counter * this->m;
        if (power - ceil(value) >= 0) {
            break;
        }
        counter++;
    }

    double diff1 = value - (counter - 1) * this->m;
    double diff2 = counter * this->m - value;
    if (diff1 < diff2) {
        return (counter - 1) * this->m;
    } else {
        return counter * this->m;
    }
}

void BPTCalculator::sage_inGen() {
    int numPatterns = 1 << t;

    std::vector<std::string> impossP;
    std::vector<std::string> possP;

    std::vector<std::string> impossP_cnf;
    std::vector<std::string> possP_cnf;

    sageInPath_ = this->Path + "/sage/";
    system(("mkdir -p " + sageInPath_).c_str());
    sageInPath_ += cipherName + "sageIn.sage";
    system(("touch " + sageInPath_).c_str());

    sageOutPath_ = this->Path + "/sage/";
    system(("mkdir -p " + sageOutPath_).c_str());
    sageOutPath_ += cipherName + "sageOut.txt";
    system(("touch " + sageOutPath_).c_str());

    std::unordered_map<std::string, int> possT;
    std::unordered_map<std::string, int> possT_cnf;

    for (int i = 0; i < numPatterns; i++) {

        std::bitset<BPT_LENGTH> xIdx = i;

        for (int j = 0; j < numPatterns; j++) {
            if (this->useOneHotEncodingMode_) {
                if (this->BPT[i][j] <= 0) {
                    continue;
                }

                std::bitset<BPT_LENGTH> yIdx = j;
                std::string poss = xIdx.to_string().substr(BPT_LENGTH - t, t) +
                                   yIdx.to_string().substr(BPT_LENGTH - t, t);
                std::string poss_cnf = poss;

                int k = (this->BPT[i][j] <= 1) ? 0 : (this->BPT[i][j] / this->m);
                if (k < 0) {
                    k = 0;
                } else if (k > this->weightnum_cnf) {
                    k = this->weightnum_cnf;
                }

                std::string enc;
                enc.reserve(this->weightnum_cnf);
                for (int s = 0; s < this->weightnum_cnf; ++s) {
                    enc += (s >= this->weightnum_cnf - k) ? '1' : '0';
                }

                poss += enc;
                poss_cnf += enc;
                possP.push_back(poss);
                possT[poss] = 1;
                possT_cnf[poss_cnf] = 1;
                possP_cnf.push_back(poss_cnf);
                continue;
            }

            bool isPossible = false;
            double targetVal = 0.0;

            if (this->useIntegerMaxSMTMode_) {
                if (this->BPT[i][j] > 0) {
                    isPossible = true;

                    targetVal = (this->BPT[i][j] == 1) ? 0.0 : static_cast<double>(this->BPT[i][j]);
                }
            } else {
                std::cout << BPTDecimal_.empty() << std::endl;

                if (this->BPTDecimal_[i][j] != -INFINITY) {
                    isPossible = true;
                    targetVal = this->BPTDecimal_[i][j];
                }
            }

            if (isPossible) {
                std::bitset<BPT_LENGTH> yIdx = j;
                std::string poss = xIdx.to_string().substr(BPT_LENGTH - t, t) + yIdx.to_string().substr(BPT_LENGTH - t, t);
                std::string poss_cnf = poss;

                std::string enc = "";
                if (this->maxSmtEncodeMap_.find(targetVal) != this->maxSmtEncodeMap_.end()) {
                    enc = this->maxSmtEncodeMap_[targetVal];
                } else {
                    enc = std::string(this->weightnum_cnf, '0');
                }

                poss += enc;
                poss_cnf += enc;

                possP.push_back(poss);
                possT[poss] = 1;

                possT_cnf[poss_cnf] = 1;
                possP_cnf.push_back(poss_cnf);
            }
        }
    }

    for (int i = 0; i < ((1 << (t * 2) + extWeighted_.size())); i++) {

        std::bitset<BPT_LENGTH * 2> enumer = i;

        std::string imposs = enumer.to_string().substr(BPT_LENGTH * 2 - (t * 2) - extWeighted_.size(),
                                                       (t * 2) + extWeighted_.size());

        if (possT[imposs] == 1)
            continue;
        else
            impossP.push_back(imposs);
    }

    for (int i = 0; i < (1 << ((t * 2) + weightnum_cnf)); i++) {
        std::bitset<BPT_LENGTH * 2> enumer = i;
        std::string imposs_cnf = enumer.to_string().substr(BPT_LENGTH * 2 - (t * 2) - weightnum_cnf,
                                                           (t * 2) + weightnum_cnf);
        if (possT_cnf[imposs_cnf] == 1)
            continue;
        else
            impossP_cnf.push_back(imposs_cnf);
    }

    possPm_ = possP;
    impossPm_ = impossP;

    possPm_cnf = possP_cnf;
    impossPm_cnf = impossP_cnf;

    std::ofstream file_save;
    file_save.open(sageInPath_);
    file_save << "points = [\n";
    for (int i = 0; i < possP.size(); ++i) {
        file_save << '[';
        for (int j = 0; j < possP[i].size(); ++j) {
            if (j != possP[i].size() - 1)
                file_save << possP[i][j] << ", ";
            else
                file_save << possP[i][j];
        }
        if (i != possP.size() - 1)
            file_save << "],\n";
        else
            file_save << "]\n";
    }
    file_save << "]\n";

    file_save << "poly = Polyhedron(vertices = points)\n";

    file_save << "print('--- Inequalities ---')\n";
    file_save << "for l in poly.inequality_generator():\n";
    file_save << "    print(l)\n";

    file_save << "print('--- Equations ---')\n";
    file_save << "for e in poly.equation_generator():\n";
    file_save << "    print(e)\n";

    file_save.close();
    printf("Finish sage_inGen!");
}

void BPTCalculator::possBintoFile() {

    std::ofstream file;
    std::string file_path = Path + "poss/";
    system(("mkdir -p " + file_path).c_str());
    file_path += name_ + ".txt";
    system(("touch " + file_path).c_str());
    file.open(file_path, std::ios::out);
    for (auto poss : possPm_) {
        file << poss << "\n";
    }
    file.close();

    file_path = Path + "poss_cnf/";
    system(("mkdir -p " + file_path).c_str());
    file_path += name_ + ".txt";
    system(("touch " + file_path).c_str());
    file.open(file_path, std::ios::out);
    for (auto poss : possPm_cnf) {
        file << poss << "\n";
    }
    file.close();

    file_path = Path + "imposs_cnf/";
    system(("mkdir -p " + file_path).c_str());
    file_path += name_ + ".txt";
    system(("touch " + file_path).c_str());
    file.open(file_path, std::ios::out);
    for (auto imposs : impossPm_cnf) {
        file << imposs << "\n";
    }
    file.close();

    file_path = Path + "imposs/";
    system(("mkdir -p " + file_path).c_str());
    file_path += name_ + ".txt";
    system(("touch " + file_path).c_str());

    this->impossPath = file_path;

    file.open(file_path, std::ios::out);

    for (auto imposs : impossPm_) {
        file << imposs << "\n";
    }
    file.close();
    printf("Finish possBintoFile!");

    this->BPTCnf = convertToInts(impossPm_cnf);
}

void BPTCalculator::sage_ext() {

    std::system((std::string("sage ") + this->sageInPath_ + " > " + this->sageOutPath_).c_str());
    std::ifstream file_extract;
    file_extract.open(this->sageOutPath_);
    std::string temp;

    bool is_equation_section = false;

    while (getline(file_extract, temp)) {

        if (temp.find("--- Equations ---") != std::string::npos) {
            is_equation_section = true;
            continue;
        }
        if (temp.find("--- Inequalities ---") != std::string::npos) {
            is_equation_section = false;
            continue;
        }

        if (temp.empty() || temp.find("An ") == std::string::npos) {
            continue;
        }

        std::string tt = "";

        bool fl = false;
        for (auto i : temp) {
            if (i == '(')
                fl = true;
            else if (i == ')')
                fl = false;
            if (fl && i != '(' && i != ',') {
                tt = tt + i;
            } else if (i == 'x') {

                char sign = temp[temp.find(i) + 2];

                int a = temp.find(i) + 4;
                if (temp[a + 1] == ' ')
                    tt = tt + " " + sign + temp[a];
                else if (temp[a + 2] == ' ')
                    tt = tt + " " + sign + temp[a] + temp[a + 1];
                else if (temp[a + 3] == ' ')
                    tt = tt + " " + sign + temp[a] + temp[a + 1] + temp[a + 2];
                else if (temp[a + 4] == ' ')
                    tt = tt + " " + sign + temp[a] + temp[a + 1] + temp[a + 2] + temp[a + 3];
            }
        }

        std::vector<int> tiq;

        std::vector<std::string> ts = utilities::split(tt, " ");
        for (const auto &i : ts) {

            if (!i.empty()) {
                tiq.push_back(std::stoi(i));
            }
        }

        if (is_equation_section) {
            this->sageEqns_.push_back(tiq);
        } else {
            this->sageIneqs_.push_back(tiq);
        }
    }
    file_extract.close();
}

void BPTCalculator::bpt_cnf_red() {
    std::string file_path;

    file_path = Path + "bpt_cnf_red/" + name_ + "/";
    system(("mkdir -p " + file_path).c_str());
    system(("rm -rf " + file_path + "*").c_str());

    std::string file_path_in, file_path_ex, file_path_type;
    file_path_in = file_path + "include.txt";
    system(("touch " + file_path_in).c_str());
    file_path_ex = file_path + "exclude.txt";
    system(("touch " + file_path_ex).c_str());
    file_path_type = file_path + "type";
    system(("touch " + file_path_type).c_str());

    std::ifstream inputFile;

    file_path = Path + "poss_cnf/" + name_ + ".txt";

    inputFile.open(file_path, std::ios::in);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open inputFile include!" << std::endl;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inputFile, line)) {

        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        lines.push_back(line);
    }
    inputFile.close();

    int N = lines.size();
    int n = lines.empty() ? 0 : lines[0].length();
    int size = n;

    std::ofstream outputFile;
    outputFile.open(file_path_type, std::ios::out);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to create outputFile type!" << std::endl;
    }
    outputFile << "explicit binary";
    outputFile.close();

    outputFile.open(file_path_in, std::ios::out);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to create outputFile include!" << std::endl;
    }

    outputFile << N << " " << n << std::endl;

    for (const auto &l : lines) {
        for (size_t i = 0; i < l.length(); ++i) {
            outputFile << l[i];
            if (i != l.length() - 1) {
                outputFile << " ";
            }
        }
        outputFile << std::endl;
    }

    outputFile.close();

    file_path = Path + "imposs_cnf/" + name_ + ".txt";

    inputFile.open(file_path, std::ios::in);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open inputFile exclude!" << std::endl;
    }

    lines.clear();
    while (std::getline(inputFile, line)) {

        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        lines.push_back(line);
    }
    inputFile.close();

    N = lines.size();
    n = lines.empty() ? 0 : lines[0].length();

    outputFile.open(file_path_ex, std::ios::out);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to create outputFile exclude!" << std::endl;
    }

    outputFile << N << " " << n << std::endl;

    for (const auto &l : lines) {
        for (size_t i = 0; i < l.length(); ++i) {
            outputFile << l[i];
            if (i != l.length() - 1) {
                outputFile << " ";
            }
        }
        outputFile << std::endl;
    }

    outputFile.close();

    file_path = Path + "bpt_cnf_red/" + name_ + "/";

    system(("mkdir -p " + file_path).c_str());

    system(("bzip2 -c " + file_path_in + " > " + file_path + "include.bz2").c_str());

    system(("bzip2 -c " + file_path_ex + " > " + file_path + "exclude.bz2").c_str());

    std::string cmd = "optimodel.boolean --cnf " + file_path + " MaxCubes SubsetMILP:solver=gurobi";

    system(cmd.c_str());

    std::string directory = file_path;

    std::string filename = find_matching_file(directory);
    if (filename.empty()) {
        std::cerr << "No matching file found in directory: " << directory << std::endl;
    }

    std::cout << "Found file: " << filename << std::endl;

    std::vector<std::vector<int>> red_cnf = read_and_generate_red_cnf(filename, size);

    std::cout << "Generated red_cnf:" << std::endl;
    for (const auto &row : red_cnf) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    this->BPTCnf = red_cnf;
}

static std::pair<std::vector<int>, long long> computeScaledTargets(const std::vector<double> &minusLog2Probs, int alpha) {
    std::vector<int> scaledTargets;
    long long beta_out = 1;

    if (alpha <= 0) {

        for (double p : minusLog2Probs) {
            scaledTargets.push_back(static_cast<int>(std::round(p)));
        }
    } else {

        long long S = 1;
        for (int i = 0; i < alpha; ++i)
            S *= 10;

        long long beta = 1;
        std::vector<long long> numerators;
        for (double p : minusLog2Probs) {
            long long N_i = static_cast<long long>(std::round(p * S));
            numerators.push_back(N_i);
            long long current_gcd = std::gcd(N_i, S);
            long long b_i = S / current_gcd;
            beta = std::lcm(beta, b_i);
        }
        for (long long N_i : numerators) {
            int target = static_cast<int>((N_i * beta) / S);
            scaledTargets.push_back(target);
        }
        beta_out = beta;
    }
    return {scaledTargets, beta_out};
}

void BPTCalculator::prepareMILPEncoding(BptEncodingType encodingType, int alpha) {
    const bool useOneHot = encodingType == BptEncodingType::ONE_HOT;
    const bool useIntegerBPT = useOneHot || alpha == 0;

    this->useOneHotEncodingMode_ = useOneHot;
    this->useIntegerMaxSMTMode_ = useIntegerBPT;
    this->beta_ = 1;
    this->maxSmtEncodeMap_.clear();
    this->extWeighted_.clear();

    std::cout << "[DBG] t=" << this->t << " m=" << this->m
              << " useIntegerBPT=" << useIntegerBPT << std::endl;
    this->computeBranchingTableLA(useIntegerBPT);
    std::cout << "[DBG] BPTDecimal_ size=" << this->BPTDecimal_.size() << std::endl;

    std::cout << "LA finish!!" << std::endl;

    if (useOneHot) {
        this->weightnum_cnf = std::max(0, this->t - 1);
        this->extWeighted_.assign(this->weightnum_cnf, 1);
    } else {
        this->computeMaxSMTEncoding(useIntegerBPT, alpha);
    }

    this->sage_inGen();
    this->possBintoFile();
    this->sage_ext();
}

void BPTCalculator::prepareSATEncoding(BptEncodingType encodingType, int alpha) {
    const bool useOneHot = encodingType == BptEncodingType::ONE_HOT;
    const bool useIntegerBPT = useOneHot || alpha == 0;

    this->useOneHotEncodingMode_ = useOneHot;
    this->useIntegerMaxSMTMode_ = useIntegerBPT;
    this->beta_ = 1;
    this->maxSmtEncodeMap_.clear();
    this->extWeighted_.clear();

    std::cout << "[DBG] t=" << this->t << " m=" << this->m
              << " useIntegerBPT=" << useIntegerBPT << std::endl;
    this->computeBranchingTableLA(useIntegerBPT);
    std::cout << "[DBG] BPTDecimal_ size=" << this->BPTDecimal_.size() << std::endl;

    std::cout << "LA finish!!" << std::endl;

    if (useOneHot) {
        this->weightnum_cnf = std::max(0, this->t - 1);
        this->extWeighted_.assign(this->weightnum_cnf, 1);
    } else {
        this->computeMaxSMTEncoding(useIntegerBPT, alpha);
    }

    this->sage_inGen();
    this->possBintoFile();
    this->bpt_cnf_red();
}

void BPTCalculator::computeMaxSMTEncoding(bool useIntegerBPT, int alpha, int maxExtSize) {

    this->useIntegerMaxSMTMode_ = useIntegerBPT;

    std::vector<double> rawProbs;
    std::set<double> seenRaw;

    if (useIntegerBPT) {

        for (const auto &row : this->BPT) {
            for (int val : row) {
                if (val == 0)
                    continue;

                double mathTarget = (val == 1) ? 0.0 : static_cast<double>(val);

                if (seenRaw.insert(mathTarget).second) {
                    rawProbs.push_back(mathTarget);
                }
            }
        }

        alpha = 0;
    } else {

        for (const auto &row : this->BPTDecimal_) {
            for (double val : row) {

                if (val != -INFINITY && seenRaw.insert(val).second) {
                    rawProbs.push_back(val);
                }
            }
        }
    }

    if (rawProbs.empty())
        return;

    auto scaledResult = computeScaledTargets(rawProbs, alpha);
    std::vector<int> rawTargets = scaledResult.first;
    long long beta = scaledResult.second;
    this->beta_ = beta;

    std::vector<int> uniqueTargets;
    std::set<int> seenTargets;

    std::cout << "合并后概率为：\n ";
    for (int t : rawTargets) {
        if (seenTargets.insert(t).second) {
            uniqueTargets.push_back(t);
            std::cout << t << " ";
        }
    }
    std::cout << std::endl;

    int numProbs = uniqueTargets.size();
    if (maxExtSize <= 0)
        maxExtSize = numProbs;

    z3::context ctx;
    z3::optimize opt(ctx);
    std::string smtLib;

    for (int i = 0; i < maxExtSize; ++i)
        smtLib += "(declare-const c_" + std::to_string(i + 1) + " Int)\n";

    for (int i = 0; i < maxExtSize; ++i) {
        for (int j = 0; j < numProbs; ++j) {
            std::string pij = "p_" + std::to_string(i + 1) + "_" + std::to_string(j + 1);
            smtLib += "(declare-const " + pij + " Int)\n";
            smtLib += "(assert (>= " + pij + " 0))\n";
            smtLib += "(assert (<= " + pij + " 1))\n";
        }
    }

    int maxTargetVal = 0;
    for (int p : uniqueTargets) {
        if (p > maxTargetVal)
            maxTargetVal = p;
    }
    for (int i = 0; i < maxExtSize; ++i) {
        std::string ci = "c_" + std::to_string(i + 1);
        smtLib += "(assert (>= " + ci + " 0))\n";
        smtLib += "(assert (<= " + ci + " " + std::to_string(maxTargetVal) + "))\n";

        if (i < maxExtSize - 1) {
            std::string cnext = "c_" + std::to_string(i + 2);
            smtLib += "(assert (>= " + ci + " " + cnext + "))\n";
        }
    }

    for (int j = 0; j < numProbs; ++j) {
        std::string sumExpr = "(+ ";
        for (int i = 0; i < maxExtSize; ++i) {
            sumExpr += "(* c_" + std::to_string(i + 1) + " p_" + std::to_string(i + 1) + "_" + std::to_string(j + 1) + ") ";
        }
        sumExpr += ")";
        smtLib += "(assert (= " + std::to_string(uniqueTargets[j]) + " " + sumExpr + "))\n";
    }

    for (int i = 0; i < maxExtSize; ++i) {
        smtLib += "(assert-soft (= c_" + std::to_string(i + 1) + " 0) :weight 1)\n";
    }

    opt.from_string(smtLib.c_str());
    z3::params p(ctx);
    p.set(":timeout", 60000u);
    opt.set(p);

    if (opt.check() == z3::sat) {
        z3::model m = opt.get_model();

        std::cout << "\n======================================================\n";
        std::cout << "[MaxSMT] 求解成功！初始分配编码位宽 (maxExtSize) = " << maxExtSize << "\n";
        if (alpha > 0) {
            std::cout << "[MaxSMT] 当前为精确小数模式 (alpha=" << alpha << ")，计算所得缩放系数 β = " << beta << "\n";
        } else {
            std::cout << "[MaxSMT] 当前为近似整数模式，无需缩放 (β = 1)\n";
        }
        std::cout << "------------------------------------------------------\n";

        this->extWeighted_.clear();
        std::vector<int> activeIndices;

        std::cout << "[基权重 (c_i)]\n";
        for (int i = 0; i < maxExtSize; ++i) {
            std::cout << std::setw(8) << ("c_" + std::to_string(i + 1));
        }
        std::cout << "\n";

        for (int i = 0; i < maxExtSize; ++i) {
            z3::expr c_var = ctx.int_const(("c_" + std::to_string(i + 1)).c_str());
            int c_val = m.eval(c_var, true).get_numeral_int();
            std::cout << std::setw(8) << c_val;

            if (c_val > 0) {
                this->extWeighted_.push_back(c_val);
                activeIndices.push_back(i);
            }
        }
        std::cout << "\n\n";

        std::map<int, std::string> targetEncodeMap;
        for (int j = 0; j < numProbs; ++j) {
            std::string enc = "";
            for (int idx : activeIndices) {
                z3::expr p_var = ctx.int_const(("p_" + std::to_string(idx + 1) + "_" + std::to_string(j + 1)).c_str());
                int p_val = m.eval(p_var, true).get_numeral_int();
                enc += (p_val == 1) ? "1" : "0";
            }
            targetEncodeMap[uniqueTargets[j]] = enc;
        }

        this->maxSmtEncodeMap_.clear();
        std::cout << "[概率编码映射 (包含精度截断后的自动合并)]\n";
        std::cout << std::setw(12) << "Original P" << std::setw(12) << "Scaled Tar";
        for (int idx : activeIndices) {
            std::cout << std::setw(8) << ("p_" + std::to_string(idx + 1));
        }
        std::cout << "\n";

        for (size_t k = 0; k < rawProbs.size(); ++k) {
            double origP = rawProbs[k];
            int scaledT = rawTargets[k];
            std::string enc = targetEncodeMap[scaledT];

            this->maxSmtEncodeMap_[origP] = enc;

            std::cout << std::setw(12) << std::fixed << std::setprecision(3) << origP
                      << std::setw(12) << scaledT;
            for (char bit : enc) {
                std::cout << std::setw(8) << bit;
            }
            std::cout << "\n";
        }
        std::cout << "======================================================\n\n";

        this->weightnum_cnf = activeIndices.size();
        std::cout << "[MaxSMT] 优化完成！实际参与 MILP 编码的布尔变量数缩减为: " << this->weightnum_cnf << "\n";

    } else {
        std::cerr << "[MaxSMT] 求解失败或超时，回退为空。" << std::endl;
    }
}
