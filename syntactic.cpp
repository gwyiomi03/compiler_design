#include "syntactic.h"
#include <iostream>
#include <stdexcept>

Parser::Parser(const std::vector<Token>& t) : tokens(t), pos(0) {}

Token Parser::peek() {
    if (pos < tokens.size()) {
        if (tokens[pos].type == UNKNOWN && tokens[pos].value != "$") {
            throw std::runtime_error("Syntax Error: Unknown token '" + tokens[pos].value + "'");
        }
        return tokens[pos];
    }
    return Token{UNKNOWN, "$"}; 
}

Token Parser::consume() {
    Token t = peek();
    pos++;
    trace.push_back({stack, t, "match " + t.value}); 
    return t;
}

void Parser::push(const std::string& symbol) {
    stack.push_back(symbol);
    trace.push_back({stack, peek(), "push " + symbol});
}

void Parser::pop(const std::string& symbol) {
    trace.push_back({stack, peek(), "pop " + symbol});
    stack.pop_back();
}

void Parser::match(TokenType expected) {
    Token t = peek();
    if (t.type == expected) {
        consume();
    } else {
        throw std::runtime_error(
            "Expected " + getTokenName(expected) + " but got '" + t.value + "'"
        );
    }
}

void Parser::expectNotUnknown() {
    if (peek().type == UNKNOWN && peek().value != "$") {
        throw std::runtime_error("Syntax Error: Unknown token '" + peek().value + "'");
    }
}

// --- Grammar implementation ---
void Parser::parse() {
    stack.clear();
    stack.push_back("$"); 
    parseS();

    if (pos < tokens.size() && peek().value == "$") {
        consume(); 
        trace.push_back({stack, Token{UNKNOWN, "$"}, "ACCEPT: input valid"});
    }
}

const std::vector<PDAAction>& Parser::getTrace() const { return trace; }

// S → StmtList
void Parser::parseS() {
    push("S");
    parseStmtList();
    pop("S");
}

// StmtList → Stmt StmtList | ε
void Parser::parseStmtList() {
    push("StmtList");
    Token t = peek();
    while (t.type != UNKNOWN || t.value != "$") {
        // Stop if FIRST(StmtList) not in {IDENTIFIER, NUMBER, LPAREN}
        if (t.type != IDENTIFIER && t.type != NUMBER && t.type != LPAREN) break;
        parseStmt();
        t = peek();
    }
    pop("StmtList"); // ε case
}

// Stmt → AssignStmt SEMICOLON | ExprStmt SEMICOLON
void Parser::parseStmt() {
    push("Stmt");
    expectNotUnknown();

    Token t = peek();
    if (t.type == IDENTIFIER) {
        // LL(1) decision: if next token is ASSIGN → assignment
        if (pos + 1 < tokens.size() && tokens[pos + 1].type == ASSIGN) {
            parseAssignStmt();
        } else {
            parseExprStmt();
        }
    } else if (t.type == NUMBER || t.type == LPAREN) {
        parseExprStmt();
    } else {
        throw std::runtime_error("Syntax Error: Unexpected token '" + t.value + "' in Stmt");
    }

    // Semicolon mandatory
    if (peek().type == SEMICOLON) match(SEMICOLON);
    else throw std::runtime_error("Syntax Error: Missing semicolon after statement");

    pop("Stmt");
}

// AssignStmt → IDENTIFIER ASSIGN Expr
void Parser::parseAssignStmt() {
    push("AssignStmt");
    match(IDENTIFIER);
    match(ASSIGN);
    parseExpr();
    pop("AssignStmt");
}

// ExprStmt → Expr
void Parser::parseExprStmt() {
    push("ExprStmt");
    parseExpr();
    pop("ExprStmt");
}

// Expr → Term ExprPrime
void Parser::parseExpr() {
    push("Expr");
    parseTerm();
    parseExprPrime();
    pop("Expr");
}

// ExprPrime → PLUS Term ExprPrime | MINUS Term ExprPrime | ε
void Parser::parseExprPrime() {
    push("ExprPrime");
    Token t = peek();
    if (t.type == PLUS || t.type == MINUS) {
        consume();
        parseTerm();
        parseExprPrime();
    }
    // ε case: do nothing
    pop("ExprPrime");
}

// Term → Factor TermPrime
void Parser::parseTerm() {
    push("Term");
    parseFactor();
    parseTermPrime();
    pop("Term");
}

// TermPrime → MULT Factor TermPrime | DIV Factor TermPrime | ε
void Parser::parseTermPrime() {
    push("TermPrime");
    Token t = peek();
    if (t.type == MULTIPLY || t.type == DIVIDE) {
        consume();
        parseFactor();
        parseTermPrime();
    }
    // ε case: do nothing
    pop("TermPrime");
}

// Factor → IDENTIFIER FactorPrime | NUMBER | LPAREN Expr RPAREN
void Parser::parseFactor() {
    push("Factor");
    Token t = peek();

    if (t.type == NUMBER) {
        match(NUMBER);
    } 
    else if (t.type == IDENTIFIER) {
        match(IDENTIFIER);
        parseFactorPrime(); // LL(1)
    } 
    else if (t.type == LPAREN) {
        match(LPAREN);
        parseExpr();
        match(RPAREN);
    } 
    else {
        throw std::runtime_error(
            "Syntax Error: Expected NUMBER, IDENTIFIER, or '(' but got '" + t.value + "'"
        );
    }

    pop("Factor");
}

// FactorPrime → LPAREN Expr RPAREN | ε
void Parser::parseFactorPrime() {
    push("FactorPrime");
    Token t = peek();
    if (t.type == LPAREN) {
        match(LPAREN);
        parseExpr();
        match(RPAREN);
    } 
    pop("FactorPrime");
}
