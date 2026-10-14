

#include "truncatedDifferentialBPT/TruncDiffBPTSWMILP.h"

extern std::map<std::string, std::vector<int>> allBox;
extern map<std::string, std::vector<int>> pboxM;
extern std::map<std::string, int> pboxMSize;
extern map<std::string, std::vector<int>> Ffm;
extern std::string cipherName;

TruncDiffBPTSWMILP::TruncDiffBPTSWMILP(const vector<ProcedureHPtr> &procedureHs, std::string target, int redMode)
    : procedureHs(procedureHs), target(move(target)), redMode(redMode) {
    this->Box = allBox;
    this->cipherName = ::cipherName;
}

void TruncDiffBPTSWMILP::MGR() {
    double sharedPreprocessing = 0;
    {
        experiment::Timer measured(sharedPreprocessing);
        preprocess(pboxM, Ffm);
    }

    this->finalResults.clear();
    this->roundSolveResults.clear();
    this->finalClockTime.clear();
    this->finalTimeTime.clear();

    this->pathPrefix += target + "/";
    this->modelPath = this->pathPrefix + "models/" + this->pathSuffix;
    this->resultsPath = this->pathPrefix + "results/" + this->pathSuffix;
    this->diffCharacteristicPath = this->pathPrefix + "truncBPTdifferentialCharacteristics/" + this->pathSuffix;
    system(("mkdir -p " + this->modelPath).c_str());
    system(("mkdir -p " + this->resultsPath).c_str());
    system(("mkdir -p " + this->diffCharacteristicPath).c_str());
    this->finalResultsPath = this->resultsPath + this->cipherName + "_FinalResults.txt";
    system(("touch " + this->finalResultsPath).c_str());

    for (int i = this->startRound; i <= endRound; ++i) {
        if (this->securityFlag)
            break;

        experimentRecord.timing = experiment::Timing{};
        this->currentRound = i;
        experimentRecord.best = Json::Value();
        experimentRecord.error.clear();
        experimentRecord.clearModel();
        experimentStatus = "UNKNOWN";
        std::cout << "\n **************** CURRENT MAX ROUOND : " << this->currentRound << "  **************** \n"
                  << std::endl;
        this->modelPath = this->pathPrefix + "models/" + this->pathSuffix + std::to_string(this->currentRound) + "_round_model.lp";
        this->resultsPath = this->pathPrefix + "results/" + this->pathSuffix + std::to_string(this->currentRound) + "_round_results.txt";
        system(("touch " + this->modelPath).c_str());
        system(("touch " + this->resultsPath).c_str());
        this->diffCharacteristicPath = this->pathPrefix + "truncBPTdifferentialCharacteristics/" + this->pathSuffix + std::to_string(this->currentRound) + "_round_differential_characteristic.txt";
        system(("touch " + this->diffCharacteristicPath).c_str());

        system(("rm " + this->modelPath).c_str());
        const auto modelStart = experiment::Clock::now();
        if (this->target == "truncBPTcryptanalysis")
            cryptanalysis();
        else if (this->target == "evaluation")
            evaluation();
        else
            assert(false);
        experimentRecord.timing.model += experiment::seconds(modelStart);

        vector<double> finalTotalRoundClockTime;
        vector<double> finalTotalRoundTimeTime;
        const auto legacyStart = experiment::Clock::now();
        solver();

        double clockTime = experiment::seconds(legacyStart);
        double timeTime = clockTime;
        finalTotalRoundClockTime.push_back(clockTime);
        finalTotalRoundTimeTime.push_back(timeTime);

        Json::Value config;
        config["beta"] = Json::Int64(beta);
        config["encoding_type"] = "MAX_SMT";
        config["objective_encoding"] = "MILP";
        config["search"] = "MILP";
        config["threads"] = gurobiThreads;
        config["timer"] = gurobiTimer;
        config["reduction"] = redMode;


        this->tanNameMxIndex.clear();
        this->rtnMxIndex.clear();
        this->rtnIdxSave.clear();
        this->dCounter = 1;
        this->ACounter = 1;
        this->PCounter = 1;
        this->xorPCounter = 1;
        this->differentialCharacteristic.clear();
        this->roundSboxIndexSave.clear();
        this->roundPcounterIndexSave.clear();
        this->roundPboxMIndexSave.clear();
        this->sboxFindFlag = false;
        this->sboxIndexSave.clear();
        std::ofstream fResults(this->finalResultsPath, std::ios::trunc);
        if (!fResults) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            if (this->target == "truncBPTcryptanalysis") {
                fResults << " ***************** FINAL RESULTS ***************** \n\n";

                for (const auto &roundResult : this->roundSolveResults) {
                    fResults << "Results Of " << roundResult.round << " Rounds : \n";

                    if (roundResult.status == GRB_OPTIMAL) {
                        fResults << "\tObj : " << roundResult.objective << "\n";
                        fResults << "\trealObj : "
                                 << static_cast<double>(roundResult.objective) / this->beta
                                 << "\n";
                    } else if (roundResult.status == GRB_INFEASIBLE) {
                        fResults << "\tObj : UNSAT (Model is infeasible)\n";
                    } else if (roundResult.status == GRB_TIME_LIMIT) {
                        fResults << "\tObj : TIMEOUT (see per-round JSON for any feasible trajectory)\n";
                    } else {
                        fResults << "\tObj : UNKNOWN (Gurobi status "
                                 << roundResult.status << ")\n";
                    }

                    fResults << "\tclockTime : " << roundResult.clockTime << "\n";
                    fResults << "\ttimeTime : " << roundResult.timeTime << "\n\n";
                }
                fResults << " ***************** FINAL RESULTS ***************** \n";
            } else if (this->target == "evaluation") {
                fResults << "-------------------------- FINAL RESULTS -------------------------- \n\n";
                for (int j = 0; j <= this->finalResults.size(); ++j) {
                    if (j == this->securityRoundNumBound - 1) {
                        fResults << "\n****************************************************************\n";
                        fResults << "the block cipher is security enough when the round number is : " << j + 1 << "\n";
                        fResults << "****************************************************************\n\n\n\n";

                        fResults << "***************** Results Of " << j + 1 << " Rounds *****************\n";

                        fResults << "\tnumber of target active sboxes for security bounding : " << this->targetNdiff << "\n";
                        fResults << "\tclockTime : " << this->finalClockTime[j] << "\n";
                        fResults << "\ttimeTime : " << this->finalTimeTime[j] << "\n\n";

                        fResults << "\ttotalRoundClockTime : " << finalTotalRoundClockTime[j] << "\n";
                        fResults << "\ttotalRoundTimeTime : " << finalTotalRoundTimeTime[j] << "\n\n\n";
                    } else {
                        fResults << "***************** Results Of " << j + 1 << " Rounds *****************\n";
                        fResults << "\tnumber of active sboxes : " << this->finalResults[j] << "\n";
                        fResults << "\tclockTime : " << this->finalClockTime[j] << "\n";
                        fResults << "\ttimeTime : " << this->finalTimeTime[j] << "\n\n";

                        fResults << "\ttotalRoundClockTime : " << finalTotalRoundClockTime[j] << "\n";
                        fResults << "\ttotalRoundTimeTime : " << finalTotalRoundTimeTime[j] << "\n\n\n";
                    }
                }
                fResults << " -------------------------- FINAL RESULTS -------------------------- \n";
            }
            fResults.close();
        }

        experimentRecord.write(resultsPath + ".json", cipherName, currentRound, "MILP", alpha,
                               experimentStatus, config, i != startRound,
                               i == startRound ? sharedPreprocessing : 0.0);
    }
    if (target == "truncBPTcryptanalysis") {
        std::cout << "\n ***************** FINAL RESULTS ***************** " << std::endl;

        for (const auto &roundResult : this->roundSolveResults) {
            std::cout << "Results Of " << roundResult.round << " Rounds : \n";

            if (roundResult.status == GRB_OPTIMAL) {
                std::cout << "\tObj : " << roundResult.objective << "\n";
                std::cout << "\trealObj : "
                          << static_cast<double>(roundResult.objective) / this->beta
                          << "\n";
            } else if (roundResult.status == GRB_INFEASIBLE) {
                std::cout << "\tObj : UNSAT (Model is infeasible)\n";
            } else if (roundResult.status == GRB_TIME_LIMIT) {
                std::cout << "\tObj : TIMEOUT (see per-round JSON for any feasible trajectory)\n";
            } else {
                std::cout << "\tObj : UNKNOWN (Gurobi status "
                          << roundResult.status << ")\n";
            }

            std::cout << "\tclockTime : " << roundResult.clockTime << "\n";
            std::cout << "\ttimeTime : " << roundResult.timeTime << "\n\n";
        }
        std::cout << " ***************** FINAL RESULTS ***************** \n"
                  << std::endl;
    } else if (target == "evaluation") {
        std::cout << "\n ***************** FINAL RESULTS ***************** " << std::endl;
        for (int i = 0; i < this->finalResults.size(); ++i) {
            if (i == this->securityRoundNumBound - 1) {
                std::cout << "\n\n****************************************************************" << std::endl;
                std::cout << "the block cipher is security enough when the round number is : " << i + 1 << std::endl;
                std::cout << "****************************************************************\n\n\n"
                          << std::endl;

                std::cout << "Results Of " << i + 1 << " Rounds : " << std::endl;
                std::cout << "\tnumber of target active sboxes for security bounding : " << this->targetNdiff << "\n";
                std::cout << "\tclockTime : " << this->finalClockTime[i] << std::endl;
                std::cout << "\ttimeTime : " << this->finalTimeTime[i] << std::endl
                          << std::endl;
            } else {
                std::cout << "Results Of " << i + 1 << " Rounds : " << std::endl;
                std::cout << "\tnumber of active sboxes : " << this->finalResults[i] << std::endl;
                std::cout << "\tclockTime : " << this->finalClockTime[i] << std::endl;
                std::cout << "\ttimeTime : " << this->finalTimeTime[i] << std::endl
                          << std::endl;
            }
        }
        std::cout << " ***************** FINAL RESULTS ***************** \n"
                  << std::endl;
    }
}

