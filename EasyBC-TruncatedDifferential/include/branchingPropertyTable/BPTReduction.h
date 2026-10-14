

#ifndef EASYBC_BPTREDUCTION_H
#define EASYBC_BPTREDUCTION_H

#include "BPTCheck.h"
#include "gurobi_c++.h"
#include "SyntaxGuided.h"
#include "superballMGR.h"
#include "BranchingPropertyTableCalculator.h"

namespace BPTRed {

void rtn_save(BPTCalculator bpt, const std::vector<std::vector<int>> &rtn, double time, std::string alg);

std::vector<std::vector<int>> greedy_sun(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int);
class ImPoint {
  private:
    int d_point_{};
    std::vector<int> b_point_;
    std::vector<BPTCheck::Ineq> by_rmd_;
    std::vector<int> by_rmd_indx_;

  public:
    explicit ImPoint(int d_point) : d_point_(d_point) {}
    explicit ImPoint(std::vector<int> b_point) : b_point_(std::move(b_point)) {}
    explicit ImPoint(int d_point, std::vector<int> b_point) : d_point_(d_point), b_point_(std::move(b_point)) {}
    void rmd_add(const BPTCheck::Ineq &ineq) { by_rmd_.push_back(ineq); }
    void rmd_indx_add(int idx) { by_rmd_indx_.push_back(idx); }
    int get_d() const { return d_point_; }
    std::vector<BPTCheck::Ineq> get_rmd() { return by_rmd_; }
    std::vector<int> get_rmd_idx() { return by_rmd_indx_; }
};

std::vector<ImPoint> ref_impoint_init(BPTCalculator bpt);

std::vector<std::vector<int>> milp_red(std::vector<BPTCheck::Ineq> ineqs, std::vector<ImPoint> ref_ans);

std::vector<std::vector<int>> sub_milp(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int);

std::vector<std::vector<int>> convex_hull_tech(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int);

class IcPrec {
  public:
    std::string a_;
    std::string u_;
    std::vector<int> supp_u_;
    std::vector<int> prec_u_;

  public:
    IcPrec(std::string a, std::string u) : a_(std::move(a)), u_(std::move(u)) {}
    std::string get_a() const { return a_; }
    std::string get_u() const { return u_; }
    bool operator==(const IcPrec &p) const { return (this->a_ == p.a_) && (this->u_ == p.u_); }
};

std::vector<int> supp(std::string a);

bool supp_intersec(std::vector<int> a, std::vector<int> b);

std::vector<int> prec_to_ineq(BPTCalculator bpt, IcPrec ic);

std::vector<std::string> ck_po(BPTCalculator bpt, std::vector<int> ineq);

std::vector<std::string> obt_impo(BPTCalculator bpt, std::vector<int> ineq);

std::vector<std::vector<int>> logic_cond(BPTCalculator bpt, bool ifComb233);

int hamming_distance(std::string a, std::string b);

std::vector<std::string> ball_points(BPTCalculator bpt, int d, std::string c);

std::vector<int> ball_ineq(BPTCalculator bpt, std::string c, int d, std::vector<std::string> bq);

std::vector<std::vector<int>> distorted_balls(BPTCalculator bpt, bool ifComb233);

std::vector<std::vector<int>> comb233(BPTCalculator bpt);

std::vector<std::vector<int>> reduction(int chooser, BPTCalculator bpt);

}

#endif
