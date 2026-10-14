

#ifndef EASYBC_TRUNCDIFFBPTMILPCONS_H
#define EASYBC_TRUNCDIFFBPTMILPCONS_H

#include <fstream>
#include "istream"
#include "iostream"
#include "vector"
#include "string"

namespace TruncDiffBPTMILPcons {

void wXorC1(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &dCounter);

void wXorC2(std::string path, int inputIdx1, int inputIdx2, int outputIdx);

void wXorC3(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &dCounter, int &Pcounter);

void wAndOrC(std::string path, int inputIdx1, int inputIdx2, int outputIdx);

void wLinearC1(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, int &dCounter, int branchMin, int branchMax);

void wLinearC2(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, int &dCounter, int branchMin, int branchMax);

void wLinearBPT(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx, int &dCounter, int branchMin, int branchMax, int &Pcounter, std::vector<std::vector<int>> ineqs, std::vector<std::vector<int>> enqs, int extLen);

void bXorC1(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &dCounter);

void bXorC2(std::string path, int inputIdx1, int inputIdx2, int outputIdx);

void bXorC3(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &dCounter);

void bMatrixEntryC12(std::string path, std::vector<std::vector<int>> inputIdx, std::vector<int> outputIdx,
                     std::vector<int> overflows, std::vector<int> module,
                     int &xCounter, int &yCounter, int &fCounter, int &dCounter, int chooser);

void bNXorC3(std::string path, std::vector<int> inputIdx, int outputIdx, int &yCounter);

void bMatrixEntryC3(std::string path, std::vector<std::vector<int>> inputIdx, std::vector<int> outputIdx,
                    std::vector<int> overflows, std::vector<int> module,
                    int &xCounter, int &yCounter, int &fCounter);

void bSboxC1(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
             std::vector<std::vector<int>> ineqs, int branchMin, int &dCounter, int &ACounter, bool ifInjective);

void bSboxC2(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
             std::vector<std::vector<int>> ineqs, int &ACounter, bool ifInjective);

void bSboxC3(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
             std::vector<std::vector<int>> ineqs, int &ACounter);

void bAddC(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx,
           std::vector<std::vector<int>> ineqs, int &dCounter, int &PCounter);

void bAndOrC(std::string path, int inputIdx1, int inputIdx2, int outputIdx);

void dSboxC1(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
             std::vector<std::vector<int>> ineqs, int branchMin, int &dCounter,
             int &PCounter, bool ifInjective, int extLen);

void dSboxC2(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
             std::vector<std::vector<int>> ineqs, int &PCounter, bool ifInjective, int extLen);

void dSboxC3(std::string path, std::vector<int> inputIdx, std::vector<int> outputIdx,
             std::vector<std::vector<int>> ineqs, int &PCounter, int extLen);

void dAddC(std::string path, std::vector<int> inputIdx1, std::vector<int> inputIdx2, std::vector<int> outputIdx,
           std::vector<std::vector<int>> ineqs, int &dCounter, int &PCounter);

void dAndOrC(std::string path, int inputIdx1, int inputIdx2, int outputIdx, int &PCounter);

}

#endif
