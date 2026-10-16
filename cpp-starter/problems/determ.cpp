#include "determ.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

void DeterminizeProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("det", "Automaták determinizálása");
}

bool DeterminizeProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("det") > 0;
}

int DeterminizeProblem::run(const cxxopts::ParseResult &args) {
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
    if (!getline(inputFile, line)) {
        cerr << "Hiba: Ures bemeneti fajl!" << endl;
        return 1;
    }
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
    if (!getline(inputFile, line)) {
        cerr << "Hiba: Hianyzo abece!" << endl;
        return 1;
    }
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
    if (!getline(inputFile, line)) {
        cerr << "Hiba: Hianyzo kezdoallapot!" << endl;
        return 1;
    }
    string startState;
    {
        istringstream ss(line);
        ss >> startState;
    }
    if (valid_states.count(startState) == 0) {
        cerr << "Hiba: A(z) '" << startState << "' kezdoallapot nem szerepel a megadott allapotok kozott!" << endl;
        return 1;
    }

    // 4. sor: Végállapotok
    if (!getline(inputFile, line)) {
        cerr << "Hiba: Hianyzo vegallapotok!" << endl;
        return 1;
    }
    set<string> orig_final_states;
    {
        istringstream ss(line);
        string token;
        while (ss >> token) {
            if (valid_states.count(token) == 0) {
                cerr << "Hiba: A(z) '" << token << "' vegallapot nem szerepel a megadott allapotok kozott!" << endl;
                return 1;
            }
            orig_final_states.insert(token);
        }
    }

    // Következő sorok: Átmenetek (honnan szimbólum hova)
    map<pair<string, string>, set<string>> nfa_transitions;
    string u, sym, v;
    while (inputFile >> u >> sym >> v) {
        if (valid_states.count(u) == 0) {
            cerr << "Hiba: Az atmenet forrasallapota ('" << u << "') nem ervényes!" << endl;
            return 1;
        }
        if (valid_alphabet.count(sym) == 0) {
            cerr << "Hiba: Az atmenet szimboluma ('" << sym << "') nem szerepel az abeceben!" << endl;
            return 1;
        }
        if (valid_states.count(v) == 0) {
            cerr << "Hiba: Az atmenet celallapota ('" << v << "') nem ervényes!" << endl;
            return 1;
        }

        nfa_transitions[{u, sym}].insert(v);
    }
    inputFile.close();

    // --- Determinizálás (Részkészlet-konstrukció) ---
    vector<set<string>> dfa_states;
    map<set<string>, int> state_to_id;

    // s0 = { startState }
    set<string> start_set = { startState };
    dfa_states.push_back(start_set);
    state_to_id[start_set] = 0;

    map<pair<int, string>, int> dfa_transitions;

    size_t current_idx = 0;
    while (current_idx < dfa_states.size()) {
        // FONTOS: Másolatot készítünk az aktuális halmazról,
        // hogy a dfa_states.push_back() miatti vektor-újraméretezés ne tegye érvénytelenné a referenciát!
        set<string> current_set = dfa_states[current_idx];

        for (const auto &s_sym : alphabet) {
            set<string> next_set;
            for (const auto &st : current_set) {
                auto it = nfa_transitions.find({st, s_sym});
                if (it != nfa_transitions.end()) {
                    next_set.insert(it->second.begin(), it->second.end());
                }
            }

            if (!next_set.empty()) {
                if (state_to_id.count(next_set) == 0) {
                    int new_id = static_cast<int>(dfa_states.size());
                    dfa_states.push_back(next_set);
                    state_to_id[next_set] = new_id;
                }
                dfa_transitions[{static_cast<int>(current_idx), s_sym}] = state_to_id[next_set];
            }
        }
        current_idx++;
    }

    // --- Kiíratás a kimeneti fájlba ---
    ofstream outputFile(outputFilename);
    if (!outputFile) {
        cerr << "Nem sikerult megnyitni a kimeneti fajlt: " << outputFilename << endl;
        return 1;
    }

    // 1. sor: DFA állapotok
    for (size_t i = 0; i < dfa_states.size(); ++i) {
        outputFile << "s" << i << (i + 1 == dfa_states.size() ? "" : " ");
    }
    outputFile << "\n";

    // 2. sor: Bemeneti ábécé
    for (size_t i = 0; i < alphabet.size(); ++i) {
        outputFile << alphabet[i] << (i + 1 == alphabet.size() ? "" : " ");
    }
    outputFile << "\n";

    // 3. sor: Kezdőállapot
    outputFile << "s0\n";

    // 4. sor: Végállapotok
    vector<string> dfa_final_states;
    for (size_t i = 0; i < dfa_states.size(); ++i) {
        bool is_final = false;
        for (const auto &st : dfa_states[i]) {
            if (orig_final_states.count(st) > 0) {
                is_final = true;
                break;
            }
        }
        if (is_final) {
            dfa_final_states.push_back("s" + to_string(i));
        }
    }

    for (size_t i = 0; i < dfa_final_states.size(); ++i) {
        outputFile << dfa_final_states[i] << (i + 1 == dfa_final_states.size() ? "" : " ");
    }
    outputFile << "\n";

    // Átmenetek kiírása (forrásállapot, él címkéje szerint rendezve)
    for (size_t i = 0; i < dfa_states.size(); ++i) {
        for (const auto &s_sym : alphabet) {
            auto it = dfa_transitions.find({static_cast<int>(i), s_sym});
            if (it != dfa_transitions.end()) {
                outputFile << "s" << i << " " << s_sym << " s" << it->second << "\n";
            }
        }
    }

    outputFile.close();
    return 0;
}