

#include <cstdlib>
#include <sys/wait.h>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <truncatedDifferentialBPT/TruncDiffBPTSWSAT.h>
#include "truncatedDifferentialBPT/TruncDiffBPTSATcons.h"

extern std::map<std::string, std::vector<int>> allBox;
extern map<std::string, std::vector<int>> pboxM;
extern std::map<std::string, int> pboxMSize;
extern map<std::string, std::vector<int>> Ffm;
extern std::string cipherName;

namespace {
void writeSATCharacteristic(const std::string &path, const std::string &status,
                            const Json::Value &best) {
    std::ofstream out(path, std::ios::trunc);
    if (!out)
        throw std::runtime_error("Cannot write SAT characteristic: " + path);
    out << "status: " << status << '\n';
    if (best.isNull()) {
        out << "trajectory_available: false\n";
    } else {
        const auto &states = best["round_states"];
        if (!states.isArray() || states.empty())
            throw std::runtime_error("Missing recovered SAT round states");
        out << std::setprecision(17);
        out << "objective_quantized: " << best["objective_quantized"].asInt64() << '\n';
        out << "quantized_total_weight: " << best["quantized_total_weight"].asDouble() << '\n';
        out << "raw_total_weight: " << best["raw_total_weight"].asDouble() << '\n';
        out << "raw_probability: " << best["raw_probability"].asDouble() << '\n';
        auto writePattern = [&out](const std::string &pattern) {
            for (size_t i = 0; i < pattern.size(); ++i) {
                out << pattern[i];
                if ((i + 1) % 8 == 0 && i + 1 < pattern.size())
                    out << ' ';
            }
            out << '\n';
        };
        for (Json::ArrayIndex i = 0; i < states.size(); ++i) {
            out << (i == 0 ? "input difference" : std::to_string(i) + "-th difference") << '\n';
            writePattern(states[i]["input"]["active_word_pattern"].asString());
        }
        out << "out difference\n";
        writePattern(states[states.size() - 1]["output"]["active_word_pattern"].asString());
        out << "local propagations\n";
        for (const auto &local : best["local_propagations"]) {
            out << "round " << local["round_index"].asInt()
                << " BPT " << local["bpt"].asString()
                << " row " << local["bpt_row"].asUInt64()
                << " column " << local["bpt_column"].asUInt64()
                << " encoding " << local["probability_encoding_assignment"].asString()
                << " p " << local["raw_bpt_probability"].asDouble() << '\n';
        }
    }
    if (!out)
        throw std::runtime_error("Failed writing SAT characteristic: " + path);
}
}

TruncDiffBPTSWSAT::TruncDiffBPTSWSAT(const vector<ProcedureHPtr> &procedureHs, std::string target)
    : procedureHs(procedureHs), target(move(target)) {
    this->Box = allBox;
    this->cipherName = ::cipherName;
}

void TruncDiffBPTSWSAT::clearPerModelObjectiveState() {

    this->round_pb_vars.clear();
    this->round_pb_weights.clear();
}

void TruncDiffBPTSWSAT::collectObjectiveTerms(std::vector<int32_t> &literals, std::vector<int64_t> &weights) const {
    literals.clear();
    weights.clear();

    if (this->round_pb_vars.size() != this->round_pb_weights.size()) {
        throw std::logic_error("collectObjectiveTerms: round PB variable/weight block count mismatch.");
    }

    for (size_t r = 0; r < this->round_pb_vars.size(); ++r) {

        if (this->round_pb_vars[r].size() != this->round_pb_weights[r].size()) {
            throw std::logic_error("collectObjectiveTerms: round PB literals and weights size mismatch.");
        }
        literals.insert(literals.end(),
                        this->round_pb_vars[r].begin(),
                        this->round_pb_vars[r].end());
        weights.insert(weights.end(),
                       this->round_pb_weights[r].begin(),
                       this->round_pb_weights[r].end());
    }

    if (literals.size() != weights.size()) {
        throw std::logic_error("collectObjectiveTerms: literals and weights size mismatch!");
    }
}

void TruncDiffBPTSWSAT::collectSCTerms(std::vector<int32_t> &literals) const {
    literals.clear();

    literals.reserve(dp_weight.size());

    literals.insert(literals.end(), dp_weight.begin(), dp_weight.end());
}

void TruncDiffBPTSWSAT::emitSCSequentialBound(const std::vector<int> &literals,
                                              std::vector<std::vector<int>> &auxiliary_var_u,
                                              int64_t bound) {
    if (bound < 0) {
        throw std::invalid_argument("emitSCSequentialBound: bound must be non-negative.");
    }
    if (bound > static_cast<int64_t>(literals.size())) {
        return;
    }
    if (bound > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("emitSCSequentialBound: bound exceeds int range.");
    }

    auxiliary_var_u.clear();
    auxiliary_var_u.reserve(literals.size());
    for (int i = 0; i < static_cast<int>(literals.size()); ++i) {
        auxiliary_var_u.push_back(std::vector<int>());
        for (int64_t j = 0; j < bound; ++j) {
            auxiliary_var_u.back().push_back(this->ACounter++);
        }
    }

    std::vector<int> sc_literals = literals;
    TruncDiffBPTSATcons::SequentialEncoding(
        this->modelPath,
        sc_literals,
        auxiliary_var_u,
        static_cast<int>(sc_literals.size()),
        static_cast<int>(bound),
        this->clauseCounter);
}

void TruncDiffBPTSWSAT::emitWeightedBound(const std::vector<int32_t> &literals,
                                          const std::vector<int64_t> &weights,
                                          int64_t bound) {
    if (this->objectiveEncoding == ObjectiveEncoding::BDD) {
        TruncDiffBPTSATcons::wPBConstraint(this->modelPath, literals, weights, bound, this->ACounter,
                                           this->clauseCounter);
    } else if (this->objectiveEncoding == ObjectiveEncoding::SWC) {
        TruncDiffBPTSATcons::wSWCConstraint(this->modelPath, literals, weights, bound, this->ACounter,
                                            this->clauseCounter);
    } else {
        return;
    }
}

void TruncDiffBPTSWSAT::emitObjectiveBound(int64_t bound) {
    std::vector<int32_t> literals;
    std::vector<int64_t> weights;

    collectObjectiveTerms(literals, weights);

    emitWeightedBound(literals, weights, bound);
}

bool TruncDiffBPTSWSAT::writeCnfHeader() {
    std::string tempPath = this->modelPath + ".tmp";
    std::ifstream inFile(this->modelPath);
    std::ofstream outFile(tempPath, std::ios::trunc);

    if (!inFile || !outFile) {
        std::cout << "Wrong file path during CNF header generation!" << std::endl;
        return false;
    }

    outFile << "p cnf " << this->ACounter - 1 << " " << this->clauseCounter + 1 << "\n";
    for (int i = 0; i < this->blockSize; ++i) {
        if (i != this->blockSize - 1) {
            outFile << i + 1 << " ";
        }
    }
    outFile << this->blockSize << " 0\n";

    std::string line;
    while (std::getline(inFile, line)) {
        outFile << line << "\n";
    }

    inFile.close();
    outFile.close();
    std::rename(tempPath.c_str(), this->modelPath.c_str());
    return true;
}

