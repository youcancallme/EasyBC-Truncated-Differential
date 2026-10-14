#ifndef EASYBC_EXPERIMENT_RECORD_H
#define EASYBC_EXPERIMENT_RECORD_H

#include "json/json.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>
#include <cstdint>

namespace experiment {
using Clock = std::chrono::steady_clock;
inline double seconds(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}
struct Timer {
    double &value;
    Clock::time_point start = Clock::now();
    explicit Timer(double &v) : value(v) {}
    ~Timer() { value += seconds(start); }
};
struct Timing {
    Clock::time_point start = Clock::now();
    double model = 0, encoding = 0, solver = 0, initialBuildWall = -1;
    int calls = 0;
};
struct Table {
    std::vector<std::vector<double>> raw;
    std::set<std::string> allowed;
    std::vector<int> weights;
    long long beta = 1;
};
struct Local {
    int round;
    std::string table;
    std::vector<std::string> input, output, probability;
};
inline std::vector<std::string> names(const std::vector<int> &ids, const std::string &prefix) {
    std::vector<std::string> result;
    for (int id : ids)
        result.push_back(prefix + std::to_string(id));
    return result;
}
using Assignment = std::map<std::string, int>;
inline int bit(const Assignment &a, const std::string &name) {
    auto it = a.find(name);
    if (it == a.end() || (it->second != 0 && it->second != 1))
        throw std::runtime_error("Missing/nonbinary assignment: " + name);
    return it->second;
}

inline Assignment readAssignment(const std::string &path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Cannot read assignment: " + path);
    Assignment result;
    std::string line;
    size_t lineNumber = 0;
    bool sat = false, array = false;
    while (std::getline(in, line)) {
        ++lineNumber;
        auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos)
            continue;
        line.erase(0, first);
        if (line == "s SATISFIABLE" || line == "s SATISFIABLE\r") {
            sat = true;
            continue;
        }
        if (line.rfind("s UNSATISFIABLE", 0) == 0)
            throw std::runtime_error("UNSAT is not an assignment");
        if (!sat || line[0] == 'c')
            continue;
        bool dimacs = line[0] == 'v';
        if (!dimacs && !array && line[0] != '[')
            continue;
        if (line[0] == '[')
            array = true;
        bool closes = line.find(']') != std::string::npos;
        for (char &c : line)
            if (c == '[' || c == ']' || c == ',' || c == 'v')
                c = ' ';
        std::istringstream row(line);
        long long literal;
        while (row >> literal) {
            if (!literal)
                continue;
            if (literal < -static_cast<long long>(std::numeric_limits<int>::max()) ||
                literal > std::numeric_limits<int>::max())
                throw std::runtime_error("Assignment ID overflow");
            std::string key = std::to_string(std::abs(literal));
            int value = literal > 0;
            auto old = result.emplace(key, value);
            if (!old.second && old.first->second != value)
                throw std::runtime_error("Conflicting assignment: " + key);
        }
        if (!row.eof())
            throw std::runtime_error("Malformed assignment line at " + path +
                                     ":" + std::to_string(lineNumber));
        if (closes)
            array = false;
    }
    if (!sat || array || result.empty())
        throw std::runtime_error("Missing/incomplete SAT assignment: " + path);
    return result;
}
inline long long weighted(const Assignment &a, const std::vector<int32_t> &vars,
                          const std::vector<int64_t> &weights) {
    if (vars.size() != weights.size())
        throw std::runtime_error("Objective dimensions differ");
    long long total = 0;
    for (size_t i = 0; i < vars.size(); ++i) {
        if (weights[i] < 0)
            throw std::runtime_error("Negative objective weight");
        int value = bit(a, std::to_string(std::abs(vars[i])));
        if (vars[i] < 0)
            value = 1 - value;
        if (value) {
            if (total > std::numeric_limits<long long>::max() - weights[i])
                throw std::runtime_error("Objective overflow");
            total += weights[i];
        }
    }
    return total;
}
inline Json::Value strings(const std::vector<std::string> &values) {
    Json::Value out(Json::arrayValue);
    for (const auto &v : values)
        out.append(v);
    return out;
}
inline std::string pattern(const Assignment &a, const std::vector<std::string> &ids) {
    std::string out;
    for (const auto &id : ids)
        out += char('0' + bit(a, id));
    return out;
}
inline size_t mask(const std::string &pattern) {
    size_t result = 0;
    for (char c : pattern)
        result = result * 2 + (c - '0');
    return result;
}
inline Json::Value state(const Assignment &a, const std::vector<std::string> &ids) {
    Json::Value out;
    out["variables"] = strings(ids);
    out["active_word_pattern"] = pattern(a, ids);
    return out;
}