void TruncDiffBPTSWMILP::cryptanalysis() {
    programGenModel();


    std::ifstream file;
    file.open(this->modelPath);
    std::string model, line;
    while (getline(file, line)) {
        model += line + "\n";
    }
    file.close();

    if (model != "") {
        std::ofstream target(this->modelPath, std::ios::trunc);
        if (!target) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            target << "Minimize\n";

            if (!this->BPTExtWeighted.empty()) {

                for (int i = 1; i < this->PCounter; i += this->BPTExtWeighted.size()) {
                    int k = 0;
                    for (auto weight : this->BPTExtWeighted) {
                        if (weight != 1)
                            target << weight << " P" << i + k;
                        else
                            target << "P" << i + k;
                        if (i + k != this->PCounter - 1)
                            target << " + ";
                        k++;
                    }
                }
            } else {

                for (int i = 1; i < this->PCounter; i++) {
                    target << "P" << i;
                    if (i != this->PCounter - 1)
                        target << " + ";
                }
            }

            if (this->xorPCounter > 1) {
                target << " + ";
                for (int i = 1; i < this->xorPCounter; i++) {
                    target << "xorP" << i;
                    if (i != this->xorPCounter - 1)
                        target << " + ";
                }
            }
            target << "\n"
                   << "Subject To\n";

            for (int i = 0; i < this->blockSize; ++i) {
                if (i != this->blockSize - 1)
                    target << "A" << std::to_string(i + 1) << " + ";
                else
                    target << "A" << std::to_string(i + 1);
            }
            target << " >= 1\n";

            target << model;
            target.close();
        }
    }
    std::ofstream binary(this->modelPath, std::ios::app);
    if (!binary) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        binary << "Binary\n";
        for (int i = 1; i < this->dCounter; ++i)
            binary << "d" << i << "\n";
        for (int i = 1; i < this->ACounter; ++i)
            binary << "A" << i << "\n";
        for (int i = 1; i < this->PCounter; ++i)
            binary << "P" << i << "\n";
        for (int i = 1; i < this->xorPCounter; ++i)
            binary << "xorP" << i << "\n";

        binary << "End";
        binary.close();
    }
}

