#ifndef TruncDiffBPTSWSAT_H
#define TruncDiffBPTSWSAT_H
#include "ExperimentRecord.h"
#include "ProcedureH.h"
#include <utility>
#include <utilities.h>
#include "Transformer.h"
#include "SboxModel.h"
#include "gurobi_c++.h"
#include "Reduction.h"
#include "BranchNum.h"
#include "TruncDiffBPTMILPcons.h"
#include "BranchingPropertyTableCalculator.h"
#include "SatSearchConfig.h"
#include <cstdint>

class TruncDiffBPTSWSAT {
    experiment::Record experimentRecord;
    experiment::Clock::time_point encodingStart;
    bool encodingPending = false;
    experiment::Clock::time_point initialBuildStart;
    bool initialBuildStarted = false;

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
    vector<double> finalClockTime;
    vector<double> finalTimeTime;

    std::vector<int> Matrix;
    double timeTime;

    vector<vector<double>> satTime;
    vector<double> roundSatTime;

    vector<vector<double>> unsatTime;
    vector<double> roundUnsatTime;

    std::string pathPrefix = std::string(DPATH) + "trunBPTdifferential/WordWiseSAT/";
    std::string pathSuffix;
    std::string modelPath;
    std::string resultsPath;
    std::string diffCharacteristicPath;
    std::string finalResultsPath;
    std::string solverch;

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

    int sboxRoundNum = 0;
    int as_low, as_high;

    bool securityFlag = false;
    int securityRoundNumBound;

    int dCounter = 1;
    int ACounter = 1;
    int PCounter = 1;

    int clauseCounter = 0;
    int InitialLowerBound = 0;
    int activesbox = InitialLowerBound;
    std::vector<vector<int>> auxiliary_var_u;
    int countProbability = InitialLowerBound;

    std::map<std::string, int> tanNameMxIndex;
    std::map<std::string, int> rtnMxIndex;
    std::vector<int> rtnIdxSave;
    bool sboxFindFlag = false;
    std::vector<int> sboxIndexSave;
    std::vector<std::vector<int>> roundSboxIndexSave;

    std::string roundID;

    std::vector<std::vector<int>> differentialCharacteristic;
    std::vector<std::vector<int>> roundPcounterIndexSave;
    std::vector<int> xorPIndexSave;

    std::vector<std::vector<int>> roundPboxMIndexSave;

    int xorPCounter = 1;

    int redMode;

    int bpt_weight_num;

    std::map<std::string, std::vector<std::vector<int>>> BPTIneqs;

    std::vector<int> BPTExtWeighted;

    std::map<std::string, std::vector<std::vector<int>>> BPTCnf;

    std::vector<int> dp_weight;

    std::vector<std::vector<int32_t>> round_pb_vars;

    std::vector<std::vector<int64_t>> round_pb_weights;


    int xCounter = 1;

    int currentRoundStatus = 0;
    vector<int> finalStatus;

    int curRoundWeightSize = 0;
    bool isInteger = true;
    int alpha = 0;
    BptEncodingType encodingType = BptEncodingType::MAX_SMT;
    ObjectiveEncoding objectiveEncoding = ObjectiveEncoding::BDD;

    long long beta;

    enum class MallobStatus {
        SAT,
        UNSAT,
        TIMEOUT,
        UNKNOWN,
        RECOVERY_ERROR
    };

    void collectObjectiveTerms(std::vector<int32_t> &literals, std::vector<int64_t> &weights) const;
    void collectSCTerms(std::vector<int32_t> &literals) const;
    void emitObjectiveBound(int64_t bound);
    void emitWeightedBound(const std::vector<int32_t> &literals, const std::vector<int64_t> &weights, int64_t bound);
    void emitSCSequentialBound(const std::vector<int> &literals,
                               std::vector<std::vector<int>> &auxiliary_var_u,
                               int64_t bound);
    bool writeCnfHeader();
    MallobStatus runMallob(const std::string &modelPath, const std::string &solutionPath, long long timeoutSeconds);

    void clearPerModelObjectiveState();

