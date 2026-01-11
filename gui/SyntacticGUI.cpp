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
    traversalTimer->setInterval(1000);   

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
            tokenItem->setBackground(QColor(255, 235, 59, 100)); 
        } else if (tokens[i].type == NUMBER) {
            tokenItem->setBackground(QColor(76, 175, 80, 100)); 
        } else if (tokens[i].type >= PLUS && tokens[i].type <= RPAREN) {
            tokenItem->setBackground(QColor(255, 87, 34, 100)); 
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
    
    pdaDiagramView = new PDAVisualizer(this);
    pdaDiagramView->setMinimumHeight(400);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    parseButton = new QPushButton("Parse");
    playPauseButton = new QPushButton("Animate");
    backwardButton = new QPushButton("Step Backward");
    forwardButton = new QPushButton("Step Forward");
    resetButton = new QPushButton("Reset");

    playPauseButton->setEnabled(false);
    backwardButton->setEnabled(false);
    forwardButton->setEnabled(false);
    resetButton->setEnabled(false);

    buttonLayout->addWidget(parseButton);
    buttonLayout->addWidget(playPauseButton);
    buttonLayout->addWidget(backwardButton);
    buttonLayout->addWidget(forwardButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->setSpacing(15);
    
    topLayout->addWidget(pdaDiagramView);
    topLayout->addLayout(buttonLayout);

    QWidget* bottomWidget = new QWidget();
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(15);

    // PDA Stack
    QWidget* stackBox = new QWidget();
    QVBoxLayout* stackVBox = new QVBoxLayout(stackBox);
    stackVBox->setContentsMargins(0, 0, 0, 0);
    stackVBox->setSpacing(10);
    QLabel* stackLabel = new QLabel("PDA Stack:");
    stackLabel->setAlignment(Qt::AlignCenter);
    stackVBox->addWidget(stackLabel);
    stackWidget = new QListWidget();
    stackVBox->addWidget(stackWidget);

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
    
    bottomLayout->addWidget(stackBox, 1); 
    bottomLayout->addWidget(traceBox, 2); 

    rightVerticalSplitter->addWidget(topWidget);
    rightVerticalSplitter->addWidget(bottomWidget);
    rightVerticalSplitter->setStretchFactor(0, 2);
    rightVerticalSplitter->setStretchFactor(1, 2);
    
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
    connect(forwardButton, &QPushButton::clicked, this, &SyntacticVisualizer::stepForward);
    connect(backwardButton, &QPushButton::clicked, this, &SyntacticVisualizer::stepBackward);
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
    playPauseButton->setText("Animate");
    playPauseButton->setStyleSheet(
            "QPushButton { "
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4caf50, stop:1 #388e3c); "
            "  color: #ffffff; border: none; padding: 10px 20px; border-radius: 8px; font-weight: bold; font-size: 11pt; "
            "}"
            "QPushButton:hover { "
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66bb6a, stop:1 #4caf50); "
            "}"
            "QPushButton:disabled { "
            "  background: #cccccc; color: #666666; " // This ensures the button looks disabled
            "}"
        );
}

void SyntacticVisualizer::inputTextChanged() {
    clearState();
}

void SyntacticVisualizer::parseClicked() {
    if (currentTokens.empty()) {
        QMessageBox::warning(this, "No Input", "Please run Lexical Analysis first.");
        return;
    } else {
        parseButton->setEnabled(false); // Disable parse once done
        playPauseButton->setEnabled(true);
        backwardButton->setEnabled(true);
        forwardButton->setEnabled(true);
        resetButton->setEnabled(true);

        traversalIndex = 0;
        updateStateAtCurrentIndex(); // Sync first step
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
        playPauseButton->setText("Animate");
        playPauseButton->setStyleSheet(
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4caf50, stop:1 #388e3c); color: white; }"
        );
    } else if (!pendingErrorMessage.isEmpty()) {
        QMessageBox::critical(this, "Syntactic Error", pendingErrorMessage);
    }
}

void SyntacticVisualizer::playPauseClicked() {
    if (traversalTimer->isActive()) {
        traversalTimer->stop();
        playPauseButton->setText("Animate");
        playPauseButton->setStyleSheet(
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4caf50, stop:1 #388e3c); color: white; }"
        );
        backwardButton->setEnabled(true);
        forwardButton->setEnabled(true);
    } else {
        if (traversalIndex >= trace.size()) {
             resetClicked();
             return;
        }
        traversalTimer->start();
        playPauseButton->setText("Pause");
        playPauseButton->setStyleSheet(
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ff9800, stop:1 #ef6c00); color: white; }"
        );
        backwardButton->setEnabled(false);
        forwardButton->setEnabled(false);
    }
}

void SyntacticVisualizer::resetClicked() {
    if (traversalTimer) {
        traversalTimer->stop();
        traversalIndex = 0; 
    }

    parseButton->setEnabled(true);
    playPauseButton->setEnabled(false);
    forwardButton->setEnabled(false);
    backwardButton->setEnabled(false);

    clearState();
    stackWidget->clear();
    traceTableWidget->setRowCount(0);
    pdaDiagramView->clearAllHighlights();
}


void SyntacticVisualizer::autoTraverse()
{
    if (pdaDiagramView->hasPendingEdges()) {
        pdaDiagramView->stepPendingEdge();
        return;
    }


    if (traversalIndex >= trace.size()) {
        traversalTimer->stop();
        playPauseButton->setText("Animate");
        playPauseButton->setStyleSheet(
            "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4caf50, stop:1 #388e3c); color: white; }"
        );
        playPauseButton->setEnabled(false);

        if (!pendingErrorMessage.isEmpty()) {
            QMessageBox::critical(
                this,
                "Syntactic Error",
                "The parser stopped due to a syntax error:\n\n" + pendingErrorMessage
            );
        } else {
            QMessageBox::information(
                this,
                "Success",
                "Parsing completed successfully!"
            );
        }
        return;
    }


    const PDAAction& step = trace[traversalIndex];
    QString actionStr = QString::fromStdString(step.action);
    QString state = "q2"; 

    QString rawAction = actionStr.toLower();
    QString displayAction = actionStr;

    
    if (traversalIndex == 0) {
        state = "q0";
        actionStr = "ε, ε → $"; // Force the label that matches drawArrow
    } else if (traversalIndex == 1) {
        state = "q1";
        actionStr = "ε, $ → S"; // Force the label that matches drawArrow
    } else if (actionStr.contains("ACCEPT", Qt::CaseInsensitive)) {
        state = "q3";
    }
    

    pdaDiagramView->updateVisualization(
        state,
        QString::fromStdString(step.currentToken.value),
        step.stack.empty() ? "" : QString::fromStdString(step.stack.back()),
        actionStr
    );


    stackWidget->clear();
    for (auto it = step.stack.rbegin(); it != step.stack.rend(); ++it) {
        QListWidgetItem* item =
            new QListWidgetItem(QString::fromStdString(*it));
        item->setTextAlignment(Qt::AlignCenter);
        stackWidget->addItem(item);
    }


    int row = traceTableWidget->rowCount();
    traceTableWidget->insertRow(row);

    // STACK column (top on left)
    QString stackStr;
    for (auto it = step.stack.rbegin(); it != step.stack.rend(); ++it) {
        stackStr += QString::fromStdString(*it) + " ";
    }

    // INPUT column
    QString inputStr;
    for (size_t i = currentInputPos; i < currentTokens.size(); ++i) {
        inputStr += QString::fromStdString(currentTokens[i].value) + " ";
    }


    QTableWidgetItem* itemStack  = new QTableWidgetItem(stackStr.trimmed());
    QTableWidgetItem* itemInput  = new QTableWidgetItem(inputStr.trimmed());
    QTableWidgetItem* itemAction = new QTableWidgetItem(displayAction);

    itemStack->setTextAlignment(Qt::AlignCenter);
    itemInput->setTextAlignment(Qt::AlignCenter);
    itemAction->setTextAlignment(Qt::AlignCenter);

    // Color coding
    if (rawAction.contains("match")) {
        itemAction->setForeground(QColor(76, 175, 80));   // green
    } else if (rawAction.contains("push") || rawAction.contains("expand")) {
        itemAction->setForeground(QColor(33, 150, 243)); // blue
    }

    traceTableWidget->setItem(row, 0, itemStack);
    traceTableWidget->setItem(row, 1, itemInput);
    traceTableWidget->setItem(row, 2, itemAction);
    traceTableWidget->scrollToBottom();

    if (rawAction.contains("match")) {
        currentInputPos++;
    }

    traversalIndex++;
}


void SyntacticVisualizer::updateStackDisplay(const vector<string>& stack) {
    stackWidget->clear();
    for (int i = stack.size() - 1; i >= 0; --i) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(stack[i]));
        item->setTextAlignment(Qt::AlignCenter);
        stackWidget->addItem(item);
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


void SyntacticVisualizer::updateStateAtCurrentIndex() {
    if (trace.empty() || traversalIndex < 0 || traversalIndex >= (int)trace.size()) return;

    const PDAAction& step = trace[traversalIndex];

    // --- 1. Update Stack Widget ---
    updateStackDisplay(step.stack);

    // --- 2. Update Trace Table (Highlight current row) ---
    traceTableWidget->selectRow(traversalIndex);
    traceTableWidget->scrollToItem(traceTableWidget->item(traversalIndex, 0));
    QString pdaAction; 
    // --- 3. Update PDA Diagram ---
    QString state = "q2"; 
    QString actionStr = QString::fromStdString(step.action);
    QString pdaLabel = actionStr;



    // Map initial states correctly for push $ and push S
    if (traversalIndex == 0) state = "q0";
    else if (traversalIndex == 1) state = "q1";
    
    // Convert trace text to PDA transition labels
    if (traversalIndex == 0) {
        state = "q0";
        pdaLabel = "ε, ε → $";
    } else if (traversalIndex == 1) {
        state = "q1";
        pdaLabel = "ε, $ → S";
    } else if (actionStr.contains("match", Qt::CaseInsensitive)) {
        // IMPORTANT: Format this to match your terminal edge label: "terminal, terminal → ε"
        QString terminal = QString::fromStdString(step.currentToken.value);
        pdaLabel = QString("%1, %1 → ε").arg(terminal);
    } else if (actionStr.contains("ACCEPT", Qt::CaseInsensitive)) {
        state = "q3";
    }

    // Trigger the PDA highlight
    pdaDiagramView->updateVisualization(
        state, 
        QString::fromStdString(step.currentToken.value), 
        step.stack.empty() ? "" : QString::fromStdString(step.stack.back()), 
        actionStr
    );

    if (!actionStr.contains("Expand")) {
        updateStackDisplay(step.stack);
    }
}

void SyntacticVisualizer::stepForward() {
    if (pdaDiagramView->hasPendingEdges()) {
        pdaDiagramView->stepPendingEdge();
        return; 
    }

    if (traversalIndex < (int)trace.size() - 1) {
        traversalIndex++;
        refreshTableToCurrentIndex();
        updateStateAtCurrentIndex(); 
    }

}


void SyntacticVisualizer::stepBackward() {
    if (traversalIndex > 0) {
        pdaDiagramView->clearAllHighlights();

        traversalIndex--;
        refreshTableToCurrentIndex();
        updateStateAtCurrentIndex(); 
    }
}


void SyntacticVisualizer::addTableRow(int index) {
    if (index < 0 || index >= (int)trace.size()) return;
    const PDAAction& step = trace[index];
    int row = traceTableWidget->rowCount();
    traceTableWidget->insertRow(row);

    // 1. Stack Column (Top on Left)
    QString stackStr;
    for (auto it = step.stack.rbegin(); it != step.stack.rend(); ++it)
        stackStr += QString::fromStdString(*it) + " ";

    // 2. NEW: Calculate Full Remaining Input String
    // We determine the current position by counting 'match' actions up to this index
    int matchCount = 0;
    for (int i = 0; i < index; ++i) {
        if (QString::fromStdString(trace[i].action).toLower().contains("match")) {
            matchCount++;
        }
    }

    QString fullInputStr;
    for (size_t i = matchCount; i < currentTokens.size(); ++i) {
        fullInputStr += QString::fromStdString(currentTokens[i].value) + " ";
    }

    // 3. Action Column and Color Logic
    QString actionStr = QString::fromStdString(step.action);
    QColor rowColor = Qt::black; 
    if (actionStr.toLower().contains("match")) rowColor = QColor(76, 175, 80); // Green
    else if (actionStr.toLower().contains("expand") || actionStr.toLower().contains("push")) 
        rowColor = QColor(33, 150, 243); // Blue

    // 4. Create and Center Items
    QTableWidgetItem* itemStack = new QTableWidgetItem(stackStr.trimmed());
    QTableWidgetItem* itemInput = new QTableWidgetItem(fullInputStr.trimmed()); // Use full string
    QTableWidgetItem* itemAction = new QTableWidgetItem(actionStr);

    for (auto* item : {itemStack, itemInput, itemAction}) {
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(rowColor);
    }

    traceTableWidget->setItem(row, 0, itemStack);
    traceTableWidget->setItem(row, 1, itemInput);
    traceTableWidget->setItem(row, 2, itemAction);
}

void SyntacticVisualizer::refreshTableToCurrentIndex() {
    traceTableWidget->setRowCount(0); // Wipe the table clean
    for (int i = 0; i <= traversalIndex; ++i) {
        addTableRow(i); // Redraw from trace[0] to trace[traversalIndex]
    }
    traceTableWidget->scrollToBottom();
}