void TruncDiffBPTSWMILP::evaluation() {
    if (this->evaluationSize == 0)
        this->evaluationSize = this->blockSize;
    this->targetNdiff = int(ceil(this->currentRound * ((this->evaluationSize / this->sboxMDP) / this->totalRoundNum))) - 1;

    programGenModel();

    std::ifstream file;
    file.open(this->modelPath);
    std::string model, line;
    while (getline(file, line)) {
        model += line + "\n";
    }
    file.close();

    if (model != "") {
        std::ofstream target(this->modelPath, std::ios::trunc);
        if (!target) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            target << "Subject To\n";
            for (int i = 0; i < sboxIndexSave.size(); ++i) {
                target << "A" << sboxIndexSave[i];
                if (i != sboxIndexSave.size() - 1) {
                    target << " + ";
                }
            }
            target << " <= " << this->targetNdiff << "\n";

                for (int i = 0; i < this->blockSize; ++i) {
                    if (i != this->blockSize - 1)
                        target << "A" << std::to_string(i + 1) << " + ";
                    else
                        target << "A" << std::to_string(i + 1);
                }
                target << " >= 1\n";

            target << model;
            target.close();
        }
    }
    std::ofstream binary(this->modelPath, std::ios::app);
    if (!binary) {
        std::cout << "Wrong file path ! " << std::endl;
    } else {
        binary << "Binary\n";
        for (int i = 1; i < dCounter; ++i)
            binary << "d" << i << "\n";
        for (int i = 1; i < ACounter; ++i)
            binary << "A" << i << "\n";
        binary << "End";
        binary.close();
    }
}