  public:
    TruncDiffBPTSWSAT(const vector<ProcedureHPtr> &procedureHs, std::string target);
    void setGurobiTimer(int timer) { this->gurobiTimer = timer; }
    void setGurobiThreads(int threads) { this->gurobiThreads = threads; }
    void setStartRound(int round) { this->startRound = round; }
    void setEndRound(int round) { this->endRound = round; }
    void setXorConSel(int select) { this->xorConSel = select; }
    void setLinearConSel(int select) { this->linearConSel = select; }
    void setILP() { this->ILPFlag = true; }
    void setTotalRoundNum(int num) { this->totalRoundNum = num; }
    void setEvaluationSize(int evaluationSize) { this->evaluationSize = evaluationSize; }
    void setSolverch(std::string solverch) { this->solverch = solverch; }
    void set_isInteger(bool is_integer) { isInteger = is_integer; }
    void set_alpha(int alpha) { this->alpha = alpha; }
    void setEncodingType(BptEncodingType encodingType) { this->encodingType = encodingType; }
    void setObjectiveEncoding(ObjectiveEncoding objectiveEncoding) { this->objectiveEncoding = objectiveEncoding; }

    void MGR();

    void solver();

    void solver_unsat();

    void solver_BS();

    void cryptanalysis();

    void programGenModel();

    void roundFunctionGenModel(const ProcedureHPtr &procedureH, const int roundCounter);

    void XORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right, const ThreeAddressNodePtr &result);

    void ANDandORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right, const ThreeAddressNodePtr &result);

    void linearPermuteGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input, const std::vector<ThreeAddressNodePtr> &output, const int curRoundNumber);

    void AddGenModel(std::vector<ThreeAddressNodePtr> &input1, std::vector<ThreeAddressNodePtr> &input2, const std::vector<ThreeAddressNodePtr> &output);

    void PboxGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input, const ThreeAddressNodePtr &output);

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

            bpt.prepareSATEncoding(this->encodingType, this->alpha);
            auto possible = bpt.get_poss();
            experimentRecord.tables[iterM->first] = {bpt.get_BPTRaw(),
                                                     std::set<std::string>(possible.begin(), possible.end()), bpt.ext_weighted(), bpt.get_beta()};

            this->beta = bpt.get_beta();

            this->bpt_weight_num = bpt.get_weightnum_cnf();
            this->BPTCnf[iterM->first] = bpt.get_BPTCnf();
            this->BPTExtWeighted = bpt.ext_weighted();

            iterM++;
        }

        this->pathSuffix = this->cipherName + "/xor" + std::to_string(this->xorConSel) +
                           "_linear" + std::to_string(this->linearConSel);
        if (this->solverch == "unsat") {
            this->pathSuffix += "_unsat";
        } else if (this->solverch == "bs") {
            this->pathSuffix += "_bs";
        } else
            this->pathSuffix += "_sat";
        this->pathSuffix += "/";
        this->pathSuffix += "alpha" + std::to_string(this->alpha) + "_bpt" +
                            std::to_string(static_cast<int>(encodingType)) + "_objective" +
                            std::to_string(static_cast<int>(objectiveEncoding)) + "/";
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

    int countTrueVarsInSolution(const std::string &filePath, const std::vector<int> &vars) {
        std::ifstream fin(filePath);
        if (!fin.is_open()) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        std::unordered_set<int> targetVars(vars.begin(), vars.end());
        std::string line;
        bool foundSat = false;

        while (std::getline(fin, line)) {

            if (line.find("s SATISFIABLE") != std::string::npos) {
                foundSat = true;
                break;
            }
        }

        if (!foundSat) {
            throw std::runtime_error("No 's SATISFIABLE' found in file.");
        }

        if (!std::getline(fin, line)) {
            throw std::runtime_error("Missing solution array line after 's SATISFIABLE'.");
        }

        for (char &ch : line) {
            if (ch == '[' || ch == ']' || ch == ',') {
                ch = ' ';
            }
        }

        std::istringstream iss(line);
        int val;
        int count = 0;

        while (iss >> val) {

            if (val > 0 && targetVars.count(val)) {
                ++count;
            }
        }

        return count;
    }

    int calculateWeightedSumInSolution(const std::string &filePath,
                                       const std::vector<int32_t> &vars,
                                       const std::vector<int64_t> &weights) {
        const auto value = experiment::weighted(experiment::readAssignment(filePath), vars, weights);
        if (value > std::numeric_limits<int>::max())
            throw std::runtime_error("SAT objective exceeds legacy int range");
        return static_cast<int>(value);
    }

};
#endif
