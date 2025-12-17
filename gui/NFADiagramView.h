#pragma once
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>

class NFADiagramView : public QWidget {
    Q_OBJECT
public:
    explicit NFADiagramView(QWidget *parent = nullptr);
    void setupNFAGraph();


private:
    QGraphicsView* view;
    QGraphicsScene* scene;

    QGraphicsEllipseItem* drawAtomicNFA(int x, int y, QString label, QString statePrefix);
    QGraphicsEllipseItem* drawState(int x, int y, QString label, bool isAccepting = false);
    void drawTransition(QGraphicsEllipseItem* from, QGraphicsEllipseItem* to, QString label, bool isJumping);
    void drawSelfLoop(QGraphicsEllipseItem* node, QString label);
    QGraphicsEllipseItem* createIDNFA(int x, int y);
    QGraphicsEllipseItem* createNumberNFA(int x, int y);
    QGraphicsEllipseItem* createSingleCharNFA(int x, int y, QString label, QString prefix);
    void drawArrowhead(QPointF tip, double angle);
    void drawLabel(QPointF pos, QString label, int yOffset);

};