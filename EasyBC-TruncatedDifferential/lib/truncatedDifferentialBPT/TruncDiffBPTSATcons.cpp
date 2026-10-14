

#include "truncatedDifferentialBPT/TruncDiffBPTSATcons.h"
#include <truncatedDifferentialBPT/TruncDiffBPTSWSAT.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

void TruncDiffBPTSATcons::wXor(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of wXOR ! " << std::endl;
    } else {
        scons << inputIdx1 << " " << inputIdx2 << " " << -(outputIdx) << " 0\n";
        scons << inputIdx1 << " " << -(inputIdx2) << " " << outputIdx << " 0\n";
        scons << -(inputIdx1) << " " << inputIdx2 << " " << outputIdx << " 0\n";

        clauseCounter += 3;
    }
    scons.close();
}

void TruncDiffBPTSATcons::wXor2(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int xorPIdx, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of wXOR ! " << std::endl;
        return;
    }

    scons << inputIdx1 << " " << inputIdx2 << " " << -(outputIdx) << " 0\n";
    scons << inputIdx1 << " " << -(inputIdx2) << " " << outputIdx << " 0\n";
    scons << -(inputIdx1) << " " << inputIdx2 << " " << outputIdx << " 0\n";
    clauseCounter += 3;

    scons << "-" << xorPIdx << " " << inputIdx1 << " 0\n";
    scons << "-" << xorPIdx << " " << inputIdx2 << " 0\n";
    scons << "-" << xorPIdx << " -" << outputIdx << " 0\n";

    scons << "-" << inputIdx1 << " -" << inputIdx2 << " " << outputIdx << " " << xorPIdx << " 0\n";

    clauseCounter += 4;
    scons.close();
}

void TruncDiffBPTSATcons::wAndOr(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of bAndC1 ! " << std::endl;
    } else {
        scons << inputIdx1 << " " << inputIdx2 << " -" << outputIdx << " 0\n";
        clauseCounter++;
    }
    scons.close();
}

void TruncDiffBPTSATcons::n_wxor(std::string path, std::vector<int> inputIdx, int outputIdx, int &ACounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of wXOR ! " << std::endl;
    } else {
        int inputIdx1 = inputIdx[0];
        int inputIdx2 = inputIdx[1];
        int outputIdx_ = ACounter;

        wXor(path, inputIdx1, inputIdx2, outputIdx_, clauseCounter);

        ACounter++;

        for (int i = 2; i < inputIdx.size() - 1; i++) {
            inputIdx1 = outputIdx_;
            inputIdx2 = inputIdx[i];
            outputIdx_ = ACounter;
            wXor(path, inputIdx1, inputIdx2, outputIdx_, clauseCounter);
            ACounter++;
        }

        inputIdx1 = outputIdx_;
        inputIdx2 = inputIdx[inputIdx.size() - 1];
        wXor(path, inputIdx1, inputIdx2, outputIdx, clauseCounter);
    }
    scons.close();
}

void TruncDiffBPTSATcons::branch(std::string path, int inputIdx, int outputIdx1, int outputIdx2, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of branch ! " << std::endl;
    } else {
        scons << inputIdx << " " << outputIdx1 << " 0\n";
        scons << inputIdx << " " << -outputIdx1 << " 0\n";
        scons << inputIdx << " " << outputIdx2 << " 0\n";
        scons << inputIdx << " " << -outputIdx2 << " 0\n";
        clauseCounter += 4;
    }
    scons.close();
}

