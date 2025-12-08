#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include "lexical.h"

using namespace std;


string getTokenName(TokenType type) {
    switch (type) {
        case IDENTIFIER: return "ID";
        case NUMBER: return "NUMBER";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case MULTIPLY: return "MULT";
        case DIVIDE: return "DIV";
        case ASSIGN: return "ASSIGN";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case SEMICOLON: return "SEMICOLON";
        case WHITESPACE: return "WHITESPACE";
        default: return "UNKNOWN";
    }
}

NFA createIdentifierNFA() {
    NFAState* start = new NFAState{0};
    NFAState* accept = new NFAState{1};
    accept->isAccepting = true;
    accept->tokenType = IDENTIFIER;

    // [a-zA-Z_]+
    for (char c = 'a'; c <= 'z'; c++) start->transitions[c].push_back(accept);
    for (char c = 'A'; c <= 'Z'; c++) start->transitions[c].push_back(accept);
    start->transitions['_'].push_back(accept);

    // Loop on accept for letters/digits/underscore
    for (char c = 'a'; c <= 'z'; c++) accept->transitions[c].push_back(accept);
    for (char c = 'A'; c <= 'Z'; c++) accept->transitions[c].push_back(accept);
    for (char c = '0'; c <= '9'; c++) accept->transitions[c].push_back(accept);
    accept->transitions['_'].push_back(accept);

    return {start, accept};
}


NFA createNumberNFA() {
    NFAState* start = new NFAState{2};
    NFAState* intPart = new NFAState{3};
    NFAState* dot = new NFAState{4};
    NFAState* fracPart = new NFAState{5};
    NFAState* accept = new NFAState{6};

    // Accepting state
    accept->isAccepting = true;
    accept->tokenType = NUMBER;


    string digits = "0123456789";

    // [0-9]+ → intPart
    for (char c : digits) start->transitions[c].push_back(intPart);
    for (char c : digits) intPart->transitions[c].push_back(intPart); //loop

    // Accept integer only if no dot follows
    intPart->epsilon.push_back(accept);


    // Optional decimal: intPart -> dot -> fracPart -> acceptFrac
    intPart->transitions['.'].push_back(dot);
    for (char c : digits) dot->transitions[c].push_back(fracPart);
    for (char c : digits) fracPart->transitions[c].push_back(fracPart);//loop

    fracPart->epsilon.push_back(accept);

    return {start, nullptr};
}



NFA createSingleCharNFA(char c, TokenType type, int idStart) {
    NFAState* start = new NFAState{idStart};
    NFAState* accept = new NFAState{idStart + 1};
    accept->isAccepting = true;
    accept->tokenType = type;
    start->transitions[c].push_back(accept);
    return {start, accept};
}

NFA combineNFAs(const vector<NFA>& nfas) {
    NFAState* newStart = new NFAState{1000};
    for (const auto& nfa : nfas) {
        newStart->epsilon.push_back(nfa.start); // epsilon transition to each NFA start
    }
    return {newStart, nullptr}; // accept states are handled individually
}


// Utility function to get epsilon-closure of a set of NFA states
set<NFAState*> epsilonClosure(const set<NFAState*>& states) {
    set<NFAState*> closure = states;
    vector<NFAState*> stack(states.begin(), states.end());

    while (!stack.empty()) {
        NFAState* s = stack.back(); stack.pop_back();
        for (auto next : s->epsilon) {
            if (closure.find(next) == closure.end()) {
                closure.insert(next);
                stack.push_back(next);
            }
        }
    }
    return closure;
}

// Subset construction
DFA convertNFAtoDFA(NFA nfa) {
    vector<DFAState*> dfaStates;
    map<set<NFAState*>, DFAState*> stateMap;

    set<NFAState*> startSet = epsilonClosure({nfa.start});
    DFAState* startDFA = new DFAState{0};
    for (auto s : startSet) if (s->isAccepting) startDFA->isAccepting = true, startDFA->tokenType = s->tokenType;
    dfaStates.push_back(startDFA);
    stateMap[startSet] = startDFA;

    vector<set<NFAState*>> unprocessed = {startSet};

    int idCounter = 1;
    while (!unprocessed.empty()) {
        set<NFAState*> currentSet = unprocessed.back(); unprocessed.pop_back();
        DFAState* currentDFA = stateMap[currentSet];

        map<char, set<NFAState*>> moves;
        for (auto s : currentSet) {
            for (auto [ch, targets] : s->transitions) {
                for (auto t : targets) moves[ch].insert(t);
            }
        }

        for (auto [ch, nextSetRaw] : moves) {
            set<NFAState*> nextSet = epsilonClosure(nextSetRaw);
            if (stateMap.find(nextSet) == stateMap.end()) {
                DFAState* newDFA = new DFAState{idCounter++};
                for (auto s : nextSet) if (s->isAccepting) newDFA->isAccepting = true, newDFA->tokenType = s->tokenType;
                dfaStates.push_back(newDFA);
                stateMap[nextSet] = newDFA;
                unprocessed.push_back(nextSet);
            }
            currentDFA->transitions[ch] = stateMap[nextSet];
        }
    }

    return {startDFA, dfaStates};
}


vector<Token> scanInput(DFA dfa, const string& input) {
    vector<Token> tokens; // store tokens for parser
    size_t pos = 0;
    const size_t n = input.size();

    while (pos < n) {
        // Skip whitespace
        if (isspace(input[pos])) {
            pos++;
            continue;
        }

        DFAState* current = dfa.start;
        size_t lastAccept = pos;
        TokenType lastToken = UNKNOWN;
        size_t i = pos;

        // Follow DFA transitions as far as possible
        while (i < n && current->transitions.count(input[i])) {
            current = current->transitions[input[i]];
            i++;
            if (current->isAccepting) {
                lastAccept = i;
                lastToken = current->tokenType;
            }
        }

        string tokenValue = input.substr(pos, lastAccept - pos);
        bool invalidToken = false;

        // Check NUMBER validity
        if (lastToken == NUMBER) {
            int dotCount = 0;
            for (char c : tokenValue) if (c == '.') dotCount++;

            if (dotCount > 1 || tokenValue.front() == '.' || tokenValue.back() == '.') {
                invalidToken = true;
            }
        }

        Token tok;
        if (lastToken == UNKNOWN) {
            size_t unknownEnd = pos;
            while (unknownEnd < n && !isspace(input[unknownEnd])) unknownEnd++;
            tok = {UNKNOWN, input.substr(pos, unknownEnd - pos)};
            pos = unknownEnd;
        } else {
            tok = {lastToken, tokenValue};
            pos = lastAccept;
        }

        // Debug print
        cout << "(" << getTokenName(tok.type) << ", " << tok.value << ")" << endl;

        tokens.push_back(tok); // add to vector for parser
    }

    return tokens;
}


