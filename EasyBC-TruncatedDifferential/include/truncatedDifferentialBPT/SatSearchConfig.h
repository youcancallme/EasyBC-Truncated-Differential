#ifndef SAT_SEARCH_CONFIG_H
#define SAT_SEARCH_CONFIG_H

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

enum class BptEncodingType {
    ONE_HOT,
    MAX_SMT
};

enum class ObjectiveEncoding {
    SC,
    BDD,
    SWC
};

struct SatSearchConfig {
    int alpha = 0;
    BptEncodingType encodingType = BptEncodingType::MAX_SMT;
    ObjectiveEncoding objectiveEncoding = ObjectiveEncoding::BDD;

    bool useOneHot() const {
        return encodingType == BptEncodingType::ONE_HOT;
    }

    bool useMaxSmt() const {
        return encodingType == BptEncodingType::MAX_SMT;
    }

    bool useSequentialCounter() const {
        return objectiveEncoding == ObjectiveEncoding::SC;
    }

    bool useBdd() const {
        return objectiveEncoding == ObjectiveEncoding::BDD;
    }

    bool useSwc() const {
        return objectiveEncoding == ObjectiveEncoding::SWC;
    }

    bool isIntegerProbability() const {
        return alpha == 0;
    }
};

namespace SatSearchConfigParser {

inline std::string upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

inline BptEncodingType parseEncodingType(const std::string &value) {
    const std::string v = upper(value);
    if (v == "ONE_HOT" || v == "ONEHOT") {
        return BptEncodingType::ONE_HOT;
    }
    if (v == "MAX_SMT" || v == "MAXSMT") {
        return BptEncodingType::MAX_SMT;
    }
    throw std::invalid_argument(
        "Invalid encoding_type: " + value + ". Expected ONE_HOT or MAX_SMT.");
}

inline ObjectiveEncoding parseObjectiveEncoding(const std::string &value) {
    const std::string v = upper(value);
    if (v == "SC") {
        return ObjectiveEncoding::SC;
    }
    if (v == "BDD") {
        return ObjectiveEncoding::BDD;
    }
    if (v == "SWC") {
        return ObjectiveEncoding::SWC;
    }
    throw std::invalid_argument(
        "Invalid objective_encoding: " + value + ". Expected SC, BDD, or SWC.");
}

inline void validateAndNormalize(SatSearchConfig &config) {
    if (config.alpha < 0) {
        throw std::invalid_argument("Invalid alpha: alpha must be >= 0.");
    }

    if (config.alpha == 0) {
        if (config.encodingType == BptEncodingType::ONE_HOT) {
            if (config.objectiveEncoding != ObjectiveEncoding::SC) {
                throw std::invalid_argument(
                    "Invalid SAT configuration: alpha == 0 with ONE_HOT requires "
                    "objective_encoding == SC.");
            }
        } else if (config.objectiveEncoding == ObjectiveEncoding::SC) {
            throw std::invalid_argument(
                "Invalid SAT configuration: SC is only allowed with ONE_HOT. "
                "Use BDD or SWC for MAX_SMT.");
        }
    } else {
        if (config.encodingType != BptEncodingType::MAX_SMT) {
            throw std::invalid_argument(
                "Invalid SAT configuration: alpha >= 1 requires encoding_type == MAX_SMT.");
        }
        if (config.objectiveEncoding == ObjectiveEncoding::SC) {
            throw std::invalid_argument(
                "Invalid SAT configuration: alpha >= 1 does not allow SC. "
                "Use BDD or SWC.");
        }
    }
}

inline SatSearchConfig fromParameterList(const std::vector<std::string> &params) {
    if (params.empty()) {
        throw std::invalid_argument("Invalid SAT configuration: empty parameter list.");
    }

    SatSearchConfig config;
    bool encodingTypeSeen = false;
    bool objectiveEncodingSeen = false;

    const int declaredParamCount = std::stoi(params[0]);
    if (declaredParamCount > static_cast<int>(params.size())) {
        throw std::invalid_argument(
            "Invalid parameter file: declared parameter count exceeds actual line count.");
    }

    for (int i = 5; i + 1 < declaredParamCount; i += 2) {
        const std::string &key = params[i];
        const std::string &value = params[i + 1];

        if (key == "alpha") {
            config.alpha = std::stoi(value);
        } else if (key == "encoding_type") {
            config.encodingType = parseEncodingType(value);
            encodingTypeSeen = true;
        } else if (key == "objective_encoding") {
            config.objectiveEncoding = parseObjectiveEncoding(value);
            objectiveEncodingSeen = true;
        }
    }

    if (!encodingTypeSeen) {
        config.encodingType = BptEncodingType::MAX_SMT;
    }
    if (!objectiveEncodingSeen) {
        config.objectiveEncoding =
            config.encodingType == BptEncodingType::ONE_HOT
                ? ObjectiveEncoding::SC
                : ObjectiveEncoding::BDD;
    }

    validateAndNormalize(config);
    return config;
}

}

#endif
