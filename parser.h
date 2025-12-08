#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "lexical.h"

using namespace std;

// ----------------------------
// Parse Tree Node
// ----------------------------
struct ParseNode {
    string symbol;                 // e.g., "Expression", "+", "IDENTIFIER"
    string lexeme;                 // terminals (tokens)
    vector<ParseNode*> children;
};

// Function declarations (no definitions)
ParseNode* makeNode(const string& sym);
ParseNode* makeLeaf(const string& sym, const string& lex);

// Parser function declaration
ParseNode* parse(const vector<Token>& tokens);

#endif

/*
Program       → StatementList
StatementList → Statement StatementList | ε
Statement     → IDENTIFIER = Expression ;
Expression    → Term ExpressionTail
ExpressionTail→ + Term ExpressionTail | - Term ExpressionTail | ε
Term          → Factor TermTail
TermTail      → *Factor TermTa il | / Factor TermTail | ε
Factor        → NUMBER | IDENTIFIER | ( Expression )
*/