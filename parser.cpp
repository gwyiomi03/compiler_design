#include "parser.h"
#include <stack> 
#include <iostream> 

using namespace std;

// Terminal string conversion
string tokenToString(TokenType type) {
    switch (type) {
        case IDENTIFIER: return "IDENTIFIER";
        case NUMBER: return "NUMBER";
        case PLUS: return "+";
        case MINUS: return "-";
        case MULTIPLY: return "*";
        case DIVIDE: return "/";
        case ASSIGN: return "=";
        case LPAREN: return "(";
        case RPAREN: return ")";
        case SEMICOLON: return ";";
        default: return "UNKNOWN";
    }
}

// ----------------------------
// PDA PARSER
// ----------------------------
struct StackItem {
    bool isTerminal;
    string symbol;
    ParseNode* node;
    
    StackItem(bool term, const string& sym, ParseNode* n)
        : isTerminal(term), symbol(sym), node(n) {}
};


ParseNode* makeNode(const string& sym) {
    return new ParseNode{sym, "", {}};
}

ParseNode* makeLeaf(const string& sym, const string& lex) {
    return new ParseNode{sym, lex, {}};
}


ParseNode* parse(const vector<Token>& tokens) {
    stack<StackItem> st;

    // Root of parse tree
    ParseNode* root = makeNode("Program");
    st.push({false, "Program", root});

    size_t pos = 0;

    while (!st.empty()) {
        StackItem top = st.top();
        st.pop();

        if (top.isTerminal) {
            if (pos >= tokens.size()) {
                cout << "Syntax Error: expected terminal " << top.symbol << endl;
                return nullptr;
            }

            string current = tokenToString(tokens[pos].type);

            if (current == top.symbol) {
                cout << "Matched terminal: " << current << " \"" << tokens[pos].value << "\"\n";
                top.node->lexeme = tokens[pos].value;
                pos++;
            } else {
                cout << "Syntax Error: expected " << top.symbol << ", found " << current << endl;
                return nullptr;
            }
        }
        else {
            // NON-TERMINALS
            cout << "Expanding <" << top.symbol << ">\n";

            // ---------- Program ----------
            if (top.symbol == "Program") {
                // Program -> StatementList
                ParseNode* SL = makeNode("StatementList");
                top.node->children.push_back(SL);

                st.push({false, "StatementList", SL});
            }

            // ---------- StatementList ----------
            else if (top.symbol == "StatementList") {
                if (pos < tokens.size()) {
                    // StatementList -> Statement StatementList
                    ParseNode* S = makeNode("Statement");
                    ParseNode* SL = makeNode("StatementList");

                    top.node->children.push_back(S);
                    top.node->children.push_back(SL);

                    st.push({false, "StatementList", SL});
                    st.push({false, "Statement", S});
                }
                else {
                    // epsilon
                }
            }

            // ---------- Statement ----------
            else if (top.symbol == "Statement") {
                // S → ID = E ;
                ParseNode *id = makeNode("IDENTIFIER");
                ParseNode *as = makeNode("=");
                ParseNode *E  = makeNode("Expression");
                ParseNode *sc = makeNode(";");

                top.node->children.push_back(id);
                top.node->children.push_back(as);
                top.node->children.push_back(E);
                top.node->children.push_back(sc);

                st.push({true, ";", sc});
                st.push({false, "Expression", E});
                st.push({true, "=", as});
                st.push({true, "IDENTIFIER", id});
            }

            // ---------- Expression ----------
            else if (top.symbol == "Expression") {
                ParseNode* T  = makeNode("Term");
                ParseNode* ET = makeNode("ExpressionTail");

                top.node->children.push_back(T);
                top.node->children.push_back(ET);

                st.push({false, "ExpressionTail", ET});
                st.push({false, "Term", T});
            }

            // ---------- ExpressionTail ----------
            else if (top.symbol == "ExpressionTail") {
                if (pos < tokens.size()) {
                    TokenType t = tokens[pos].type;

                    if (t == PLUS || t == MINUS) {
                        string op = tokenToString(t);
                        ParseNode* opN = makeNode(op);
                        ParseNode* T  = makeNode("Term");
                        ParseNode* ET = makeNode("ExpressionTail");

                        top.node->children.push_back(opN);
                        top.node->children.push_back(T);
                        top.node->children.push_back(ET);

                        st.push({false, "ExpressionTail", ET});
                        st.push({false, "Term", T});
                        st.push({true, op, opN});
                    }
                }
            }

            // ---------- Term ----------
            else if (top.symbol == "Term") {
                ParseNode* F = makeNode("Factor");
                ParseNode* TT = makeNode("TermTail");

                top.node->children.push_back(F);
                top.node->children.push_back(TT);

                st.push({false, "TermTail", TT});
                st.push({false, "Factor", F});
            }

            // ---------- TermTail ----------
            else if (top.symbol == "TermTail") {
                if (pos < tokens.size()) {
                    TokenType t = tokens[pos].type;
                    if (t == MULTIPLY || t == DIVIDE) {
                        string op = tokenToString(t);
                        ParseNode* opN = makeNode(op);
                        ParseNode* F = makeNode("Factor");
                        ParseNode* TT = makeNode("TermTail");

                        top.node->children.push_back(opN);
                        top.node->children.push_back(F);
                        top.node->children.push_back(TT);

                        st.push({false, "TermTail", TT});
                        st.push({false, "Factor", F});
                        st.push({true, op, opN});
                    }
                }
            }

            // ---------- Factor ----------
            else if (top.symbol == "Factor") {
                TokenType t = tokens[pos].type;

                if (t == NUMBER) {
                    ParseNode* N = makeNode("NUMBER");
                    top.node->children.push_back(N);
                    st.push({true, "NUMBER", N});
                }
                else if (t == IDENTIFIER) {
                    ParseNode* I = makeNode("IDENTIFIER");
                    top.node->children.push_back(I);
                    st.push({true, "IDENTIFIER", I});
                }
                else if (t == LPAREN) {
                    ParseNode* lp = makeNode("(");
                    ParseNode* E = makeNode("Expression");
                    ParseNode* rp = makeNode(")");

                    top.node->children.push_back(lp);
                    top.node->children.push_back(E);
                    top.node->children.push_back(rp);

                    st.push({true, ")", rp});
                    st.push({false, "Expression", E});
                    st.push({true, "(", lp});
                }
                else {
                    cout << "Syntax Error: unexpected token '" << tokens[pos].value << "' in Factor\n";
                    return nullptr;
                }
            }

        } // END non-terminal handling
    }

    if (pos == tokens.size()) {
        cout << "\nInput accepted by PDA!\n";
        return root;
    }
    else {
        cout << "Syntax Error: Extra input remaining.\n";
        return nullptr;
    }
}
