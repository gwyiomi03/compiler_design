#include "mainwindow.h"
#include "../lexical.h"
#include "../parser.h"
#include <sstream>
#include <iostream>
#include <QDebug>
#include <QTreeWidgetItem>

// Helper class to manage the temporary redirection of std::cout
// This ensures console prints (tokens, errors, etc.) are captured for the GUI
class ConsoleCapturer {
public:
    ConsoleCapturer(std::stringstream& ss) : ss_(ss) {
        // Save old buffer
        oldCoutStreamBuf_ = std::cout.rdbuf();
        // Redirect std::cout to the stringstream
        std::cout.rdbuf(ss_.rdbuf());
    }

    ~ConsoleCapturer() {
        // Restore old buffer (This is why prints still appear in the console)
        std::cout.rdbuf(oldCoutStreamBuf_);
    }
private:
    std::stringstream& ss_;
    std::streambuf* oldCoutStreamBuf_;
};


// --- Recursive function to populate the QTreeWidget ---
void displayParseTree(QTreeWidgetItem* parent, ParseNode* node) {
    if (!node) return;

    QString nodeText;
    if (node->lexeme.empty()) {
        // Non-terminal node: show symbol only
        nodeText = QString::fromStdString(node->symbol);
    } else {
        // Terminal node: show symbol (Token Type) and lexeme (Value)
        nodeText = QStringLiteral("%1: \"%2\"").arg(
            QString::fromStdString(node->symbol), 
            QString::fromStdString(node->lexeme)
        );
    }
    
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, nodeText);

    for (ParseNode* child : node->children) {
        displayParseTree(item, child);
    }
}


// --- Helper Function: Lexer Setup (Called only once) ---
DFA setupLexer() {
    // [Setup code remains the same: creating NFAs, combining, and converting to DFA]
    auto idNFA = createIdentifierNFA();
    auto numNFA = createNumberNFA();
    auto plusNFA = createSingleCharNFA('+', PLUS, 10);
    auto minusNFA = createSingleCharNFA('-', MINUS, 12);
    auto mulNFA = createSingleCharNFA('*', MULTIPLY, 14);
    auto divNFA = createSingleCharNFA('/', DIVIDE, 16);
    auto assignNFA = createSingleCharNFA('=', ASSIGN, 18);
    auto LeftParenNFA = createSingleCharNFA('(', LPAREN, 20);
    auto RightParenNFA = createSingleCharNFA(')', RPAREN, 21);
    auto SemiNFA = createSingleCharNFA(';', SEMICOLON, 22); 

    NFA combined = combineNFAs({
        idNFA, numNFA, plusNFA, minusNFA, mulNFA, divNFA, 
        assignNFA, LeftParenNFA, RightParenNFA, SemiNFA
    });
    
    return convertNFAtoDFA(combined);
}


// --- Core Analysis Logic ---
void performAnalysis(const QString& inputCode, QString& tokenOutputStr, QString& consoleOutputStr, QTreeWidget* parseTreeWidget) {
    static DFA dfa = setupLexer(); 
    string input = inputCode.toStdString();

    vector<Token> tokens;
    
    // --- 1. CAPTURE CONSOLE OUTPUT (Errors, PDA Trace, Token Debug) ---
    stringstream consoleStream;
    ConsoleCapturer capturer(consoleStream); // Starts redirecting std::cout
    
    // --- 2. LEXICAL ANALYSIS ---
    cout << "\n--- LEXICAL ANALYSIS START ---\n" << endl;
    tokens = scanInput(dfa, input); // Prints tokens to console
    cout << "\n--- LEXICAL ANALYSIS END ---\n" << endl;

    // --- 3. TOKEN FORMATTING FOR GUI DISPLAY ---
    stringstream tokenStream;
    for (const auto& tok : tokens) {
        // Format requested: (tokentype, string)
        tokenStream << "(" << getTokenName(tok.type) << ", \"" << tok.value << "\")\n";
    }
    /*// Add EOF token for parser compatibility
    Token eof = {UNKNOWN, "EOF", (int)input.size()};
    tokens.push_back(eof);
    tokenStream << "(" << getTokenName(eof.type) << ", \"" << eof.value << "\")\n";*/

    // --- 4. SYNTACTIC ANALYSIS (Parse Tree Building) ---
    cout << "\n--- SYNTACTIC ANALYSIS START (Parse Tree PDA) ---" << endl;
    
    // Call the parser function
    ParseNode* root = parse(tokens); // Prints errors/success to console
    
    cout << "\n--- SYNTACTIC ANALYSIS END ---\n" << endl;
    
    // --- 5. DISPLAY RESULTS IN GUI ---
    
    // Display Parse Tree
    parseTreeWidget->clear(); // Clear old tree
    if (root) {
        QTreeWidgetItem *rootItem = new QTreeWidgetItem(parseTreeWidget);
        rootItem->setText(0, "PROGRAM");
        displayParseTree(rootItem, root);
        parseTreeWidget->expandAll();
        
        // Cleanup memory for the generated tree
        // NOTE: In a production app, better memory management (e.g., smart pointers) 
        // should be used, but for this demo, delete the root.
        // QTreeWidget takes ownership of the top-level item, but we need to delete the ParseNode structure itself.
        // Since the ParseNode structure is simple and created with `new`, we need a cleanup function.
        
        // We'll define a quick cleanup function here for simplicity:
        function<void(ParseNode*)> deleteParseTree = 
            [&](ParseNode* node) {
                if (!node) return;
                for (auto child : node->children) deleteParseTree(child);
                delete node;
            };
        deleteParseTree(root);
        
    } else {
        QTreeWidgetItem *errorItem = new QTreeWidgetItem(parseTreeWidget);
        errorItem->setText(0, "SYNTAX ERROR: See Console Output");
        // The error message itself is captured in consoleStream.
    }
    
    // Restore cout happens automatically now.
    
    // Update GUI output strings
    tokenOutputStr = QString::fromStdString(tokenStream.str());
    consoleOutputStr = QString::fromStdString(consoleStream.str());
}


