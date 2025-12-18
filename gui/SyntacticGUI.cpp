#include "SyntacticGUI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <cctype>
#include <algorithm>

#include "PDAView.h"

using namespace std;


extern vector<Token> simpleTokenize(const QString& input); 

SyntacticVisualizer::SyntacticVisualizer(QWidget *parent)
    : QWidget(parent), parser(nullptr), traversalIndex(0), currentInputPos(0) {

    traversalTimer = new QTimer(this);   
    traversalTimer->setInterval(400);   

    setupUI();
    setupConnections();
    //updateGrammarTable();
    clearState();
}


void SyntacticVisualizer::receiveTokens(const vector<Token>& tokens, const QString& rawInput) {
    clearState(); 
    
    currentTokens = tokens; // Store the new tokens
    currentInputString = rawInput; 
    inputDisplay->setText(rawInput);

    tokensTableWidget->setRowCount(tokens.size());
    for (int i = 0; i < tokens.size(); ++i) {
        tokensTableWidget->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(getTokenName(tokens[i].type))));
        tokensTableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(tokens[i].value)));
    }

    parseButton->setEnabled(true); 
}


void SyntacticVisualizer::setupUI() {
    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    QSplitter* mainHorizontalSplitter = new QSplitter(Qt::Horizontal);

    // ================= LEFT COLUMN =================
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    
    // input box
    QLabel* inputLabel = new QLabel("<b>Input String:<b>");
    inputDisplay = new QTextEdit();
    inputDisplay->setReadOnly(true);
    inputDisplay->setMaximumHeight(100);

    // token table    
    QLabel* tokenLabel = new QLabel("<b>Token Table:<b>");
    tokensTableWidget = new QTableWidget(0, 2);
    tokensTableWidget->setHorizontalHeaderLabels({"Token", "Value"});
    tokensTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    leftLayout->addWidget(inputLabel);
    leftLayout->addWidget(inputDisplay);
    leftLayout->addWidget(tokenLabel);
    leftLayout->addWidget(tokensTableWidget);
 
   // ================= RIGHT COLUMN =================
    QSplitter* rightVerticalSplitter = new QSplitter(Qt::Vertical);
    QWidget* topWidget = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout(topWidget);
    
    // 1. grammar 
    QLabel* PDAlabel = new QLabel("<b>Context-Free Grammar:<b>");
    pdaDiagramView = new PDAVisualizer(this);
    pdaDiagramView->setMinimumHeight(100);


     // Control Buttons (Add at the bottom of the main layout)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    parseButton = new QPushButton("Parse");
    playPauseButton = new QPushButton("Play/Pause");
    resetButton = new QPushButton("Reset");
    buttonLayout->addWidget(parseButton);
    buttonLayout->addWidget(playPauseButton);
    buttonLayout->addWidget(resetButton);
    
    topLayout->addWidget(PDAlabel);
    topLayout->addWidget(pdaDiagramView);
    topLayout->addLayout(buttonLayout);

    // 2. Bottom Section (Stack and Trace)
    QWidget* bottomWidget = new QWidget();
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomWidget);

    // Left: PDA Stack
    QWidget* stackBox = new QWidget();
    QVBoxLayout* stackVBox = new QVBoxLayout(stackBox);
    stackVBox->addWidget(new QLabel("<b>PDA Stack:</b>"), 0, Qt::AlignCenter);
    stackWidget = new QListWidget();
    stackVBox->addWidget(stackWidget);

    // Right: Parsing Trace
    QWidget* traceBox = new QWidget();
    QVBoxLayout* traceVBox = new QVBoxLayout(traceBox);
    traceVBox->addWidget(new QLabel("<b>Parsing Trace:<b>"), 0, Qt::AlignCenter);
    traceTableWidget = new QTableWidget(0, 3);
    traceTableWidget->setHorizontalHeaderLabels({"Stack", "Input", "Action"});
    traceTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    traceVBox->addWidget(traceTableWidget);
    
    bottomLayout->addWidget(stackBox, 1); // Left 1/2 of bottom
    bottomLayout->addWidget(traceBox, 2); // Right 1/2 of bottom

    // Add sections to right splitter
    rightVerticalSplitter->addWidget(topWidget);
    rightVerticalSplitter->addWidget(bottomWidget);
    rightVerticalSplitter->setStretchFactor(0, 2);
    rightVerticalSplitter->setStretchFactor(1, 2);
    
 // ================= FINALIZE =================
    mainHorizontalSplitter->addWidget(leftWidget);
    mainHorizontalSplitter->addWidget(rightVerticalSplitter);
    mainHorizontalSplitter->setStretchFactor(1, 2);



    rootLayout->addWidget(mainHorizontalSplitter);
}


