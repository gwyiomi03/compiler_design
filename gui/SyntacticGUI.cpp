#include "SyntacticGUI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <cctype>
#include <algorithm>
#include <QSplitter>
#include <QApplication>
#include <QStyleFactory>
#include <QIcon>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

#include "PDAView.h"
#include "CodeEditor.h"


using namespace std;

extern vector<Token> simpleTokenize(const QString& input); 

SyntacticVisualizer::SyntacticVisualizer(QWidget *parent)
    : QWidget(parent), parser(nullptr), traversalIndex(0), currentInputPos(0) {

    // Apply a lively, modern stylesheet with vibrant colors and effects
    setStyleSheet(
    // Main Background
    "QWidget { font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; "
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #e3f2fdff, stop:1 #b8daf5ff); ""}" // Set default text to black

    // FORCE Labels to Black
    "QLabel {color: #000000;font-weight: bold; font-size: 11pt; background: transparent; }"
    
    // Buttons (Keep white text only for buttons)
    "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4caf50, stop:1 #388e3c); "
    "color: #ffffff; border: none; padding: 10px 20px; border-radius: 8px; font-weight: bold; font-size: 11pt; }"
    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66bb6a, stop:1 #4caf50); }"
    "QPushButton:disabled { background: #cccccc; color: #666666; }"

    // Input and Tables
    "QTextEdit { border: 2px solid #1f2020; border-radius: 8px; padding: 6px; background-color: #ffffff; color: #000000; }"
    "QTableWidget { border: 2px solid #2196f3; border-radius: 8px; background-color: #ffffff; color: #000000; }"
    "QHeaderView::section { background: #1976d2; color: #ffffff; font-weight: bold; }"
    
    // Stack and Lists
    "QListWidget { border: 2px solid #2196f3; border-radius: 8px; background-color: #ffffff; color: #000000; }"
    "QListWidget::item { color: #000000; }"
    );

    // Set a modern style and enable animations
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    setAttribute(Qt::WA_StyledBackground, true);

    // Add a subtle shadow effect to the widget
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(2, 2);
    setGraphicsEffect(shadow);

    traversalTimer = new QTimer(this);   
    traversalTimer->setInterval(400);   

    setupUI();
    setupConnections();
    clearState();
}

void SyntacticVisualizer::receiveTokens(const vector<Token>& tokens, const QString& rawInput) {
    clearState(); 
    
    currentTokens = tokens; 
    currentInputString = rawInput; 
    inputDisplay->setPlainText(rawInput);

    tokensTableWidget->setRowCount(tokens.size());
    for (int i = 0; i < tokens.size(); ++i) {
        QTableWidgetItem* tokenItem = new QTableWidgetItem(QString::fromStdString(getTokenName(tokens[i].type)));
        QTableWidgetItem* valueItem = new QTableWidgetItem(QString::fromStdString(tokens[i].value));
        
        // Add color coding for different token types
        if (tokens[i].type == IDENTIFIER) {
            tokenItem->setBackground(QColor(255, 235, 59, 100)); // Yellow for identifiers
        } else if (tokens[i].type == NUMBER) {
            tokenItem->setBackground(QColor(76, 175, 80, 100)); // Green for numbers
        } else if (tokens[i].type >= PLUS && tokens[i].type <= RPAREN) {
            tokenItem->setBackground(QColor(255, 87, 34, 100)); // Red-orange for operators
        }
        
        tokensTableWidget->setItem(i, 0, tokenItem);
        tokensTableWidget->setItem(i, 1, valueItem);
    }

    parseButton->setEnabled(true); 
}