void TruncDiffBPTSWMILP::solver() {
    const auto setupStart = experiment::Clock::now();
    GRBEnv env = GRBEnv(true);
    env.start();

    env.set(GRB_IntParam_Threads, this->gurobiThreads);
    env.set(GRB_IntParam_MIPFocus, 3);
    env.start();
    GRBModel model = GRBModel(env, modelPath);
    model.set(GRB_IntParam_MIPFocus, 2);
    model.set(GRB_DoubleParam_TimeLimit, this->gurobiTimer);
    experimentRecord.timing.model += experiment::seconds(setupStart);
    ++experimentRecord.timing.calls;
    {
        experiment::Timer measured(experimentRecord.timing.solver);
        model.optimize();
    }

    int optimstatus = model.get(GRB_IntAttr_Status);
    experimentStatus = optimstatus == GRB_OPTIMAL ? "OPTIMAL" : optimstatus == GRB_INFEASIBLE ? "UNSAT"
                                                            : optimstatus == GRB_TIME_LIMIT   ? "TIMEOUT"
                                                                                              : "UNKNOWN";
    int result = 0;
    double clockTime = experimentRecord.timing.solver;
    double timeTime = clockTime;

    if (this->target == "truncBPTcryptanalysis") {

        if (optimstatus == GRB_OPTIMAL ||
            (optimstatus == GRB_TIME_LIMIT && model.get(GRB_IntAttr_SolCount) > 0)) {
            result = static_cast<int>(std::llround(model.get(GRB_DoubleAttr_ObjVal)));

            std::ofstream f(this->resultsPath, std::ios::trunc);
            f << "solving the MILP " << this->modelPath << "\n";
            f << "obj is : " << result << "\n";
            f << "clock time : " << clockTime << "s\n";
            f << "time time : " << timeTime << "s\n";

            map<std::string, int> dcVarMaps;
            experiment::Assignment assignment;
            GRBVar *a = model.getVars();
            int varNum = model.get(GRB_IntAttr_NumVars);
            for (int i = 0; i < varNum; ++i) {
                f << a[i].get(GRB_StringAttr_VarName) << " = " << a[i].get(GRB_DoubleAttr_X) << "\n";
                dcVarMaps[a[i].get(GRB_StringAttr_VarName)] = int(a[i].get(GRB_DoubleAttr_X));
                const double value = a[i].get(GRB_DoubleAttr_X);
                if (std::abs(value - std::round(value)) > 1e-5)
                    throw std::runtime_error("Nonbinary MILP assignment");
                assignment[a[i].get(GRB_StringAttr_VarName)] = static_cast<int>(std::llround(value));
            }
            delete[] a;
            f.close();
            experimentRecord.best = experimentRecord.recover(assignment, differentialCharacteristic,
                                                             "A", currentRound, result, 1.0 / beta);
            experimentRecord.best["gurobi_status"] = optimstatus;
            experimentRecord.best["solution_count"] = model.get(GRB_IntAttr_SolCount);
            const double objectiveBound = model.get(GRB_DoubleAttr_ObjBound);
            const double mipGap = model.get(GRB_DoubleAttr_MIPGap);
            if (std::isfinite(objectiveBound))
                experimentRecord.best["objective_bound_quantized"] = objectiveBound;
            if (std::isfinite(mipGap))
                experimentRecord.best["mip_gap"] = mipGap;
            experimentRecord.best["assignment_path"] = resultsPath;
            experimentRecord.best["model_path"] = modelPath;
            for (const auto &kv : assignment)
                experimentRecord.best["assignment"][kv.first] = kv.second;

            std::ofstream dcs(this->diffCharacteristicPath, std::ios::trunc);
            if (!dcs) {
                std::cout << "Wrong file path ! " << std::endl;
            } else {
                for (int j = 0; j < this->differentialCharacteristic.size(); ++j) {

                    if (j == 0)
                        dcs << "input difference\n";
                    else if (j > 0 && j < this->differentialCharacteristic.size() - 1)
                        dcs << j << "-th difference\n";
                    else
                        dcs << "out difference\n";
                    for (int k = 0; k < this->differentialCharacteristic[j].size(); ++k) {
                        dcs << dcVarMaps["A" + std::to_string(this->differentialCharacteristic[j][k])];
                        if ((k + 1) % 8 == 0)
                            dcs << " ";
                        if (k == this->differentialCharacteristic[j].size() - 1)
                            dcs << "\n";
                    }
                }
                dcs << "\n PCounter\n";

                int currPcounterSize = this->roundPcounterIndexSave[0].size();
                if (!this->BPTExtWeighted.empty()) {

                    int mcSize = this->roundPcounterIndexSave[0].size() / this->BPTExtWeighted.size();
                    for (int i = 0; i < this->roundPcounterIndexSave.size(); i++) {

                        for (int j = 0; j < this->roundPcounterIndexSave[i].size(); j += this->BPTExtWeighted.size()) {
                            int k = 0;
                            for (auto weight : this->BPTExtWeighted) {
                                int tt = i * currPcounterSize + j + k + 1;
                                if (weight != 1)
                                    dcs << weight * dcVarMaps["P" + std::to_string(tt)];
                                else
                                    dcs << dcVarMaps["P" + std::to_string(tt)];
                                k++;
                            }
                            dcs << " ";
                        }
                        dcs << "\n";
                    }

                } else {
                }
                dcs << "\n\n xorPCounter\n";
                if (this->xorPCounter > 1) {
                    for (int i = 1; i < this->xorPCounter; i++) {
                        dcs << dcVarMaps["xorP" + std::to_string(i)];
                        if (i % 4 == 0)
                            dcs << "\n";
                    }
                }
            }
            dcs.close();

            std::cout << "***********************************" << std::endl;
            std::cout << "      Obj is  : " << result << std::endl;
            std::cout << "***********************************" << std::endl;

            if (optimstatus == GRB_OPTIMAL)
                this->finalResults.push_back(result);
            this->finalClockTime.push_back(clockTime);
            this->finalTimeTime.push_back(timeTime);

            this->roundSolveResults.push_back({this->currentRound,
                                               optimstatus,
                                               result,
                                               clockTime,
                                               timeTime});
        } else if (optimstatus == GRB_INFEASIBLE) {
            std::cout
                << "************************* THE CONSTRUCTED MODEL ARE INFEASIBLE *************************"
                << std::endl;

            std::cout << "      clockTime is : " << clockTime << std::endl;
            std::cout << "      timeTime is  : " << timeTime << std::endl;

            this->roundSolveResults.push_back({this->currentRound,
                                               optimstatus,
                                               0,
                                               clockTime,
                                               timeTime});

            std::ofstream f(this->resultsPath, std::ios::trunc);
            f << "status : INFEASIBLE\n";
            f << "clock time : " << clockTime << "s\n";
            f << "time time : " << timeTime << "s\n";
        } else {
            this->roundSolveResults.push_back({this->currentRound, optimstatus, 0, clockTime, timeTime});
            std::ofstream f(this->resultsPath, std::ios::trunc);
            f << "status : " << experimentStatus << " (no feasible trajectory)\n";
        }
    } else if (this->target == "evaluation") {
        if (optimstatus == 2) {
            std::ofstream f(this->resultsPath, std::ios::trunc);
            f << "solving the MILP " << this->modelPath << "\n";
            f << "obj is : " << this->targetNdiff << "\n";
            f << "clock time : " << clockTime << "s\n";
            f << "time time : " << timeTime << "s\n";

            map<std::string, int> dcVarMaps;
            GRBVar *a = model.getVars();
            int varNum = model.get(GRB_IntAttr_NumVars);
            for (int i = 0; i < varNum; ++i) {
                f << a[i].get(GRB_StringAttr_VarName) << " = " << a[i].get(GRB_DoubleAttr_X) << "\n";
                dcVarMaps[a[i].get(GRB_StringAttr_VarName)] = int(a[i].get(GRB_DoubleAttr_X));
            }
            f.close();

            std::ofstream dcs(this->diffCharacteristicPath, std::ios::trunc);
            if (!dcs) {
                std::cout << "Wrong file path ! " << std::endl;
            } else {
                for (int j = 0; j < this->differentialCharacteristic.size(); ++j) {
                    if (j % 2 == 0)
                        dcs << j / 2 << "-th input difference\n";
                    else
                        dcs << (j - 1) / 2 << "-th output difference\n";
                    for (int k = 0; k < this->differentialCharacteristic[j].size(); ++k) {
                        dcs << dcVarMaps["A" + std::to_string(this->differentialCharacteristic[j][k])];
                        if ((k + 1) % 8 == 0)
                            dcs << " ";
                        if (k == this->differentialCharacteristic[j].size() - 1)
                            dcs << "\n";
                    }
                }
            }
            dcs.close();

            std::cout << "***********************************" << std::endl;
            std::cout << "      number of active sboxes  : " << this->targetNdiff << std::endl;
            std::cout << "      clockTime is  : " << clockTime << std::endl;
            std::cout << "      timeTime is  : " << timeTime << std::endl;
            std::cout << "***********************************" << std::endl;
            this->finalResults.push_back(this->targetNdiff);
            this->finalClockTime.push_back(clockTime);
            this->finalTimeTime.push_back(timeTime);
        } else if (optimstatus == 3) {
            this->securityFlag = true;
            this->securityRoundNumBound = this->currentRound;
            std::cout << "\n\n\n****************************************************************" << std::endl;
            std::cout << "the block cipher is security enough when the round number is : "
                      << this->currentRound << std::endl;
            std::cout << "****************************************************************\n\n\n"
                      << std::endl;

            std::cout << "***********************************" << std::endl;
            std::cout << "      clockTime is  : " << clockTime << std::endl;
            std::cout << "      timeTime is  : " << timeTime << std::endl;
            std::cout << "***********************************" << std::endl;
            this->finalClockTime.push_back(clockTime);
            this->finalTimeTime.push_back(timeTime);

            std::ofstream f(this->resultsPath, std::ios::trunc);
            f << "clock time : " << clockTime << "s\n";
            f << "time time : " << timeTime << "s\n";
            f.close();
        } else
            std::cout << "optimstatus : " << optimstatus << std::endl;
    }
}