TruncDiffBPTSWSAT::MallobStatus TruncDiffBPTSWSAT::runMallob(
    const std::string &modelPath,
    const std::string &solutionPath,
    long long timeoutSeconds) {

    if (encodingPending) {
        experimentRecord.timing.encoding += experiment::seconds(encodingStart);
        encodingPending = false;
    }
    if (initialBuildStarted && experimentRecord.timing.initialBuildWall < 0)
        experimentRecord.timing.initialBuildWall = experiment::seconds(initialBuildStart);

    if (timeoutSeconds <= 0) {
        return MallobStatus::TIMEOUT;
    }

    std::string cmd = "timeout --signal=TERM --kill-after=10s " + std::to_string(timeoutSeconds) + "s mallob -v=0 -mono='" + modelPath + "' -t=" + std::to_string(this->gurobiThreads) + " > '" + solutionPath + "' 2> '" + solutionPath + ".stderr'";

    ++experimentRecord.timing.calls;
    int ret;
    {
        experiment::Timer measured(experimentRecord.timing.solver);
        ret = system(cmd.c_str());
    }

    if (ret == -1) {
        return MallobStatus::UNKNOWN;
    }

    if (WIFEXITED(ret)) {
        const int exitCode = WEXITSTATUS(ret);

        if (exitCode == 124 || exitCode == 137) {
            return MallobStatus::TIMEOUT;
        }
    }

    if (WIFSIGNALED(ret)) {
        return MallobStatus::UNKNOWN;
    }

    std::ifstream solution(solutionPath);
    if (!solution) {
        return MallobStatus::UNKNOWN;
    }

    std::string line;
    while (std::getline(solution, line)) {
        if (line.find("s UNSATISFIABLE") != std::string::npos) {
            return MallobStatus::UNSAT;
        }

        if (line.find("s SATISFIABLE") != std::string::npos) {
            try {
                const auto assignment = experiment::readAssignment(solutionPath);
                std::vector<int32_t> vars;
                std::vector<int64_t> weights;
                if (objectiveEncoding == ObjectiveEncoding::SC) {
                    collectSCTerms(vars);
                    weights.assign(vars.size(), 1);
                } else
                    collectObjectiveTerms(vars, weights);
                const auto objective = experiment::weighted(assignment, vars, weights);
                if (experimentRecord.best.isNull() ||
                    objective < experimentRecord.best["objective_quantized"].asInt64()) {
                    const double unit = encodingType == BptEncodingType::ONE_HOT ? wordSize : 1.0 / beta;
                    auto candidate = experimentRecord.recover(assignment, differentialCharacteristic,
                                                              "", currentRound, objective, unit);
                    const auto stem = resultsPath + "Round" + std::to_string(currentRound) + "-best";
                    std::filesystem::copy_file(modelPath, stem + ".cnf", std::filesystem::copy_options::overwrite_existing);
                    std::filesystem::copy_file(solutionPath, stem + ".out", std::filesystem::copy_options::overwrite_existing);
                    candidate["assignment_path"] = stem + ".out";
                    candidate["model_path"] = stem + ".cnf";

                    for (const auto &kv : assignment)
                        candidate["assignment"][kv.first] = kv.second;
                    experimentRecord.best = std::move(candidate);
                }
            } catch (const std::exception &error) {
                experimentRecord.error = error.what();
                std::cerr << "[RECOVERY ERROR] " << error.what() << std::endl;
                return MallobStatus::RECOVERY_ERROR;
            }
            return MallobStatus::SAT;
        }
    }

    return MallobStatus::UNKNOWN;
}

