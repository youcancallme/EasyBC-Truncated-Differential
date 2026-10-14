
#include "BPTCheck.h"

std::vector<int> BPTCheck::ref_poss_init(BPTCalculator bpt) {
    std::vector<int> ref_ans;
    std::vector<std::string> ref_initial = bpt.get_poss();
    for (const auto &i : ref_initial)
        ref_ans.push_back(utilities::b_to_d(std::stol(i)));
    std::cout << "totally " << ref_ans.size() << " possible points" << std::endl;
    return ref_ans;
}

std::vector<int> BPTCheck::ref_imposs_init(BPTCalculator bpt) {
    std::vector<int> ref_ans;
    std::vector<std::string> ref_initial = bpt.get_imposs();
    for (const auto &i : ref_initial)
        ref_ans.push_back(utilities::b_to_d(std::stol(i)));
    std::cout << "totally " << ref_ans.size() << " impossible points" << std::endl;
    return ref_ans;
}

std::vector<BPTCheck::Ineq> BPTCheck::ineq_obt(std::vector<std::vector<int>> ineq_int) {
    std::vector<Ineq> ineq;
    if (ineq_int.empty()) {
        std::cout << "There is no inequality to be checked !" << std::endl;
        assert(false);
    }
    for (auto &item : ineq_int) {
        if (item.empty())
            continue;
        int b = item[item.size() - 1];
        item.pop_back();
        ineq.push_back(Ineq(item, b));
    }

    std::cout << "INEQUALITY NUM " << ineq.size() << " \n"
              << std::endl;
    return ineq;
}

void BPTCheck::check(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int) {
    std::cout << "\n*********************** CHECKING START ***********************" << std::endl;

    std::ios_base::sync_with_stdio(false);

    auto start = std::chrono::steady_clock::now();

    std::vector<int> ref_ans = ref_poss_init(bpt);
    std::vector<Ineq> ineq = ineq_obt(ineq_int);

    int dim = ineq[0].get_dim();
    int *sol = new int[1 << dim];
    int *tmp = new int[1 << dim];
    bool *bsol = new bool[1 << dim];
    bool *bref = new bool[1 << dim];

    for (int i = 0; i < (1 << dim); i++)
        bref[i] = false;
    for (int i = 0; i < ref_ans.size(); i++)
        bref[ref_ans[i]] = true;

    for (int i = 0; i < (1 << dim); i++)
        sol[i] = i, bsol[i] = true;
    int sol_sz = (1 << dim);
    int tmp_sz = 0;

    omp_lock_t *lock = new omp_lock_t[1 << dim];
    for (int i = 0; i < (1 << dim); i++)
        omp_init_lock(&lock[i]);

    omp_set_num_threads(CK_THD);

    for (int i = 0; i < ineq.size(); i++) {
        if (i % 512 == 0)
            std::cout << "CHECKING inequality " << i << std::endl;
        Ineq &t = ineq[i];

#pragma omp parallel for shared(t, sol, bsol)
        for (int j = 0; j < sol_sz; j++) {

            if (bsol[sol[j]] && !t(sol[j])) {
                omp_set_lock(&lock[j]);
                bsol[sol[j]] = false;

                if (bref[sol[j]]) {
                    std::cout << "WRONG: " << sol[j] << " " << utilities::d_to_b(sol[j]) << " is in refence set\n";
                    std::cout << "but not in ";
                    for (int k = 0; k < ineq[i].get_ineq().size(); ++k) {
                        std::cout << ineq[i].get_ineq()[k] << " ";
                    }
                    std::cout << std::endl;
                    exit(-1);
                }
                omp_unset_lock(&lock[j]);
            }
        }

        tmp_sz = 0;
        for (int j = 0; j < sol_sz; j++)
            if (bsol[sol[j]])
                tmp[tmp_sz++] = sol[j];
        std::swap(sol, tmp);
        sol_sz = tmp_sz;
    }

    for (int i = 0; i < sol_sz; i++)
        bsol[sol[i]] = true;

    std::vector<int> redund;
    std::vector<int> miss;
    for (int i = 0; i < (1 << dim); i++) {
        if (bref[i] == bsol[i])
            continue;
        else if (bref[i] && !bsol[i])
            miss.push_back(i);
        else if (!bref[i] && bsol[i])
            redund.push_back(i);
    }
    std::cout << "done\n\n";

    if (redund.size() == 0 && miss.size() == 0)
        std::cout << "CORRECT\n"
                  << "solution set has indentical " << sol_sz << " points" << std::endl;
    else {
        std::cout << "WRONG\n"
                  << std::endl;
        std::cout << "solution set has " << sol_sz << " points\n\n";
        if (redund.size()) {
            std::cout << "wrong points in sution set:\n";
            for (int i = 0; i < redund.size(); i++)
                std::cout << redund[i] << " ";
            std::cout << std::endl;
        }
        if (miss.size()) {
            std::cout << "missing points:\n";
            for (int i = 0; i < miss.size(); i++)
                std::cout << miss[i] << " ";
            std::cout << std::endl;
        }
    }

    for (int i = 0; i < (1 << dim); i++)
        omp_destroy_lock(&lock[i]);
    delete[] sol;
    delete[] tmp;
    delete[] bsol;
    delete[] bref;
    delete[] lock;

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";
    std::cout << "*********************** CHECKING OVER! ***********************\n"
              << std::endl;
}

