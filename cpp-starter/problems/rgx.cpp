#include "rgx.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <regex>

using namespace std;

void RegexProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("rgx", "Regex feladat sorszama (1-5)", cxxopts::value<int>());
}

bool RegexProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("rgx") > 0;
}

int RegexProblem::run(const cxxopts::ParseResult &args) {
    if (args.count("input") == 0 || args.count("output") == 0) {
        cerr << "Hiba: Hianyzo input vagy output fajl nev!" << endl;
        return 1;
    }

    string inputFilename = args["input"].as<string>();
    string outputFilename = args["output"].as<string>();
    int taskNum = args["rgx"].as<int>();

    ifstream inputFile(inputFilename);
    if (!inputFile) {
        cerr << "Nem sikerult megnyitni a bemeneti fajlt: " << inputFilename << endl;
        return 1;
    }

    vector<string> lines;
    string line;
    while (getline(inputFile, line)) {
        lines.push_back(line);
    }
    inputFile.close();

    ofstream outputFile(outputFilename);
    if (!outputFile) {
        cerr << "Nem sikerult megnyitni a kimeneti fajlt: " << outputFilename << endl;
        return 1;
    }

    // Általános logcat sor illesztő (1: dátum, 2: idő, 3: PID, 4: TID, 5: szint, 6: tag, 7: message)
    regex logcat_line_rx(R"(^([0-9-]+)\s+([0-9:.]+)\s+([0-9]+)\s+([0-9]+)\s+([IDVEFAW])\s+(.+?):(?:\s*(.*))?\r?$)");

    if (taskNum == 1) {
        // 1. feladat: 10 db reguláris kifejezés megszámlálása
        vector<regex> patterns = {
            // 1. 10. hónap 15-én jött létre
            regex(R"(^10-15\s+)"),
            // 2. 10. hónap 15-én 10:18:51-kor jött létre
            regex(R"(^10-15\s+10:18:51(?:\.[0-9]+)?\s+)"),
            // 3. 51 másodperckor jött létre (mindegy a perc)
            regex(R"(^[0-9-]+\s+[0-9]{1,2}:[0-9]{2}:51(?:\.[0-9]+)?\s+)"),
            // 4. lowmemorykiller taget tartalmazó sor
            regex(R"(^[0-9-]+\s+[0-9:.]+\s+[0-9]+\s+[0-9]+\s+[IDVEFAW]\s+[^:]*\blowmemorykiller\b[^:]*:)"),
            // 5. 10:18:51-kor keletkező 221-es PID-jú sor
            regex(R"(^[0-9-]+\s+10:18:51(?:\.[0-9]+)?\s+221\s+[0-9]+\s+)"),
            // 6. összes hibát tartalmazó sor (E)
            regex(R"(^[0-9-]+\s+[0-9:.]+\s+[0-9]+\s+[0-9]+\s+E\s+)"),
            // 7. összes warning (W), amit a PackageManager küld
            regex(R"(^[0-9-]+\s+[0-9:.]+\s+[0-9]+\s+[0-9]+\s+W\s+[^:]*\bPackageManager\b[^:]*:)"),
            // 8. összes ExoPlayer-es debug (D) infó (kis- és nagybetű függetlenül)
            regex(R"(^[0-9-]+\s+[0-9:.]+\s+[0-9]+\s+[0-9]+\s+D\s+.*ExoPlayer)"),
            // 9. összes java állomány pozíciót tartalmazó hiba és warning (E/W + .java:)
            regex(R"(^[0-9-]+\s+[0-9:.]+\s+[0-9]+\s+[0-9]+\s+[EW]\s+.*\.java:)"),
            // 10. tartalmazza a Thread szót a message részben (nem a tagben)
            regex(R"(^[0-9-]+\s+[0-9:.]+\s+[0-9]+\s+[0-9]+\s+[IDVEFAW]\s+.+?:\s+.*\bThread\b)")
        };

        vector<int> counts(patterns.size(), 0);
        for (const auto& l : lines) {
            for (size_t i = 0; i < patterns.size(); ++i) {
                if (regex_search(l, patterns[i])) {
                    counts[i]++;
                }
            }
        }

        for (size_t i = 0; i < counts.size(); ++i) {
            outputFile << counts[i] << "\n";
        }

    } else if (taskNum == 2) {
        // 2. feladat: Hibák (E) stack trace-éből a .java fájlnevek kinyerése előfordulási sorrendben
        regex error_msg_rx(R"(^[0-9-]+\s+[0-9:.]+\s+[0-9]+\s+[0-9]+\s+E\s+.+?:\s*(.*)\r?$)");
        regex java_file_rx(R"(([A-Za-z0-9_$]+\.java):[0-9]+)");

        for (const auto& l : lines) {
            smatch line_match;
            if (regex_search(l, line_match, error_msg_rx)) {
                string msg = line_match[1].str();
                auto begin = sregex_iterator(msg.begin(), msg.end(), java_file_rx);
                auto end = sregex_iterator();
                for (auto it = begin; it != end; ++it) {
                    outputFile << (*it)[1].str() << "\n";
                }
            }
        }

    } else if (taskNum == 3) {
        // 3. feladat: Összes különböző PID az első előfordulás sorrendjében
        set<string> seen_pids;
        vector<string> ordered_pids;

        for (const auto& l : lines) {
            smatch m;
            if (regex_match(l, m, logcat_line_rx)) {
                string pid = m[3].str();
                if (seen_pids.insert(pid).second) {
                    ordered_pids.push_back(pid);
                }
            }
        }

        for (const auto& pid : ordered_pids) {
            outputFile << pid << "\n";
        }

    } else if (taskNum == 4) {
        // 4. feladat: A legtöbb errort (E) tartalmazó processz
        map<string, int> error_counts;
        vector<string> pid_order;

        for (const auto& l : lines) {
            smatch m;
            if (regex_match(l, m, logcat_line_rx)) {
                string pid = m[3].str();
                string level = m[5].str();
                if (level == "E") {
                    if (error_counts[pid] == 0) {
                        pid_order.push_back(pid);
                    }
                    error_counts[pid]++;
                }
            }
        }

        string max_pid;
        int max_errors = -1;
        for (const auto& pid : pid_order) {
            if (error_counts[pid] > max_errors) {
                max_errors = error_counts[pid];
                max_pid = pid;
            }
        }

        if (!max_pid.empty()) {
            outputFile << "PID=" << max_pid << " (" << max_errors << " errors)\n";
        }

    } else if (taskNum == 5) {
        // 5. feladat: Fő szálon történt hibák száma (E szint, ahol PID == TID)
        int main_thread_errors = 0;

        for (const auto& l : lines) {
            smatch m;
            if (regex_match(l, m, logcat_line_rx)) {
                string pid = m[3].str();
                string tid = m[4].str();
                string level = m[5].str();
                if (level == "E" && pid == tid) {
                    main_thread_errors++;
                }
            }
        }

        outputFile << main_thread_errors << "\n";

    } else {
        cerr << "Hiba: Ismeretlen regex feladat sorszam: " << taskNum << endl;
        outputFile.close();
        return 1;
    }

    outputFile.close();
    return 0;
}