void TruncDiffBPTSATcons::wLinear(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, int branchMin, int branchMax, int &ACounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of branch ! " << std::endl;
    } else {

        int n = inputIdx.size();
        std::vector<std::vector<int>> clauses = generateCombinations(2 * n, 1, branchMin - 1);
        for (auto clause : clauses) {
            for (int i = 0; i < clause.size(); i++) {
                if (i < inputIdx.size()) {
                    if (clause[i] == 0) {
                        scons << inputIdx[i] << " ";
                    } else if (clause[i] == 1) {
                        scons << "-" << inputIdx[i] << " ";
                    }
                } else if (i < inputIdx.size() + outputIdx.size()) {
                    if (clause[i] == 0) {
                        scons << outputIdx[i - inputIdx.size()] << " ";
                    } else if (clause[i] == 1) {
                        scons << "-" << outputIdx[i - inputIdx.size()] << " ";
                    }
                }
            }
            scons << "0\n";
            clauseCounter++;
        }

        if (n > branchMin) {
            std::vector<std::vector<int>> clauses_back = generateCombinations_back(2 * n, branchMin, n);
            for (auto clause : clauses_back) {
                for (int i = 0; i < clause.size(); i++) {
                    if (i < inputIdx.size()) {
                        if (clause[i] == 0) {
                            scons << inputIdx[i] << " ";
                        } else if (clause[i] == 1) {
                            scons << "-" << inputIdx[i] << " ";
                        }
                    } else if (i < inputIdx.size() + outputIdx.size()) {
                        if (clause[i] == 0) {
                            scons << outputIdx[i - inputIdx.size()] << " ";
                        } else if (clause[i] == 1) {
                            scons << "-" << outputIdx[i - inputIdx.size()] << " ";
                        }
                    }
                }
                scons << "0\n";
                clauseCounter++;
            }
        }
    }
    scons.close();
}

void TruncDiffBPTSATcons::wLinear_2(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, std::vector<int> &Matrix, int &ACounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of branch ! " << std::endl;
    } else {
        int n = inputIdx.size();
        std::vector<int> vec_nxor;
        for (int i = 0; i < n; i++) {
            for (int j = n * i; j < n * (i + 1); j++) {
                if (Matrix[j] != 0) {
                    vec_nxor.push_back(inputIdx[j - n * i]);
                }
            }
            n_wxor(path, vec_nxor, outputIdx[i], ACounter, clauseCounter);
            vec_nxor.clear();
        }
    }
    scons.close();
}

std::vector<std::vector<int>> TruncDiffBPTSATcons::generateCombinations(int n, int minOnes, int maxOnes) {
    std::vector<std::vector<int>> result;

    for (int mask = 0; mask < (1 << n); ++mask) {
        int count = 0;

        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) {
                ++count;
            }
        }

        if (count >= minOnes && count <= maxOnes) {
            std::vector<int> combination;
            for (int i = 0; i < n; ++i) {
                combination.push_back((mask >> i) & 1);
            }
            result.push_back(combination);
        }
    }

    return result;
}

vector<vector<int>> TruncDiffBPTSATcons::generateCombinations_back(int n, int minOnes, int maxOnes) {
    vector<vector<int>> result;
    int half = n / 2;
    int totalCombinations = 1 << half;

    for (int mask = 0; mask < totalCombinations; ++mask) {
        int count = 0;

        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) {
                ++count;
            }
        }

        if (count >= minOnes && count <= maxOnes) {
            vector<int> combination(n, 0);

            for (int i = 0; i < half; ++i) {
                combination[half + i] = (mask >> i) & 1;
            }
            result.push_back(combination);
        }
    }

    return result;
}

vector<vector<int>> TruncDiffBPTSATcons::generateCombinations_front(int n, int minOnes, int maxOnes) {
    vector<vector<int>> result;
    int half = n / 2;
    int totalCombinations = 1 << half;

    for (int mask = 0; mask < totalCombinations; ++mask) {
        int count = 0;

        for (int i = 0; i < half; ++i) {
            if (mask & (1 << i)) {
                ++count;
            }
        }

        if (count >= minOnes && count <= maxOnes) {
            vector<int> combination(n, 0);

            for (int i = 0; i < half; ++i) {
                combination[i] = (mask >> i) & 1;
            }

            result.push_back(combination);
        }
    }

    return result;
}

