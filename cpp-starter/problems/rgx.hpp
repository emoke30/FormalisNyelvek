#ifndef PROBLEMS_RGX_HPP
#define PROBLEMS_RGX_HPP

#include <string>
#include <vector>

#include "../problem.hpp"

class RegexProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;

private:
    std::vector<std::string> lines;

    bool loadLines(const std::string &filename);

    std::vector<std::string> countMatchingLines() const;     // --rgx 1
    std::vector<std::string> findErrorJavaFiles() const;     // --rgx 2
    std::vector<std::string> findProcessIds() const;         // --rgx 3
    std::vector<std::string> findMostErrors() const;         // --rgx 4
    std::vector<std::string> countMainThreadErrors() const;  // --rgx 5
};

#endif // PROBLEMS_RGX_HPP