void TruncDiffBPTSWMILP::programGenModel() {
    int roundCounter = this->currentRound;
    for (const auto &proc : this->procedureHs) {
        if (proc->getName() == "main") {
            std::string roundFuncId;
            bool newRoundFlag = false;
            int tempSizeCounter = 0;
            int roundFlag = false, modelCheckFlag = true;
            for (const auto &ele : proc->getBlock()) {
                if (ele->getLhs()->getNodeType() == UINT) {
                    newRoundFlag = true;
                    continue;
                }
                if (newRoundFlag and roundCounter > 0) {
                    if (ele->getNodeName() == "plaintext_push")
                        tempSizeCounter++;
                    if (ele->getOp() == ASTNode::CALL) {
                        this->blockSize = tempSizeCounter;
                        tempSizeCounter = 0;
                        roundFuncId = ele->getLhs()->getNodeName().substr(0, ele->getLhs()->getNodeName().find("@"));
                        for (const auto &tproc : procedureHs) {
                            if (tproc->getName() == roundFuncId) {

                                roundFunctionGenModel(tproc, roundCounter);
                                roundFlag = true;
                                break;
                            }
                        }
                        newRoundFlag = false;
                        roundCounter--;
                    }
                }
                if (roundFlag and modelCheckFlag) {
                    std::ifstream file;
                    file.open(modelPath);
                    std::string model, line;
                    while (getline(file, line))
                        model = model + line + "\n";
                    file.close();
                    if (model == "") {

                        this->tanNameMxIndex.clear();
                        this->rtnMxIndex.clear();
                        this->rtnIdxSave.clear();
                        this->dCounter = 1;
                        this->ACounter = 1;
                        this->PCounter = 1;
                        this->xorPCounter = 1;
                        roundFlag = false;
                        experimentRecord.clearModel();
                        this->differentialCharacteristic.clear();
                        this->roundSboxIndexSave.clear();
                        this->roundPcounterIndexSave.clear();
                        this->roundPboxMIndexSave.clear();
                        this->sboxFindFlag = false;
                        this->sboxIndexSave.clear();
                    } else
                        modelCheckFlag = false;
                }
            }
            break;
        }
    }
}