void TruncDiffBPTSWSAT::MGR() {
    double sharedPreprocessing = 0;
    {
        experiment::Timer measured(sharedPreprocessing);
        preprocess(pboxM, Ffm);
    }

    this->satTime.clear();
    this->unsatTime.clear();

    this->finalResults.clear();
    this->finalClockTime.clear();
    this->finalTimeTime.clear();
    this->finalStatus.clear();

    this->round_pb_vars.clear();
    this->round_pb_weights.clear();

    this->pathPrefix += target + "/";
    this->modelPath = this->pathPrefix + "models/" + this->pathSuffix;
    this->resultsPath = this->pathPrefix + "results/" + this->pathSuffix;
    this->diffCharacteristicPath = this->pathPrefix + "differentialCharacteristics/" + this->pathSuffix;
    system(("mkdir -p " + this->modelPath).c_str());
    system(("mkdir -p " + this->resultsPath).c_str());
    system(("mkdir -p " + this->diffCharacteristicPath).c_str());
    this->finalResultsPath = this->resultsPath + this->cipherName + "_FinalResults.txt";
    system(("touch " + this->finalResultsPath).c_str());

    for (int i = this->startRound; i <= endRound; ++i) {
        if (this->securityFlag)
            break;

        experimentRecord.timing = experiment::Timing{};
        this->currentRoundStatus = 0;
        this->currentRound = i;
        experimentRecord.best = Json::Value();
        experimentRecord.error.clear();
        encodingPending = false;
        initialBuildStarted = false;

        std::cout << "\n **************** CURRENT MAX ROUOND : " << this->currentRound << "  **************** \n"
                  << std::endl;
        system(("touch " + this->resultsPath).c_str());
        this->diffCharacteristicPath = this->pathPrefix + "differentialCharacteristics/" + this->pathSuffix +
                                       std::to_string(this->currentRound) + "_round_differential_characteristic.txt";
        system(("touch " + this->diffCharacteristicPath).c_str());

        if (this->target == "cryptanalysis") {
            vector<double> finalTotalRoundClockTime;
            vector<double> finalTotalRoundTimeTime;
            clock_t startTime, endTime;
            startTime = clock();

            if (this->solverch == "unsat") {
                solver_unsat();
            } else if (this->solverch == "bs") {
                solver_BS();
            } else if (this->solverch == "sat" || this->solverch.empty()) {
                solver();
            } else {
                solver();
            }

            endTime = clock();

            double clockTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;

            finalTotalRoundClockTime.push_back(clockTime);
            finalTotalRoundTimeTime.push_back(timeTime);

            if (this->solverch == "unsat") {
                this->finalResults.push_back(countProbability < 0 ? 0 : countProbability + 1);
            } else {
                this->finalResults.push_back(countProbability);
            }
            this->finalClockTime.push_back(clockTime);
            this->finalTimeTime.push_back(timeTime);

            this->finalStatus.push_back(currentRoundStatus);

            if (solverch == "sat" || solverch.empty() || solverch == "bs" || solverch == "unsat") {
                if (encodingPending) {
                    experimentRecord.timing.encoding += experiment::seconds(encodingStart);
                    encodingPending = false;
                }
                if (currentRoundStatus == 0 && (experimentRecord.best.isNull() ||
                                                experimentRecord.best["objective_quantized"].asInt64() != finalResults.back()))
                    throw std::runtime_error("Final SAT objective has no matching recovered assignment");
                Json::Value config;
                config["beta"] = Json::Int64(beta);
                config["encoding_type"] = encodingType == BptEncodingType::ONE_HOT ? "ONE_HOT" : "MAX_SMT";
                config["objective_encoding"] = objectiveEncoding == ObjectiveEncoding::SC ? "SC" : objectiveEncoding == ObjectiveEncoding::SWC ? "SWC"
                                                                                                                                               : "BDD";
                config["search"] = solverch;
                config["threads"] = gurobiThreads;
                config["timer"] = gurobiTimer;
                const std::string status = currentRoundStatus == 0 ? "OPTIMAL" : currentRoundStatus == 1 ? "UNSAT"
                                                                             : currentRoundStatus == 2   ? "TIMEOUT"
                                                                             : currentRoundStatus == 4   ? "RECOVERY_ERROR"
                                                                                                         : "UNKNOWN";
                if (currentRoundStatus == 4)
                    config["last_solver_status"] = "SAT";
                experimentRecord.write(resultsPath + "Round" + std::to_string(currentRound) + ".json",
                                       cipherName, currentRound, "SAT", alpha, status, config, i != startRound,
                                       i == startRound ? sharedPreprocessing : 0.0);
                writeSATCharacteristic(diffCharacteristicPath, status, experimentRecord.best);
            }
        }

        else
            assert(false);

        std::ofstream fResults(this->finalResultsPath, std::ios::trunc);
        if (!fResults) {
            std::cout << "Wrong file path ! " << std::endl;
        } else {
            if (this->target == "cryptanalysis") {
                fResults << " ***************** FINAL RESULTS ***************** \n\n";
                for (int j = 0; j < this->finalResults.size(); ++j) {
                    fResults << "Results Of " << j + 1 << " Rounds : \n";

                    if (this->finalStatus[j] == 2) {
                        fResults << "\tObj : TIMEOUT (Time limit exceeded before finding optimal bound)" << "\n";
                    } else if (this->finalStatus[j] == 1) {
                        fResults << "\tObj : UNSAT (Model is completely unsatisfiable)" << "\n";
                    } else if (this->finalStatus[j] == 4) {
                        fResults << "\tObj : RECOVERY_ERROR (solver returned SAT; assignment/trajectory recovery failed; optimum not established)\n";
                    } else if (this->finalStatus[j] == 3) {
                        fResults << "\tObj : UNKNOWN (solver returned no conclusive status)\n";
                    } else {
                        fResults << "\tObj : " << this->finalResults[j] << "\n";
                    }

                    fResults << "\tclockTime : " << this->finalClockTime[j] << "\n";
                    fResults << "\ttimeTime : " << this->finalTimeTime[j] << "\n\n";

                    if (this->solverch == "unsat" || this->solverch == "bs") {
                        fResults << "per_sat_time ";
                        for (int k = 0; k < this->satTime[j].size(); ++k) {
                            fResults << satTime[j][k] << " ";
                        }
                        fResults << "\n\n";
                    }
                    if (this->solverch == "sat" || this->solverch == "bs") {
                        fResults << "per_unsat_time ";

                        if (j < this->unsatTime.size()) {
                            for (int k = 0; k < this->unsatTime[j].size(); ++k) {
                                fResults << unsatTime[j][k] << " ";
                            }
                        }
                        fResults << "\n\n";
                    }
                }
                fResults << " ***************** FINAL RESULTS ***************** \n";
            }
        }

        this->tanNameMxIndex.clear();
        this->rtnMxIndex.clear();
        this->rtnIdxSave.clear();
        this->dCounter = 1;
        this->ACounter = 1;
        this->clauseCounter = 0;
        this->differentialCharacteristic.clear();
        this->roundSboxIndexSave.clear();
        this->sboxFindFlag = false;
        this->sboxIndexSave.clear();
        this->auxiliary_var_u.clear();

        this->round_pb_vars.clear();
        this->round_pb_weights.clear();

        this->PCounter = 1;
        this->roundPcounterIndexSave.clear();
        this->roundPboxMIndexSave.clear();
        this->dp_weight.clear();
        this->xorPIndexSave.clear();
        this->curRoundWeightSize = 0;

        if (this->currentRoundStatus != 0) {
            break;
        }
    }
    if (target == "cryptanalysis") {
        std::cout << "\n ***************** FINAL RESULTS ***************** " << std::endl;
        for (int i = 0; i < this->finalResults.size(); ++i) {
            std::cout << "Results Of " << i + 1 << " Rounds : " << std::endl;

            if (this->finalStatus[i] == 2) {
                std::cout << "\tObj : TIMEOUT (Time limit exceeded before finding optimal bound)" << std::endl;
            } else if (this->finalStatus[i] == 4) {
                std::cout << "\tObj : RECOVERY_ERROR (solver returned SAT; assignment/trajectory recovery failed; optimum not established)" << std::endl;
            } else if (this->finalStatus[i] == 3) {
                std::cout << "\tObj : UNKNOWN (solver returned no conclusive status)" << std::endl;
            } else if (this->finalStatus[i] == 1) {
                std::cout << "\tObj : UNSAT (Model is completely unsatisfiable)" << std::endl;
            } else {
                std::cout << "\tObj : " << this->finalResults[i] << std::endl;
            }

            std::cout << "\tclockTime : " << this->finalClockTime[i] << std::endl;
            std::cout << "\ttimeTime : " << this->finalTimeTime[i] << std::endl
                      << std::endl;
            if (this->solverch == "unsat" || this->solverch == "bs") {
                std::cout << "per_sat_time ";
                for (int k = 0; k < this->satTime[i].size(); ++k) {
                    std::cout << satTime[i][k] << " ";
                }
                std::cout << std::endl
                          << std::endl;
            }
            if (this->solverch == "sat" || this->solverch == "bs") {
                std::cout << "per_unsat_time ";

                if (i < this->unsatTime.size()) {
                    for (int k = 0; k < this->unsatTime[i].size(); ++k) {
                        std::cout << unsatTime[i][k] << " ";
                    }
                }
                std::cout << std::endl
                          << std::endl;
            }
        }
        std::cout << " ***************** FINAL RESULTS ***************** \n"
                  << std::endl;
    }
}

void TruncDiffBPTSWSAT::cryptanalysis() {
    if (!initialBuildStarted) {
        initialBuildStart = experiment::Clock::now();
        initialBuildStarted = true;
    }
    const auto modelStart = experiment::Clock::now();
    experimentRecord.clearModel();
    programGenModel();


    experimentRecord.timing.model += experiment::seconds(modelStart);
    encodingStart = experiment::Clock::now();
    encodingPending = true;
}

