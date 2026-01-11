#ifndef SYNTACTIC_H
#define SYNTACTIC_H

#include <vector>
#include <string>
#include <map>
#include "lexical.h"
#include "pda_tracer.h"

using namespace std;

class Parser {
public:
    Parser(const vector<Token>& tokens);
    void parse();                          // Entry point (S)
    const vector<PDAAction>& getTrace() const;

private:
    vector<Token> tokens;
    size_t pos = 0;

    vector<string> stack; 
    vector<PDAAction> trace;     

    // PDA helpers
    void setupTable();  
    void match(const string& expectedTerminal);
    void Push_pop(const string& nonTerminal, const vector<string>& production);

    Token peek();
    string getLookaheadKey(Token t);
    map<string, map<string, vector<string>>> parsingTable;
};

#endif