void TruncDiffBPTSATcons::SequentialEncoding(std::string path, std::vector<int> &sbox_vector, std::vector<std::vector<int>> &auxiliary_var_u,
                                             int var_num, int cardinalitycons, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of sequentialEncoding ! " << std::endl;
    } else {

        int n = var_num;
        int k = cardinalitycons;
        std::vector<int> x = sbox_vector;
        std::vector<std::vector<int>> u = auxiliary_var_u;

        if (k > 0) {
            scons << "-" << x[0] << " " << u[0][0] << " 0\n";
            clauseCounter++;
            for (int j = 1; j < k; j++) {
                scons << "-" << u[0][j] << " 0\n";
                clauseCounter++;
            }
            for (int i = 1; i < n - 1; i++) {
                scons << "-" << x[i] << " " << u[i][0] << " 0\n";
                clauseCounter++;
                scons << "-" << u[i - 1][0] << " " << u[i][0] << " 0\n";
                clauseCounter++;
                scons << "-" << x[i] << " " << "-" << u[i - 1][k - 1] << " 0\n";
                clauseCounter++;
            }
            for (int j = 1; j < k; j++) {
                for (int i = 1; i < n - 1; i++) {
                    scons << "-" << x[i] << " " << "-" << u[i - 1][j - 1] << " " << u[i][j] << " 0\n";
                    clauseCounter++;
                    scons << "-" << u[i - 1][j] << " " << u[i][j] << " 0\n";
                    clauseCounter++;
                }
            }
            scons << "-" << x[n - 1] << " " << "-" << u[n - 2][k - 1] << " 0\n";
            clauseCounter++;
        }

        if (k == 0) {
            for (int i = 0; i < n; i++) {
                scons << "-" << x[i] << " 0\n";
                clauseCounter++;
            }
        }
    }
    scons.close();
}

void TruncDiffBPTSATcons::SequentialEncoding_least(std::string path, std::vector<int> &sbox_vector, std::vector<std::vector<int>> &auxiliary_var_u,
                                                   int var_num, int cardinalitycons, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of sequentialEncoding ! " << std::endl;
    } else {
        int n = var_num;
        int k = n - cardinalitycons;
        std::vector<int> x = sbox_vector;
        std::vector<std::vector<int>> u = auxiliary_var_u;
        if (k > 0) {
            scons << x[0] << " " << u[0][0] << " 0\n";
            clauseCounter++;
            for (int j = 1; j < k; j++) {
                scons << "-" << u[0][j] << " 0\n";
                clauseCounter++;
            }
            for (int i = 1; i < n - 1; i++) {
                scons << x[i] << " " << u[i][0] << " 0\n";
                clauseCounter++;
                scons << "-" << u[i - 1][0] << " " << u[i][0] << " 0\n";
                clauseCounter++;
                scons << x[i] << " " << "-" << u[i - 1][k - 1] << " 0\n";
                clauseCounter++;
            }
            for (int j = 1; j < k; j++) {
                for (int i = 1; i < n - 1; i++) {
                    scons << x[i] << " " << "-" << u[i - 1][j - 1] << " " << u[i][j] << " 0\n";
                    clauseCounter++;
                    scons << "-" << u[i - 1][j] << " " << u[i][j] << " 0\n";
                    clauseCounter++;
                }
            }
            scons << x[n - 1] << " " << "-" << u[n - 2][k - 1] << " 0\n";
            clauseCounter++;
        }
        if (k == 0) {
            for (int i = 0; i < n; i++) {
                scons << x[i] << " 0\n";
                clauseCounter++;
            }
        }
    }
    scons.close();
}

void TruncDiffBPTSATcons::bXor(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of bXOR ! " << std::endl;
    } else {
        scons << inputIdx1 << " " << inputIdx2 << " -" << outputIdx << " 0\n";
        scons << inputIdx1 << " -" << inputIdx2 << " " << outputIdx << " 0\n";
        scons << -inputIdx1 << " " << inputIdx2 << " " << outputIdx << " 0\n";
        scons << -inputIdx1 << " -" << inputIdx2 << " -" << outputIdx << " 0\n";
        clauseCounter += 4;
    }
    scons.close();
}

void TruncDiffBPTSATcons::bSbox(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, std::vector<std::vector<int>> sboxcnf,
                                int &ACounter, std::vector<int> &AtoSbox, bool ifInjective, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {

        for (int i = 0; i < inputIdx.size(); i++) {
            scons << "-" << inputIdx[i] << " " << AtoSbox[ACounter - 1] << " 0\n";
            clauseCounter++;
        }
        scons << -AtoSbox[ACounter - 1];
        for (int i = 0; i < inputIdx.size(); i++) {
            scons << " " << inputIdx[i];
        }
        scons << " 0\n";
        clauseCounter++;
        ACounter++;

        for (auto sboxc : sboxcnf) {
            for (int i = 0; i < sboxc.size(); i++) {
                if (i < inputIdx.size()) {
                    if (sboxc[i] == 0) {
                        scons << "-" << inputIdx[i] << " ";
                    } else if (sboxc[i] == 1) {
                        scons << inputIdx[i] << " ";
                    }
                } else if (i < inputIdx.size() + outputIdx.size()) {
                    if (sboxc[i] == 0) {
                        scons << "-" << outputIdx[i - inputIdx.size()] << " ";
                    } else if (sboxc[i] == 1) {
                        scons << outputIdx[i - inputIdx.size()] << " ";
                    }
                }
            }
            scons << "0\n";
            clauseCounter++;
        }
        scons.close();
    }
}