void TruncDiffBPTSWSAT::solver() {
    bool flagsat = false;

    auto start_time = std::chrono::high_resolution_clock::now();

    const long long TIMEOUT_LIMIT_SECONDS = this->gurobiTimer;
    int num = 0;

    int64_t current_max_bound = 0;

    while (!flagsat) {

        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        long long remaining_seconds = TIMEOUT_LIMIT_SECONDS - elapsed_seconds;

        if (remaining_seconds <= 0) {
            currentRoundStatus = 2;
            std::cout << "------TIMEOUT!!!------" << std::endl;
            this->unsatTime.push_back(this->roundUnsatTime);
            this->satTime.push_back(this->roundSatTime);
            this->roundUnsatTime.clear();
            this->roundSatTime.clear();
            break;
        }

        auto s_time = std::chrono::high_resolution_clock::now();

        this->modelPath = this->pathPrefix + "models/" + this->pathSuffix + std::to_string(this->currentRound) +
                          "_round_model_" + std::to_string(this->countProbability) + ".cnf";

        {
            std::ofstream ofs(this->modelPath, std::ios::out | std::ios::trunc);
        }

        clearPerModelObjectiveState();
        cryptanalysis();

        if (num == 0) {
            if (this->objectiveEncoding == ObjectiveEncoding::SC) {

                int64_t current_round_max_increment = this->curRoundWeightSize + this->xorPIndexSave.size();

                if (this->currentRound == 1) {
                    current_max_bound = current_round_max_increment;
                } else {

                    current_max_bound = this->finalResults[this->currentRound - 2] + current_round_max_increment;
                }
                std::cout << "[INFO] Round " << this->currentRound << " SC tight max_bound initialized to: " << current_max_bound << std::endl;
            } else {

                int bpt_size = this->BPTExtWeighted.size();

                int newly_added_bpts = (bpt_size > 0) ? (this->curRoundWeightSize / bpt_size) : 0;

                int64_t single_bpt_max_weight = 0;
                for (int64_t w : this->BPTExtWeighted) {
                    single_bpt_max_weight += w;
                }
                int64_t xor_weight = static_cast<int64_t>(this->beta) * this->wordSize;

                int64_t max_added_weight = newly_added_bpts * single_bpt_max_weight;
                int64_t max_xor_added_weight = this->xorPIndexSave.size() * xor_weight;
                int64_t current_round_max_increment = max_added_weight + max_xor_added_weight;

                if (this->currentRound == 1) {
                    current_max_bound = current_round_max_increment;
                } else {

                    current_max_bound = this->finalResults[this->currentRound - 2] + current_round_max_increment;
                }
                std::cout << "[INFO] Round " << this->currentRound << " MaxSMT tight max_bound initialized to: " << current_max_bound << std::endl;
            }
        }

        std::vector<int32_t> pb_literals;
        std::vector<int64_t> pb_weights;

        int64_t target_bound = countProbability;
        if (this->objectiveEncoding == ObjectiveEncoding::SC) {
            std::vector<int32_t> sc_terms;
            collectSCTerms(sc_terms);

            std::vector<int> inputP_u;
            inputP_u.reserve(sc_terms.size());
            pb_literals.clear();
            pb_weights.clear();

            for (int32_t literal : sc_terms) {
                inputP_u.push_back(static_cast<int>(literal));
                pb_literals.push_back(literal);
                pb_weights.push_back(1);
            }

            std::vector<std::vector<int>> sc_auxiliary_var_u;
            emitSCSequentialBound(inputP_u, sc_auxiliary_var_u, target_bound);
        } else if (this->objectiveEncoding == ObjectiveEncoding::BDD) {
            collectObjectiveTerms(pb_literals, pb_weights);
            emitObjectiveBound(target_bound);
        } else {

            collectObjectiveTerms(pb_literals, pb_weights);
            emitWeightedBound(pb_literals, pb_weights, target_bound);
        }

        printf("current_max_bound:%d\n", current_max_bound);
        printf("countProbability:%d\n", countProbability);

        if (!writeCnfHeader()) {
            currentRoundStatus = 1;
            break;
        }

        std::string satresult, ls, unsatresult, unls;
        std::string rasolution = this->pathPrefix + "results/" + this->pathSuffix + "Round" +
                                 std::to_string(this->currentRound) + "-" + std::to_string(countProbability) +
                                 "-solution.out";

        MallobStatus mallobStatus = runMallob(this->modelPath, rasolution, remaining_seconds);

        if (mallobStatus == MallobStatus::TIMEOUT) {
            std::cout << "------TIMEOUT!!!------" << std::endl;
            currentRoundStatus = 2;
            this->unsatTime.push_back(this->roundUnsatTime);
            this->satTime.push_back(this->roundSatTime);
            this->roundUnsatTime.clear();
            this->roundSatTime.clear();
            break;
        }

        if (mallobStatus == MallobStatus::UNKNOWN || mallobStatus == MallobStatus::RECOVERY_ERROR) {
            std::cout << (mallobStatus == MallobStatus::RECOVERY_ERROR ? "------SAT RECOVERY ERROR------" : "------MALLOB UNKNOWN------") << std::endl;
            currentRoundStatus = mallobStatus == MallobStatus::RECOVERY_ERROR ? 4 : 3;
            this->unsatTime.push_back(this->roundUnsatTime);
            this->satTime.push_back(this->roundSatTime);
            this->roundUnsatTime.clear();
            this->roundSatTime.clear();
            break;
        }

        bool sat_found = false;
        bool unsat_found = false;

        std::ifstream fin(rasolution);

        std::string line;

        while (std::getline(fin, line)) {
            if (line.find("s SATISFIABLE") != std::string::npos)
                sat_found = true;

            if (line.find("s UNSATISFIABLE") != std::string::npos)
                unsat_found = true;
        }

        fin.close();

        auto e_time = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(e_time - s_time).count();
        double tTime = (double)dur / 1000;

        if (!sat_found && unsat_found) {
            this->roundUnsatTime.push_back(tTime);

            flagsat = false;
            this->tanNameMxIndex.clear();
            this->rtnMxIndex.clear();
            this->rtnIdxSave.clear();
            this->dCounter = 1;
            this->ACounter = 1;
            this->xCounter = 1;
            this->clauseCounter = 0;
            this->differentialCharacteristic.clear();
            this->roundSboxIndexSave.clear();
            this->sboxFindFlag = false;
            this->sboxIndexSave.clear();
            this->auxiliary_var_u.clear();

            clearPerModelObjectiveState();
            activesbox++;

            if (countProbability >= current_max_bound) {
                std::cout << "flagsat = " << flagsat << std::endl;
                flagsat = false;
                currentRoundStatus = 1;
                break;
            }

            this->dp_weight.clear();
            countProbability++;
            this->PCounter = 1;
            this->xorPCounter = 1;
            this->roundPcounterIndexSave.clear();
            this->roundPboxMIndexSave.clear();
            this->xorPIndexSave.clear();
            this->curRoundWeightSize = 0;
            num++;
        }

        else if (sat_found && !unsat_found) {
            this->roundSatTime.push_back(tTime);
            this->unsatTime.push_back(roundUnsatTime);
            this->satTime.push_back(roundSatTime);

            std::vector<int64_t> solution_weights = pb_weights;
            if (this->objectiveEncoding == ObjectiveEncoding::SC) {
                solution_weights.assign(pb_literals.size(), 1);
            }
            int64_t actual_weight_sum = calculateWeightedSumInSolution(rasolution, pb_literals, solution_weights);
            if (actual_weight_sum >= 0 && actual_weight_sum < countProbability) {
                std::cout << "[INFO] SAT bound " << countProbability
                          << " has actual objective " << actual_weight_sum << std::endl;
                countProbability = static_cast<int>(actual_weight_sum);
            }

            roundUnsatTime.clear();
            roundSatTime.clear();
            flagsat = true;
            currentRoundStatus = 0;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    this->timeTime = (double)duration / 1000;
}

void TruncDiffBPTSWSAT::solver_BS() {
    auto start_time = std::chrono::high_resolution_clock::now();

    const long long TIMEOUT_LIMIT_SECONDS = this->gurobiTimer;

    this->modelPath = this->pathPrefix + "models/" + this->pathSuffix + "temp_parse.cnf";
    system(("touch " + this->modelPath).c_str());
    system(("rm " + this->modelPath).c_str());

    clearPerModelObjectiveState();
    cryptanalysis();

    int64_t as_low = 0;
    int64_t as_high = 0;

    if (this->objectiveEncoding == ObjectiveEncoding::SC) {

        int64_t current_round_max_increment = 0;
        if (!this->roundPcounterIndexSave.empty()) {
            current_round_max_increment += this->roundPcounterIndexSave.back().size();
        }

        current_round_max_increment += this->xorPIndexSave.size();

        if (this->currentRound == 1) {
            as_low = 0;
            as_high = current_round_max_increment;
        } else {

            as_low = this->finalResults[this->currentRound - 2];
            as_high = this->finalResults[this->currentRound - 2] + current_round_max_increment;
        }
        std::cout << "[INFO] SC Mode Bound -> Increment: " << current_round_max_increment << std::endl;
    } else {

        int bpt_size = this->BPTExtWeighted.size();
        int newly_added_bpts = (bpt_size > 0) ? (this->curRoundWeightSize / bpt_size) : 0;

        int64_t single_bpt_max_weight = 0;
        for (int64_t w : this->BPTExtWeighted) {
            single_bpt_max_weight += w;
        }
        int64_t xor_weight = static_cast<int64_t>(this->beta) * this->wordSize;

        int64_t max_added_weight = newly_added_bpts * single_bpt_max_weight;
        int64_t max_xor_added_weight = this->xorPIndexSave.size() * xor_weight;
        int64_t current_round_max_increment = max_added_weight + max_xor_added_weight;

        if (this->currentRound == 1) {
            as_low = 0;
            as_high = current_round_max_increment;
        } else {

            as_low = this->finalResults[this->currentRound - 2];
            as_high = this->finalResults[this->currentRound - 2] + current_round_max_increment;
        }
        std::cout << "[INFO] MaxSMT Mode Bound -> Increment: " << current_round_max_increment << std::endl;
    }

    sboxRoundNum = dp_weight.size() + xorPIndexSave.size();

    this->tanNameMxIndex.clear();
    this->rtnMxIndex.clear();
    this->rtnIdxSave.clear();
    this->sboxIndexSave.clear();
    this->clauseCounter = 0;
    this->dCounter = 1;
    this->ACounter = 1;
    this->xCounter = 1;
    this->PCounter = 1;
    this->differentialCharacteristic.clear();
    this->roundSboxIndexSave.clear();
    this->sboxFindFlag = false;
    this->sboxIndexSave.clear();
    this->auxiliary_var_u.clear();
    this->xorPCounter = 1;
    this->roundPcounterIndexSave.clear();
    this->roundPboxMIndexSave.clear();
    this->dp_weight.clear();
    this->xorPIndexSave.clear();
    this->curRoundWeightSize = 0;

    clearPerModelObjectiveState();

    std::cout << "[INFO] BS Initialization -> Round: " << this->currentRound
              << ", as_low: " << as_low << ", as_high: " << as_high << std::endl;

    int64_t best_sat = -1;
    bool searchAborted = false;

    while (as_low <= as_high) {

        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        long long remaining_seconds = TIMEOUT_LIMIT_SECONDS - elapsed_seconds;

        if (remaining_seconds <= 0) {
            std::cout << "------TIMEOUT!!!------" << std::endl;
            currentRoundStatus = 2;
            searchAborted = true;
            break;
        }
        auto s_time = std::chrono::high_resolution_clock::now();

        int64_t as_mid = as_low + (as_high - as_low) / 2;
        this->modelPath = this->pathPrefix + "models/" + this->pathSuffix + std::to_string(this->currentRound) +
                          "_round_model_active" + std::to_string(as_mid) + ".cnf";
        system(("touch " + this->modelPath).c_str());
        system(("rm " + this->modelPath).c_str());

        clearPerModelObjectiveState();
        cryptanalysis();

        std::vector<int32_t> pb_literals;
        std::vector<int64_t> pb_weights;

        if (this->objectiveEncoding == ObjectiveEncoding::SC) {
            std::vector<int32_t> sc_terms;
            collectSCTerms(sc_terms);

            std::vector<int> inputP_u;
            inputP_u.reserve(sc_terms.size());
            pb_literals.clear();
            pb_weights.clear();
            for (int32_t literal : sc_terms) {
                inputP_u.push_back(static_cast<int>(literal));
                pb_literals.push_back(literal);
                pb_weights.push_back(1);
            }

            std::vector<std::vector<int>> sc_auxiliary_var_u;
            emitSCSequentialBound(inputP_u, sc_auxiliary_var_u, as_mid);
        } else if (this->objectiveEncoding == ObjectiveEncoding::BDD) {
            collectObjectiveTerms(pb_literals, pb_weights);
            emitObjectiveBound(as_mid);
        } else {
            collectObjectiveTerms(pb_literals, pb_weights);
            emitWeightedBound(pb_literals, pb_weights, as_mid);
        }

        if (!writeCnfHeader()) {
            currentRoundStatus = 1;
            break;
        }

        std::string satresult, ls, unsatresult, unls;
        std::string rasolution = this->pathPrefix + "results/" + this->pathSuffix + "Round" +
                                 std::to_string(this->currentRound) + "-Active" + std::to_string(as_mid) +
                                 "-solution.out";

        MallobStatus mallobStatus = runMallob(this->modelPath, rasolution, remaining_seconds);

        if (mallobStatus == MallobStatus::TIMEOUT) {
            std::cout << "------TIMEOUT!!!------" << std::endl;
            currentRoundStatus = 2;
            searchAborted = true;
            break;
        }
        if (mallobStatus == MallobStatus::UNKNOWN || mallobStatus == MallobStatus::RECOVERY_ERROR) {
            std::cout << (mallobStatus == MallobStatus::RECOVERY_ERROR ? "------SAT RECOVERY ERROR------" : "------MALLOB UNKNOWN------") << std::endl;
            currentRoundStatus = mallobStatus == MallobStatus::RECOVERY_ERROR ? 4 : 3;
            searchAborted = true;
            break;
        }

        std::string solutionout = this->pathPrefix + "results/" + this->pathSuffix + "SatSolution.out";
        std::string unsolutionout = this->pathPrefix + "results/" + this->pathSuffix + "UnsatSolution.out";
        system(("> " + solutionout).c_str());
        system(("> " + unsolutionout).c_str());

        system(("sed -n '/s SATISFIABLE/p' " + rasolution + " > " + solutionout).c_str());
        system(("sed -n '/s UNSATISFIABLE/p' " + rasolution + " > " + unsolutionout).c_str());
        std::ifstream satsol(solutionout), unsatsol(unsolutionout);

        while (getline(satsol, ls)) {
            satresult += ls + "\n";
        }
        while (getline(unsatsol, unls)) {
            unsatresult += unls + "\n";
        }

        satsol.close();
        unsatsol.close();

        auto e_time = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(e_time - s_time).count();
        double tTime = (double)dur / 1000;

        this->tanNameMxIndex.clear();
        this->rtnMxIndex.clear();
        this->rtnIdxSave.clear();
        this->dCounter = 1;
        this->ACounter = 1;
        this->PCounter = 1;
        this->xCounter = 1;

        this->clauseCounter = 0;
        this->differentialCharacteristic.clear();
        this->roundSboxIndexSave.clear();
        this->sboxFindFlag = false;
        this->sboxIndexSave.clear();
        this->auxiliary_var_u.clear();

        clearPerModelObjectiveState();

        this->xorPCounter = 1;
        this->roundPcounterIndexSave.clear();
        this->roundPboxMIndexSave.clear();

        this->dp_weight.clear();
        this->xorPIndexSave.clear();
        this->curRoundWeightSize = 0;

        if (satresult.length() == 0 && unsatresult.length() > 0) {

            this->roundUnsatTime.push_back(tTime);
            as_low = as_mid + 1;
        } else if (satresult.length() > 0 && unsatresult.length() == 0) {
            this->roundSatTime.push_back(tTime);

            std::vector<int64_t> solution_weights = pb_weights;
            if (this->objectiveEncoding == ObjectiveEncoding::SC) {
                solution_weights.assign(pb_literals.size(), 1);
            }
            int64_t actual_weight_sum = calculateWeightedSumInSolution(rasolution, pb_literals, solution_weights);

            best_sat = actual_weight_sum;

            as_high = actual_weight_sum - 1;
        }
    }

    if (!searchAborted) {
        if (best_sat != -1) {
            countProbability = best_sat;
            currentRoundStatus = 0;
        } else {
            currentRoundStatus = 1;
        }
    }

    this->satTime.push_back(this->roundSatTime);
    this->roundSatTime.clear();

    this->unsatTime.push_back(this->roundUnsatTime);
    this->roundUnsatTime.clear();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    this->timeTime = (double)duration / 1000;
}

void TruncDiffBPTSWSAT::solver_unsat() {
    bool flagsat = false;
    int num = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    const long long TIMEOUT_LIMIT_SECONDS = this->gurobiTimer;

    int64_t single_bpt_max_weight = 0;
    for (int64_t w : this->BPTExtWeighted) {
        single_bpt_max_weight += w;
    }
    int64_t xor_weight = static_cast<int64_t>(this->beta) * this->wordSize;

    while (!flagsat) {

        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        long long remaining_seconds = TIMEOUT_LIMIT_SECONDS - elapsed_seconds;

        if (remaining_seconds <= 0) {
            std::cout << "------TIMEOUT!!!------" << std::endl;
            currentRoundStatus = 2;
            this->satTime.push_back(this->roundSatTime);
            this->unsatTime.push_back(this->roundUnsatTime);
            this->roundSatTime.clear();
            this->roundUnsatTime.clear();
            break;
        }

        auto s_time = std::chrono::high_resolution_clock::now();

        this->modelPath = this->pathPrefix + "models/" + this->pathSuffix + std::to_string(this->currentRound) +
                          "_round_model_active" + std::to_string(this->countProbability) + ".cnf";
        system(("touch " + this->modelPath).c_str());
        system(("rm " + this->modelPath).c_str());

        clearPerModelObjectiveState();
        cryptanalysis();

        if (num == 0) {
            if (this->objectiveEncoding == ObjectiveEncoding::SC) {
                int64_t current_round_max_increment = 0;
                if (!this->roundPcounterIndexSave.empty()) {
                    current_round_max_increment += this->roundPcounterIndexSave.back().size();
                }
                current_round_max_increment += this->xorPIndexSave.size();

                if (this->currentRound == 1) {
                    countProbability = current_round_max_increment;
                } else {
                    countProbability = this->finalResults[this->currentRound - 2] + current_round_max_increment;
                }
                std::cout << "\n[INFO] Round " << this->currentRound << " SC Initial Upper Bound set to: " << countProbability << std::endl;
            } else {
                int bpt_size = this->BPTExtWeighted.size();
                int newly_added_bpts = (bpt_size > 0) ? (this->curRoundWeightSize / bpt_size) : 0;

                int64_t max_added_weight = newly_added_bpts * single_bpt_max_weight;
                int64_t max_xor_added_weight = this->xorPIndexSave.size() * xor_weight;
                int64_t current_round_max_increment = max_added_weight + max_xor_added_weight;

                if (this->currentRound == 1) {
                    countProbability = current_round_max_increment;
                } else {
                    countProbability = this->finalResults[this->currentRound - 2] + current_round_max_increment;
                }
                std::cout << "\n[INFO] Round " << this->currentRound << " MaxSMT Initial Upper Bound set to: " << countProbability << std::endl;
            }
        }

        std::vector<int32_t> pb_literals;
        std::vector<int64_t> pb_weights;

        int64_t target_bound = countProbability;
        if (this->objectiveEncoding == ObjectiveEncoding::SC) {
            std::vector<int32_t> sc_terms;
            collectSCTerms(sc_terms);

            std::vector<int> inputP_u;
            inputP_u.reserve(sc_terms.size());
            pb_literals.clear();
            pb_weights.clear();
            for (int32_t literal : sc_terms) {
                inputP_u.push_back(static_cast<int>(literal));
                pb_literals.push_back(literal);
                pb_weights.push_back(1);
            }
            std::vector<std::vector<int>> sc_auxiliary_var_u;
            emitSCSequentialBound(inputP_u, sc_auxiliary_var_u, target_bound);
        } else if (this->objectiveEncoding == ObjectiveEncoding::BDD) {
            collectObjectiveTerms(pb_literals, pb_weights);
            emitObjectiveBound(target_bound);
        } else {
            collectObjectiveTerms(pb_literals, pb_weights);
            emitWeightedBound(pb_literals, pb_weights, target_bound);
        }

        if (!writeCnfHeader()) {
            currentRoundStatus = 1;

            this->satTime.push_back(this->roundSatTime);
            this->unsatTime.push_back(this->roundUnsatTime);

            this->roundSatTime.clear();
            this->roundUnsatTime.clear();

            break;
        }

        std::string satresult, ls, unsatresult, unls;
        std::string rasolution = this->pathPrefix + "results/" + this->pathSuffix + "Round" +
                                 std::to_string(this->currentRound) + "-Active" + std::to_string(countProbability) +
                                 "-solution.out";

        MallobStatus mallobStatus = runMallob(this->modelPath, rasolution, remaining_seconds);

        if (mallobStatus == MallobStatus::TIMEOUT) {
            std::cout << "------TIMEOUT!!!------" << std::endl;
            currentRoundStatus = 2;
            this->unsatTime.push_back(this->roundUnsatTime);
            this->satTime.push_back(this->roundSatTime);
            this->roundUnsatTime.clear();
            this->roundSatTime.clear();
            break;
        }
        if (mallobStatus == MallobStatus::UNKNOWN || mallobStatus == MallobStatus::RECOVERY_ERROR) {
            std::cout << (mallobStatus == MallobStatus::RECOVERY_ERROR ? "------SAT RECOVERY ERROR------" : "------MALLOB UNKNOWN------") << std::endl;
            currentRoundStatus = mallobStatus == MallobStatus::RECOVERY_ERROR ? 4 : 3;
            this->unsatTime.push_back(this->roundUnsatTime);
            this->satTime.push_back(this->roundSatTime);
            this->roundUnsatTime.clear();
            this->roundSatTime.clear();
            break;
        }
        const bool satFound = mallobStatus == MallobStatus::SAT;
        const bool unsatFound = mallobStatus == MallobStatus::UNSAT;

        std::string solutionout = this->pathPrefix + "results/" + this->pathSuffix + "SatSolution.out";
        std::string unsolutionout = this->pathPrefix + "results/" + this->pathSuffix + "UnsatSolution.out";
        system(("> " + solutionout).c_str());
        system(("> " + unsolutionout).c_str());

        system(("sed -n '/s SATISFIABLE/p' " + rasolution + " > " + solutionout).c_str());
        system(("sed -n '/s UNSATISFIABLE/p' " + rasolution + " > " + unsolutionout).c_str());

        std::ifstream satsol(solutionout), unsatsol(unsolutionout);
        while (getline(satsol, ls)) {
            satresult += ls + "\n";
        }
        while (getline(unsatsol, unls)) {
            unsatresult += unls + "\n";
        }
        satsol.close();
        unsatsol.close();

        auto e_time = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(e_time - s_time).count();
        double tTime = (double)dur / 1000;

        const bool isUnsat = satresult.empty() && !unsatresult.empty();
        const bool isSat = !satresult.empty() && unsatresult.empty();

        if (isUnsat) {
            this->roundUnsatTime.push_back(tTime);
            this->satTime.push_back(this->roundSatTime);
            this->unsatTime.push_back(this->roundUnsatTime);
            this->roundSatTime.clear();
            this->roundUnsatTime.clear();

            if (num == 0) {
                currentRoundStatus = 1;
                break;
            }

            flagsat = true;
            currentRoundStatus = 0;
        } else if (isSat) {
            std::vector<int64_t> solution_weights = pb_weights;
            if (this->objectiveEncoding == ObjectiveEncoding::SC) {
                solution_weights.assign(pb_literals.size(), 1);
            }

            const int actualWeight = calculateWeightedSumInSolution(rasolution, pb_literals, solution_weights);

            if (actualWeight < 0) {
                throw std::runtime_error("solver_unsat: failed to calculate the objective value of the SAT solution.");
            }

            if (actualWeight > target_bound) {
                std::cerr << "[ERROR] solver_unsat objective mismatch: round=" << this->currentRound << ", target_bound=" << target_bound << ", actualWeight=" << actualWeight << ", objectiveTerms=" << pb_literals.size() << ", modelPath=" << this->modelPath << ", solutionPath=" << rasolution << std::endl;
                throw std::runtime_error("solver_unsat: SAT solution violates the current objective bound.");
            }

            this->roundSatTime.push_back(tTime);

            if (actualWeight == 0) {
                countProbability = -1;
                flagsat = true;
                currentRoundStatus = 0;

                this->satTime.push_back(this->roundSatTime);
                this->unsatTime.push_back(this->roundUnsatTime);
                this->roundSatTime.clear();
                this->roundUnsatTime.clear();
                break;
            }

            flagsat = false;
            currentRoundStatus = 0;

            this->tanNameMxIndex.clear();
            this->rtnMxIndex.clear();
            this->rtnIdxSave.clear();
            this->dCounter = 1;
            this->ACounter = 1;
            this->xCounter = 1;
            this->clauseCounter = 0;
            this->differentialCharacteristic.clear();
            this->roundSboxIndexSave.clear();
            this->sboxFindFlag = false;
            this->sboxIndexSave.clear();
            this->auxiliary_var_u.clear();
            this->dp_weight.clear();
            this->xorPIndexSave.clear();

            countProbability = std::min(actualWeight - 1, countProbability - 1);

            this->PCounter = 1;
            this->xorPCounter = 1;
            this->roundPcounterIndexSave.clear();
            this->roundPboxMIndexSave.clear();
            this->curRoundWeightSize = 0;

            clearPerModelObjectiveState();
            ++num;
        } else {
            throw std::runtime_error("solver_unsat: solver returned neither SAT nor UNSAT.");
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    this->timeTime = (double)duration / 1000;
}

void TruncDiffBPTSWSAT::programGenModel() {
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
                        this->xCounter = 1;
                        this->clauseCounter = 0;
                        roundFlag = false;
                        experimentRecord.clearModel();
                        this->differentialCharacteristic.clear();
                        this->roundSboxIndexSave.clear();
                        this->sboxFindFlag = false;
                        this->sboxIndexSave.clear();
                        this->PCounter = 1;
                        this->roundPcounterIndexSave.clear();
                        this->roundPboxMIndexSave.clear();
                        this->xorPIndexSave.clear();
                        this->curRoundWeightSize = 0;

                        clearPerModelObjectiveState();
                    } else
                        modelCheckFlag = false;
                }
            }
            break;
        }
    }
}

