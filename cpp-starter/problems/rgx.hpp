#ifndef PROBLEMS_RGX_HPP
#define PROBLEMS_RGX_HPP

#include <string>
#include <vector>
#include "../problem.hpp"

class RegexProblem : public Problem {
private:
    std::vector<std::string> lines;

    bool loadLines(const std::string &filename);
    std::vector<std::string> countMatchingLines() const;
    std::vector<std::string> findErrorJavaFiles() const;
    std::vector<std::string> findProcessIds() const;
    std::vector<std::string> findMostErrors() const;
    std::vector<std::string> countMainThreadErrors() const;

public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // PROBLEMS_RGX_HPP