void TruncDiffBPTSATcons::n_xor(std::string path, std::vector<int> inputIdx, int outputIdx, int &xCounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of wXOR ! " << std::endl;
    } else {
        int inputIdx1 = inputIdx[0];
        int inputIdx2 = inputIdx[1];
        int outputIdx_ = xCounter;

        bXor(path, inputIdx1, inputIdx2, outputIdx_, clauseCounter);

        xCounter++;

        for (int i = 2; i < inputIdx.size() - 1; i++) {
            inputIdx1 = outputIdx_;
            inputIdx2 = inputIdx[i];
            outputIdx_ = xCounter;
            bXor(path, inputIdx1, inputIdx2, outputIdx_, clauseCounter);
            xCounter++;
        }

        inputIdx1 = outputIdx_;
        inputIdx2 = inputIdx[inputIdx.size() - 1];
        bXor(path, inputIdx1, inputIdx2, outputIdx, clauseCounter);
    }
    scons.close();
}

void TruncDiffBPTSATcons::n_xor_2(std::string path, std::vector<int> inputIdx, int outputIdx, int &xCounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of n_XOR ! " << std::endl;
    } else {
        int n = inputIdx.size();
        std::vector<std::vector<int>> tuples = generateTuples(n);
        for (auto tuple : tuples) {
            for (int i = 0; i < tuple.size(); i++) {
                if (i < inputIdx.size()) {
                    if (tuple[i] == 0) {
                        scons << inputIdx[i] << " ";
                    } else if (tuple[i] == 1) {
                        scons << "-" << inputIdx[i] << " ";
                    }
                } else {
                    if (tuple[i] == 0) {
                        scons << outputIdx << " ";
                    } else if (tuple[i] == 1) {
                        scons << "-" << outputIdx << " ";
                    }
                }
            }
            scons << "0\n";
            clauseCounter++;
        }
    }
    scons.close();
}

bool TruncDiffBPTSATcons::calculateXOR(const vector<int> &tuple) {
    int result = 0;
    for (int bit : tuple) {
        result ^= bit;
    }
    return result == 1;
}

vector<vector<int>> TruncDiffBPTSATcons::generateTuples(int k) {
    vector<vector<int>> result;
    int totalTuples = 1 << (k + 1);

    for (int mask = 0; mask < totalTuples; ++mask) {
        vector<int> tuple;

        for (int i = 0; i <= k; ++i) {
            tuple.push_back((mask >> i) & 1);
        }

        if (TruncDiffBPTSATcons::calculateXOR(tuple)) {
            result.push_back(tuple);
        }
    }

    return result;
}

void TruncDiffBPTSATcons::bMatrixEntry(std::string path, std::vector<std::vector<int>> inputIdx, std::vector<int> outputIdx,
                                       std::vector<std::vector<int>> eq_matrix, int rowcount, int &xCounter, int &clauseCounter) {

    std::vector<int> inputIdx_row;
    std::vector<int> vec_nxor;

    for (int i = 0; i < inputIdx.size(); i++) {

        for (int j = 0; j < inputIdx[i].size(); j++) {
            inputIdx_row.push_back(inputIdx[i][j]);
        }
    }

    for (int i = 0; i < outputIdx.size(); i++) {
        for (int j = 0; j < eq_matrix[rowcount + i].size(); j++) {
            if (eq_matrix[rowcount + i][j] == 1) {
                vec_nxor.push_back(inputIdx_row[j]);
            }
        }
        n_xor_2(path, vec_nxor, outputIdx[i], xCounter, clauseCounter);
        vec_nxor.clear();
    }
}

