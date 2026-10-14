#include "BPTReduction.h"

void BPTRed::rtn_save(BPTCalculator bpt, const std::vector<std::vector<int>> &rtn, double time, std::string alg) {
    std::string rtn_save = bpt.get_path() + "reduction/" + "bits/";
    std::string rtn_stat = bpt.get_path() + "statistics/" + "bits/";
    system(("mkdir -p " + rtn_save).c_str());
    system(("mkdir -p " + rtn_stat).c_str());

    rtn_save += bpt.get_name() + ".txt";
    rtn_stat += bpt.get_name() + ".txt";

    system(("touch " + rtn_save).c_str());
    system(("touch " + rtn_stat).c_str());
    std::ofstream fs;
    fs.open(rtn_save);
    std::cout << "the results of " + alg + " : " << std::endl;
    for (auto &i : rtn) {
        for (int j : i) {
            std::cout << j << " ";
            fs << j << " ";
        }
        std::cout << std::endl;
        fs << std::endl;
    }
    fs.close();
    std::cout << "Total num of " + alg + "'s results : " << rtn.size() << std::endl;
    std::cout << "Reduction time : " << time << "s" << std::endl
              << std::endl;
    std::ofstream stat_s;
    stat_s.open(rtn_stat, std::ios::app);
    stat_s << alg + " : " << std::endl;
    stat_s << "#constrains : " << rtn.size() << std::endl;
    stat_s << "time : " << time << "s\n\n"
           << std::endl;
    stat_s.close();
}

std::vector<std::vector<int>> fast_boolean_cuts(BPTCalculator bpt) {
    std::vector<std::vector<int>> rtn;
    std::vector<std::string> impos = bpt.get_imposs();

    for (const std::string &point : impos) {
        std::vector<int> coeff;
        int const_term = 0;

        for (char bit : point) {
            if (bit == '1') {
                coeff.push_back(-1);
                const_term++;
            } else {
                coeff.push_back(1);
            }
        }

        coeff.push_back(const_term - 1);
        rtn.push_back(coeff);
    }
    return rtn;
}

std::vector<std::vector<int>> BPTRed::greedy_sun(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int) {
    std::vector<std::vector<int>> rtn;

    if (ineq_int.empty())
        return rtn;

    std::vector<BPTCheck::Ineq> ineqs = BPTCheck::ineq_obt(ineq_int);
    std::vector<int> ref_ans = BPTCheck::ref_imposs_init(bpt);
    int dim = ineqs[0].get_dim();

    time_t startTime, endTime;
    time(&startTime);

    int max_points = 1 << dim;
    std::vector<bool> bref(max_points, false);
    for (int ref_an : ref_ans) {
        bref[ref_an] = true;
    }

    int po_n = ref_ans.size();

    int max_iterations = 1000;
    int iteration_count = 0;

    while (true) {
        iteration_count++;

        if (iteration_count > max_iterations) {
            std::cerr << "Maximum iterations reached" << std::endl;
            break;
        }

        int counter = 0;
        std::vector<int> re, ineq;

        for (auto &i : ineqs) {
            std::vector<int> rm;
            int it = 0;

            for (int ref_an : ref_ans) {
                if (bref[ref_an] && !i(ref_an)) {
                    rm.push_back(ref_an);
                    it++;
                    i.add_mv_point(ref_an);
                }
            }

            if (it > counter) {
                counter = it;
                re = rm;
                ineq = i.get_ineq();
            }
        }

        if (counter == 0) {
            std::cerr << "Warning: Cannot cover remaining impossible points. Exiting greed loop early." << std::endl;
            break;
        }

        rtn.push_back(ineq);
        po_n -= counter;

        for (int i : re)
            bref[i] = false;

        for (auto &i : ineqs)
            i.reset_mv_points();

        if (po_n <= 0) {
            break;
        }
    }

    std::cout << "the number of impossible points left : " << po_n << std::endl;
    time(&endTime);
    rtn_save(bpt, rtn, difftime(endTime, startTime), "greedy_sun");

    return rtn;
}

std::vector<BPTRed::ImPoint> BPTRed::ref_impoint_init(BPTCalculator bpt) {
    std::vector<ImPoint> ref_ans;
    std::vector<std::string> ref_initial = bpt.get_imposs();
    for (const auto &i : ref_initial) {
        ref_ans.push_back(ImPoint(utilities::b_to_d(stol(i))));
    }
    std::cout << "totally " << ref_ans.size() << " impossible points objects" << std::endl;
    return ref_ans;
}

