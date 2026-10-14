

#include "branchingPropertyTable/BranchingPropertyTableCalculator.h"

#include <vector>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <string>
#include <cassert>
#include <sstream>

using u128 = unsigned __int128;

namespace {

static inline size_t words_for_bits(size_t nbits) {
    return (nbits + 63) / 64;
}

static inline void set_bit_vec(std::vector<uint64_t> &vec, size_t pos) {
    vec[pos / 64] |= (uint64_t(1) << (pos % 64));
}
static inline bool test_bit_vec(const std::vector<uint64_t> &vec, size_t pos) {
    return (vec[pos / 64] >> (pos % 64)) & 1ull;
}
static inline void xor_vec(std::vector<uint64_t> &dst, const std::vector<uint64_t> &src) {
    size_t w = dst.size();
    for (size_t i = 0; i < w; ++i)
        dst[i] ^= src[i];
}

static int rank_gf2(std::vector<std::vector<uint64_t>> &rows, size_t ncols) {
    size_t nrows = rows.size();
    int rank = 0;
    for (size_t col = 0; col < ncols && rank < (int)nrows; ++col) {

        size_t pivot = rank;
        while (pivot < nrows && !test_bit_vec(rows[pivot], col))
            ++pivot;
        if (pivot == nrows)
            continue;
        if (pivot != (size_t)rank)
            std::swap(rows[pivot], rows[rank]);
        for (size_t r = 0; r < nrows; ++r) {
            if (r != (size_t)rank && test_bit_vec(rows[r], col)) {
                xor_vec(rows[r], rows[rank]);
            }
        }
        ++rank;
    }
    return rank;
}

static inline bool u128_is_zero(u128 x) { return x == (u128)0; }

static inline unsigned u128_bit_length(u128 x) {
    if (x == (u128)0)
        return 0;
    unsigned bits = 0;
    while (x) {
        x >>= 1;
        ++bits;
    }
    return bits;
}

static std::string u128_to_string(u128 v) {
    if (v == (u128)0)
        return "0";

    std::string s;
    while (v != (u128)0) {
        unsigned digit = (unsigned)(v % 10u);
        s.push_back(char('0' + digit));
        v /= 10u;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

static inline long double log2_u128(u128 x) {

    unsigned bits = u128_bit_length(x);
    int k = (int)bits - 1;

    u128 top;
    if (k >= 63) {
        top = x >> (unsigned)(k - 63);
    } else {
        top = x << (unsigned)(63 - k);
    }

    uint64_t top64 = (uint64_t)top;
    const long double two63 = 9223372036854775808.0L;
    long double m = (long double)top64 / two63;

    return (long double)k + log2l(m);
}

static inline std::vector<int> elementToBits(int element, int m) {
    std::vector<int> bits(m, 0);
    for (int b = 0; b < m; ++b)
        bits[b] = (element >> b) & 1;
    return bits;
}

}

void BPTCalculator::computeBranchingTableLA(bool isInteger) {

    int t = this->t;
    int m = this->m;
    if (t <= 0 || m <= 0) {
        std::cerr << "[BPT LA] invalid t or m\n";
        return;
    }

    size_t n = (size_t)t * (size_t)m;
    size_t words = words_for_bits(n);
    std::vector<std::vector<uint64_t>> L2_rows(n, std::vector<uint64_t>(words, 0));

    for (int i = 0; i < t; ++i) {
        for (int b = 0; b < m; ++b) {
            int col_index = i * m + b;
            int basis_el = (1 << b);
            for (int j = 0; j < t; ++j) {
                int prod = this->gf_multiply(this->M[j][i], basis_el);
                std::vector<int> bits = elementToBits(prod, m);
                for (int c = 0; c < m; ++c) {
                    if (bits[c]) {
                        size_t row = j * m + c;
                        set_bit_vec(L2_rows[row], col_index);
                    }
                }
            }
        }
    }

    size_t nPatterns = 1u << t;

    std::vector<std::vector<u128>> looseCounts(nPatterns, std::vector<u128>(nPatterns, (u128)0));

    auto compute_loose_count = [&](uint32_t alpha_mask, uint32_t beta_mask) -> u128 {
        std::vector<int> basis_cols;
        for (int i = 0; i < t; ++i) {
            if ((alpha_mask >> i) & 1u) {
                for (int b = 0; b < m; ++b)
                    basis_cols.push_back(i * m + b);
            }
        }
        size_t dim_p = basis_cols.size();
        if (dim_p == 0) {

            return (u128)1;
        }

        std::vector<int> proj_rows;
        for (int j = 0; j < t; ++j) {
            if (((beta_mask >> j) & 1u) == 0u) {
                for (int c = 0; c < m; ++c)
                    proj_rows.push_back(j * m + c);
            }
        }

        size_t nrows = proj_rows.size();
        size_t ncols = dim_p;
        if (nrows == 0) {

            u128 cnt = (u128)1;
            cnt <<= (unsigned)dim_p;
            return cnt;
        }

        size_t w = words_for_bits(ncols);
        std::vector<std::vector<uint64_t>> rows_bitset;
        rows_bitset.reserve(nrows);
        for (size_t ri = 0; ri < nrows; ++ri) {
            rows_bitset.emplace_back(std::vector<uint64_t>(w, 0));
            int row_idx = proj_rows[ri];
            for (size_t p = 0; p < basis_cols.size(); ++p) {
                int col_index = basis_cols[p];
                if (test_bit_vec(L2_rows[row_idx], col_index)) {
                    set_bit_vec(rows_bitset[ri], p);
                }
            }
        }

        int r = rank_gf2(rows_bitset, ncols);
        int exp = (int)dim_p - r;
        u128 cnt = (u128)1;
        cnt <<= (unsigned)exp;
        return cnt;
    };

    for (uint32_t a = 0; a < (uint32_t)nPatterns; ++a) {
        for (uint32_t b = 0; b < (uint32_t)nPatterns; ++b) {
            looseCounts[a][b] = compute_loose_count(a, b);
        }
    }

    std::vector<std::vector<u128>> exactCounts(nPatterns, std::vector<u128>(nPatterns, (u128)0));
    std::vector<std::vector<uint32_t>> masks_by_pop(t + 1);
    for (uint32_t mask = 0; mask < (uint32_t)nPatterns; ++mask) {
        int pc = __builtin_popcount(mask);
        masks_by_pop[pc].push_back(mask);
    }

    for (int pa = 0; pa <= t; ++pa) {
        for (uint32_t ea : masks_by_pop[pa]) {
            for (int pb = 0; pb <= t; ++pb) {
                for (uint32_t eb : masks_by_pop[pb]) {

                    u128 loose = looseCounts[ea][eb];
                    u128 sum_sub = (u128)0;

                    uint32_t sa = ea;
                    while (true) {
                        uint32_t sb = eb;
                        while (true) {
                            if (!(sa == ea && sb == eb)) {
                                sum_sub += exactCounts[sa][sb];
                            }
                            if (sb == 0)
                                break;
                            sb = (sb - 1) & eb;
                        }
                        if (sa == 0)
                            break;
                        sa = (sa - 1) & ea;
                    }

                    exactCounts[ea][eb] = loose - sum_sub;
                }
            }
        }
    }

    this->BPT.assign(nPatterns, std::vector<int>(nPatterns, 0));

    this->BPTDecimal_.assign(nPatterns, std::vector<double>(nPatterns, -INFINITY));
    this->BPTRaw_.assign(nPatterns, std::vector<double>(nPatterns, -INFINITY));

    std::ofstream f(this->BPTPath, std::ios::trunc);
    if (!f.is_open()) {
        std::cerr << "[BPT LA] cannot open " << this->BPTPath << "\n";
    }

    for (uint32_t ea = 0; ea < (uint32_t)nPatterns; ++ea) {
        int ka = __builtin_popcount(ea);

        u128 base = ((u128)1 << (unsigned)m) - (u128)1;
        u128 p_alpha = (u128)1;
        for (int i = 0; i < ka; ++i)
            p_alpha *= base;

        for (uint32_t eb = 0; eb < (uint32_t)nPatterns; ++eb) {
            u128 num = exactCounts[ea][eb];
            if (num != 0)
                this->BPTRaw_[ea][eb] = static_cast<double>(log2_u128(p_alpha) - log2_u128(num));

            if (!isInteger) {
                if (num == (u128)0) {
                    f << std::setw(3) << "-";
                } else {

                    long double log2_prob = log2_u128(num) - log2_u128(p_alpha);
                    f << std::setw(6) << std::fixed << std::setprecision(3) << (double)log2_prob;

                    this->BPTDecimal_[ea][eb] = (double)(-log2_prob);
                }
            }

            else {
                if (num == (u128)0) {
                    f << std::setw(3) << "0";
                    this->BPT[ea][eb] = 0;
                } else {

                    unsigned log2_num = u128_bit_length(num);
                    if (log2_num > 0)
                        log2_num -= 1;
                    unsigned log2_palpha = u128_bit_length(p_alpha);
                    if (log2_palpha > 0)
                        log2_palpha -= 1;

                    double approx_minus_log2_prob = double((int)log2_palpha - (int)log2_num);

                    int approx_value = this->nearest_power_of_2(approx_minus_log2_prob);

                    this->BPT[ea][eb] = approx_value;
                    f << std::setw(3) << approx_value;

                    this->BPTDecimal_[ea][eb] = approx_value;
                }
            }

            if (eb + 1 < (uint32_t)nPatterns)
                f << ", ";
        }
        f << std::endl;
    }
    f.close();

    std::cout << "[BPT LA] finished. t=" << t << " m=" << m << " patterns=" << nPatterns << std::endl;
}
