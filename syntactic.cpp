#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "syntactic.h"

Token previousToken(UNKNOWN, "", 1);
bool hasPrevious = false;

Parser::Parser(const std::vector<Token>& t) : tokens(t), pos(0) {
    setupTable(); 
}



void Parser::setupTable() {
    for (auto k : {"IDENTIFIER", "print"}) 
        parsingTable["S"][k] = {"Stmt", "S"};
    parsingTable["S"]["$"] = {}; 

    for (auto k : {"IDENTIFIER", "print"}) 
        parsingTable["S"][k] = {"Stmt", "S"};
    parsingTable["S"]["$"] = {}; 

    parsingTable["Stmt"]["IDENTIFIER"] = {"IDENTIFIER", "=", "Expr"};
    parsingTable["Stmt"]["print"]      = {"print", "(", "Expr", ")"};

    for (auto k : {"NUMBER", "IDENTIFIER", "FUNCTION", "("}) 
        parsingTable["Expr"][k] = {"Term", "Expr'"};

    parsingTable["Expr'"]["+"] = {"+", "Term", "Expr'"};
    parsingTable["Expr'"]["-"] = {"-", "Term", "Expr'"};
    for (auto k : {")", "$", "print", "IDENTIFIER"}) parsingTable["Expr'"][k] = {}; 

    for (auto k : {"NUMBER", "IDENTIFIER", "FUNCTION", "("}) 
        parsingTable["Term"][k] = {"Factor", "Term'"};

    parsingTable["Term'"]["*"] = {"*", "Factor", "Term'"};
    parsingTable["Term'"]["/"] = {"/", "Factor", "Term'"};
    parsingTable["Term'"]["%"] = {"%", "Factor", "Term'"};
    for (auto k : {"+", "-", ")", "$", "print", "IDENTIFIER"}) parsingTable["Term'"][k] = {};

    parsingTable["Factor"]["NUMBER"]     = {"NUMBER"};
    parsingTable["Factor"]["IDENTIFIER"] = {"IDENTIFIER"};
    parsingTable["Factor"]["FUNCTION"] = {"FUNCTION", "(", "Expr", ")"};
    parsingTable["Factor"]["("]          = {"(", "Expr", ")"};
    
}

Token Parser::peek() {
    if (pos < tokens.size()) {
        const Token& current = tokens[pos];
        if (tokens[pos].type == UNKNOWN && tokens[pos].value != "$") {
            throw std::runtime_error("Syntax Error: Unknown token '" + tokens[pos].value + "'" + " at line " + std::to_string(current.line));
        }
        return tokens[pos];
    }
    int eofLine = tokens.empty() ? 1 : tokens.back().line;
    return Token{ UNKNOWN, "$", eofLine };
}


const vector<PDAAction>& Parser::getTrace() const { return trace; }


void Parser::Push_pop(const string& nonTerminal, const vector<string>& production) {
    string rhsStr = "";
    if (production.empty()) {
        rhsStr = "ε";
    } else {
        for (size_t i = 0; i < production.size(); ++i) {
            rhsStr += production[i] + (i < production.size() - 1 ? " " : "");
        }
    }
    
    string actionLabel = "Expand " + nonTerminal + " → " + rhsStr;
    trace.push_back({stack, peek(), actionLabel});
    stack.pop_back(); 

    if (!production.empty()) {
        for (auto it = production.rbegin(); it != production.rend(); ++it) {
            stack.push_back(*it);
        }
    }

}

void Parser::match(const string& expectedTerminal) {
    Token t = peek();
    string actual = getLookaheadKey(t);

    if (actual == expectedTerminal) {
        string actionLabel = "match " + actual + " → pop";
        
        stack.pop_back(); 
        trace.push_back({stack, t, actionLabel});
        
        if (expectedTerminal != "$") pos++; 
    } else {
        throw std::runtime_error(
            "Syntax Error: Expected " + expectedTerminal + 
            " at line " + std::to_string(previousToken.line)
        );
    }
}

// --- Grammar implementation ---
void Parser::parse() {
    trace.clear();
    stack.clear();
    pos = 0;

    // Initial sequence
    stack.push_back("$");
    trace.push_back({stack, peek(), "push $"});
    stack.push_back("S");
    trace.push_back({stack, peek(), "push S"});

    try {
        while (!stack.empty()) {
            string top = stack.back();
            string key = getLookaheadKey(peek());

            // Check if top is a terminal
            bool isTerminal = (top == "IDENTIFIER" || top == "NUMBER" || top == "=" || 
                               top == "+" || top == "-" || top == "*" || top == "/" || top == "FUNCTION" ||
                               top == "%" || top == "(" || top == ")" || top == "print" || top == "$");

            if (top == "$" && key == "$") {
                match("$");
                trace.push_back({stack, peek(), "ACCEPTED"});
                break;
            }

            if (isTerminal) {
                if (top == "FUNCTION" && peek().type == FUNCTION) {
                    match("FUNCTION");
                } else if (top == key) {
                    match(top);
                } else {
                    throw std::runtime_error("Syntax Error: Expected " + top);
                }
            } else if (parsingTable.count(top) && parsingTable[top].count(key)) {
                Push_pop(top, parsingTable[top][key]);
            } else {
                throw std::runtime_error("Syntax Error at " + key);
            }
        }
    } catch (const std::runtime_error& e) {
        //trace.push_back({stack, peek(), "ERROR: " + string(e.what())});
        throw;
    }
}


string Parser::getLookaheadKey(Token t) {
    if (t.value == "$") return "$";
    if (t.value == "%") return "%";
    
    // Check for specific keywords first
    if (t.type == PRINT && t.value == "print") return "print";
    
    // Map token types to the string labels used in your LL(1) Table
    switch (t.type) {
        case IDENTIFIER: return "IDENTIFIER";
        case NUMBER:     return "NUMBER";
        case FUNCTION:   return "FUNCTION";
        case PRINT:      return "print";
        case MOD:        return "%";
        case PLUS:       return "+";
        case MINUS:      return "-";
        case MULTIPLY:   return "*";
        case DIVIDE:     return "/";
        case LPAREN:     return "(";
        case RPAREN:     return ")";
        case ASSIGN:     return "=";
        default:         return t.value; // Fallback for raw symbols
    }
}