std::vector<std::vector<int>> BPTRed::milp_red(std::vector<BPTCheck::Ineq> ineqs, std::vector<ImPoint> ref_ans) {
    std::vector<std::vector<int>> rtn;
    for (auto &ref_an : ref_ans)
        for (int j = 0; j < ineqs.size(); ++j)
            if (!ineqs[j](ref_an.get_d())) {
                ref_an.rmd_add(ineqs[j]);
                ref_an.rmd_indx_add(j);
            }
    GRBEnv env = GRBEnv(true);
    env.start();
    GRBModel model = GRBModel(env);
    GRBVar *Elem;
    int vs = ineqs.size();
    Elem = model.addVars(vs, GRB_BINARY);
    for (int i = 0; i < vs; ++i) {
        std::ostringstream vname;
        vname << "inq_" << i;
        Elem[i].set(GRB_StringAttr_VarName, vname.str());
    }

    for (auto &ref_an : ref_ans) {
        GRBLinExpr cons = 0;
        for (int j = 0; j < ref_an.get_rmd_idx().size(); ++j)
            cons += Elem[ref_an.get_rmd_idx()[j]];
        model.addConstr(cons >= 1);
    }

    GRBLinExpr obj = 0;
    for (int i = 0; i < vs; ++i)
        obj += Elem[i];
    model.setObjective(obj, GRB_MINIMIZE);
    model.optimize();

    for (int i = 0; i < vs; ++i)
        if (Elem[i].get(GRB_DoubleAttr_X) == 1)
            rtn.push_back(ineqs[std::stoi((Elem[i].get(GRB_StringAttr_VarName)).substr(4))].get_ineq());
    return rtn;
}

std::vector<std::vector<int>> BPTRed::sub_milp(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int) {
    std::vector<BPTCheck::Ineq> ineqs = BPTCheck::ineq_obt(ineq_int);
    std::vector<ImPoint> ref_ans = ref_impoint_init(bpt);
    time_t startTime, endTime;
    time(&startTime);
    std::vector<std::vector<int>> rtn = milp_red(ineqs, ref_ans);
    time(&endTime);
    rtn_save(bpt, rtn, difftime(endTime, startTime), "sub_milp");
    return rtn;
}

std::vector<std::vector<int>> BPTRed::convex_hull_tech(BPTCalculator bpt, std::vector<std::vector<int>> ineq_int) {
    std::vector<BPTCheck::Ineq> ineqs = BPTCheck::ineq_obt(ineq_int);
    std::vector<ImPoint> ref_ans = ref_impoint_init(bpt);
    std::vector<int> ref_ans_int = BPTCheck::ref_imposs_init(bpt);
    time_t startTime, endTime;
    time(&startTime);

    std::set<std::vector<int>> mvd_pos;
    for (auto &ineq : ineqs) {
        for (int j : ref_ans_int)
            if (!ineq(j))
                ineq.add_mv_point(j);
        if (mvd_pos.find(ineq.get_mv_points()) == mvd_pos.end())
            mvd_pos.insert(ineq.get_mv_points());
    }
    std::cout << "Start traversing nodes in convex hull algorithm : " << std::endl;
    std::vector<BPTCheck::Ineq> n_ineqs;
    for (int l = 0; l < ref_ans.size(); ++l) {
        for (int i = 0; i < ineqs.size(); ++i) {
            for (int j = 0; j < ineqs.size(); ++j) {
                if (ineqs[i](ref_ans[l].get_d()) && ineqs[j](ref_ans[l].get_d()) && !ineqs[i].equal_to(ineqs[j])) {
                    std::vector<int> t1 = ineqs[i].get_cos();
                    std::vector<int> t2 = ineqs[j].get_cos();
                    std::vector<int> ma;
                    for (int m = 0; m < t1.size(); ++m)
                        ma.push_back(t1[m] + t2[m]);
                    int mb = ineqs[i].get_nco() + ineqs[j].get_nco();
                    BPTCheck::Ineq ti = BPTCheck::Ineq(ma, mb);

                    for (int m : ref_ans_int)
                        if (!ti(m))
                            ti.add_mv_point(m);

                    if (mvd_pos.find(ti.get_mv_points()) == mvd_pos.end()) {
                        mvd_pos.insert(ti.get_mv_points());
                        n_ineqs.push_back(ti);
                    }
                }
            }
        }
        std::cout << l << " points" << std::endl;
    }
    for (auto &n_ineq : n_ineqs)
        ineqs.push_back(n_ineq);

    std::vector<std::vector<int>> rtn = milp_red(ineqs, ref_ans);
    time(&endTime);
    rtn_save(bpt, rtn, difftime(endTime, startTime), "convex_hull");
    return rtn;
}

