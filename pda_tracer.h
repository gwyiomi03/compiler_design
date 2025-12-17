#pragma once
#include <vector>
#include <string>
#include "lexical.h"

using namespace std;

struct StackFrame {
    string symbol;    // Nonterminal or terminal
    int tokenIndex;        // Input token position
};

struct PDAAction {
    vector<string> stack;
    Token currentToken;
    string action;    // e.g., "push Expr", "match NUMBER", "pop Factor"
};