void SyntacticVisualizer::setupConnections() {
    connect(parseButton, &QPushButton::clicked, this, &SyntacticVisualizer::parseClicked);
    connect(playPauseButton, &QPushButton::clicked, this, &SyntacticVisualizer::playPauseClicked);
    connect(resetButton, &QPushButton::clicked, this, &SyntacticVisualizer::resetClicked);
    connect(traversalTimer, &QTimer::timeout, this, &SyntacticVisualizer::autoTraverse);
}


void SyntacticVisualizer::clearState() {
    if (parser) { 
        delete parser; 
        parser = nullptr; 
    }

    trace.clear();
    traversalIndex = 0;
    currentInputPos = 0;
    pendingErrorMessage = ""; 
    

    traceTableWidget->setRowCount(0);
    stackWidget->clear();

    QListWidgetItem* item = new QListWidgetItem("$");
    item->setForeground(Qt::red);
    item->setFont(QFont("", -1, QFont::Bold));
    item->setTextAlignment(Qt::AlignCenter); 
    stackWidget->addItem(item);

    parseButton->setEnabled(true);
    playPauseButton->setEnabled(false);
    playPauseButton->setText("Play");
}


void SyntacticVisualizer::inputTextChanged() {
    clearState();
}


void SyntacticVisualizer::parseClicked() {
    if (currentTokens.empty()) {
        QMessageBox::warning(this, "No Input", "Please run Lexical Analysis first.");
        return;
    }

    // Prepare tokens
    vector<Token> tokensToParse = currentTokens;
    if (tokensToParse.empty() || tokensToParse.back().value != "$") {
        tokensToParse.push_back({UNKNOWN, "$"}); 
    }

    // Reset UI state
    if (traversalTimer) traversalTimer->stop();
    if (parser) { delete parser; parser = nullptr; }
    trace.clear();
    traversalIndex = 0;
    pendingErrorMessage = ""; // Reset error message
    traceTableWidget->setRowCount(0);
    stackWidget->clear();

    parser = new Parser(tokensToParse);
    
    try {
        parser->parse(); 
    } catch (const exception& e) {
        pendingErrorMessage = QString::fromStdString(e.what());
    }

    trace = parser->getTrace();
    
    if (!trace.empty()) {
        parseButton->setEnabled(false);
        playPauseButton->setEnabled(true);
        playPauseButton->setText("Pause");
        traversalTimer->start();
    } else if (!pendingErrorMessage.isEmpty()) {
        QMessageBox::critical(this, "Syntactic Error", pendingErrorMessage);
    }
}


void SyntacticVisualizer::playPauseClicked() {
    if (traversalTimer->isActive()) {
        traversalTimer->stop();
        playPauseButton->setText("Play");
    } else {
        if (traversalIndex >= trace.size()) {
             resetClicked();
             return;
        }
        traversalTimer->start();
        playPauseButton->setText("Pause");
    }
}

void SyntacticVisualizer::resetClicked() {
    if (traversalTimer) traversalTimer->stop(); 
    clearState();
}



