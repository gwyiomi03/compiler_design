#ifndef PDAVIEW_H
#define PDAVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QMap>

// Custom item for PDA States
class PDAStateNode : public QGraphicsEllipseItem {
public:
    PDAStateNode(QString label, bool isAccepting = false);
    void setHighlighted(bool highlight);
    void setTextPos(qreal x, qreal y);
private:
    QGraphicsTextItem* text;
    bool accepting;

};


class PDAVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit PDAVisualizer(QWidget *parent = nullptr);
    
    // Logic to highlight the current transition based on Parser action
    void updateVisualization(const QString& currentState, const QString& inputSymbol, 
                             const QString& stackTop, const QString& action);
    
    void stepAnimation(const QString& action, const QString& stackTop, const QString& input);
    void drawArrow(PDAStateNode* from, PDAStateNode* to, const QString& label);
    void drawSelfLoop(PDAStateNode* node, QString label);
    void drawNonTerminalLoop(PDAStateNode* node, const QStringList& group, double loopHeight);
    void drawTerminalLoop(PDAStateNode* node, const QStringList& terminals);                                


private:
    void setupGraph(); // Define the fixed positions for your grammar's PDA
    void displayGrammar();
    void setupParsingTable();


    QMap<QString, PDAStateNode*> nodes;
    QMap<QString, QGraphicsItem*> transitions; 
    QGraphicsScene* scene;
    QTextEdit* grammarText; 
    QTableWidget* parsingTable;


    // Add these to your class definition in PDAView.h
    QMap<QString, QGraphicsTextItem*> transitionLabels;
    QMap<QString, QGraphicsPathItem*> transitionPaths; 

};

#endif