void TruncDiffBPTSATcons::bAndOr(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of bAndC1 ! " << std::endl;
    } else {
        scons << inputIdx1 << " " << inputIdx2 << " -" << outputIdx << " 0\n";
        clauseCounter++;
    }
    scons.close();
}

void TruncDiffBPTSATcons::bAdd(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx, int &xCounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        int n = inputIdx1.size();

        for (int i = 0; i < inputIdx1.size() - 1; i++) {
            scons << inputIdx1[i] << " " << inputIdx2[i] << " -" << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << inputIdx1[i] << " -" << inputIdx2[i] << " " << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " " << inputIdx2[i] << " " << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " -" << inputIdx2[i] << " -" << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << inputIdx1[i] << " " << inputIdx2[i] << " " << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << inputIdx1[i] << " -" << inputIdx2[i] << " -" << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " " << inputIdx2[i] << " -" << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " -" << inputIdx2[i] << " " << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;
        }

        bXor(path, inputIdx1[n - 1], inputIdx2[n - 1], outputIdx[n - 1], clauseCounter);
    }
    scons.close();
}

void TruncDiffBPTSATcons::bAdd_dp(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx, std::vector<int> &dp_weight, int &xCounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        int n = inputIdx1.size();

        for (int i = 0; i < inputIdx1.size() - 1; i++) {
            scons << inputIdx1[i] << " " << inputIdx2[i] << " -" << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << inputIdx1[i] << " -" << inputIdx2[i] << " " << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " " << inputIdx2[i] << " " << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " -" << inputIdx2[i] << " -" << outputIdx[i]
                  << " " << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << inputIdx1[i] << " " << inputIdx2[i] << " " << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << inputIdx1[i] << " -" << inputIdx2[i] << " -" << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " " << inputIdx2[i] << " -" << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;

            scons << -inputIdx1[i] << " -" << inputIdx2[i] << " " << outputIdx[i]
                  << " -" << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " 0\n";
            clauseCounter++;
        }

        bXor(path, inputIdx1[n - 1], inputIdx2[n - 1], outputIdx[n - 1], clauseCounter);

        for (int i = 0; i < inputIdx1.size() - 1; i++) {
            dp_weight.push_back(xCounter);

            scons << -inputIdx1[i + 1] << " " << outputIdx[i + 1] << " " << xCounter << " 0\n";
            scons << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " " << xCounter << " 0\n";
            scons << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " " << xCounter << " 0\n";
            scons << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " -" << xCounter << " 0\n";
            scons << -inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " -" << xCounter << " 0\n";

            clauseCounter += 5;
            xCounter++;
        }
    }
    scons.close();
}

void TruncDiffBPTSATcons::bAdd_dp_2(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx, std::vector<int> &dp_weight, int &xCounter, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        int n = inputIdx1.size();

        bXor(path, inputIdx1[n - 1], inputIdx2[n - 1], outputIdx[n - 1], clauseCounter);

        for (int i = 0; i < inputIdx1.size() - 1; i++) {
            dp_weight.push_back(xCounter);

            scons << -inputIdx1[i + 1] << " " << outputIdx[i + 1] << " " << xCounter << " 0\n";
            scons << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " " << xCounter << " 0\n";
            scons << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " " << xCounter << " 0\n";
            scons << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " -" << xCounter << " 0\n";
            scons << -inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " -" << xCounter << " 0\n";

            clauseCounter += 5;

            scons << inputIdx2[i] << " -" << xCounter + 1 << " " << xCounter << " 0\n";
            scons << -inputIdx2[i] << " " << xCounter + 1 << " " << xCounter << " 0\n";

            scons << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " " << xCounter + 1 << " 0\n";
            scons << inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " -" << xCounter + 1 << " 0\n";
            scons << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " " << xCounter + 1 << " 0\n";
            scons << -inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " " << xCounter + 1 << " 0\n";

            scons << inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " -" << xCounter + 1 << " 0\n";
            scons << -inputIdx1[i + 1] << " " << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " -" << xCounter + 1 << " 0\n";
            scons << -inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " -" << outputIdx[i + 1] << " " << xCounter + 1 << " 0\n";
            scons << -inputIdx1[i + 1] << " -" << inputIdx2[i + 1] << " " << outputIdx[i + 1] << " -" << xCounter + 1 << " 0\n";

            clauseCounter += 10;
            xCounter += 2;
        }
    }
    scons.close();
}

