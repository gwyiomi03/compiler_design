#include "syntactic.h"
#include <iostream>
#include <stdexcept>

Parser::Parser(const std::vector<Token>& t) : tokens(t), pos(0) {}


Token Parser::peek() {
    if (pos < tokens.size()) {
        // Allow the $ marker even if it's UNKNOWN
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

    // Check for unknown tokens, but ignore the $ sentinel
    if (t.type == UNKNOWN && t.value != "$") {
        throw std::runtime_error("Syntax Error: Unknown token '" + t.value + "'");
    }

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
    stack.push_back("$"); // Initial stack marker
    
    parseS();

    // Consume the marker if it's the current token
    if (pos < tokens.size() && peek().value == "$") {
        consume(); 
        trace.push_back({stack, Token{UNKNOWN, "$"}, "ACCEPT: input valid"});
    }
}

const std::vector<PDAAction>& Parser::getTrace() const { return trace; }


void Parser::parseS() {
    push("S");
    parseStmtList();
    pop("S");
}


void Parser::parseStmtList() {
    push("StmtList");
    while (peek().value != "$") {
        int lastPos = pos;
        parseStmt();

        if (pos == lastPos) {
            throw std::runtime_error("Parser stuck at token: " + peek().value);
        }
    }
    pop("StmtList");
}
void Parser::parseStmt() {
    push("Stmt");

    expectNotUnknown();  

    if (peek().type == IDENTIFIER && (pos + 1 < tokens.size()) && tokens[pos+1].type == ASSIGN) {
        parseAssignStmt();
    } else {
        parseExprStmt();
    }

    // Semicolon is mandatory
    if (peek().type == SEMICOLON) {
        match(SEMICOLON);
    } else {
        throw std::runtime_error("Syntax Error: Missing semicolon at token '" + peek().value + "'");
    }

    pop("Stmt");
}


void Parser::parseExprStmt() {
    push("ExprStmt");
    parseExpr();
    pop("ExprStmt");
}




void Parser::parseExpr() {
    push("Expr");
    parseTerm();
    parseExprPrime();
    pop("Expr");
}

void Parser::parseAssignStmt() {
    push("AssignStmt");
    match(IDENTIFIER); // Use match for better trace recording
    match(ASSIGN);
    parseExpr();
    pop("AssignStmt");
}




void Parser::parseExprPrime() {
    push("ExprPrime");
    TokenType t = peek().type;
    if (t == PLUS || t == MINUS) {
        consume(); // Match PLUS/MINUS
        parseTerm();
        parseExprPrime();
    }
    pop("ExprPrime"); // Epsilon case
}

void Parser::parseTerm() {
    push("Term");
    parseFactor();
    parseTermPrime();
    pop("Term");
}


void Parser::parseTermPrime() {
    push("TermPrime");
    TokenType t = peek().type;
    if (t == MULTIPLY || t == DIVIDE) {
        consume();
        parseFactor();
        parseTermPrime();
    }
    pop("TermPrime");
}


void Parser::parseFactor() {
    push("Factor");

    Token t = peek();

    if (t.type == NUMBER) {
        consume();
    }
    else if (t.type == IDENTIFIER) {
        consume();

        if (pos < tokens.size() && peek().type == LPAREN) {
            match(LPAREN);
            parseExpr();
            match(RPAREN);
        }
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

void Parser::parseFuncCall() {
    push("FuncCall");
    consume(); // IDENTIFIER
    match(LPAREN);
    parseExpr();
    match(RPAREN);
    pop("FuncCall");
}