// --- MainWindow Implementation ---

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    setWindowTitle("Compiler Project - Lexical & Syntactic Analysis");
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 1. Input Area
    QLabel *inputLabel = new QLabel("Source Code Input:", centralWidget);
    codeInput = new QTextEdit(centralWidget);
    codeInput->setFont(QFont("Monospace", 10));
    codeInput->setPlaceholderText("Ex: x = 1 + (2 * y);\ny = 3.5 / 2;");

    analyzeButton = new QPushButton("Analyze Code", centralWidget);
    connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::analyzeButtonClicked);

    // 2. Output Area (Splitter: Tokens | Parse Tree | Console/Errors)
    QSplitter *outputSplitter = new QSplitter(Qt::Horizontal, centralWidget);

    // Left: Tokens
    QWidget *tokenWidget = new QWidget(outputSplitter);
    QVBoxLayout *tokenLayout = new QVBoxLayout(tokenWidget);
    QLabel *tokenLabel = new QLabel("Lexical Analysis (Tokens):", tokenWidget);
    tokenOutput = new QTextBrowser(tokenWidget);
    tokenOutput->setFont(QFont("Monospace", 10));
    tokenLayout->addWidget(tokenLabel);
    tokenLayout->addWidget(tokenOutput);

    // Middle: Parse Tree
    QWidget *treeWidget = new QWidget(outputSplitter);
    QVBoxLayout *treeLayout = new QVBoxLayout(treeWidget);
    QLabel *treeLabel = new QLabel("Syntactic Analysis (Parse Tree):", treeWidget);
    parseTreeOutput = new QTreeWidget(treeWidget);
    parseTreeOutput->setHeaderLabels({"Symbol/Token"}); // Single column header
    parseTreeOutput->setFont(QFont("Monospace", 9));
    treeLayout->addWidget(treeLabel);
    treeLayout->addWidget(parseTreeOutput);

    // Right: Console Prints / Errors
    QWidget *consoleWidget = new QWidget(outputSplitter);
    QVBoxLayout *consoleLayout = new QVBoxLayout(consoleWidget);
    QLabel *consoleLabel = new QLabel("Console Output (PDA Trace & Errors):", consoleWidget);
    consoleOutput = new QTextBrowser(consoleWidget);
    consoleOutput->setFont(QFont("Monospace", 9));
    consoleLayout->addWidget(consoleLabel);
    consoleLayout->addWidget(consoleOutput);

    // Add widgets to main layout
    mainLayout->addWidget(inputLabel);
    mainLayout->addWidget(codeInput);
    mainLayout->addWidget(analyzeButton);
    mainLayout->addWidget(outputSplitter, 1);
    
    // Set initial split sizes
    outputSplitter->setSizes({200, 300, 300});
    resize(1200, 800);
}

void MainWindow::analyzeButtonClicked()
{
    QString inputCode = codeInput->toPlainText();
    QString tokensResult;
    QString consoleResult;

    tokenOutput->clear();
    consoleOutput->clear();
    
    // The core logic is now in performAnalysis
    performAnalysis(inputCode, tokensResult, consoleResult, parseTreeOutput);

    // Display results in the GUI
    tokenOutput->setText(tokensResult);
    consoleOutput->setText(consoleResult);
}