void TruncDiffBPTSATcons::bSbox_dp(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, std::vector<std::vector<int>> sboxcnf,
                                   std::vector<int> sbox_weight_idx, int sbox_weight_num, bool ifInjective, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {

        for (auto sboxc : sboxcnf) {
            for (int i = 0; i < sboxc.size(); i++) {
                if (i < inputIdx.size()) {
                    if (sboxc[i] == 0) {
                        scons << "-" << inputIdx[i] << " ";
                    } else if (sboxc[i] == 1) {
                        scons << inputIdx[i] << " ";
                    }
                } else if (i < inputIdx.size() + outputIdx.size()) {
                    if (sboxc[i] == 0) {
                        scons << "-" << outputIdx[i - inputIdx.size()] << " ";
                    } else if (sboxc[i] == 1) {
                        scons << outputIdx[i - inputIdx.size()] << " ";
                    }
                } else {
                    if (sboxc[i] == 0) {
                        scons << "-" << sbox_weight_idx[i - inputIdx.size() - outputIdx.size()] << " ";
                    } else if (sboxc[i] == 1) {
                        scons << sbox_weight_idx[i - inputIdx.size() - outputIdx.size()] << " ";
                    }
                }
            }
            scons << "0\n";
            clauseCounter++;
        }
        scons.close();
    }
}

void TruncDiffBPTSATcons::bAndOr_dp(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int weight_idx, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of bAndC1 ! " << std::endl;
    } else {
        scons << -inputIdx1 << " " << weight_idx << " 0\n";
        scons << -inputIdx2 << " " << weight_idx << " 0\n";
        scons << -outputIdx << " " << weight_idx << " 0\n";
        scons << inputIdx1 << " " << inputIdx2 << " " << -weight_idx << " 0\n";
    }
    clauseCounter += 4;
    scons.close();
}

void TruncDiffBPTSATcons::wLinearBPT(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, std::vector<std::vector<int>> bptcnf,
                                     std::vector<int> bpt_weight_idx, int bpt_weight_num, bool ifInjective, int &clauseCounter) {
    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {

        for (auto bptc : bptcnf) {
            for (int i = 0; i < bptc.size(); i++) {
                if (i < inputIdx.size()) {
                    if (bptc[i] == 0) {
                        scons << "-" << inputIdx[i] << " ";
                    } else if (bptc[i] == 1) {
                        scons << inputIdx[i] << " ";
                    }
                } else if (i < inputIdx.size() + outputIdx.size()) {
                    if (bptc[i] == 0) {
                        scons << "-" << outputIdx[i - inputIdx.size()] << " ";
                    } else if (bptc[i] == 1) {
                        scons << outputIdx[i - inputIdx.size()] << " ";
                    }
                } else {
                    if (bptc[i] == 0) {
                        scons << "-" << bpt_weight_idx[i - inputIdx.size() - outputIdx.size()] << " ";
                    } else if (bptc[i] == 1) {
                        scons << bpt_weight_idx[i - inputIdx.size() - outputIdx.size()] << " ";
                    }
                }
            }
            scons << "0\n";
            clauseCounter++;
        }
        scons.close();
    }
}

void TruncDiffBPTSATcons::wPBConstraint(const std::string &path,
                                        const std::vector<int32_t> &literals,
                                        const std::vector<int64_t> &weights,
                                        int64_t bound,
                                        int &ACounter,
                                        int &clauseCounter) {

    if (literals.empty() || weights.empty()) {
        return;
    }

    PBConfig config = make_shared<PBConfigClass>();

    config->pb_encoder = PB_ENCODER::BDD;

    PB2CNF pb2cnf(config);
    std::vector<std::vector<int32_t>> pb_formula;

    int32_t max_var_used = pb2cnf.encodeLeq(weights, literals, bound, pb_formula, ACounter);

    if (pb_formula.empty()) {
        return;
    }

    ACounter = max_var_used + 1;

    std::ofstream scons;

    char write_buffer[65536];
    scons.rdbuf()->pubsetbuf(write_buffer, sizeof(write_buffer));

    scons.open(path, std::ios::app);
    if (!scons.is_open()) {
        std::cout << "Wrong file path in wPBConstraint: " << path << std::endl;
        return;
    }

    std::ostringstream local_buffer;
    for (const auto &clause : pb_formula) {
        for (int32_t lit : clause) {
            local_buffer << lit << " ";
        }
        local_buffer << "0\n";
        clauseCounter++;

        if (local_buffer.tellp() > 8192) {
            scons << local_buffer.str();
            local_buffer.str("");
            local_buffer.clear();
        }
    }

    if (local_buffer.tellp() > 0) {
        scons << local_buffer.str();
    }

    scons.close();
}