void TruncDiffBPTSWMILP::roundFunctionGenModel(const ProcedureHPtr &procedureH, int roundCounter) {
    std::string keyId = procedureH->getParameters().at(1).at(0)->getNodeName().substr(0, procedureH->getParameters().at(1).at(0)->getNodeName().find("0"));
    for (const auto &i : procedureH->getReturns()) {
        if (i->getNodeType() != NodeType::PARAMETER) {
            this->wordSize = Transformer::getNodeTypeSize(i->getNodeType());
            break;
        }
    }

    std::vector<int> currRoundSboxIndex;

    int currPcounter = this->PCounter;
    int currXorPCounter = this->xorPCounter;

    if (!this->rtnIdxSave.empty()) {
        for (int i = 0; i < this->rtnIdxSave.size(); ++i)
            this->tanNameMxIndex[procedureH->getParameters().at(2).at(i)->getNodeName()] = this->rtnIdxSave[i];
        this->rtnIdxSave.clear();
        this->rtnMxIndex.clear();
    } else {
        for (const auto &i : procedureH->getParameters().at(2)) {
            this->tanNameMxIndex[i->getNodeName()] = this->ACounter;
            this->ACounter++;
        }
        std::vector<int> inputC;
        for (int i = 1; i < this->ACounter; ++i) {
            inputC.push_back(i);
        }
        this->differentialCharacteristic.push_back(inputC);
    }

    bool functionCallFlag;
    for (int i = 0; i < procedureH->getBlock().size(); ++i) {
        functionCallFlag = false;
        ThreeAddressNodePtr ele = procedureH->getBlock().at(i);

        if (ele->getOp() == ASTNode::XOR) {
            if (ele->getLhs()->getNodeName().find(keyId) != std::string::npos or ele->getRhs()->getNodeName().find(keyId) != std::string::npos) {
            } else if (this->isConstant(ele->getLhs()) or this->isConstant(ele->getRhs())) {
            } else {
                functionCallFlag = true;
                XORGenModel(ele->getLhs(), ele->getRhs(), ele);
            }
        } else if (ele->getOp() == ASTNode::AND or ele->getOp() == ASTNode::OR) {
            functionCallFlag = true;
            ANDandORGenModel(ele->getLhs(), ele->getRhs(), ele);
        } else if (ele->getOp() == ASTNode::BOXOP) {
            if (ele->getLhs()->getNodeName().substr(0, 4) == "sbox") {
                sboxFindFlag = true;
            } else if (ele->getLhs()->getNodeName().substr(0, 4) == "pbox") {
                functionCallFlag = true;
                PboxGenModel(ele->getLhs(), ele->getRhs(), ele);
            }
        } else if (ele->getOp() == ASTNode::SYMBOLINDEX) {
            ThreeAddressNodePtr left = ele->getLhs();
            if (left->getOp() == ASTNode::FFTIMES) {
                if (left->getLhs()->getNodeName().substr(0, 5) == "pboxm") {
                    functionCallFlag = true;
                    std::vector<ThreeAddressNodePtr> output;
                    int pboxSize = pboxM[left->getLhs()->getNodeName()].size();
                    int outputNum = (int)sqrt(pboxSize);
                    while (outputNum > 0) {
                        output.push_back(ele);
                        i++;
                        if (i == procedureH->getBlock().size()) {
                            i--;
                            break;
                        } else {
                            ele = procedureH->getBlock().at(i);
                            left = ele->getLhs();
                            if (left != nullptr and left->getOp() == ASTNode::FFTIMES and
                                left->getLhs()->getNodeName().substr(0, 5) == "pboxm") {
                                outputNum--;
                                continue;
                            } else {
                                i--;
                                left = procedureH->getBlock().at(i)->getLhs();
                                break;
                            }
                        }
                    }
                    if (outputNum == 0)
                        i--;
                    left = procedureH->getBlock().at(i)->getLhs();
                    linearPermuteGenModel(left->getLhs(), left->getRhs(), output, this->currentRound - roundCounter);
                } else
                    assert(false);
            }
        } else if (ele->getOp() == ASTNode::TOUINT) {
        } else if (ele->getOp() == ASTNode::INDEX) {
        } else if (ele->getOp() == ASTNode::ADD or ele->getOp() == ASTNode::MINUS) {
            functionCallFlag = true;
            std::string outputName = ele->getNodeName().substr(0, ele->getNodeName().find_last_of("_"));
            std::vector<ThreeAddressNodePtr> input1, input2, output;
            while (true) {
                input1.push_back(ele->getLhs());
                input2.push_back(ele->getRhs());
                output.push_back(ele);
                i++;
                if (i == procedureH->getBlock().size()) {
                    i--;
                    break;
                }
                ele = procedureH->getBlock().at(i);
                std::string aaa = ele->getNodeName().substr(0, outputName.size());

                if (ele != nullptr and ele->getOp() == ASTNode::ADD and ele->getNodeName().substr(0, outputName.size()) == outputName)
                    continue;
                else {
                    i--;
                    break;
                }
            }
            AddGenModel(input1, input2, output);
        } else
            assert(false);

        if (!functionCallFlag) {
            if (ele->getLhs()->getNodeType() == NodeType::ARRAY and ele->getOp() == ASTNode::SYMBOLINDEX) {
                std::string finderName = ele->getLhs()->getNodeName() + "_$B$_" + ele->getRhs()->getNodeName();
                for (const auto &pair : this->tanNameMxIndex) {
                    if (finderName == pair.first)
                        this->tanNameMxIndex[ele->getNodeName()] = pair.second;
                }
            } else {
                for (const auto &pair : this->tanNameMxIndex) {
                    if (ele->getLhs() != nullptr)
                        if (ele->getLhs()->getNodeName() == pair.first)
                            this->tanNameMxIndex[ele->getNodeName()] = pair.second;
                    if (ele->getRhs() != nullptr)
                        if (ele->getRhs()->getNodeName() == pair.first)
                            this->tanNameMxIndex[ele->getNodeName()] = pair.second;
                }
            }
        } else
            functionCallFlag = false;

        if (this->sboxFindFlag) {
            this->sboxFindFlag = false;
            for (const auto &name : this->tanNameMxIndex) {
                if (name.first == ele->getNodeName()) {
                    this->sboxIndexSave.push_back(name.second);
                    currRoundSboxIndex.push_back(name.second);
                }
            }
        }
    }

    for (const auto &rtn : procedureH->getReturns()) {
        for (const auto &pair : this->tanNameMxIndex) {
            if (rtn->getNodeName() == pair.first) {
                this->rtnMxIndex[pair.first] = pair.second;
                this->rtnIdxSave.push_back(pair.second);
            }
        }
    }

    std::vector<int> outputC;
    for (int &i : this->rtnIdxSave) {
        outputC.push_back(i);
    }
    this->differentialCharacteristic.push_back(outputC);
    experimentRecord.saveNodes(this->currentRound - roundCounter + 1, tanNameMxIndex, "A");

    this->roundSboxIndexSave.push_back(currRoundSboxIndex);

    std::vector<int> PcounterIndexSave;
    for (int i = currPcounter; i < this->PCounter; ++i) {
        PcounterIndexSave.push_back(i);
    }
    this->roundPcounterIndexSave.push_back(PcounterIndexSave);

    bool isPthenS = true;

    if (roundPboxMIndexSave[this->currentRound - roundCounter].size() == currRoundSboxIndex.size()) {

        for (int i = 0; i < outputC.size(); ++i) {

            if (std::find(currRoundSboxIndex.begin(), currRoundSboxIndex.end(), outputC[i]) == currRoundSboxIndex.end()) {
                isPthenS = false;
                break;
            }
        }
    } else

    {
        isPthenS = false;
        for (int i = 0; i < roundPboxMIndexSave[this->currentRound - roundCounter].size(); ++i) {

            if (std::find(currRoundSboxIndex.begin(), currRoundSboxIndex.end(), roundPboxMIndexSave[this->currentRound - roundCounter][i]) != currRoundSboxIndex.end()) {
                isPthenS = true;
                break;
            }
        }
    }

    if (roundCounter == 1 && !isPthenS && (this->currentRound - roundCounter > 0)) {

        std::ofstream scons(this->modelPath, std::ios::app);
        if (!scons) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {

            int currPcounterSize = PcounterIndexSave.size();
            if (!this->BPTExtWeighted.empty()) {

                for (int i = 1; i < this->PCounter - currPcounterSize; i += this->BPTExtWeighted.size()) {
                    int k = 0;
                    for (auto weight : this->BPTExtWeighted) {
                        if (weight != 1)
                            scons << weight << " P" << i + k;
                        else
                            scons << "P" << i + k;

                        if (i + k != this->PCounter - currPcounterSize - 1)
                            scons << " + ";
                        k++;
                    }
                }
            } else {

                for (int i = 1; i < this->PCounter - currPcounterSize; i++) {
                    scons << "P" << i;
                    if (i != this->PCounter - currPcounterSize - 1)
                        scons << " + ";
                }
            }

            if (currXorPCounter > 1) {
                scons << " + ";
                for (int i = 1; i < currXorPCounter; i++) {
                    scons << "xorP" << i;
                    if (i != currXorPCounter - 1)
                        scons << " + ";
                }
            }

            scons << " + ";

            long long param = this->beta * this->wordSize;
            if (this->blockSize == currRoundSboxIndex.size()) {

                for (int i = 0; i < currRoundSboxIndex.size() - 1; ++i) {
                    scons << param << " " << "A" << currRoundSboxIndex[i] << " + ";
                }
                scons << param << " " << "A" << currRoundSboxIndex[currRoundSboxIndex.size() - 1] << " <= " << param * this->blockSize - 1 << "\n";

            }

            else

            {

                int DClength = this->differentialCharacteristic.size() - 2;
                if (DClength > -1) {
                    for (int i = 0; i < this->differentialCharacteristic[DClength].size() - 1; i++) {
                        scons << "A" << this->differentialCharacteristic[DClength][i] << " + ";
                    }
                    scons << "A" << differentialCharacteristic[DClength][this->differentialCharacteristic[DClength].size() - 1] << " <= " << this->blockSize - 1 << "\n";
                }
            }

            scons.close();
        }
    }

    if (roundCounter == 1 && isPthenS) {

        std::ofstream scons(this->modelPath, std::ios::app);
        if (!scons) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {

            if (!this->BPTExtWeighted.empty()) {

                for (int i = 1; i < this->PCounter; i += this->BPTExtWeighted.size()) {
                    int k = 0;
                    for (auto weight : this->BPTExtWeighted) {
                        if (weight != 1)
                            scons << weight << " P" << i + k;
                        else
                            scons << "P" << i + k;
                        if (i + k != this->PCounter - 1)
                            scons << " + ";
                        k++;
                    }
                }
            } else {

                for (int i = 1; i < this->PCounter; i++) {
                    scons << "P" << i;
                    if (i != this->PCounter - 1)
                        scons << " + ";
                }
            }

            if (this->xorPCounter > 1) {
                scons << " + ";
                for (int i = 1; i < this->xorPCounter; i++) {
                    scons << "xorP" << i;
                    if (i != this->xorPCounter - 1)
                        scons << " + ";
                }
            }

            if (this->PCounter > 1)
                scons << " + ";

            long long param = this->beta * this->wordSize;
            for (int i = 0; i < outputC.size() - 1; ++i) {
                scons << param << " " << "A" << outputC[i] << " + ";
            }

            scons << param << " " << "A" << outputC[outputC.size() - 1];

            scons << " <= " << param * this->blockSize - 1 << "\n";

            scons.close();
        }
    }

    if (roundCounter == 1) {
        std::ofstream scons(this->modelPath, std::ios::app);
        if (!scons) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {

            for (int i = 0; i < outputC.size() - 1; ++i) {
                scons << "A" << outputC[i] << " + ";
            }
            scons << "A" << outputC[outputC.size() - 1] << " <= " << this->blockSize - 1 << "\n";

            scons.close();
        }
    }
}

