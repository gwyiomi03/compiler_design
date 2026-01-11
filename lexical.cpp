#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <queue>
#include <string>
#include <cctype> // for isspace
#include <list>   // Used for NFA transitions in the provided code, though your lexical.h uses vector
#include <algorithm>
#include "lexical.h"

using namespace std;


int nextStateNumber = 0;
static const char EPSILON = '\0';


string getTokenName(TokenType type) {
    switch (type) {
        case IDENTIFIER: return "ID";
        case NUMBER: return "NUMBER";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case MULTIPLY: return "MULT";
        case DIVIDE: return "DIV";
        case MOD: return "MOD";
        case ASSIGN: return "ASSIGN";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case PRINT: return "PRINT";
        case FUNCTION: return "FUNCTION";
        case WHITESPACE: return "WHITESPACE";
        default: return "UNKNOWN";
    }
}


int precedence(TokenType t) {
    switch (t) {
        case WHITESPACE:
            return 1;

        case PRINT:
        case FUNCTION:
            return 2;

        case ASSIGN:
        case PLUS:
        case MINUS:
        case MULTIPLY:
        case DIVIDE:
        case MOD:
        case LPAREN:
        case RPAREN:
            return 3;

        case NUMBER:
            return 4;

        case IDENTIFIER:
            return 5;

        default:
            return 100;
    }
}

void addRange(NFAState* from, char start, char end, NFAState* to) {
    for (char c = start; c <= end; c++) {
        from->transitions[c].push_back(to);
    }
}


NFA createIdentifierNFA() {
    NFAState* start  = new NFAState(nextStateNumber++);
    NFAState* s1     = new NFAState(nextStateNumber++);
    NFAState* s2     = new NFAState(nextStateNumber++);
    NFAState* s3     = new NFAState(nextStateNumber++);
    NFAState* accept     = new NFAState(nextStateNumber++);

    accept->isAccepting = true;
    accept->tokenType = IDENTIFIER;

    // [a-zA-Z_]
    for (char c = 'a'; c <= 'z'; c++) start->transitions[c].push_back(s1);
    for (char c = 'A'; c <= 'Z'; c++) start->transitions[c].push_back(s1);
    start->transitions['_'].push_back(s1);


    // star entry
    s1->epsilon.push_back(s2);  
    s1->epsilon.push_back(accept); // skip *

    // [a-zA-Z0-9_]*
    for (char c = 'a'; c <= 'z'; c++) s2->transitions[c].push_back(s3);
    for (char c = 'A'; c <= 'Z'; c++) s2->transitions[c].push_back(s3);
    for (char c = '0'; c <= '9'; c++) s2->transitions[c].push_back(s3);
    s2->transitions['_'].push_back(s3);

    s3->epsilon.push_back(s2);
    s3->epsilon.push_back(accept); // loop

    return { start, accept };
}



NFA createNumberNFA() {
    NFAState* start = new NFAState(nextStateNumber++); 
    NFAState* s1 = new NFAState(nextStateNumber++); 
    NFAState* s2 = new NFAState(nextStateNumber++); 
    NFAState* s3 = new NFAState(nextStateNumber++); 
    NFAState* s4 = new NFAState(nextStateNumber++); 
    NFAState* s5 = new NFAState(nextStateNumber++);
    NFAState* s6 = new NFAState(nextStateNumber++);
    NFAState* s7 = new NFAState(nextStateNumber++); 
    NFAState* s8 = new NFAState(nextStateNumber++);
    NFAState* s9 = new NFAState(nextStateNumber++);
    NFAState* s10 = new NFAState(nextStateNumber++);
    NFAState* s11 = new NFAState(nextStateNumber++);
    NFAState* s12 = new NFAState(nextStateNumber++);
    NFAState* accept = new NFAState(nextStateNumber++); 

    accept->isAccepting = true;
    accept->tokenType = NUMBER;

    // [0-9]+ == [0-9][0-9]*
    // [0-9]
    for (char c = '0'; c <= '9'; c++) start->transitions[c].push_back(s1);

    // [0-9]*
    s1->epsilon.push_back(s2);
    s1->epsilon.push_back(s4);
    for (char c = '0'; c <= '9'; c++) s2->transitions[c].push_back(s3);
    s3->epsilon.push_back(s4);
    s3->epsilon.push_back(s2);

    // (\.[0-9]+)? ---
    s4->epsilon.push_back(s5); // ? up
    s5->epsilon.push_back(s6);
    s6->epsilon.push_back(accept); // into jumpt to final state
    
    s4->epsilon.push_back(s7); // ? down
    s7->transitions['.'].push_back(s8); // .
    for (char c = '0'; c <= '9'; c++) s8->transitions[c].push_back(s9); //[0-9]


    s9->epsilon.push_back(s10); // [0-9]*
    s9->epsilon.push_back(s12);
    for (char c = '0'; c <= '9'; c++) s10->transitions[c].push_back(s11);
    s11->epsilon.push_back(s12);
    s11->epsilon.push_back(s10);


    s12->epsilon.push_back(accept); //final state for 

    return { start, accept };
}

