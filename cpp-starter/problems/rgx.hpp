#ifndef PROBLEMS_RGX_HPP
#define PROBLEMS_RGX_HPP

#include "../problem.hpp"

class RegexProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // PROBLEMS_RGX_HPP