void SyntacticVisualizer::setupUI() {
    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(15, 15, 15, 15);
    rootLayout->setSpacing(15);

    QSplitter* mainHorizontalSplitter = new QSplitter(Qt::Horizontal);

    // ================= LEFT COLUMN =================
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);
    
    // input box
    QLabel* inputLabel = new QLabel("Input String:");
    inputDisplay = new CodeEditor(this);
    inputDisplay->setReadOnly(true);
    inputDisplay->setMaximumHeight(120);

    inputDisplay->setStyleSheet(
        "CodeEditor {"
        "  background-color: white;"
        "  border: 2px solid #1E88E5;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
        "CodeEditor:focus {"
        "  border: 2px solid #1565C0;"
        "}"
    );

    // token table    
    QLabel* tokenLabel = new QLabel("Token Table:");
    tokensTableWidget = new QTableWidget(0, 2);
    tokensTableWidget->setHorizontalHeaderLabels({"Token", "Value"});
    tokensTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tokensTableWidget->setAlternatingRowColors(true);
    
    leftLayout->addWidget(inputLabel);
    leftLayout->addWidget(inputDisplay);
    leftLayout->addWidget(tokenLabel);
    leftLayout->addWidget(tokensTableWidget);
 
    // ================= RIGHT COLUMN =================
    QSplitter* rightVerticalSplitter = new QSplitter(Qt::Vertical);
    QWidget* topWidget = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(15);
    
    // 1. grammar 
    pdaDiagramView = new PDAVisualizer(this);
    pdaDiagramView->setMinimumHeight(400);

    // Control Buttons with icons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    parseButton = new QPushButton("Parse");
    parseButton->setIcon(QIcon(":/icons/parse.png"));
    playPauseButton = new QPushButton("Play");
    playPauseButton->setIcon(QIcon(":/icons/play.png"));
    resetButton = new QPushButton("Reset");
    resetButton->setIcon(QIcon(":/icons/reset.png"));
    buttonLayout->addWidget(parseButton);
    buttonLayout->addWidget(playPauseButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->setSpacing(15);
    
    topLayout->addWidget(pdaDiagramView);
    topLayout->addLayout(buttonLayout);

    // 2. Bottom Section (Stack and Trace)
    QWidget* bottomWidget = new QWidget();
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(15);

    // Left: PDA Stack
    QWidget* stackBox = new QWidget();
    QVBoxLayout* stackVBox = new QVBoxLayout(stackBox);
    stackVBox->setContentsMargins(0, 0, 0, 0);
    stackVBox->setSpacing(10);
    QLabel* stackLabel = new QLabel("PDA Stack:");
    stackLabel->setAlignment(Qt::AlignCenter);
    stackVBox->addWidget(stackLabel);
    stackWidget = new QListWidget();
    stackVBox->addWidget(stackWidget);

    // Right: Parsing Trace
    QWidget* traceBox = new QWidget();
    QVBoxLayout* traceVBox = new QVBoxLayout(traceBox);
    traceVBox->setContentsMargins(0, 0, 0, 0);
    traceVBox->setSpacing(10);
    QLabel* traceLabel = new QLabel("Parsing Trace:");
    traceLabel->setAlignment(Qt::AlignCenter);
    traceVBox->addWidget(traceLabel);
    traceTableWidget = new QTableWidget(0, 3);
    traceTableWidget->setHorizontalHeaderLabels({"Stack", "Input", "Action"});
    traceTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    traceTableWidget->setAlternatingRowColors(true);

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
        tokensToParse.push_back({UNKNOWN, "$", 1}); 
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

    // --- Update the PDA Diagram ---
    QString state = "q2"; // Default to the expansion/matching hub
    QString actionStr = QString::fromStdString(step.action);
    QString lookupAction = actionStr;
    
    // Initial transitions
    if (traversalIndex == 0) state = "q0";
    else if (traversalIndex == 1) state = "q1";
    else if (traversalIndex == 2) state = "q2";

    if (actionStr == "push $") lookupAction = "ε, ε → $";
    else if (actionStr == "push S") lookupAction = "ε, $ → S";
    else if (actionStr == "match $") lookupAction = "ε, $ → ε";
    
    // Final transition
    if (actionStr.contains("ACCEPT", Qt::CaseInsensitive)) {
        state = "q3";
    }

    // --- Update the PDA Diagram ---
    pdaDiagramView->updateVisualization(
            state, 
            QString::fromStdString(step.currentToken.value),
            step.stack.empty() ? "" : QString::fromStdString(step.stack.back()),
            lookupAction
    );

    //stack table
    stackWidget->clear();
    for (auto it = step.stack.rbegin(); it != step.stack.rend(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(*it));
        item->setTextAlignment(Qt::AlignCenter);

        if (*it == "$") {
            item->setForeground(QColor(244, 67, 54)); // Vibrant red
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
        itemAction->setForeground(QColor(76, 175, 80)); // Green
    } 
    else if (actionText.contains("push")) {
        itemAction->setForeground(QColor(33, 150, 243)); // Blue    
    } 
    else if (actionText.contains("pop")) {
        itemAction->setForeground(QColor(255, 152, 0)); // Orange
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