void TruncDiffBPTSWSAT::roundFunctionGenModel(const ProcedureHPtr &procedureH, int roundCounter) {
    std::string keyId = procedureH->getParameters().at(1).at(0)->getNodeName().substr(
        0, procedureH->getParameters().at(1).at(0)->getNodeName().find("0"));
    for (const auto &i : procedureH->getReturns()) {
        if (i->getNodeType() != NodeType::PARAMETER) {
            this->wordSize = Transformer::getNodeTypeSize(i->getNodeType());
            break;
        }
    }

    std::vector<int> currRoundSboxIndex;

    int currPcounter = this->PCounter;

    int currXorPCounter = xorPIndexSave.size();

    int currWeightSize = this->dp_weight.size();

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
    experimentRecord.saveNodes(this->currentRound - roundCounter + 1, tanNameMxIndex, "");
    this->roundSboxIndexSave.push_back(currRoundSboxIndex);

    std::vector<int> WeightCounterIndexSave;

    std::vector<int32_t> round_vars;
    std::vector<int64_t> round_weights;

    for (int i = currWeightSize; i < this->dp_weight.size(); ++i) {
        if (roundCounter == 1) {
            curRoundWeightSize++;
        }
        WeightCounterIndexSave.push_back(dp_weight[i]);
        int32_t v = dp_weight[i];
        int64_t w = this->BPTExtWeighted[i % this->BPTExtWeighted.size()];
        round_vars.push_back(v);
        round_weights.push_back(w);
    }

    this->round_pb_vars.push_back(round_vars);
    this->round_pb_weights.push_back(round_weights);

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
    } else {
        isPthenS = false;
        for (int i = 0; i < roundPboxMIndexSave[this->currentRound - roundCounter].size(); ++i) {
            if (std::find(currRoundSboxIndex.begin(), currRoundSboxIndex.end(),
                          roundPboxMIndexSave[this->currentRound - roundCounter][i]) != currRoundSboxIndex.end()) {
                isPthenS = true;
                break;
            }
        }
    }

    int64_t active_weight = static_cast<int64_t>(this->beta) * this->wordSize;

    int64_t xor_weight = static_cast<int64_t>(this->beta) * this->wordSize;

    int64_t boundary_target = active_weight * this->blockSize - 1;

    std::vector<int32_t> pb_literals;
    std::vector<int64_t> pb_weights;
    auto emitRoundBoundary = [&](const std::vector<int32_t> &literals,
                                 const std::vector<int64_t> &weights,
                                 int64_t sc_bound,
                                 int64_t weighted_bound) {
        if (this->objectiveEncoding == ObjectiveEncoding::SC) {
            std::vector<int> sc_literals;
            sc_literals.reserve(literals.size());
            for (int32_t literal : literals) {
                sc_literals.push_back(static_cast<int>(literal));
            }
            std::vector<std::vector<int>> sc_auxiliary_var_u;
            emitSCSequentialBound(sc_literals, sc_auxiliary_var_u, sc_bound);
        } else {
            emitWeightedBound(literals, weights, weighted_bound);
        }
    };

    if (roundCounter == 1 && !isPthenS && (this->currentRound - roundCounter > 0)) {
        pb_literals.clear();
        pb_weights.clear();

        for (int i = 0; i < currWeightSize; i++) {
            pb_literals.push_back(dp_weight[i]);
            pb_weights.push_back(this->BPTExtWeighted[i % this->BPTExtWeighted.size()]);
        }

        if (currXorPCounter > 0) {
            for (int i = 0; i < currXorPCounter; ++i) {
                pb_literals.push_back(xorPIndexSave[i]);
                pb_weights.push_back(xor_weight);
            }
        }

        if (this->blockSize == currRoundSboxIndex.size()) {
            for (int i = 0; i < currRoundSboxIndex.size(); ++i) {
                pb_literals.push_back(currRoundSboxIndex[i]);
                pb_weights.push_back(active_weight);
            }
        } else {

            int DClength = this->differentialCharacteristic.size() - 2;
            if (DClength > -1) {
                for (int i = 0; i < this->differentialCharacteristic[DClength].size(); i++) {
                    pb_literals.push_back(this->differentialCharacteristic[DClength][i]);
                    pb_weights.push_back(active_weight);
                }
            }
        }

        emitRoundBoundary(pb_literals, pb_weights, this->blockSize - 1, boundary_target);
    }

    if (roundCounter == 1 && isPthenS && (this->currentRound - roundCounter > 0)) {
        pb_literals.clear();
        pb_weights.clear();

        for (int i = 0; i < dp_weight.size(); i++) {
            pb_literals.push_back(dp_weight[i]);
            pb_weights.push_back(this->BPTExtWeighted[i % this->BPTExtWeighted.size()]);
        }

        if (xorPIndexSave.size() > 0) {
            for (int i = 0; i < xorPIndexSave.size(); ++i) {
                pb_literals.push_back(xorPIndexSave[i]);
                pb_weights.push_back(xor_weight);
            }
        }

        for (int i = 0; i < outputC.size(); ++i) {
            pb_literals.push_back(outputC[i]);
            pb_weights.push_back(active_weight);
        }

        emitRoundBoundary(pb_literals, pb_weights, this->blockSize - 1, boundary_target);
    }

    if (roundCounter == 1) {
        pb_literals.clear();
        pb_weights.clear();

        for (int i = 0; i < outputC.size(); ++i) {
            pb_literals.push_back(outputC[i]);
            pb_weights.push_back(1);
        }

        int64_t pure_count_bound = this->blockSize - 1;

        emitRoundBoundary(pb_literals, pb_weights, pure_count_bound, pure_count_bound);
    }
}

