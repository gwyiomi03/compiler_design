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
#include "CodeEditor.h"

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
    void stepForward();
    void stepBackward();
    void updateStateAtCurrentIndex();




private:
    // --- UI Elements ---
    QPushButton* parseButton;
    QPushButton* playPauseButton;
    QPushButton* resetButton;
    CodeEditor* inputDisplay;
    QString currentInputString;
    QTableWidget* tokensTableWidget;
    QString pendingErrorMessage;
    QPushButton* forwardButton;
    QPushButton* backwardButton;
    QString edgeLabel;

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
    void updateStackDisplay(const vector<string>& stack);
    void updateTraceTable(const vector<PDAAction>& traceData);
    void clearState();
    void refreshTableToCurrentIndex();
    void addTableRow(int index);

};