std::vector<int> BPTRed::supp(std::string a) {
    std::vector<int> rtn;
    for (int i = 0; i < a.size(); ++i)
        if (a[i] == '1')
            rtn.push_back(i);
    return rtn;
}

bool BPTRed::supp_intersec(std::vector<int> a, std::vector<int> b) {
    if (a.size() <= b.size()) {
        for (int &i : a)
            if (std::find(b.begin(), b.end(), i) != b.end())
                return false;
        return true;
    } else {
        for (int &i : b)
            if (std::find(a.begin(), a.end(), i) != a.end())
                return false;
        return true;
    }
}

std::vector<int> BPTRed::prec_to_ineq(BPTCalculator bpt, BPTRed::IcPrec ic) {
    std::vector<int> supp_a = supp(ic.get_a());
    std::vector<int> supp_u = supp(ic.get_u());
    std::vector<int> ii;
    int dim = bpt.get_dim() - 1;
    for (int i = 0; i < dim; ++i)
        ii.push_back(i);

    for (int &i : supp_a)
        ii.erase(std::find(ii.begin(), ii.end(), i));
    for (int &i : supp_u)
        ii.erase(std::find(ii.begin(), ii.end(), i));

    std::vector<int> cos;

    for (int i = 0; i < dim; ++i) {
        auto it_a = std::find(supp_a.begin(), supp_a.end(), i);
        auto it_ii = std::find(ii.begin(), ii.end(), i);
        if (it_a != supp_a.end() && it_ii == ii.end())
            cos.push_back(-1);
        else if (it_a == supp_a.end() && it_ii != ii.end())
            cos.push_back(1);
        else
            cos.push_back(0);
    }
    cos.push_back(utilities::weight(ic.get_a()) - 1);
    return cos;
}

std::vector<std::string> BPTRed::ck_po(BPTCalculator bpt, std::vector<int> ineq) {
    std::vector<std::string> pos = bpt.get_poss();
    int b = ineq[ineq.size() - 1];
    ineq.pop_back();
    BPTCheck::Ineq t = BPTCheck::Ineq(ineq, b);
    std::vector<std::string> pos_rtn;
    for (auto &po : pos)
        if (!t(utilities::b_to_d(std::stoll(po))))
            pos_rtn.push_back(po);
    return pos_rtn;
}

std::vector<std::string> BPTRed::obt_impo(BPTCalculator bpt, std::vector<int> ineq) {
    std::vector<std::string> impos = bpt.get_imposs();
    int b = ineq[ineq.size() - 1];
    ineq.pop_back();
    BPTCheck::Ineq t = BPTCheck::Ineq(ineq, b);
    std::vector<std::string> impos_rtn;
    for (auto &impo : impos)
        if (!t(utilities::b_to_d(std::stoll(impo))))
            impos_rtn.push_back(impo);
    return impos_rtn;
}