struct Record {
    std::map<std::string, Table> tables;
    std::vector<Local> locals;
    Json::Value nodes{Json::arrayValue};
    Json::Value best;
    std::string error;
    Timing timing;

    void clearModel() {
        locals.clear();
        nodes = Json::Value(Json::arrayValue);
    }
    void saveNodes(int round, const std::map<std::string, int> &mapping, const std::string &prefix) {
        Json::Value row;
        row["round_index"] = round;
        for (const auto &kv : mapping)
            row["node_variables"][kv.first] = prefix + std::to_string(kv.second);
        nodes.append(row);
    }
    Json::Value recover(const Assignment &a, const std::vector<std::vector<int>> &boundaries,
                        const std::string &prefix, int rounds, long long objective, double unit) const {
        if (!std::isfinite(unit) || unit <= 0 || objective < 0)
            throw std::runtime_error("Invalid objective unit/value");
        if (boundaries.size() != static_cast<size_t>(rounds + 1) || locals.empty())
            throw std::runtime_error("Incomplete round/BPT mapping");
        Json::Value out;
        out["round_states"] = Json::Value(Json::arrayValue);
        out["local_propagations"] = Json::Value(Json::arrayValue);
        for (int r = 0; r < rounds; ++r) {
            Json::Value row;
            row["round_index"] = r + 1;
            row["input"] = state(a, names(boundaries[r], prefix));
            row["output"] = state(a, names(boundaries[r + 1], prefix));
            out["round_states"].append(row);
        }
        double raw = 0;
        long long quantized = 0;
        for (const auto &local : locals) {
            const auto &table = tables.at(local.table);
            auto input = pattern(a, local.input), output = pattern(a, local.output);
            auto encoding = pattern(a, local.probability);
            size_t i = mask(input), j = mask(output);
            if (i >= table.raw.size() || j >= table.raw.at(i).size() || !std::isfinite(table.raw[i][j]))
                throw std::runtime_error("Propagation absent from raw BPT: " + local.table + ":" + input + "/" + output);
            if (!table.allowed.count(input + output + encoding))
                throw std::runtime_error("BPT probability encoding mismatch");
            if (encoding.size() != table.weights.size())
                throw std::runtime_error("BPT weight width mismatch");
            long long q = 0;
            for (size_t k = 0; k < encoding.size(); ++k)
                q += (encoding[k] - '0') * static_cast<long long>(table.weights[k]);
            Json::Value row;
            row["round_index"] = local.round;
            row["bpt"] = local.table;
            row["input"] = state(a, local.input);
            row["output"] = state(a, local.output);
            row["bpt_row"] = Json::UInt64(i);
            row["bpt_column"] = Json::UInt64(j);
            row["probability_variables"] = strings(local.probability);
            row["probability_encoding_assignment"] = encoding;
            row["objective_quantized"] = Json::Int64(q);
            row["quantized_weight"] = q * unit;
            row["raw_weight"] = table.raw[i][j];
            row["raw_bpt_probability"] = std::exp2(-table.raw[i][j]);
            out["local_propagations"].append(row);
            raw += table.raw[i][j];
            quantized += q;
        }
        if (quantized != objective)
            throw std::runtime_error("Local weights do not sum to objective (unsupported non-BPT terms?)");
        out["intermediate_states"] = nodes;
        for (auto &row : out["intermediate_states"]) {
            for (const auto &name : row["node_variables"].getMemberNames()) {
                const auto id = row["node_variables"][name].asString();
                row["active_words"][name] = bit(a, id);
            }
        }
        out["objective_quantized"] = Json::Int64(objective);
        out["objective_unit_bits"] = unit;
        out["quantized_total_weight"] = objective * unit;
        out["raw_total_weight"] = raw;
        out["quantized_probability"] = std::exp2(-objective * unit);
        out["raw_probability"] = std::exp2(-raw);
        out["weight_error"] = objective * unit - raw;
        out["delta_W"] = objective * unit - raw;
        out["probability_semantics"] = "product of original local BPT probabilities; not a differential hull";
        out["pattern_order"] = "constraint vector order, MSB-first BPT row/column index";
        return out;
    }
    void write(const std::string &path, const std::string &cipher, int rounds,
               const std::string &backend, int alpha, const std::string &status,
               const Json::Value &config, bool reused, double sharedPreprocessing = -1) {
        Json::Value out = best.isNull() ? Json::Value(Json::objectValue) : best;
        out["schema_version"] = 1;
        out["cipher"] = cipher;
        out["rounds"] = rounds;
        out["backend"] = backend;
        out["alpha"] = alpha;
        out["status"] = status;
        out["config"] = config;
        out["optimality_proven"] = status == "OPTIMAL";
        out["trajectory_available"] = !best.isNull();
        if (!error.empty())
            out["recovery_error"] = error;
        if (best.isNull())
            out["objective_quantized"] = Json::Value();
        out["preprocessing_reused"] = reused;
        out["timing_scope"] = "steady_clock: first instance starts at MGR entry (includes preprocessing); later instances start at round entry with reused BPT; ends after recovery, before JSON serialization";
        out["model_time"] = timing.model;
        out["search_encoding_time"] = timing.encoding;
        out["solver_time_sum"] = timing.solver;
        out["solver_time"] = timing.solver;
        out["solver_call_count"] = timing.calls;
        out["bpt_tables"] = Json::Value(Json::objectValue);
        for (const auto &kv : tables) {
            Json::Value table;
            table["beta"] = Json::Int64(kv.second.beta);
            table["weights"] = Json::Value(Json::arrayValue);
            for (int w : kv.second.weights)
                table["weights"].append(w);
            table["allowed_encodings"] = Json::Value(Json::arrayValue);
            for (const auto &encoding : kv.second.allowed)
                table["allowed_encodings"].append(encoding);
            table["entries"] = Json::Value(Json::arrayValue);
            for (size_t i = 0; i < kv.second.raw.size(); ++i) {
                for (size_t j = 0; j < kv.second.raw[i].size(); ++j) {
                    if (!std::isfinite(kv.second.raw[i][j]))
                        continue;
                    Json::Value entry;
                    entry["row"] = Json::UInt64(i);
                    entry["column"] = Json::UInt64(j);
                    entry["raw_weight"] = kv.second.raw[i][j];
                    entry["p"] = std::exp2(-kv.second.raw[i][j]);
                    table["entries"].append(entry);
                }
            }
            out["bpt_tables"][kv.first] = table;
        }
        out["total_time"] = seconds(timing.start);

        if (sharedPreprocessing >= 0) {
            out["timing_version"] = backend == "SAT" ? "sat_round_wall_v2" : "milp_round_wall_v2";
            out["timing_unit"] = "seconds";
            out["timing_scope"] = "steady_clock: round entry after shared preprocessing and batch setup; ends after round output/cleanup and result object construction, before timing-record serialization/open/write";
            if (backend == "SAT")
                out["timing_scope"] = "steady_clock: round entry after shared preprocessing and batch setup; ends after final assignment/trajectory recovery and result object construction, before timing-record serialization and batch summary/cleanup";
            out["shared_preprocessing_time"] = sharedPreprocessing;
            out["shared_preprocessing_scope"] = "complete preprocess() call in MGR, including S-box analysis, BPT generation, quantization, shared constraint generation/reduction; reported once on first executed round";
            if (backend == "SAT" && timing.initialBuildWall >= 0) {
                out["timing_split_version"] = "sat_initial_build_v1";
                out["initial_build_wall_time"] = timing.initialBuildWall;
                out["initial_build_scope"] = "first cryptanalysis entry through first runMallob entry; includes binary-search structural preparse; excludes shared preprocessing and solver execution";
                out["build_time"] = timing.initialBuildWall;
                out["solve_time"] = timing.solver + timing.model + timing.encoding - timing.initialBuildWall;
            } else {
                out["build_time"] = timing.model + timing.encoding;
                out["solve_time"] = timing.solver;
            }
            out["other_time"] = out["total_time"].asDouble() - timing.model - timing.encoding - timing.solver;
        }
        std::ofstream file(path);
        if (!file)
            throw std::runtime_error("Cannot write experiment record: " + path);
        Json::StreamWriterBuilder writer;
        writer["precision"] = 17;
        file << Json::writeString(writer, out) << '\n';
        file.close();
        if (!file)
            throw std::runtime_error("Failed writing experiment record: " + path);
    }
};
}
#endif
