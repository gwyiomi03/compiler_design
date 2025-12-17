#ifndef LEXICAL_H
#define LEXICAL_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstddef>

using namespace std;

enum TokenType {
    IDENTIFIER,
    NUMBER,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    ASSIGN,
    LPAREN,
    RPAREN,
    SEMICOLON,
    WHITESPACE,
    UNKNOWN
};


struct Token {
    TokenType type;
    string value;
};


// NFA and DFA structures
struct NFAState {     
    int id;
    map<char, vector<NFAState*>> transitions;
    vector<NFAState*> epsilon;
    TokenType tokenType = UNKNOWN;
    bool isAccepting = false;

    NFAState(int id) : id(id) {}
};


struct NFA { 
    NFAState* start; 
    NFAState* accept; 
};


struct DFAState {
    int id;
    bool isAccepting = false;
    TokenType tokenType = UNKNOWN;
    map<char, DFAState*> transitions;
    vector<TokenType> tokenTypes;
    bool isKeywordPath = false;

    DFAState(int id) : id(id) {}
};


struct DFA {
    DFAState* start;
    vector<DFAState*> allStates;
};


struct DFAStep {
    int fromState;
    int toState;
    char symbol;
};

// Only **declare** the function here
string getTokenName(TokenType type);

// Function declarations
NFA createIdentifierNFA();
NFA createNumberNFA();
NFA createSingleCharNFA(char c, TokenType type);
NFA combineNFAs(const vector<NFA>& nfas);
NFA createKeywordNFA(const string& name, TokenType type);
DFA convertNFAtoDFA(NFA nfa);

struct TransitionTrace {
    int sourceId;
    int targetId;
};

struct ScanResult {
    bool foundToken = false;
    Token token = {UNKNOWN, ""};
    size_t newPosition = 0;

    vector<TransitionTrace> traversalPath;
};

ScanResult scanNextToken(const DFA& dfa, const string& input, size_t pos);
extern int nextStateNumber;



#endif
