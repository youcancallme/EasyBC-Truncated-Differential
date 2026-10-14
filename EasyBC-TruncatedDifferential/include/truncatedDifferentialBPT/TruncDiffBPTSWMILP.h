

#ifndef EASYBC_TruncDiffBPTSWMILP_H
#define EASYBC_TruncDiffBPTSWMILP_H
#include "ExperimentRecord.h"

#include "ProcedureH.h"
#include <utility>
#include <utilities.h>

#include "BranchingPropertyTableCalculator.h"
#include "Transformer.h"
#include "SboxModel.h"
#include "gurobi_c++.h"
#include "Reduction.h"
#include "BPTReduction.h"
#include "BranchNum.h"
#include "TruncDiffBPTMILPcons.h"

class TruncDiffBPTSWMILP {
    experiment::Record experimentRecord;
    std::string experimentStatus = "UNKNOWN";

  private:
    std::string cipherName;
    std::vector<ProcedureHPtr> procedureHs;

    std::string target;
    int gurobiTimer = 3600 * 12;
    int gurobiThreads = 8;
    int startRound = 1;
    int endRound = 5;
    int currentRound;

    int xorConSel = 1;
    int linearConSel = 1;

    bool ILPFlag = false;

    vector<int> finalResults;
    struct RoundSolveResult {
        int round;
        int status;
        int objective;
        double clockTime;
        double timeTime;
    };

    std::vector<RoundSolveResult> roundSolveResults;

    vector<double> finalClockTime;
    vector<double> finalTimeTime;

    std::string pathPrefix = std::string(DPATH) + "trunBPTdifferential/WordWiseMILP/";
    std::string pathSuffix;
    std::string modelPath;
    std::string resultsPath;
    std::string diffCharacteristicPath;
    std::string finalResultsPath;

    std::map<std::string, std::vector<int>> Box;
    std::map<std::string, int> branchNumMin;
    std::map<std::string, int> branchNumMax;

    int keySize;
    int totalRoundNum;
    float sboxMDP;
    int targetNdiff;

    int blockSize;

    int wordSize;
    int evaluationSize = 0;

    bool securityFlag = false;
    int securityRoundNumBound;

    int dCounter = 1;
    int ACounter = 1;

    int PCounter = 1;

    int xorPCounter = 1;

    int redMode;

    int bpt_weight_num;

    long long beta;
    bool isInteger = true;
    int alpha = 0;

    std::map<std::string, int> tanNameMxIndex;
    std::map<std::string, int> rtnMxIndex;
    std::vector<int> rtnIdxSave;
    bool sboxFindFlag = false;
    std::vector<int> sboxIndexSave;
    std::vector<std::vector<int>> roundSboxIndexSave;

    std::string roundID;

    std::vector<std::vector<int>> differentialCharacteristic;

    std::vector<std::vector<int>> roundPcounterIndexSave;

    std::vector<std::vector<int>> roundPboxMIndexSave;

    std::map<std::string, std::vector<std::vector<int>>> BPTIneqs;

    std::map<std::string, std::vector<std::vector<int>>> BPTenqs;

    std::vector<int> BPTExtWeighted;

    std::map<std::string, std::vector<std::vector<int>>> BPTCnf;

  public:
    TruncDiffBPTSWMILP(const vector<ProcedureHPtr> &procedureHs, std::string target, int redMode);

    void setGurobiTimer(int timer) { this->gurobiTimer = timer; }
    void setGurobiThreads(int threads) { this->gurobiThreads = threads; }
    void setStartRound(int round) { this->startRound = round; }
    void setEndRound(int round) { this->endRound = round; }
    void setXorConSel(int select) { this->xorConSel = select; }
    void setLinearConSel(int select) { this->linearConSel = select; }
    void setILP() { this->ILPFlag = true; }
    void setTotalRoundNum(int num) { this->totalRoundNum = num; }
    void setEvaluationSize(int evaluationSize) { this->evaluationSize = evaluationSize; }
    void set_isInteger(bool is_integer) { isInteger = is_integer; }
    void set_alpha(int alpha) { this->alpha = alpha; }

    void MGR();

    void cryptanalysis();

    void evaluation();

    void solver();

    void programGenModel();

    void roundFunctionGenModel(const ProcedureHPtr &procedureH, const int roundCounter);