void TruncDiffBPTSWMILP::XORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
                                     const ThreeAddressNodePtr &result) {
    int inputIdx1 = 0, inputIdx2 = 0;
    for (const auto &pair : tanNameMxIndex) {
        if (pair.first == left->getNodeName())
            inputIdx1 = pair.second;
        if (pair.first == right->getNodeName())
            inputIdx2 = pair.second;
    }
    std::vector<int> inputIdx = {inputIdx1, inputIdx2};
    for (auto item : inputIdx) {
        if (item == 0) {
            if (!this->rtnIdxSave.empty()) {
                item = this->rtnIdxSave.front();
                this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                this->tanNameMxIndex[left->getNodeName()] = item;
            } else {
                item = this->ACounter;
                this->tanNameMxIndex[left->getNodeName()] = item;
                this->ACounter++;
            }
        }
    }
    int outputIdx = ACounter;
    tanNameMxIndex[result->getNodeName()] = outputIdx;
    ACounter++;
    switch (this->xorConSel) {
    case 1:

        TruncDiffBPTMILPcons::wXorC3(this->modelPath, inputIdx1, inputIdx2, outputIdx, dCounter, xorPCounter);
        break;
    case 2:
        TruncDiffBPTMILPcons::wXorC2(this->modelPath, inputIdx1, inputIdx2, outputIdx);
        break;
    default:
        TruncDiffBPTMILPcons::wXorC3(this->modelPath, inputIdx1, inputIdx2, outputIdx, dCounter, xorPCounter);
    }
}

void TruncDiffBPTSWMILP::ANDandORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
                                          const ThreeAddressNodePtr &result) {
    int inputIdx1 = 0, inputIdx2 = 0;
    for (const auto &pair : tanNameMxIndex) {
        if (pair.first == left->getNodeName())
            inputIdx1 = pair.second;
        if (pair.first == right->getNodeName())
            inputIdx2 = pair.second;
    }
    std::vector<int> inputIdx = {inputIdx1, inputIdx2};
    for (auto item : inputIdx) {
        if (item == 0) {
            if (!this->rtnIdxSave.empty()) {
                item = this->rtnIdxSave.front();
                this->rtnIdxSave.erase(this->rtnIdxSave.cbegin());
                this->tanNameMxIndex[left->getNodeName()] = item;
            } else {
                item = this->ACounter;
                this->tanNameMxIndex[left->getNodeName()] = item;
                this->ACounter++;
            }
        }
    }
    int outputIdx = ACounter;
    tanNameMxIndex[result->getNodeName()] = outputIdx;
    ACounter++;
    TruncDiffBPTMILPcons::wAndOrC(this->modelPath, inputIdx1, inputIdx2, outputIdx);
}

void TruncDiffBPTSWMILP::PboxGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input,
                                      const ThreeAddressNodePtr &output) {
    std::vector<int> pboxValue = this->Box[pbox->getNodeName()];
    std::vector<int> inputIdx = extIdxFromTOUINTorBOXINDEX(input);
    std::vector<int> outputIdx;
    for (int i = 0; i < pboxValue.size(); ++i)
        outputIdx.push_back(0);
    for (int i = 0; i < pboxValue.size(); ++i)
        outputIdx[i] = inputIdx[pboxValue[i]];
    for (int i = 0; i < outputIdx.size(); ++i)
        this->tanNameMxIndex[output->getNodeName() + "_$B$_" + std::to_string(i)] = outputIdx[i];
}