std::vector<BPTCheck::Ineq> BPTCheck::eqn_obt(std::vector<std::vector<int>> eqn_int) {
    std::vector<Ineq> eqn;
    if (eqn_int.empty()) {
        std::cout << "Notice: There is no equation to be checked (Affine dimension is full)." << std::endl;
        return eqn;
    }
    for (auto &item : eqn_int) {
        if (item.empty())
            continue;

        int b = item[item.size() - 1];
        item.pop_back();
        eqn.push_back(Ineq(item, b));
    }
    std::cout << "EQUATION NUM " << eqn.size() << " \n"
              << std::endl;
    return eqn;
}

void BPTCheck::check2(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int, std::vector<std::vector<int>> eqn_int) {
    std::cout << "\n*********************** CHECKING START ***********************" << std::endl;
    std::ios_base::sync_with_stdio(false);
    auto start = std::chrono::steady_clock::now();

    std::vector<int> ref_ans = ref_poss_init(bpt);

    std::vector<Ineq> ineq = ineq_obt(ineq_int);
    std::vector<Ineq> eqn = eqn_obt(eqn_int);

    int dim = ineq.empty() ? (eqn.empty() ? 0 : eqn[0].get_dim()) : ineq[0].get_dim();
    assert(dim > 0);

    int *sol = new int[1 << dim];
    int *tmp = new int[1 << dim];
    bool *bsol = new bool[1 << dim];
    bool *bref = new bool[1 << dim];

    for (int i = 0; i < (1 << dim); i++)
        bref[i] = false;
    for (int i = 0; i < ref_ans.size(); i++)
        bref[ref_ans[i]] = true;

    for (int i = 0; i < (1 << dim); i++)
        sol[i] = i, bsol[i] = true;
    int sol_sz = (1 << dim);
    int tmp_sz = 0;

    omp_lock_t *lock = new omp_lock_t[1 << dim];
    for (int i = 0; i < (1 << dim); i++)
        omp_init_lock(&lock[i]);
    omp_set_num_threads(CK_THD);

    for (int i = 0; i < eqn.size(); i++) {

        std::cout << "CHECKING equation " << i << std::endl;
        Ineq &e = eqn[i];

#pragma omp parallel for shared(e, sol, bsol)
        for (int j = 0; j < sol_sz; j++) {

            if (bsol[sol[j]] && !e.check_equal(sol[j])) {
                omp_set_lock(&lock[j]);
                bsol[sol[j]] = false;
                if (bref[sol[j]]) {
                    std::cout << "WRONG: " << sol[j] << " " << utilities::d_to_b(sol[j]) << " is in reference set\n";
                    std::cout << "but violated EQUATION. Coefficients: ";
                    for (int k = 0; k < eqn[i].get_ineq().size(); ++k) {
                        std::cout << eqn[i].get_ineq()[k] << " ";
                    }
                    std::cout << std::endl;
                    exit(-1);
                }
                omp_unset_lock(&lock[j]);
            }
        }

        tmp_sz = 0;
        for (int j = 0; j < sol_sz; j++)
            if (bsol[sol[j]])
                tmp[tmp_sz++] = sol[j];
        std::swap(sol, tmp);
        sol_sz = tmp_sz;
    }

    for (int i = 0; i < ineq.size(); i++) {

        std::cout << "CHECKING inequality " << i << std::endl;
        Ineq &t = ineq[i];

#pragma omp parallel for shared(t, sol, bsol)
        for (int j = 0; j < sol_sz; j++) {
            if (bsol[sol[j]] && !t(sol[j])) {
                omp_set_lock(&lock[j]);
                bsol[sol[j]] = false;
                if (bref[sol[j]]) {
                    std::cout << "WRONG: " << sol[j] << " " << utilities::d_to_b(sol[j]) << " is in reference set\n";
                    std::cout << "but violated INEQUALITY. Coefficients: ";
                    for (int k = 0; k < ineq[i].get_ineq().size(); ++k) {
                        std::cout << ineq[i].get_ineq()[k] << " ";
                    }
                    std::cout << std::endl;
                    exit(-1);
                }
                omp_unset_lock(&lock[j]);
            }
        }

        tmp_sz = 0;
        for (int j = 0; j < sol_sz; j++)
            if (bsol[sol[j]])
                tmp[tmp_sz++] = sol[j];
        std::swap(sol, tmp);
        sol_sz = tmp_sz;
    }

    for (int i = 0; i < sol_sz; i++)
        bsol[sol[i]] = true;

    std::vector<int> redund;
    std::vector<int> miss;
    for (int i = 0; i < (1 << dim); i++) {
        if (bref[i] == bsol[i])
            continue;
        else if (bref[i] && !bsol[i])
            miss.push_back(i);
        else if (!bref[i] && bsol[i])
            redund.push_back(i);
    }

    std::cout << "done\n\n";
    if (redund.empty() && miss.empty())
        std::cout << "CORRECT\nsolution set has identical " << sol_sz << " points" << std::endl;
    else {
        std::cout << "WRONG\n\nsolution set has " << sol_sz << " points\n\n";
        if (!redund.empty()) {
            std::cout << "wrong points in solution set:\n";
            for (int i : redund)
                std::cout << i << " ";
            std::cout << std::endl;
        }
        if (!miss.empty()) {
            std::cout << "missing points:\n";
            for (int i : miss)
                std::cout << i << " ";
            std::cout << std::endl;
        }
    }

    for (int i = 0; i < (1 << dim); i++)
        omp_destroy_lock(&lock[i]);
    delete[] sol;
    delete[] tmp;
    delete[] bsol;
    delete[] bref;
    delete[] lock;

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";
    std::cout << "*********************** CHECKING OVER! ***********************\n"
              << std::endl;
}
