#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QListWidget>
#include <QTimer>
#include <QLabel>
#include <QSplitter>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <vector>

#include "../syntactic.h" 
#include "../pda_tracer.h"
#include "PDAView.h"

using namespace std;

class SyntacticVisualizer : public QWidget {
    Q_OBJECT

public:
    explicit SyntacticVisualizer(QWidget *parent = nullptr);

public slots:
    void receiveTokens(const vector<Token>& tokens, const QString& rawInput);

private slots:
    void inputTextChanged();
    void parseClicked();
    void playPauseClicked();
    void resetClicked();
    void autoTraverse();

private:
    // --- UI Elements ---
    QPushButton* parseButton;
    QPushButton* playPauseButton;
    QPushButton* resetButton;
    QTextEdit* inputDisplay;
    QString currentInputString;
    QTableWidget* tokensTableWidget;
    QString pendingErrorMessage;

    // PDA Visualization Elements
    QListWidget* stackWidget;
    QTableWidget* traceTableWidget;
    QTableWidget* grammarTableWidget;

    // --- Parsing Data & State ---
    vector<Token> currentTokens;
    Parser* parser;
    vector<PDAAction> trace;
    PDAVisualizer* pdaDiagramView;
    
    QTimer* traversalTimer;
    int traversalIndex;
    int currentInputPos; 
    
    // --- Helper Methods ---
    void setupUI();
    void setupConnections();
    void highlightInputToken(int index);
    void updateStackDisplay(const vector<string>& stack);
    void updateTraceTable(const vector<PDAAction>& traceData);
    void updateGrammarTable(); 
    void clearState();
    
};