void TruncDiffBPTSWMILP::linearPermuteGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input,
                                               const vector<ThreeAddressNodePtr> &output, const int curRoundNumber) {
    std::vector<int> inputIdx;
    std::vector<ThreeAddressNodePtr> inputTAN;
    if (input->getOp() == ASTNode::BOXINDEX) {
        ThreeAddressNodePtr left = input;
        while (left->getOp() == ASTNode::BOXINDEX) {
            inputTAN.push_back(left->getLhs());
            left = left->getRhs();
        }
        inputTAN.push_back(left);
    } else
        assert(false);

    for (auto &i : inputTAN) {
        bool iFlag = false;
        for (const auto &pair : tanNameMxIndex) {
            if (pair.first == i->getNodeName()) {
                inputIdx.push_back(pair.second);
                iFlag = true;
            }
        }
        if (!iFlag) {
            inputIdx.push_back(ACounter);
            tanNameMxIndex[i->getNodeName()] = ACounter;
            ACounter++;
        }
    }

    std::vector<int> outputIdx;
    for (const auto &i : output) {
        outputIdx.push_back(ACounter);
        tanNameMxIndex[i->getNodeName()] = ACounter;
        ACounter++;
    }

    if (this->roundPboxMIndexSave.size() < curRoundNumber + 1) {
        this->roundPboxMIndexSave.push_back(outputIdx);
    }

    else {
        this->roundPboxMIndexSave[curRoundNumber].insert(this->roundPboxMIndexSave[curRoundNumber].end(), outputIdx.begin(), outputIdx.end());
    }

    if (this->cipherName == "MIBS_word_wise")
        this->branchNumMin[pbox->getNodeName()] = 5;
    int extLen = this->BPTExtWeighted.size();
    const int firstProbability = this->PCounter;
    switch (this->linearConSel) {
    case 1:

        TruncDiffBPTMILPcons::wLinearBPT(this->modelPath, inputIdx, outputIdx, dCounter, branchNumMin[pbox->getNodeName()], branchNumMax[pbox->getNodeName()], this->PCounter, this->BPTIneqs[pbox->getNodeName()], this->BPTenqs[pbox->getNodeName()], extLen);
        break;
    case 2:
        TruncDiffBPTMILPcons::wLinearC2(this->modelPath, inputIdx, outputIdx, dCounter, branchNumMin[pbox->getNodeName()], branchNumMax[pbox->getNodeName()]);
        break;
    default:

        TruncDiffBPTMILPcons::wLinearBPT(this->modelPath, inputIdx, outputIdx, dCounter, branchNumMin[pbox->getNodeName()], branchNumMax[pbox->getNodeName()], this->PCounter, this->BPTIneqs[pbox->getNodeName()], this->BPTenqs[pbox->getNodeName()], extLen);
    }
    std::vector<int> probabilities;
    for (int id = firstProbability; id < PCounter; ++id)
        probabilities.push_back(id);
    experimentRecord.locals.push_back({curRoundNumber + 1, pbox->getNodeName(),
                                       experiment::names(inputIdx, "A"), experiment::names(outputIdx, "A"), experiment::names(probabilities, "P")});
}

void TruncDiffBPTSWMILP::AddGenModel(vector<ThreeAddressNodePtr> &input1, vector<ThreeAddressNodePtr> &input2,
                                     const vector<ThreeAddressNodePtr> &output) {
    std::vector<int> inputIdx1, inputIdx2;
    for (auto in : input1) {
        if (tanNameMxIndex.count(in->getNodeName()) != 0) {
            inputIdx1.push_back(tanNameMxIndex.find(in->getNodeName())->second);
        } else if (!rtnIdxSave.empty()) {
            tanNameMxIndex[in->getNodeName()] = rtnIdxSave.front();
            inputIdx1.push_back(rtnIdxSave.front());
            rtnIdxSave.erase(rtnIdxSave.cbegin());
        } else {
            tanNameMxIndex[in->getNodeName()] = ACounter;
            inputIdx1.push_back(ACounter);
            ACounter++;
        }
    }
    for (auto in : input2) {
        if (tanNameMxIndex.count(in->getNodeName()) != 0) {
            inputIdx2.push_back(tanNameMxIndex.find(in->getNodeName())->second);
        } else if (!rtnIdxSave.empty()) {
            tanNameMxIndex[in->getNodeName()] = rtnIdxSave.front();
            inputIdx2.push_back(rtnIdxSave.front());
            rtnIdxSave.erase(rtnIdxSave.cbegin());
        } else {
            tanNameMxIndex[in->getNodeName()] = ACounter;
            inputIdx2.push_back(ACounter);
            ACounter++;
        }
    }

    std::vector<int> outputIdx;
    for (const auto &ele : output) {
        tanNameMxIndex[ele->getNodeName()] = ACounter;
        outputIdx.push_back(ACounter);
        ACounter++;
    }

    for (int i = 0; i < inputIdx1.size(); ++i) {
        switch (xorConSel) {
        case 1:
            TruncDiffBPTMILPcons::wXorC3(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i], dCounter, xorPCounter);

            break;
        case 2:
            TruncDiffBPTMILPcons::wXorC2(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i]);
            break;
        default:
            TruncDiffBPTMILPcons::wXorC3(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i], dCounter, xorPCounter);
        }
    }
}

bool TruncDiffBPTSWMILP::isConstant(ThreeAddressNodePtr threeAddressNodePtr) {
    ThreeAddressNodePtr left = threeAddressNodePtr->getLhs();
    ThreeAddressNodePtr right = threeAddressNodePtr->getRhs();

    if (threeAddressNodePtr->getNodeType() == UINT)
        return true;
    else if (threeAddressNodePtr->getNodeName() == this->roundID)
        return true;
    else if (left == nullptr and right == nullptr)
        return false;
    else if (left != nullptr and right != nullptr) {
        if (left->getNodeType() == UINT and right->getNodeType() == UINT)
            return true;
        else if (left->getNodeType() == UINT and right->getNodeName() == roundID)
            return true;
        else if (right->getNodeType() == UINT and left->getNodeName() == roundID)
            return true;
        else
            return false;
    } else if (left != nullptr and right == nullptr) {
        if (left->getNodeType() == UINT)
            return true;
        if (left->getNodeName() == this->roundID)
            return true;
        else if (isConstant(left))
            return true;
        else
            return false;
    } else if (left == nullptr and right != nullptr) {
        if (right->getNodeType() == UINT)
            return true;
        if (right->getNodeName() == this->roundID)
            return true;
        else if (isConstant(right))
            return true;
        else
            return false;
    } else
        assert(false);
}
