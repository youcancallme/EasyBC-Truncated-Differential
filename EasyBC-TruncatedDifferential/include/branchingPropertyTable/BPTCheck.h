
#ifndef EASYBC_BPTCHECK_H
#define EASYBC_BPTCHECK_H

#include "BranchingPropertyTableCalculator.h"
#include <bits/stdc++.h>

namespace BPTCheck {

class Ineq {
  private:
    std::vector<int> m_a_;
    int m_b_;
    std::vector<int> mvd_points_;

    inline int calculate_sum(const int point) const {
        int ret = 0;
        int dim = m_a_.size();

        for (int i = 0; i < ((dim >> 2) << 2); i += 4) {
            if ((point >> i) & 1)
                ret += m_a_[dim - 1 - i];
            if ((point >> (i + 1)) & 1)
                ret += m_a_[dim - 2 - i];
            if ((point >> (i + 2)) & 1)
                ret += m_a_[dim - 3 - i];
            if ((point >> (i + 3)) & 1)
                ret += m_a_[dim - 4 - i];
        }

        for (int i = ((dim >> 2) << 2); i < dim; i++)
            if ((point >> i) & 1)
                ret += m_a_[dim - 1 - i];

        return ret + m_b_;
    }

  public:
    Ineq(std::vector<int> a, int b) : m_a_(std::move(a)), m_b_(b) {}

    bool operator()(const int point) const {
        return calculate_sum(point) >= 0;
    }

    bool check_equal(const int point) const {
        return calculate_sum(point) == 0;
    }

    int operator[](const int i) const {
        int dim = m_a_.size();
        return i == dim ? m_b_ : m_a_[i];
    }

    int get_dim() { return m_a_.size(); }

    void add_mv_point(int point) {
        mvd_points_.push_back(point);
    }

    std::vector<int> get_mv_points() {
        return mvd_points_;
    }

    std::vector<int> get_ineq() {
        std::vector<int> re = m_a_;
        re.push_back(m_b_);
        return re;
    }

    void get_ineq(int n) {
        std::vector<int> re = m_a_;
        re.push_back(m_b_);
        if (n == 1) {
            for (int i : re) {
                std::cout << i << " ";
            }
            std::cout << std::endl;
        }
    }

    std::vector<int> get_cos() {
        return m_a_;
    }

    int get_nco() const {
        return m_b_;
    }

    void reset_mv_points() {
    }

    bool equal_to(Ineq ineq) {
        if (this->get_ineq().size() != ineq.get_ineq().size())
            return false;
        for (int i = 0; i < ineq.get_ineq().size(); ++i)
            if (this->get_ineq()[i] != ineq.get_ineq()[i])
                return false;
        return true;
    }

    int mv_pos_equal(Ineq ineq) {
        std::vector<int> mv_pos = ineq.get_mv_points();

        int counter = 0;
        for (int &mv_po : mv_pos) {
            auto it = std::find(mvd_points_.begin(), mvd_points_.end(), mv_po);
            if (it != mvd_points_.end())
                counter++;
        }

        if (counter == mv_pos.size() && mvd_points_.size() != mv_pos.size())
            return 1;
        else if (counter == mv_pos.size() && mvd_points_.size() == mv_pos.size())
            return 2;
        else
            return 3;
    }
};

std::vector<int> ref_poss_init(BPTCalculator bpt);

std::vector<int> ref_imposs_init(BPTCalculator bpt);

std::vector<Ineq> ineq_obt(std::vector<std::vector<int>> ineq_int);

void check(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int);

std::vector<Ineq> eqn_obt(std::vector<std::vector<int>> eqn_int);

void check2(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int, std::vector<std::vector<int>> eqn_int);
}

#endif
