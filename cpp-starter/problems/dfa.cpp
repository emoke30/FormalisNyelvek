
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "dfa.hpp"

using namespace std;

// A parancssori kapcsolo beallitasa
void DFAProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Ellenorizendo szavak", cxxopts::value<string>());
}

// Megnezzuk, hogy a DFA feladatot valasztottak-e
bool DFAProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0;
}

// Egyetlen szo ellenorzese
bool accepts(
    const string &word,
    const string &startState,
    const set<string> &finalStates,
    const map<pair<string, char>, string> &transitions
) {
    string currentState = startState;

    for (char c : word) {
        auto key = make_pair(currentState, c);
        auto it = transitions.find(key);

        // Nincs ilyen atmenet: a szo elutasitva
        if (it == transitions.end()) {
            return false;
        }

        // Atlepunk a kovetkezo allapotba
        currentState = it->second;
    }

    // A szo vegen ellenorizzuk, hogy vegallapotban vagyunk-e
    return finalStates.count(currentState) > 0;
}

// A teljes feladat vegrehajtasa
int DFAProblem::run(const cxxopts::ParseResult &args) {
    string inputFilename = args["input"].as<string>();
    string outputFilename = args["output"].as<string>();
    string words = args["check"].as<string>();

    // Bemeneti fajl megnyitasa
    ifstream inputFile(inputFilename);

    if (!inputFile) {
        cerr << "Nem sikerult megnyitni a bemeneti fajlt: "
             << inputFilename << endl;
        return 1;
    }

    // Az automata adatainak tarolasa
    vector<string> states;
    vector<char> alphabet;
    string startState;
    set<string> finalStates;
    map<pair<string, char>, string> transitions;

    string line;
    string token;

    // 1. sor: allapotok
    getline(inputFile, line);
    istringstream statesLine(line);

    while (statesLine >> token) {
        states.push_back(token);
    }

    // 2. sor: abéce
    getline(inputFile, line);
    istringstream alphabetLine(line);

    while (alphabetLine >> token) {
        alphabet.push_back(token[0]);
    }

    // 3. sor: kezdoallapot
    getline(inputFile, startState);

    // Eltavolitjuk az esetleges szokozoket a sor elejerol
    startState.erase(0, startState.find_first_not_of(" \t\r"));

    // 4. sor: vegallapotok
    getline(inputFile, line);
    istringstream finalLine(line);

    while (finalLine >> token) {
        finalStates.insert(token);
    }

    // A tobbi sor: atmenetek
    string fromState;
    string symbol;
    string toState;

    while (inputFile >> fromState >> symbol >> toState) {
        transitions[{fromState, symbol[0]}] = toState;
    }

    inputFile.close();

    // Kimeneti fajl megnyitasa
    ofstream outputFile(outputFilename);

    if (!outputFile) {
        cerr << "Nem sikerult megnyitni a kimeneti fajlt: "
             << outputFilename << endl;
        return 1;
    }

    // A vesszovel elvalasztott szavak feldolgozasa
    istringstream wordsStream(words);
    string word;
    bool first = true;

    while (getline(wordsStream, word, ',')) {
        if (!first) {
            outputFile << '\n';
        }

        if (accepts(word, startState, finalStates, transitions)) {
            outputFile << "IGEN";
        } else {
            outputFile << "NEM";
        }

        first = false;
    }

    outputFile.close();

    return 0;
}