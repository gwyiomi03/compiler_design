#pragma once
#include <vector>
#include <string>
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

    vector<string> stack;        // PDA stack
    vector<PDAAction> trace;          // PDA trace

    // PDA helpers
    void push(const string& symbol);
    void pop(const string& symbol);
    void expectNotUnknown();    

    // Grammar rules
    void parseS();
    void parseStmtList();
    void parseStmt();
    void parseAssignStmt();
    void parseExprStmt();
    void parseExpr();
    void parseExprPrime(); 
    void parseTerm();
    void parseTermPrime(); 
    void parseFactor();
    void parseFuncCall();

    Token peek();
    Token consume();
    void match(TokenType expected);
    void error(const string& msg);
};
