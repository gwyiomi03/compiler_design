#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "syntactic.h"

Token previousToken(UNKNOWN, "", 1);
bool hasPrevious = false;

Parser::Parser(const std::vector<Token>& t) : tokens(t), pos(0) {
    setupTable(); 
}


std::string getFriendlyName(TokenType type) {
    switch (type) {
        case LPAREN:    return "(";
        case RPAREN:    return ")";
        case PLUS:      return "+";
        case MINUS:     return "-";
        case MULTIPLY:  return "*";
        case DIVIDE:    return "/";
        case ASSIGN:    return "=";
        case NUMBER:    return "a number";
        case IDENTIFIER:return "an identifier";
        default:        return "unknown token";
    }
}

void Parser::setupTable() {
    // --- S -> StmtList ---
    parsingTable["S"]["IDENTIFIER"] = {"StmtList"};
    parsingTable["S"]["print"]      = {"StmtList"};
    parsingTable["S"]["$"]          = {"StmtList"};

    // --- StmtList -> Stmt StmtList | ε ---
    parsingTable["StmtList"]["IDENTIFIER"] = {"Stmt", "StmtList"};
    parsingTable["StmtList"]["print"]      = {"Stmt", "StmtList"};
    parsingTable["StmtList"]["$"]          = {}; // ε 

    // --- Stmt -> AssignStmt | PrintStmt ---
    parsingTable["Stmt"]["IDENTIFIER"] = {"AssignStmt"};
    parsingTable["Stmt"]["print"]      = {"PrintStmt"};

    // --- AssignStmt -> IDENTIFIER = Expr ---
    parsingTable["AssignStmt"]["IDENTIFIER"] = {"IDENTIFIER", "=", "Expr"};

    // --- PrintStmt -> print ( Expr ) ---
    parsingTable["PrintStmt"]["print"] = {"print", "(", "Expr", ")"};

    // --- Expr -> Term ExprPrime ---
    for (auto k : {"NUMBER", "IDENTIFIER", "FUNCTION", "("}) 
        parsingTable["Expr"][k] = {"Term", "ExprPrime"};

    // --- ExprPrime -> + Term ExprPrime | - Term ExprPrime | ε ---
    parsingTable["ExprPrime"]["+"] = {"+", "Term", "ExprPrime"};
    parsingTable["ExprPrime"]["-"] = {"-", "Term", "ExprPrime"};
    for (auto k : {")", "$", "print", "IDENTIFIER"}) {
        parsingTable["ExprPrime"][k] = {}; // ε rule
    }

    // --- Term -> Factor TermPrime ---
    for (auto k : {"NUMBER", "IDENTIFIER", "FUNCTION", "("}) 
        parsingTable["Term"][k] = {"Factor", "TermPrime"};

    // --- TermPrime -> * Factor TermPrime | / Factor TermPrime | ε ---
    parsingTable["TermPrime"]["*"] = {"*", "Factor", "TermPrime"};
    parsingTable["TermPrime"]["/"] = {"/", "Factor", "TermPrime"};
    
    // Epsilon transitions for TermPrime (Follow Set)
    // Term is finished if we see + or - (ExprPrime) or a new statement
    for (auto k : {"+", "-", ")", "$", "print", "IDENTIFIER"}) {
        parsingTable["TermPrime"][k] = {}; // ε rule
    }

    // --- Factor -> NUMBER | IDENTIFIER | FUNCTION ( Expr ) | ( Expr ) ---
    parsingTable["Factor"]["NUMBER"]     = {"NUMBER"};
    parsingTable["Factor"]["IDENTIFIER"] = {"IDENTIFIER"};
    parsingTable["Factor"]["FUNCTION"]   = {"FUNCTION", "(", "Expr", ")"};
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
    // 1. First, pop the current non-terminal
    stack.pop_back();
    trace.push_back({stack, peek(), "pop " + nonTerminal});

    // 2. If the production is epsilon (empty), no need to push anything
    if (production.empty()) {
        trace.push_back({stack, peek(), "ε"});
    } else {
        // Push in reverse order for LL(1)
        for (auto it = production.rbegin(); it != production.rend(); ++it) {
            stack.push_back(*it);
            trace.push_back({stack, peek(), "push " + *it});
        }
    }
}


void Parser::match(const string& expectedTerminal) {
    Token t = peek();
    string actual = getLookaheadKey(t);
    

    if (actual == expectedTerminal) {
        previousToken = t;
        hasPrevious = true;

        trace.push_back({stack, t, "match " + t.value}); 
        stack.pop_back(); 
        if (expectedTerminal != "$") pos++; 
    } else {
        // Matches your previous error format
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

    stack.push_back("$");
    trace.push_back({stack, peek(), "push $"});

    stack.push_back("S");
    trace.push_back({stack, peek(), "push S"});
 
    Push_pop("S", parsingTable["S"][getLookaheadKey(peek())]);

    try {
        while (!stack.empty()) {
            string top = stack.back();
            Token lookahead = peek();
            string key = getLookaheadKey(lookahead);

            bool isTerminal = (top == "IDENTIFIER" || top == "NUMBER" || top == "FUNCTION" || 
                               top == "print" || top == "(" || top == ")" || 
                               top == "+" || top == "-" || top == "*" || top == "/" || 
                               top == "=" || top == "$");
            
            // Special case for final acceptance
            if (top == "$" && key == "$") {
                match(top);
                trace.push_back({stack, peek(), "ACCEPTED"}); 
                break;
            }

            if (isTerminal) {
                match(top);
                previousToken = lookahead;
            } 
            else if (parsingTable.count(top)) {
                if (parsingTable[top].count(key)) {
                    Push_pop(top, parsingTable[top][key]);
                } else {
                    string errorMsg;
                    if (key == "$") {
                        errorMsg = "Syntax Error: Incomplete expression. Expected content for '" + top + 
                                "' at the end of line " + std::to_string(previousToken.line);
                    } else {
                        errorMsg = "Syntax Error: Expected '" + top + 
                                "' at line " + std::to_string(lookahead.line);
                    }
                    throw std::runtime_error(errorMsg);
                }
            } else {
                throw std::runtime_error("Critical Error: Unknown grammar symbol: " + top);
            }
        }
    } catch (const std::runtime_error& e) {
        //trace.push_back({stack, peek(), "ERROR: " + string(e.what())});
        throw;
    }
}


string Parser::getLookaheadKey(Token t) {
    if (t.value == "$") return "$";
    
    // Check for specific keywords first
    if (t.type == PRINT && t.value == "print") return "print";
    
    // Map token types to the string labels used in your LL(1) Table
    switch (t.type) {
        case IDENTIFIER: return "IDENTIFIER";
        case NUMBER:     return "NUMBER";
        case FUNCTION:   return "FUNCTION";
        case PRINT:   return "PRINT";
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