std::vector<std::vector<int>> BPTRed::logic_cond(BPTCalculator bpt, bool ifComb233) {
    std::vector<std::string> ref_ans = bpt.get_imposs();
    std::vector<IcPrec> ff;
    int dim = bpt.get_dim();
    time_t startTime, endTime;
    time(&startTime);

    for (int i = 0; i < ref_ans.size(); ++i) {
        std::vector<IcPrec> is;
        std::vector<std::vector<IcPrec>> ws;
        std::vector<std::vector<std::string>> wu;

        for (int j = 0; j < dim; ++j) {
            std::vector<IcPrec> ti;
            ws.push_back(ti);
            std::vector<std::string> ts;
            wu.push_back(ts);
        }

        for (int j = 0; j < ref_ans.size(); ++j) {

            std::string u = utilities::po_xor(ref_ans[i], ref_ans[j]);

            if (supp_intersec(supp(ref_ans[i]), supp(u))) {
                IcPrec t = IcPrec(ref_ans[i], u);
                std::vector<int> tq = prec_to_ineq(bpt, t);
                std::vector<std::string> pos_rtn = ck_po(bpt, tq);

                if (pos_rtn.empty())
                    wu[utilities::weight(u)].push_back(u);
            }
        }

        if (wu[1].empty())
            for (auto &j : wu[0]) {
                IcPrec t = IcPrec(ref_ans[i], j);
                is.push_back(t);
            }
        else
            for (auto &j : wu[1]) {
                IcPrec t = IcPrec(ref_ans[i], j);
                is.push_back(t);
            }

        for (auto &j : wu[0]) {
            IcPrec t = IcPrec(ref_ans[i], j);
            ws[0].push_back(t);
        }
        for (auto &j : wu[1]) {
            IcPrec t = IcPrec(ref_ans[i], j);
            ws[1].push_back(t);
        }

        for (int j = 2; j < dim; ++j) {
            for (int k = 0; k < wu[j].size(); ++k) {
                for (int l = 0; l < wu[j - 1].size(); ++l) {
                    IcPrec t = IcPrec(ref_ans[i], wu[j - 1][l]);
                    auto it = std::find(ws[j - 1].begin(), ws[j - 1].end(), t);
                    if (it == ws[j - 1].end())
                        break;
                }

                IcPrec t3 = IcPrec(ref_ans[i], wu[j][k]);
                ws[j].push_back(t3);
                std::vector<int> tq3 = prec_to_ineq(bpt, t3);
                std::vector<std::string> impos_rtn3 = obt_impo(bpt, tq3);

                for (auto &l : wu[j - 1]) {
                    IcPrec t4 = IcPrec(ref_ans[i], l);
                    std::vector<int> tq4 = prec_to_ineq(bpt, t4);
                    std::vector<std::string> impos_rtn4 = obt_impo(bpt, tq4);
                    auto it = std::find(is.begin(), is.end(), t4);
                    if (it != is.end() && utilities::sub_prec_ck(impos_rtn3, impos_rtn4))
                        is.erase(it);
                }
            }
            is.insert(is.end(), ws[j].begin(), ws[j].end());
        }
        ff.insert(ff.end(), is.begin(), is.end());
        std::cout << i << std::endl;
    }
    std::vector<std::vector<int>> aprec_rtn;
    for (auto &i : ff)
        aprec_rtn.push_back(prec_to_ineq(bpt, i));
    std::cout << "aprec find over " << std::endl;

    if (!ifComb233) {
        std::vector<BPTCheck::Ineq> ineqs = BPTCheck::ineq_obt(aprec_rtn);
        std::vector<ImPoint> ref_ans_impoint = ref_impoint_init(bpt);
        std::vector<std::vector<int>> rtn = milp_red(ineqs, ref_ans_impoint);
        time(&endTime);
        rtn_save(bpt, rtn, difftime(endTime, startTime), "logic_cond");
        return rtn;
    } else
        return aprec_rtn;
}

int BPTRed::hamming_distance(std::string a, std::string b) {
    int sum = 0;
    if (a.size() == b.size()) {
        for (int i = 0; i < a.size(); i++)
            if (a[i] != b[i])
                sum++;
        return sum;
    } else
        return -1;
}

std::vector<std::string> BPTRed::ball_points(BPTCalculator bpt, int d, std::string c) {
    std::vector<std::string> rtn, ref_ans = bpt.get_poss();
    for (const auto &tmp : ref_ans) {
        if (hamming_distance(tmp, c) <= d && hamming_distance(tmp, c) != -1)
            rtn.push_back(tmp);
    }
    return rtn;
}

std::vector<int> BPTRed::ball_ineq(BPTCalculator bpt, std::string c, int d, std::vector<std::string> bq) {
    std::vector<int> rtn;
    std::bitset<SBOX_LENGTH> it = 0;

    std::string q = it.to_string().substr(0, bpt.get_dim() - 1);
    for (auto &i : bq)
        for (int j = 0; j < i.size(); ++j) {
            std::string tb_c = utilities::po_xor(i, c);
            if (tb_c[j] == '1' && q[j] != '1')
                q.replace(j, 1, "1");
        }
    int ct = 0;
    for (int i = 0; i < c.size(); ++i) {
        if ((c[i] - 48) == 0) {
            if (q[i] - 48 == 1)
                rtn.emplace_back(2);
            else
                rtn.emplace_back(1);
        } else if ((c[i] - 48) == 1) {
            if (q[i] - 48 == 1) {
                rtn.emplace_back(-2);
                ct = ct + 2;
            } else {
                rtn.emplace_back(-1);
                ct++;
            }
        } else {
            rtn.emplace_back(0);
            continue;
        }
    }
    rtn.emplace_back(ct - 1 - d);
    return rtn;
}

