#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <cctype> // for isspace
#include <list>   // Used for NFA transitions in the provided code, though your lexical.h uses vector
#include "lexical.h"

using namespace std;


int nextStateNumber = 0;
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
    NFAState* start  = new NFAState(nextStateNumber++);
    NFAState* s1     = new NFAState(nextStateNumber++);
    NFAState* s2     = new NFAState(nextStateNumber++); // star entry
    NFAState* s3     = new NFAState(nextStateNumber++);
    NFAState* s4     = new NFAState(nextStateNumber++);// symbol start
    NFAState* accept = new NFAState(nextStateNumber++);

    accept->isAccepting = true;
    accept->tokenType = IDENTIFIER;

    // [a-zA-Z_]
    for (char c = 'a'; c <= 'z'; c++) start->transitions[c].push_back(s1);
    for (char c = 'A'; c <= 'Z'; c++) start->transitions[c].push_back(s1);
    start->transitions['_'].push_back(s1);


    // star entry
    s1->epsilon.push_back(s2);  
    s2->epsilon.push_back(accept); // skip *

    // [a-zA-Z0-9_]*
    s2->epsilon.push_back(s3);
    for (char c = 'a'; c <= 'z'; c++) s3->transitions[c].push_back(s1);
    for (char c = 'A'; c <= 'Z'; c++) s3->transitions[c].push_back(s1);
    for (char c = '0'; c <= '9'; c++) s3->transitions[c].push_back(s1);
    s3->transitions['_'].push_back(s1);
  

    return { start, accept };
}


NFA createNumberNFA() {
    NFAState* start = new NFAState(nextStateNumber++); 
    NFAState* s1 = new NFAState(nextStateNumber++); 
    NFAState* s2 = new NFAState(nextStateNumber++); 
    NFAState* s3 = new NFAState(nextStateNumber++); 
    NFAState* s4 = new NFAState(nextStateNumber++); 
    NFAState* s5 = new NFAState(nextStateNumber++);
    NFAState* accept = new NFAState(nextStateNumber++); 

    accept->isAccepting = true;
    accept->tokenType = NUMBER;

    // --- Integer part [0-9]+ ---
    for (char c = '0'; c <= '9'; c++) start->transitions[c].push_back(s1);
    for (char c = '0'; c <= '9'; c++) s1->transitions[c].push_back(s1);
    s1->epsilon.push_back(accept); // allow integer-only number

    // --- Fractional part (\.[0-9]+)? ---
    s1->transitions['.'].push_back(s3);
    for (char c = '0'; c <= '9'; c++) s3->transitions[c].push_back(s4);
    for (char c = '0'; c <= '9'; c++) s4->transitions[c].push_back(s4);
    s4->epsilon.push_back(accept);

    return { start, accept };
}




NFA createSingleCharNFA(char c, TokenType type) {
    NFAState* start = new NFAState(nextStateNumber++);
    NFAState* accept = new NFAState(nextStateNumber++);
    accept->isAccepting = true;
    accept->tokenType = type;
    start->transitions[c].push_back(accept);
    return {start, accept};
}





NFA combineNFAs(const vector<NFA>& nfas) {
    NFAState* newStart = new NFAState(nextStateNumber++);
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
    DFAState* startDFA = new DFAState(0);
    // Find the highest precedence accepting state in the closure set
    for (auto s : startSet) if (s->isAccepting) { startDFA->isAccepting = true; startDFA->tokenType = s->tokenType; break; }
    
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
            if (nextSet.empty()) continue; // Skip if no reachable states

            if (stateMap.find(nextSet) == stateMap.end()) {
                DFAState* newDFA = new DFAState(idCounter++);
                // Set acceptance and token type (Maximal Munch -> highest precedence)
                for (auto s : nextSet) if (s->isAccepting) { newDFA->isAccepting = true; newDFA->tokenType = s->tokenType; break; }
                
                dfaStates.push_back(newDFA);
                stateMap[nextSet] = newDFA;
                unprocessed.push_back(nextSet);
            }
            currentDFA->transitions[ch] = stateMap[nextSet];
        }
    }

    return {startDFA, dfaStates};
}


ScanResult scanNextToken(const DFA& dfa, const string& input, size_t pos) {
    ScanResult result;
    const size_t n = input.size();

    // 1. Skip whitespace
    size_t scanStartPos = pos;
    while (scanStartPos < n && isspace(static_cast<unsigned char>(input[scanStartPos]))) {
        scanStartPos++;
    }

    if (scanStartPos >= n) {
        result.foundToken = false;
        result.newPosition = n;
        return result;
    }

    // 2. DFA Scan setup
    DFAState* current = dfa.start;
    size_t lastAccept = scanStartPos;
    TokenType lastToken = UNKNOWN;
    size_t i = scanStartPos;
    
    // Path tracking variables ADDED
    vector<TransitionTrace> fullPath; 
    vector<TransitionTrace> acceptedPath; 
    
    // Check initial state acceptance 
    if (current->isAccepting) {
        lastAccept = i;
        lastToken = current->tokenType;
    }

    // Follow DFA transitions as far as possible
    while (i < n) {
        char currentChar = input[i];
        
        if (current->transitions.count(currentChar)) {
            DFAState* next = current->transitions.at(currentChar);
            
            // Record the transition 
            TransitionTrace trace = {current->id, next->id};
            fullPath.push_back(trace);
            
            current = next;
            i++;
            
            // Record the last (longest) accepting state reached
            if (current->isAccepting) {
                lastAccept = i;
                lastToken = current->tokenType;
                
                // Store the path up to this point
                acceptedPath = fullPath;
            }
        } else {
            // Dead end, break the loop
            break;
        }
    }

    // 3. Extract Token
    if (lastToken != UNKNOWN) {
        result.foundToken = true;
        result.token = {lastToken, input.substr(scanStartPos, lastAccept - scanStartPos)};
        result.newPosition = lastAccept; 
        
        // Assign the path
        result.traversalPath = acceptedPath; 
        
    } else {
        result.foundToken = false;
        result.newPosition = scanStartPos + 1; 
    }

    return result;
}

