#include <QApplication>
#include "gui/mainwindow.h"
#include <iostream>
#include "lexical.h"
#include "parser.h"


int main(int argc, char *argv[]) {

    QApplication app(argc, argv);

    /* // --- Build DFAs ---
    auto idNFA = createIdentifierNFA();
    auto numNFA = createNumberNFA();
    auto plusNFA = createSingleCharNFA('+', PLUS, 10);
    auto minusNFA = createSingleCharNFA('-', MINUS, 12);
    auto mulNFA = createSingleCharNFA('*', MULTIPLY, 14);
    auto divNFA = createSingleCharNFA('/', DIVIDE, 16);
    auto assignNFA = createSingleCharNFA('=', ASSIGN, 18);
    auto leftNFA = createSingleCharNFA('(', LPAREN, 20);
    auto rightNFA = createSingleCharNFA(')', RPAREN, 22);
    auto semiNFA = createSingleCharNFA(';', SEMICOLON, 24);

    NFA combined = combineNFAs({
        idNFA, numNFA, plusNFA, minusNFA, mulNFA,
        divNFA, assignNFA, leftNFA, rightNFA, semiNFA
    });

    DFA dfa = convertNFAtoDFA(combined);

    // --- Input ---
    string input = "y_x12 = 1 + (2 * y) + 3; y = 3.5 / 2;";

    cout << "=== LEXICAL TOKENS ===\n";
    vector<Token> tokens = scanInput(dfa, input);

    cout << "\n=== START PARSING (PDA TRACE) ===\n";
    ParseNode* tree = parse(tokens);

    if (tree)
        cout << "\n=== PARSE TREE GENERATED (PASS TO GUI) ===\n";
    else
        cout << "\n=== SYNTAX ERROR ===\n";

    return 0; */

    MainWindow w;
    w.show();

    return app.exec();
}



