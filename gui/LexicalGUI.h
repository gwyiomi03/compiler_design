#ifndef LEXICALGUI_H
#define LEXICALGUI_H

#include <QWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTableWidget>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QColor>
#include <QMap>
#include <QTimer>
#include "../lexical.h"
#include "CodeEditor.h"


QGraphicsItem* drawArrowHead(QGraphicsScene* scene, const QPointF& tip, const QPointF& direction);
QGraphicsPolygonItem* createArrowHeadItem(const QPointF& tip, const QPointF& direction);
class QGraphicsItemGroup;



class StateNode : public QGraphicsEllipseItem {
public:
    StateNode(int id, TokenType type, bool isAccepting, bool isDead);
    int stateId;
    TokenType tokenType;
    bool isAccepting;
    bool isDead;
    void setHighlighted(bool highlight);
};



class LexicalVisualizer : public QWidget {
    Q_OBJECT

public:
    LexicalVisualizer(QWidget *parent = nullptr);
    ~LexicalVisualizer() {}

signals:
    void tokensReady(const std::vector<Token>& tokens, const QString& rawInput);

private slots:
    void tokenizeClicked();
    void resetClicked();
    void inputTextChanged();
    void playPauseClicked();
    void autoTraverse();



private:
    // --- UI Components ---
    CodeEditor* inputEditor;
    QListWidget* tokenListWidget;
    QTableWidget* tokenTableWidget;
    QPushButton* tokenizeButton;
    QPushButton* stepButton;
    QPushButton* resetButton;
    QGraphicsScene* dfaScene;
    QGraphicsView* dfaView;
    

    
    // --- Lexical/DFA Data ---
    DFA dfa;
    DFAState* walkState = nullptr;
    size_t walkPos = 0;
    QMap<int, StateNode*> stateNodes; 
    int currentline = 1;


    QMap<QPair<int,int>, QGraphicsItemGroup*> transitionGroups;
    QGraphicsItemGroup* currentHighlightedTransition = nullptr;

    // ---- DFA Traversal State ----
    ScanResult currentResult;
    size_t traversalIndex = 0;
    bool isTraversing = false;
    size_t currentScanPos = 0;
    QTimer* traversalTimer = nullptr;
    QPushButton* playPauseButton = nullptr;
    




    // --- Visualization Helpers ---
    void setupDFA();
    void drawDFA();
    QGraphicsItemGroup* drawDFATransition(DFAState* source, DFAState* target, const QString& labelText, 
                           const QPointF& sourcePos, const QPointF& targetPos, 
                           int offsetIndex, bool isLoop = false);
    void setTransitionGroupColor(QGraphicsItemGroup* group, const QColor& color);
    void highlightDFATransition(int sourceId, int targetId);
    void highlightDFAState(int id);
    void highlightInput(size_t start, size_t end);
    void updateTokenList(const Token& token);
    void highlightTransition(int fromId, int toId);

    vector<Token> finalTokens;
    QString rawInputString;
};


#endif