/*
NFA createIdentifierNFA() {
    NFAState* start = new NFAState(nextStateNumber++);
    NFAState* accept = new NFAState(nextStateNumber++, true, IDENTIFIER);

    // Initial character: [a-zA-Z_]
    addRange(start, 'a', 'z', accept);
    addRange(start, 'A', 'Z', accept);
    start->transitions['_'].push_back(accept);

    // Subsequent characters: [a-zA-Z0-9_]* (Self-loop on accept state)
    addRange(accept, 'a', 'z', accept);
    addRange(accept, 'A', 'Z', accept);
    addRange(accept, '0', '9', accept);
    accept->transitions['_'].push_back(accept);

    return { start, accept };
}


NFA createNumberNFA() {
    NFAState* start = new NFAState(nextStateNumber++);
    NFAState* intPart = new NFAState(nextStateNumber++, true, NUMBER);
    NFAState* dot = new NFAState(nextStateNumber++);
    NFAState* fracPart = new NFAState(nextStateNumber++, true, NUMBER);

    // Integer part: [0-9]+
    addRange(start, '0', '9', intPart);
    addRange(intPart, '0', '9', intPart);

    // Optional Decimal: .[0-9]+
    intPart->transitions['.'].push_back(dot);
    addRange(dot, '0', '9', fracPart);
    addRange(fracPart, '0', '9', fracPart);

    return { start, fracPart };
}
*/


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
set<NFAState*> epsilonClosure(set<NFAState*> states) {
    set<NFAState*> closure = states;
    stack<NFAState*> stack;
    for (NFAState* s : states) stack.push(s);

    while (!stack.empty()) {
        NFAState* u = stack.top();
        stack.pop();

        for (NFAState* v : u->epsilon) {
            if (closure.find(v) == closure.end()) {
                closure.insert(v);
                stack.push(v);
            }
        }
    }
    return closure;
}


// Subset construction
DFA convertNFAtoDFA(NFA nfa) {
    vector<DFAState*> dfaStates;
    map<set<NFAState*>, DFAState*> stateMap;
    queue<set<NFAState*>> worklist;
    set<char> alphabet;

    // Helper: update accepting status using precedence
    auto updateAcceptance = [&](DFAState* dState, const set<NFAState*>& nSet) {
        for (NFAState* s : nSet) {
            if (s->isAccepting) {
                if (!dState->isAccepting ||
                    precedence(s->tokenType) < precedence(dState->tokenType)) {
                    dState->isAccepting = true;
                    dState->tokenType = s->tokenType;
                }
            }
        }
    };

    // Start state
    set<NFAState*> startSet = epsilonClosure({ nfa.start });
    DFAState* startDFA = new DFAState(0);
    updateAcceptance(startDFA, startSet);

    stateMap[startSet] = startDFA;
    dfaStates.push_back(startDFA);
    worklist.push(startSet);

    int idCounter = 1;

    // Subset construction
    while (!worklist.empty()) {
        set<NFAState*> currentSet = worklist.front();
        worklist.pop();
        DFAState* currentDFA = stateMap[currentSet];

        map<char, set<NFAState*>> moves;

        // Collect transitions and alphabet
        for (NFAState* s : currentSet) {
            for (auto& [ch, targets] : s->transitions) {
                alphabet.insert(ch);
                for (NFAState* t : targets) {
                    moves[ch].insert(t);
                }
            }
        }

        // Create transitions
        for (auto& [ch, targetSet] : moves) {
            set<NFAState*> nextSet = epsilonClosure(targetSet);

            if (stateMap.find(nextSet) == stateMap.end()) {
                DFAState* newDFA = new DFAState(idCounter++);
                updateAcceptance(newDFA, nextSet);
                stateMap[nextSet] = newDFA;
                dfaStates.push_back(newDFA);
                worklist.push(nextSet);
            }

            currentDFA->transitions[ch] = stateMap[nextSet];
        }
    }

    // ---- DEAD (SINK) STATE ----
    DFAState* dead = new DFAState(idCounter++);
    dead->isAccepting = false;

    // Self-loops on all symbols
    for (char c : alphabet) {
        dead->transitions[c] = dead;
    }

    // Complete missing transitions
    for (DFAState* state : dfaStates) {
        for (char c : alphabet) {
            if (state->transitions.count(c) == 0) {
                state->transitions[c] = dead;
            }
        }
    }

    dfaStates.push_back(dead);

    return { startDFA, dfaStates };
}

map<string, TokenType> print = {{"print", PRINT}};
map<string, TokenType> functions = {{"sin", FUNCTION}, {"cos", FUNCTION}, {"tan", FUNCTION}, 
                                    {"sqrt", FUNCTION}, {"abs", FUNCTION}, {"ceil", FUNCTION}, {"floor", FUNCTION}};

                                    


ScanResult scanNextToken(const DFA& dfa, const string& input, size_t pos, int& line) {
    ScanResult result;
    const size_t n = input.size();

    //Skip whitespace
    size_t scanStartPos = pos;
    while (scanStartPos < n && isspace(static_cast<unsigned char>(input[scanStartPos]))) {
        if (input[scanStartPos] == '\n') {
            line++;
        }
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
            TransitionTrace trace = {current->id, next->id};
            fullPath.push_back(trace);
            current = next;
            i++;

            if (current->isAccepting) {
                lastAccept = i;
                lastToken = current->tokenType;
                acceptedPath = fullPath;
            }
        } else {
            break;
        }
    }

    // 3. Extract Token
    if (lastToken != UNKNOWN) {
        string lexeme = input.substr(scanStartPos, lastAccept - scanStartPos);

        // Check if the lexeme matches a keyword
        if (lastToken == IDENTIFIER) {
            auto it_kw = print.find(lexeme);
            if (it_kw != print.end()) {
                lastToken = it_kw->second; 
            } else {
                auto it_func = functions.find(lexeme);
                if (it_func != functions.end()) {
                    lastToken = it_func->second;
                }
            }
        }

        result.foundToken = true;
        result.token = Token{lastToken, lexeme, line};
        result.newPosition = lastAccept;
        result.traversalPath = acceptedPath; 

        for (char c : lexeme) {
            if (c == '\n') line++;
        }
    } else {
        result.foundToken = false;
        result.newPosition = scanStartPos + 1; 
        if (scanStartPos < n && input[scanStartPos] == '\n') line++;
    }

    return result;
}