void TruncDiffBPTSWSAT::XORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
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

    this->xorPIndexSave.push_back(ACounter);

    TruncDiffBPTSATcons::wXor2(this->modelPath, inputIdx1, inputIdx2, outputIdx, ACounter, clauseCounter);

    ACounter++;
}

void TruncDiffBPTSWSAT::linearPermuteGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input,
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
    if (this->cipherName == "MIBS_word_wise")
        this->branchNumMin[pbox->getNodeName()] = 5;

    if (this->roundPboxMIndexSave.size() < curRoundNumber + 1) {
        this->roundPboxMIndexSave.push_back(outputIdx);
    }

    else {
        this->roundPboxMIndexSave[curRoundNumber].insert(this->roundPboxMIndexSave[curRoundNumber].end(),
                                                         outputIdx.begin(), outputIdx.end());
    }

    std::vector<int> weight_idx;
    for (int i = 0; i < this->bpt_weight_num; i++) {

        this->dp_weight.push_back(ACounter);
        weight_idx.push_back(ACounter);
        PCounter++;
        ACounter++;
    }
    TruncDiffBPTSATcons::wLinearBPT(this->modelPath, inputIdx, outputIdx, this->BPTCnf[pbox->getNodeName()], weight_idx,
                                    this->bpt_weight_num, true, clauseCounter);
    experimentRecord.locals.push_back({curRoundNumber + 1, pbox->getNodeName(),
                                       experiment::names(inputIdx, ""), experiment::names(outputIdx, ""), experiment::names(weight_idx, "")});
    weight_idx.clear();
}

void TruncDiffBPTSWSAT::ANDandORGenModel(const ThreeAddressNodePtr &left, const ThreeAddressNodePtr &right,
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
    TruncDiffBPTSATcons::wAndOr(this->modelPath, inputIdx1, inputIdx2, outputIdx, clauseCounter);
}

void TruncDiffBPTSWSAT::PboxGenModel(const ThreeAddressNodePtr &pbox, const ThreeAddressNodePtr &input,
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

void TruncDiffBPTSWSAT::AddGenModel(vector<ThreeAddressNodePtr> &input1, vector<ThreeAddressNodePtr> &input2,
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
            TruncDiffBPTSATcons::wXor(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i], clauseCounter);
            break;
        case 2:
            TruncDiffBPTSATcons::wXor(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i], clauseCounter);
            break;
        default:
            TruncDiffBPTSATcons::wXor(this->modelPath, inputIdx1[i], inputIdx2[i], outputIdx[i], clauseCounter);
        }
    }
}

bool TruncDiffBPTSWSAT::isConstant(ThreeAddressNodePtr threeAddressNodePtr) {
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
