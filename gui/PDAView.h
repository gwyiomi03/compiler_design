#ifndef PDAVIEW_H
#define PDAVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QMap>

// Custom item for PDA States
class PDAStateNode : public QGraphicsEllipseItem {
public:
    PDAStateNode(QString label, bool isAccepting = false);
    void setHighlighted(bool highlight);
private:
    QGraphicsTextItem* text;
    bool accepting;
};


class PDAVisualizer : public QGraphicsView {
    Q_OBJECT
public:
    explicit PDAVisualizer(QWidget *parent = nullptr);
    
    // Logic to highlight the current transition based on Parser action
    void updateVisualization(const QString& currentState, const QString& inputSymbol, 
                             const QString& stackTop, const QString& action);
    
    void stepAnimation(const QString& action, const QString& stackTop, const QString& input);
    void drawArrow(PDAStateNode* from, PDAStateNode* to, QString label);
    void drawSelfLoop(PDAStateNode* node, QString label);

private:
    QGraphicsScene* scene;
    void setupGraph(); // Define the fixed positions for your grammar's PDA



    QMap<QString, PDAStateNode*> nodes;
    // Store transitions to highlight them during parsing
    QMap<QString, QGraphicsItem*> transitions; 
};

#endif