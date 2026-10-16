//
// Created by Lenovo on 10/16/2026.
//

#ifndef PROJECT_DETERM_H
#define PROJECT_DETERM_H
#include "../problem.hpp"

class DeterminizeProblem : public Problem{
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};


#endif //PROJECT_DETERM_H