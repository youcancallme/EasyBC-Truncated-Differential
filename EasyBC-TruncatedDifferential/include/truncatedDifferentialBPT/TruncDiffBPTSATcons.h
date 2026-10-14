

#ifndef TruncDiffBPTSATcons_H
#define TruncDiffBPTSATcons_H

#include <fstream>
#include "istream"
#include "iostream"
#include "vector"
#include "string"
#include <cstdint>
#include "pblib/pb2cnf.h"

namespace TruncDiffBPTSATcons {
void wXor(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter);

void wXor2(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int xorPIdx, int &clauseCounter);

void wAndOr(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter);

void wLinear(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, int branchMin, int branchMax, int &ACounter, int &clauseCounter);

void n_wxor(std::string path, std::vector<int> inputIdx, int outputIdx, int &xCounter, int &clauseCounter);

void wLinear_2(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, std::vector<int> &Matrix, int &ACounter, int &clauseCounter);

std::vector<std::vector<int>> generateCombinations(int n, int minOnes, int maxOnes);

std::vector<std::vector<int>> generateCombinations_back(int n, int minOnes, int maxOnes);

std::vector<std::vector<int>> generateCombinations_front(int n, int minOnes, int maxOnes);

void branch(std::string path, int inputIdx, int outputIdx1, int outputIdx2, int &clauseCounter);

void SequentialEncoding(std::string path, std::vector<int> &sbox_vector, std::vector<std::vector<int>> &auxiliary_var_u,
                        int var_num, int cardinalitycons, int &clauseCounter);

void SequentialEncoding_least(std::string path, std::vector<int> &sbox_vector, std::vector<std::vector<int>> &auxiliary_var_u,
                              int var_num, int cardinalitycons, int &clauseCounter);

void bXor(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter);

void n_xor(std::string path, std::vector<int> inputIdx, int outputIdx, int &xCounter, int &clauseCounter);

void n_xor_2(std::string path, std::vector<int> inputIdx, int outputIdx, int &xCounter, int &clauseCounter);

bool calculateXOR(const std::vector<int> &tuple);

std::vector<std::vector<int>> generateTuples(int k);

void bSbox(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
           std::vector<std::vector<int>> sboxcnf, int &ACounter, std::vector<int> &AtoSbox, bool ifInjective, int &clauseCounter);

void bMatrixEntry(std::string path, std::vector<std::vector<int>> inputIdx, std::vector<int> outputIdx,
                  std::vector<std::vector<int>> eq_matrix, int rowcount, int &xCounter, int &clauseCounter);

void bAndOr(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &clauseCounter);

void bSbox_dp(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
              std::vector<std::vector<int>> sboxcnf, std::vector<int> sbox_weight_idx, int sbox_weight_num, bool ifInjective, int &clauseCounter);

void bAdd(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx, int &xCounter, int &clauseCounter);

void bAdd_dp(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx, std::vector<int> &dp_weight, int &xCounter, int &clauseCounter);

void bAdd_dp_2(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx, std::vector<int> &dp_weight, int &xCounter, int &clauseCounter);

void bAndOr_dp(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int weight_idx, int &clauseCounter);

void wLinearBPT(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
                std::vector<std::vector<int>> bptcnf, std::vector<int> bpt_weight_idx, int bpt_weight_num, bool ifInjective, int &clauseCounter);

void wPBConstraint(const std::string &path,
                   const std::vector<int32_t> &literals,
                   const std::vector<int64_t> &weights,
                   int64_t bound,
                   int &ACounter,
                   int &clauseCounter);

void wSWCConstraint(std::string path,
                    const std::vector<int32_t> &literals,
                    const std::vector<int64_t> &weights,
                    int64_t bound,
                    int &ACounter,
                    int &clauseCounter);

}
#endif
