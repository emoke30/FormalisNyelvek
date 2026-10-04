
#ifndef PROBLEMS_DFA_H
#define PROBLEMS_DFA_H

#include "../problem.hpp"

class DFAProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // PROBLEMS_DFA_H