void SyntacticVisualizer::autoTraverse() {
    if (traversalIndex >= trace.size()) {
        traversalTimer->stop();
        playPauseButton->setText("Play");
        playPauseButton->setEnabled(false);

        if (!pendingErrorMessage.isEmpty()) {
            QMessageBox::critical(this, "Syntactic Error", 
                "The parser stopped due to a syntax error:\n\n" + pendingErrorMessage);
        } else {
            QMessageBox::information(this, "Success", "Parsing completed successfully!");
        }
        return;
    }

    const PDAAction& step = trace[traversalIndex];

    /*// --- Update the PDA Diagram ---
    QString state = "q1"; 
    if (traversalIndex == 0) state = "q0"; 
    

    if (step.action.find("ACCEPT") != string::npos) {
        state = "q2";
    }

    pdaDiagramView->stepAnimation(
        QString::fromStdString(step.action), 
        step.stack.empty() ? "" : QString::fromStdString(step.stack.back()), 
        QString::fromStdString(step.currentToken.value)
    );
    
    pdaDiagramView->updateVisualization(
        state, 
        QString::fromStdString(step.currentToken.value),
        step.stack.empty() ? "" : QString::fromStdString(step.stack.back()),
        QString::fromStdString(step.action)
    );*/


    //stack table
    stackWidget->clear();
    for (auto it = step.stack.rbegin(); it != step.stack.rend(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(*it));
        item->setTextAlignment(Qt::AlignCenter);

        if (*it == "$") {
            item->setForeground(Qt::red);
            item->setTextAlignment(Qt::AlignCenter); 
            item->setFont(QFont("", -1, QFont::Bold));
        }
        stackWidget->addItem(item);
    }

    int row = traceTableWidget->rowCount();
    traceTableWidget->insertRow(row);
    QString fullStackStr;
    for (const string& s : step.stack) fullStackStr += QString::fromStdString(s) + " ";
    
    QTableWidgetItem* itemStack = new QTableWidgetItem(fullStackStr.trimmed());
    QTableWidgetItem* itemInput = new QTableWidgetItem(QString::fromStdString(step.currentToken.value));
    QTableWidgetItem* itemAction = new QTableWidgetItem(QString::fromStdString(step.action));

    // Center all text
    itemStack->setTextAlignment(Qt::AlignCenter);
    itemInput->setTextAlignment(Qt::AlignCenter);
    itemAction->setTextAlignment(Qt::AlignCenter);

    // --- APPLY ACTION COLORS ---
    QString actionText = QString::fromStdString(step.action).toLower();
    
    if (actionText.contains("match") || actionText.contains("accept")) {
        itemAction->setForeground(Qt::darkGreen);
    } 
    else if (actionText.contains("push")) {
        itemAction->setForeground(Qt::blue);    
    } 
    else if (actionText.contains("pop")) {
        itemAction->setForeground(QColor(200, 150, 0)); 
    }

    traceTableWidget->setItem(row, 0, itemStack);
    traceTableWidget->setItem(row, 1, itemInput);
    traceTableWidget->setItem(row, 2, itemAction);
    
    traceTableWidget->scrollToBottom();
    traversalIndex++;
}


void SyntacticVisualizer::updateStackDisplay(const vector<string>& stack) {
    stackWidget->clear();
    for (int i = stack.size() - 1; i >= 0; --i) {
        stackWidget->addItem(QString::fromStdString(stack[i]));
    }
}

void SyntacticVisualizer::updateTraceTable(const vector<PDAAction>& traceData) {
    traceTableWidget->setRowCount(traceData.size());
    for (int i = 0; i < traceData.size(); ++i) {
        const PDAAction& action = traceData[i];
        traceTableWidget->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(action.action)));
        traceTableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(action.currentToken.value)));
        QString stackStr;
        for (auto& s : action.stack) stackStr += QString::fromStdString(s) + " ";
        traceTableWidget->setItem(i, 2, new QTableWidgetItem(stackStr.trimmed()));
    }
}

void SyntacticVisualizer::updateGrammarTable() {
    grammarTableWidget->setRowCount(0);
    QMap<QString, QString> grammar = {
        {"S", "StmtList"},
        {"StmtList", "Stmt StmtList | ε"},
        {"Stmt", "AssignStmt SEMICOLON | ExprStmt SEMICOLON"},
        {"AssignStmt", "IDENTIFIER ASSIGN Expr"},
        {"ExprStmt", "Expr"},
        {"Expr", "Term ExprPrime"},
        {"ExprPrime", "PLUS Term ExprPrime | MINUS Term ExprPrime | ε"},
        {"Term", "Factor TermPrime"},
        {"TermPrime", "MULTIPLY Factor TermPrime | DIVIDE Factor TermPrime | ε"},
        {"Factor", "NUMBER | IDENTIFIER | FuncCall | LPAREN Expr RPAREN"},
        {"FuncCall", "IDENTIFIER LPAREN Expr RPAREN"}
    };
    int row = 0;
    for (auto it = grammar.begin(); it != grammar.end(); ++it, ++row) {
        grammarTableWidget->insertRow(row);
        grammarTableWidget->setItem(row, 0, new QTableWidgetItem(it.key()));
        grammarTableWidget->setItem(row, 1, new QTableWidgetItem(it.value()));
    }
}
