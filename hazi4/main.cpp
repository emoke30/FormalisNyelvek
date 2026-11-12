#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "problems/Regex.h"

namespace {

// egy logsor fejlece: 1=idobelyeg, 2=PID, 3=TID, 4=szint, 5=tag, 6=uzenet
const std::regex HEADER(
    R"(^([0-9-]+ [0-9:.]+) +([0-9]+) +([0-9]+) ([IDVEFAW]) (.+?):(?: (.+))?)");

// a fejlec eleje (datum ido PID TID), a tobbi regex ezt hasznalja
const std::string PREFIX = R"(^\S+ \S+ +\d+ +\d+ )";

// beolvassa a fajlt soronkent; siker eseten true
bool readLines(const std::string& filename, std::vector<std::string>& lines) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Hiba: a bemeneti fajl nem nyithato meg: " << filename << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();   // CRLF
        lines.push_back(line);
    }
    return true;
}

// 1. feladat: 10 darab szamlalo
void task1(const std::vector<std::string>& lines, std::ostream& out) {
    const std::vector<std::regex> patterns = {
        std::regex(R"(^10-15 )"),                                      // 10. ho 15.
        std::regex(R"(^10-15 10:18:51\.)"),                            // 10-15 10:18:51
        std::regex(R"(^\S+ \d{2}:\d{2}:51\.)"),                        // 51. masodperc
        std::regex(PREFIX + R"([IDVEFAW] lowmemorykiller:)"),          // lowmemorykiller tag
        std::regex(R"(^\S+ 10:18:51\.\d+ +221 )"),                     // 10:18:51, PID 221
        std::regex(PREFIX + R"(E )"),                                  // hibak (E)
        std::regex(PREFIX + R"(W PackageManager:)"),                   // PackageManager warning
        std::regex(PREFIX + R"(D [^:]*ExoPlayer[^:]*:)"),              // ExoPlayer debug
        std::regex(PREFIX + R"([EW] .*\.java:)"),                      // .java pozicios E/W
        std::regex(PREFIX + R"([IDVEFAW] [^:]*:.*Thread)")             // "Thread" az uzenetben
    };

    std::vector<int> counts(patterns.size(), 0);
    for (const std::string& line : lines) {
        for (size_t i = 0; i < patterns.size(); ++i) {
            if (std::regex_search(line, patterns[i])) ++counts[i];
        }
    }
    for (int c : counts) out << c << '\n';
}

// 2. feladat: hibak (E) szarmazasi helye (.java fajlnevek)
void task2(const std::vector<std::string>& lines, std::ostream& out) {
    const std::regex javaFile(R"([A-Za-z0-9_$]+\.java(?=:))");

    bool inError = false;   // az aktualis bejegyzes E szintu-e
    for (const std::string& line : lines) {
        std::smatch m;
        if (std::regex_search(line, m, HEADER)) {
            inError = (m[4].str() == "E");
        }
        // fejlecsor eseten az E sor sajat tartalma, egyebkent a stack trace sora
        if (inError) {
            for (std::sregex_iterator it(line.begin(), line.end(), javaFile), end;
                 it != end; ++it) {
                out << it->str() << '\n';
            }
        }
    }
}

// 3. feladat: kulonbozo PID-ek elofordulasi sorrendben
void task3(const std::vector<std::string>& lines, std::ostream& out) {
    std::set<std::string> seen;
    for (const std::string& line : lines) {
        std::smatch m;
        if (std::regex_search(line, m, HEADER)) {
            if (seen.insert(m[2].str()).second) {
                out << m[2].str() << '\n';
            }
        }
    }
}

// 4. feladat: a legtobb hibat tartalmazo PID
void task4(const std::vector<std::string>& lines, std::ostream& out) {
    std::map<std::string, int> errors;
    std::vector<std::string> order;   // PID-ek elso elofordulasi sorrendben

    for (const std::string& line : lines) {
        std::smatch m;
        if (std::regex_search(line, m, HEADER) && m[4].str() == "E") {
            const std::string pid = m[2].str();
            if (errors.find(pid) == errors.end()) order.push_back(pid);
            ++errors[pid];
        }
    }

    // dontetlennel az nyer, amelyik hamarabb jelent meg
    std::string best;
    int bestCount = 0;
    for (const std::string& pid : order) {
        if (errors[pid] > bestCount) {
            bestCount = errors[pid];
            best = pid;
        }
    }

    if (bestCount > 0) {
        out << "PID=" << best << " (" << bestCount << " errors)\n";
    }
}

// 5. feladat: fo szalon tortent hibak (PID == TID), visszahivatkozassal
void task5(const std::vector<std::string>& lines, std::ostream& out) {
    const std::regex mainThreadError(R"(^\S+ \S+ +(\d+) +\1 E )");

    int count = 0;
    for (const std::string& line : lines) {
        if (std::regex_search(line, mainThreadError)) ++count;
    }
    out << count << '\n';
}

} // namespace

void RegexProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("rgx", "Run regex task (1-5) on the log file", cxxopts::value<int>());
}

bool RegexProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("rgx") > 0;
}

int RegexProblem::run(const cxxopts::ParseResult &args) {
    if (!args.count("input") || !args.count("output")) {
        std::cerr << "Hiba: --input es --output megadasa kotelezo" << std::endl;
        return 1;
    }

    const int task = args["rgx"].as<int>();
    if (task < 1 || task > 5) {
        std::cerr << "Hiba: ervenytelen feladatszam (1-5): " << task << std::endl;
        return 1;
    }

    std::vector<std::string> lines;
    if (!readLines(args["input"].as<std::string>(), lines)) {
        return 1;
    }

    const std::string outputFilename = args["output"].as<std::string>();
    std::ofstream out(outputFilename);
    if (!out) {
        std::cerr << "Hiba: a kimeneti fajl nem irhato: " << outputFilename << std::endl;
        return 1;
    }

    switch (task) {
        case 1: task1(lines, out); break;
        case 2: task2(lines, out); break;
        case 3: task3(lines, out); break;
        case 4: task4(lines, out); break;
        case 5: task5(lines, out); break;
    }

    return 0;
}