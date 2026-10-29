#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include <string>
#include "cxxopts.hpp"

class Problem {
public:
    virtual void initialize_parser(cxxopts::Options &options) = 0;
    virtual bool is_chosen_problem(const cxxopts::ParseResult &args) = 0;
    virtual int run(const cxxopts::ParseResult &args) = 0;
};

class MinimizeProblem : public Problem { // <- Így kapcsolódik a meglévő keretedhez
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // PROBLEM_HPP