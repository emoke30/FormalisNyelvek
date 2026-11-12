#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>

#include "rgx.hpp"

// The start of every log line: date, time, process id, thread id
// e.g. "10-15 10:18:51.254   601   601 "
// Group 1 is the process id, group 2 is the thread id
static const std::string HEAD = "^[0-9-]+ [0-9:.]+ +([0-9]+) +([0-9]+) ";

// Register the --rgx flag
void RegexProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("rgx", "Search the log file with regexes, the value is the task number (1-5)", cxxopts::value<int>());
}

// The regex problem runs when --rgx is given
bool RegexProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("rgx") > 0;
}

// Reads every line of the log file
bool RegexProblem::loadLines(const std::string &filename) {
    std::ifstream file(filename);

    if (!file) {
        return false;
    }

    lines.clear();
    std::string line;

    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return true;
}

// --rgx 1: for every regex, counts the lines that match it
std::vector<std::string> RegexProblem::countMatchingLines() const {
    std::vector<std::regex> patterns = {
        // created on 10-15
        std::regex("^10-15 "),
        // created on 10-15 at 10:18:51
        std::regex("^10-15 10:18:51\\."),
        // created at second 51 (any minute)
        std::regex("^[0-9-]+ [0-9]+:[0-9]+:51\\."),
        // the tag is lowmemorykiller
        std::regex(HEAD + "[IDVEFAW] lowmemorykiller *:"),
        // created at 10:18:51 with process id 221
        std::regex("^[0-9-]+ 10:18:51\\.[0-9]+ +221 "),
        // errors
        std::regex(HEAD + "E "),
        // warnings sent by PackageManager
        std::regex(HEAD + "W PackageManager *:"),
        // debug lines with ExoPlayer anywhere in them
        std::regex(HEAD + "D .*ExoPlayer"),
        // errors and warnings with a java file position, e.g. "HashMap.java:500"
        std::regex(HEAD + "[EW] .*\\.java:[0-9]+"),
        // "Thread" in the message: the tag ends at the first ':', so we only look after it
        std::regex(HEAD + "[IDVEFAW] [^:]+:.*Thread"),
    };

    std::vector<std::string> result;

    for (const std::regex &pattern : patterns) {
        int count = 0;

        for (const std::string &line : lines) {
            if (std::regex_search(line, pattern)) {
                count++;
            }
        }

        result.push_back(std::to_string(count));
    }

    return result;
}

// --rgx 2: the java files from the error lines, e.g. "(HashMap.java:500)" -> "HashMap.java"
std::vector<std::string> RegexProblem::findErrorJavaFiles() const {
    std::regex errorLine(HEAD + "E ");
    std::regex javaFile("([A-Za-z0-9_]+\\.java):[0-9]+");

    std::vector<std::string> result;

    for (const std::string &line : lines) {
        if (!std::regex_search(line, errorLine)) {
            continue;
        }

        // Every java file of the line is kept, even the repeated ones
        std::sregex_iterator end;
        for (std::sregex_iterator it(line.begin(), line.end(), javaFile); it != end; ++it) {
            result.push_back((*it)[1]);
        }
    }

    return result;
}

// --rgx 3: the different process ids, in the order they first appear
std::vector<std::string> RegexProblem::findProcessIds() const {
    std::regex logLine(HEAD);

    std::vector<std::string> result;
    std::set<std::string> seen;

    for (const std::string &line : lines) {
        std::smatch match;

        if (std::regex_search(line, match, logLine)) {
            std::string processId = match[1];

            if (seen.count(processId) == 0) {
                seen.insert(processId);
                result.push_back(processId);
            }
        }
    }

    return result;
}

// --rgx 4: the process with the most errors, e.g. "PID=601 (12 errors)"
std::vector<std::string> RegexProblem::findMostErrors() const {
    std::regex errorLine(HEAD + "E ");

    // process id -> number of errors
    std::map<std::string, int> errorCount;
    // process ids in the order they first appear
    std::vector<std::string> order;

    for (const std::string &line : lines) {
        std::smatch match;

        if (std::regex_search(line, match, errorLine)) {
            std::string processId = match[1];

            if (errorCount.count(processId) == 0) {
                order.push_back(processId);
            }

            errorCount[processId]++;
        }
    }

    // No errors at all
    if (order.empty()) {
        return {};
    }

    // If two processes have the same number of errors, the first one wins
    std::string best = order[0];
    for (const std::string &processId : order) {
        if (errorCount[processId] > errorCount[best]) {
            best = processId;
        }
    }

    return {"PID=" + best + " (" + std::to_string(errorCount[best]) + " errors)"};
}

// --rgx 5: the number of errors on the main thread (process id == thread id)
std::vector<std::string> RegexProblem::countMainThreadErrors() const {
    // \1 means: the same text as group 1, so the thread id is the same as the process id
    std::regex mainThreadError("^[0-9-]+ [0-9:.]+ +([0-9]+) +\\1 E ");

    int count = 0;

    for (const std::string &line : lines) {
        if (std::regex_search(line, mainThreadError)) {
            count++;
        }
    }

    return {std::to_string(count)};
}

// Run the regex problem
int RegexProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    int task = args["rgx"].as<int>();

    if (!loadLines(inputFilename)) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    std::vector<std::string> result;

    if (task == 1) {
        result = countMatchingLines();
    } else if (task == 2) {
        result = findErrorJavaFiles();
    } else if (task == 3) {
        result = findProcessIds();
    } else if (task == 4) {
        result = findMostErrors();
    } else if (task == 5) {
        result = countMainThreadErrors();
    } else {
        std::cerr << "Unknown task number: " << task << std::endl;
        return 1;
    }

    std::ofstream outputFile(outputFilename);

    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    for (size_t i = 0; i < result.size(); i++) {
        if (i > 0) {
            outputFile << '\n';
        }

        outputFile << result[i];
    }

    return 0;
}