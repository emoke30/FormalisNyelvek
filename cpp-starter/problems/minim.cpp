#include "minim.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cctype>

using namespace std;

// Állapotnevek természetes rendezése (pl. q0 < q1 < q2 < q3 < q10)
struct StateCompare {
    bool operator()(const string& a, const string& b) const {
        string prefix_a, prefix_b;
        int num_a = -1, num_b = -1;

        size_t i = 0;
        while (i < a.size() && !isdigit(a[i])) i++;
        prefix_a = a.substr(0, i);
        if (i < a.size()) num_a = stoi(a.substr(i));

        size_t j = 0;
        while (j < b.size() && !isdigit(b[j])) j++;
        prefix_b = b.substr(0, j);
        if (j < b.size()) num_b = stoi(b.substr(j));

        if (prefix_a != prefix_b) return prefix_a < prefix_b;
        if (num_a != -1 && num_b != -1) return num_a < num_b;
        return a < b;
    }
};

// Állapotnév átalakítása 's' kezdetűre a legkisebb elem alapján (pl. q3 -> s3, q0 -> s0)
static string to_s_name(const string& orig) {
    if (!orig.empty() && orig[0] == 'q') return "s" + orig.substr(1);
    if (!orig.empty() && orig[0] == 's') return orig;
    return "s" + orig;
}

void MinimizeProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("mini", "Automatak minimalizalasa");
}

bool MinimizeProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("mini") > 0;
}