    void XORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right, const ThreeAddressNodePtr &result);

    void ANDandORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right, const ThreeAddressNodePtr &result);

    void PboxGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input, const ThreeAddressNodePtr &output);

    void linearPermuteGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input, const std::vector<ThreeAddressNodePtr> &output, const int curRoundNumber);

    void AddGenModel(std::vector<ThreeAddressNodePtr> &input1, std::vector<ThreeAddressNodePtr> &input2, const std::vector<ThreeAddressNodePtr> &output);

    void preprocess(std::map<std::string, std::vector<int>> pboxM, std::map<std::string, std::vector<int>> Ffm) {
        auto iterator = Box.begin();
        while (iterator != Box.end()) {
            if (iterator->first.substr(0, 4) == "sbox") {
                std::string sboxName = iterator->first;
                if (iterator->second.size() <= 32) {
                    SboxM sboxM(sboxName, iterator->second, "AS");
                    this->sboxMDP = sboxM.getSboxMDP();
                } else if (this->cipherName == "AES_128_128") {
                    this->sboxMDP = 6;
                }
            }
            this->branchNumMin[iterator->first] = Red::branch_num_of_sbox(iterator->second);
            iterator++;
        }

        auto iterM = pboxM.begin();
        while (iterM != pboxM.end()) {

            BPTCalculator bpt(iterM->first, iterM->second, Ffm[iterM->first]);

            bpt.prepareMILPEncoding(BptEncodingType::MAX_SMT, this->alpha);
            auto possible = bpt.get_poss();
            experimentRecord.tables[iterM->first] = {bpt.get_BPTRaw(),
                                                     std::set<std::string>(possible.begin(), possible.end()), bpt.ext_weighted(), bpt.get_beta()};

            this->beta = bpt.get_beta();

            this->BPTExtWeighted = bpt.ext_weighted();

            this->bpt_weight_num = bpt.get_weightnum_cnf();
            this->BPTCnf[iterM->first] = bpt.get_BPTCnf();

            this->BPTIneqs[iterM->first] = BPTRed::reduction(this->redMode, bpt);

            this->BPTenqs[iterM->first] = bpt.sage_eqns();

            iterM++;
        }

        this->pathSuffix = this->cipherName + "/xor" + std::to_string(this->xorConSel);

        switch (this->redMode) {
        case 1:
            this->pathSuffix += "_greedy_sun";
            break;
        case 2:
            this->pathSuffix += "_sub_milp";
            break;
        case 3:
            this->pathSuffix += "_convex_hull_tech";
            break;
        case 4:
            this->pathSuffix += "_logic_cond";
            break;
        case 5:
            this->pathSuffix += "_comb233";
            break;

        case 7:
            this->pathSuffix += "_preAlpha=1_logic_cond";
            break;
        default:
            this->pathSuffix += "_greedy_sun";
            break;
        }

        this->pathSuffix += "/";
        this->pathSuffix += "alpha" + std::to_string(this->alpha) + "/";
    }

    bool isConstant(ThreeAddressNodePtr threeAddressNodePtr);

    std::vector<int> extIdxFromTOUINTorBOXINDEX(const ThreeAddressNodePtr &input) {
        std::vector<int> extIdx;
        if (input->getOp() == ASTNode::TOUINT) {
            ThreeAddressNodePtr left = input->getLhs();
            while (left->getOp() == ASTNode::BOXINDEX) {
                if (this->tanNameMxIndex.count(left->getLhs()->getNodeName()) != 0) {
                    extIdx.push_back(this->tanNameMxIndex.find(left->getLhs()->getNodeName())->second);
                    left = left->getRhs();
                } else if (!this->rtnIdxSave.empty()) {
                    this->tanNameMxIndex[left->getLhs()->getNodeName()] = this->rtnIdxSave.front();
                    extIdx.push_back(this->rtnIdxSave.front());
                    this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                    left = left->getRhs();
                } else {
                    this->tanNameMxIndex[left->getLhs()->getNodeName()] = this->ACounter;
                    extIdx.push_back(this->ACounter);
                    this->ACounter++;
                    left = left->getRhs();
                }
            }
            if (!this->rtnIdxSave.empty()) {
                this->tanNameMxIndex[left->getLhs()->getNodeName()] = this->rtnIdxSave.front();
                extIdx.push_back(this->rtnIdxSave.front());
                this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
            } else if (this->tanNameMxIndex.count(left->getNodeName()) != 0) {
                extIdx.push_back(this->tanNameMxIndex.find(left->getNodeName())->second);
            } else {
                this->tanNameMxIndex[left->getLhs()->getNodeName()] = this->ACounter;
                extIdx.push_back(this->ACounter);
                this->ACounter++;
            }
        } else if (input->getOp() == ASTNode::BOXINDEX) {
            ThreeAddressNodePtr left = input;
            while (left->getOp() == ASTNode::BOXINDEX) {
                if (this->tanNameMxIndex.count(left->getLhs()->getNodeName()) != 0) {
                    extIdx.push_back(this->tanNameMxIndex.find(left->getLhs()->getNodeName())->second);
                    left = left->getRhs();
                } else if (left->getLhs() != nullptr and left->getRhs() != nullptr) {
                    if (left->getLhs()->getOp() == ASTNode::SYMBOLINDEX and left->getLhs()->getLhs()->getNodeType() != UINT1) {
                        if (this->tanNameMxIndex.count(left->getLhs()->getLhs()->getNodeName() + "_$B$_" +
                                                       left->getLhs()->getRhs()->getNodeName()) != 0) {
                            extIdx.push_back(this->tanNameMxIndex.find(left->getLhs()->getLhs()->getNodeName() + "_$B$_" +
                                                                       left->getLhs()->getRhs()->getNodeName())
                                                 ->second);
                            this->tanNameMxIndex[left->getNodeName()] = this->tanNameMxIndex.find(left->getLhs()->getLhs()->getNodeName() + "_$B$_" +
                                                                                                  left->getLhs()->getRhs()->getNodeName())
                                                                            ->second;
                            left = left->getRhs();
                        } else
                            assert(false);
                    }
                } else if (!this->rtnIdxSave.empty()) {
                    this->tanNameMxIndex[left->getLhs()->getNodeName()] = this->rtnIdxSave.front();
                    extIdx.push_back(this->rtnIdxSave.front());
                    this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                    left = left->getRhs();
                } else {
                    this->tanNameMxIndex[left->getLhs()->getNodeName()] = this->ACounter;
                    extIdx.push_back(this->ACounter);
                    this->ACounter++;
                    left = left->getRhs();
                }
            }
            if (!this->rtnIdxSave.empty()) {
                this->tanNameMxIndex[left->getNodeName()] = this->rtnIdxSave.front();
                extIdx.push_back(this->rtnIdxSave.front());
                this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
            } else if (this->tanNameMxIndex.count(left->getNodeName()) != 0) {
                extIdx.push_back(this->tanNameMxIndex.find(left->getNodeName())->second);
            } else {
                this->tanNameMxIndex[left->getNodeName()] = this->ACounter;
                extIdx.push_back(this->ACounter);
                this->ACounter++;
            }
        } else
            assert(false);
        return extIdx;
    }
};

#endif