std::vector<std::vector<int>> BPTRed::distorted_balls(BPTCalculator bpt, bool ifComb233) {
    std::vector<std::vector<int>> ball_rtn;
    std::vector<std::string> impos = bpt.get_imposs();
    time_t startTime, endTime;
    time(&startTime);

    for (int i = 0; i < impos.size(); ++i) {
        for (int j = 0; j < impos.size(); ++j) {
            for (int k = 0; k < impos.size(); ++k) {
                if (utilities::weight(utilities::po_xor(impos[i], impos[j])) == 1 && utilities::weight(utilities::po_xor(impos[i], impos[k])) == 1 && impos[j] != impos[k]) {
                    std::string a = utilities::po_xor(utilities::po_xor(impos[i], impos[j]), impos[k]);
                    auto it = std::find(impos.begin(), impos.end(), a);
                    if (it != impos.end()) {
                        std::vector<std::string> pa = ball_points(bpt, 1, impos[i]);
                        pa.push_back(impos[k]);
                        std::vector<std::string> ra = pa;
                        pa.erase(pa.end());
                        std::vector<std::string> pb = ball_points(bpt, 1, impos[j]);
                        std::vector<std::string> rb = pb;
                        std::vector<std::string> rc = ball_points(bpt, 1, impos[k]);

                        for (auto &l : pa) {
                            std::string ab = utilities::po_xor(utilities::po_xor(l, impos[i]), impos[j]);
                            rb.push_back(ab);
                            std::string ac = utilities::po_xor(utilities::po_xor(l, impos[i]), impos[k]);
                            rc.push_back(ac);
                        }

                        for (auto &l : pb) {
                            std::string tt = utilities::po_xor(utilities::po_xor(l, impos[j]), impos[k]);
                            rc.push_back(tt);
                        }

                        std::vector<int> ta = ball_ineq(bpt, impos[i], 1, ra), tb = ball_ineq(bpt, impos[j], 1, rb), tc = ball_ineq(bpt, impos[k], 1, rc);
                        std::vector<int> cn;
                        for (int l = 0; l < ta.size(); ++l) {
                            if (l == ta.size() - 1) {
                                cn.push_back(ta[l] + tb[l] + tc[l] - 2);
                            } else {
                                cn.push_back(ta[l] + tb[l] + tc[l]);
                            }
                        }
                        ball_rtn.push_back(cn);
                    }
                }
            }
        }
    }

    if (!ifComb233) {
        std::vector<BPTCheck::Ineq> ineqs = BPTCheck::ineq_obt(ball_rtn);
        std::vector<ImPoint> ref_ans_impoint = ref_impoint_init(bpt);
        std::vector<std::vector<int>> rtn = milp_red(ineqs, ref_ans_impoint);
        time(&endTime);

        return rtn;
    } else
        return ball_rtn;
}

std::vector<std::vector<int>> BPTRed::comb233(BPTCalculator bpt) {
    time_t startTime, endTime;
    time(&startTime);
    std::vector<std::vector<int>> lc = logic_cond(bpt, true);
    std::vector<std::vector<int>> db = distorted_balls(bpt, true);
    lc.insert(lc.begin(), db.begin(), db.end());
    std::vector<BPTCheck::Ineq> ineqs = BPTCheck::ineq_obt(lc);
    std::vector<ImPoint> ref_ans_impoint = ref_impoint_init(bpt);
    std::vector<std::vector<int>> rtn = milp_red(ineqs, ref_ans_impoint);
    time(&endTime);
    rtn_save(bpt, rtn, difftime(endTime, startTime), "comb233");
    return rtn;
}

std::vector<std::vector<int>> BPTRed::reduction(int chooser, BPTCalculator bpt) {

    std::vector<std::vector<int>> ineq_int = bpt.sage_ineqs();
    std::vector<std::vector<int>> eqn_int = bpt.sage_eqns();
    std::vector<std::vector<int>> rtn;

    switch (chooser) {

    case 1: {

        rtn = greedy_sun(bpt, ineq_int);

        BPTCheck::check2(bpt, rtn, eqn_int);
        break;
    }
    case 2:
        rtn = sub_milp(bpt, ineq_int);
        BPTCheck::check(bpt, rtn);
        break;
    case 3:
        rtn = convex_hull_tech(bpt, ineq_int);
        BPTCheck::check(bpt, rtn);
        break;
    case 4:
        rtn = logic_cond(bpt, false);
        break;
    case 5:
        rtn = comb233(bpt);
        break;

    case 7: {
        std::string path = bpt.get_path() + "reduction/" + "PreAlpha=1.txt";

        std::ifstream file(path);
        std::string line;
        while (getline(file, line)) {
            std::vector<std::string> coffS = utilities::split(line, " ");
            std::vector<int> coff;
            for (auto &j : coffS)
                coff.push_back(std::stoi(j));
            rtn.push_back(coff);
        }
        file.close();
        BPTCheck::check(bpt, rtn);
        rtn_save(bpt, rtn, 0, "Pre-Alpha=1");
        break;
    }
    default:
        rtn = greedy_sun(bpt, ineq_int);
        break;
    }
    return rtn;
}