int MinimizeProblem::run(const cxxopts::ParseResult &args) {
    if (args.count("input") == 0 || args.count("output") == 0) {
        cerr << "Hiba: Hianyzo input vagy output fajl nev!" << endl;
        return 1;
    }

    string inputFilename = args["input"].as<string>();
    string outputFilename = args["output"].as<string>();

    ifstream inputFile(inputFilename);
    if (!inputFile) {
        cerr << "Nem sikerult megnyitni a bemeneti fajlt: " << inputFilename << endl;
        return 1;
    }

    string line;

    // 1. sor: Állapotok
    if (!getline(inputFile, line)) return 1;
    vector<string> orig_states;
    set<string> valid_states;
    {
        istringstream ss(line);
        string token;
        while (ss >> token) {
            orig_states.push_back(token);
            valid_states.insert(token);
        }
    }

    // 2. sor: Bemeneti ábécé
    if (!getline(inputFile, line)) return 1;
    vector<string> alphabet;
    set<string> valid_alphabet;
    {
        istringstream ss(line);
        string token;
        while (ss >> token) {
            alphabet.push_back(token);
            valid_alphabet.insert(token);
        }
    }

    // 3. sor: Kezdőállapot
    if (!getline(inputFile, line)) return 1;
    string startState;
    {
        istringstream ss(line);
        ss >> startState;
    }

    // 4. sor: Végállapotok
    if (!getline(inputFile, line)) return 1;
    set<string> orig_final_states;
    {
        istringstream ss(line);
        string token;
        while (ss >> token) {
            orig_final_states.insert(token);
        }
    }

    // Átmenetek beolvasása (DFA)
    map<pair<string, string>, string> dfa_transitions;
    string u, sym, v;
    while (inputFile >> u >> sym >> v) {
        dfa_transitions[{u, sym}] = v;
    }
    inputFile.close();

    // ----------------------------------------------------
    // 1. LÉPÉS: Elérhetetlen állapotok kiszűrése (BFS)
    // ----------------------------------------------------
    set<string> reachable;
    vector<string> q;
    q.push_back(startState);
    reachable.insert(startState);

    size_t head = 0;
    while (head < q.size()) {
        string curr = q[head++];
        for (const auto& s_sym : alphabet) {
            auto it = dfa_transitions.find({curr, s_sym});
            if (it != dfa_transitions.end()) {
                if (reachable.count(it->second) == 0) {
                    reachable.insert(it->second);
                    q.push_back(it->second);
                }
            }
        }
    }

    // ----------------------------------------------------
    // 2. LÉPÉS: Partíció-finomítás (Moore-algoritmus)
    // ----------------------------------------------------
    set<string, StateCompare> reachable_final;
    set<string, StateCompare> reachable_non_final;

    for (const auto& st : reachable) {
        if (orig_final_states.count(st) > 0) reachable_final.insert(st);
        else reachable_non_final.insert(st);
    }

    vector<set<string, StateCompare>> partition;
    if (!reachable_non_final.empty()) partition.push_back(reachable_non_final);
    if (!reachable_final.empty()) partition.push_back(reachable_final);

    bool changed = true;
    while (changed) {
        changed = false;
        vector<set<string, StateCompare>> next_partition;

        map<string, int> state_to_group;
        for (size_t g = 0; g < partition.size(); ++g) {
            for (const auto& st : partition[g]) state_to_group[st] = static_cast<int>(g);
        }

        for (const auto& group : partition) {
            if (group.size() <= 1) {
                next_partition.push_back(group);
                continue;
            }

            map<vector<int>, set<string, StateCompare>> sub_groups;
            for (const auto& st : group) {
                vector<int> target_groups;
                for (const auto& s_sym : alphabet) {
                    auto it = dfa_transitions.find({st, s_sym});
                    if (it != dfa_transitions.end() && reachable.count(it->second) > 0) {
                        target_groups.push_back(state_to_group[it->second]);
                    } else {
                        target_groups.push_back(-1);
                    }
                }
                sub_groups[target_groups].insert(st);
            }

            if (sub_groups.size() > 1) changed = true;
            for (auto& pair : sub_groups) next_partition.push_back(pair.second);
        }
        partition = next_partition;
    }

    // ----------------------------------------------------
    // 3. LÉPÉS: Csoportok gyűjtése és elnevezése
    // A csoport neve a benne lévő LEGKISEBB állapot neve lesz!
    // ----------------------------------------------------
    struct GroupInfo {
        string min_state;
        string s_name;
        set<string, StateCompare> group_set;
    };

    vector<GroupInfo> group_infos;
    for (const auto& group : partition) {
        string min_st = *group.begin(); // set elso eleme a legkisebb a StateCompare szerint
        string s_name = to_s_name(min_st);
        group_infos.push_back({min_st, s_name, group});
    }

    // A csoportokat a bennük lévő legkisebb állapot szerint sorbarendezzük
    sort(group_infos.begin(), group_infos.end(), [](const GroupInfo& a, const GroupInfo& b) {
        StateCompare cmp;
        return cmp(a.min_state, b.min_state);
    });

    map<string, string> old_to_new_name;
    vector<string> new_state_names;
    set<string> new_final_states_set;
    string new_start_state;

    for (const auto& info : group_infos) {
        new_state_names.push_back(info.s_name);

        for (const auto& st : info.group_set) {
            old_to_new_name[st] = info.s_name;
            if (st == startState) new_start_state = info.s_name;
            if (orig_final_states.count(st) > 0) new_final_states_set.insert(info.s_name);
        }
    }

    // Átmenetek leképezése az új állapotnevekre
    map<pair<string, string>, string> min_transitions;
    for (const auto& info : group_infos) {
        string rep = info.min_state;
        string src_name = old_to_new_name[rep];

        for (const auto& s_sym : alphabet) {
            auto it = dfa_transitions.find({rep, s_sym});
            if (it != dfa_transitions.end() && old_to_new_name.count(it->second) > 0) {
                min_transitions[{src_name, s_sym}] = old_to_new_name[it->second];
            }
        }
    }

    // ----------------------------------------------------
    // 4. LÉPÉS: Kiíratás a kimeneti fájlba
    // ----------------------------------------------------
    ofstream outputFile(outputFilename);
    if (!outputFile) return 1;

    // 1. sor: Állapotok
    for (size_t i = 0; i < new_state_names.size(); ++i) {
        outputFile << new_state_names[i] << (i + 1 == new_state_names.size() ? "" : " ");
    }
    outputFile << "\n";

    // 2. sor: Bemeneti ábécé
    for (size_t i = 0; i < alphabet.size(); ++i) {
        outputFile << alphabet[i] << (i + 1 == alphabet.size() ? "" : " ");
    }
    outputFile << "\n";

    // 3. sor: Kezdőállapot
    outputFile << new_start_state << "\n";

    // 4. sor: Végállapotok
    vector<string> sorted_final_states;
    for (const auto& name : new_state_names) {
        if (new_final_states_set.count(name) > 0) sorted_final_states.push_back(name);
    }
    for (size_t i = 0; i < sorted_final_states.size(); ++i) {
        outputFile << sorted_final_states[i] << (i + 1 == sorted_final_states.size() ? "" : " ");
    }
    outputFile << "\n";

    // Átmenetek kiírása (forrásállapot, majd ábécé szimbólum szerint sorbarendezve)
    for (const auto& src : new_state_names) {
        for (const auto& s_sym : alphabet) {
            auto it = min_transitions.find({src, s_sym});
            if (it != min_transitions.end()) {
                outputFile << src << " " << s_sym << " " << it->second << "\n";
            }
        }
    }

    outputFile.close();
    return 0;
}