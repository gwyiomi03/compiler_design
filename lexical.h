#ifndef LEXICAL_H
#define LEXICAL_H

#include <string>
#include <vector>
#include <map>
#include <set>

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
    int position; 
};

// NFA and DFA structures
struct NFAState {     
    int id;
    map<char, vector<NFAState*>> transitions;
    vector<NFAState*> epsilon;
    TokenType tokenType = UNKNOWN;
    bool isAccepting = false;
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
};

struct DFA {
    DFAState* start;
    vector<DFAState*> states;
};

// Only **declare** the function here
string getTokenName(TokenType type);

// Function declarations
NFA createIdentifierNFA();
NFA createNumberNFA();
NFA createSingleCharNFA(char c, TokenType type, int idStart);
NFA combineNFAs(const vector<NFA>& nfas);
DFA convertNFAtoDFA(NFA nfa);
vector<Token> scanInput(DFA dfa, const string& input);

#endif