void TruncDiffBPTSATcons::wSWCConstraint(std::string path,
                                         const std::vector<int32_t> &literals,
                                         const std::vector<int64_t> &weights,
                                         int64_t bound,
                                         int &ACounter,
                                         int &clauseCounter) {
    if (literals.size() != weights.size()) {
        throw std::invalid_argument(
            "wSWCConstraint: literals and weights must have the same size.");
    }
    if (bound < 0) {
        throw std::invalid_argument("wSWCConstraint: bound must be non-negative.");
    }

    for (int64_t weight : weights) {
        if (weight < 0) {
            throw std::invalid_argument("wSWCConstraint: weights must be non-negative.");
        }
    }

    if (bound > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("wSWCConstraint: bound exceeds int range.");
    }

    std::vector<int> x;
    std::vector<int> w;
    int64_t totalWeight = 0;
    x.reserve(literals.size());
    w.reserve(weights.size());

    std::ofstream scons(path, std::ios::app);
    if (!scons) {
        std::cout << "Wrong file path of wSWCConstraint ! " << std::endl;
        return;
    }

    for (size_t i = 0; i < literals.size(); ++i) {
        if (weights[i] == 0) {
            continue;
        }
        if (weights[i] > static_cast<int64_t>(std::numeric_limits<int>::max())) {
            throw std::invalid_argument("wSWCConstraint: weight exceeds int range.");
        }
        x.push_back(static_cast<int>(literals[i]));
        w.push_back(static_cast<int>(weights[i]));
        totalWeight += weights[i];
    }

    if (x.empty() || totalWeight <= bound) {
        scons.close();
        return;
    }

    int n = static_cast<int>(x.size());
    int k = static_cast<int>(bound);

    if (k == 0) {
        for (int i = 0; i < n; i++) {
            scons << "-" << x[i] << " 0\n";
            clauseCounter++;
        }
        scons.close();
        return;
    }

    for (int i = 0; i < n; ++i) {
        if (w[i] > k) {
            scons << "-" << x[i] << " 0\n";
            clauseCounter++;
        }
    }

    if (n == 1) {
        scons.close();
        return;
    }

    std::vector<std::vector<int>> s;
    s.reserve(n - 1);
    for (int i = 0; i < n - 1; i++) {
        s.push_back(std::vector<int>());
        s.back().reserve(k);
        for (int j = 0; j < k; j++) {
            s.back().push_back(ACounter);
            ACounter++;
        }
    }

    for (int j = 0; j < w[0] && j < k; j++) {
        scons << "-" << x[0] << " " << s[0][j] << " 0\n";
        clauseCounter++;
    }

    for (int i = 1; i < n - 1; i++) {

        for (int j = 0; j < k; j++) {
            scons << "-" << s[i - 1][j] << " " << s[i][j] << " 0\n";
            clauseCounter++;
        }

        for (int j = 0; j < w[i] && j < k; j++) {
            scons << "-" << x[i] << " " << s[i][j] << " 0\n";
            clauseCounter++;
        }

        for (int j = 0; j < k - w[i]; j++) {
            scons << "-" << s[i - 1][j] << " "
                  << "-" << x[i] << " "
                  << s[i][j + w[i]] << " 0\n";
            clauseCounter++;
        }

        if (w[i] <= k) {
            scons << "-" << s[i - 1][k - w[i]] << " "
                  << "-" << x[i] << " 0\n";
            clauseCounter++;
        }
    }

    if (w[n - 1] <= k) {
        scons << "-" << s[n - 2][k - w[n - 1]] << " "
              << "-" << x[n - 1] << " 0\n";
        clauseCounter++;
    }

    scons.close();
}
