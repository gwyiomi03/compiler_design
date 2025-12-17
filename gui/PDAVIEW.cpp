#include "PDAView.h"
#include <QPen>
#include <QBrush>
#include <cmath>

PDAStateNode::PDAStateNode(QString label, bool isAccepting) : accepting(isAccepting) {
    setRect(-25, -25, 50, 50);
    setPen(QPen(Qt::black, 2));
    setBrush(Qt::white);

    text = new QGraphicsTextItem(label, this);
    text->setPos(-10, -15);

    if (isAccepting) {
        // Draw the inner circle for final states
        QGraphicsEllipseItem* inner = new QGraphicsEllipseItem(-20, -20, 40, 40, this);
        inner->setPen(QPen(Qt::black, 1));
    }
}

void PDAStateNode::setHighlighted(bool highlight) {
    setPen(QPen(highlight ? Qt::red : Qt::black, highlight ? 3 : 2));
}

PDAVisualizer::PDAVisualizer(QWidget *parent) : QGraphicsView(parent) {
    scene = new QGraphicsScene(this);
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
    setupGraph();
}

void PDAVisualizer::setupGraph() {
    scene->clear();
    nodes.clear();

    // Create Nodes
    auto* startNode = new PDAStateNode("q0");
    auto* loopNode = new PDAStateNode("q1");
    auto* acceptNode = new PDAStateNode("q2", true);

    // Positions
    startNode->setPos(100, 100);
    loopNode->setPos(300, 100);
    acceptNode->setPos(500, 100);

    // Store in map for lookup
    nodes["q0"] = startNode;
    nodes["q1"] = loopNode;
    nodes["q2"] = acceptNode;

    scene->addItem(startNode);
    scene->addItem(loopNode);
    scene->addItem(acceptNode);

    drawArrow(startNode, loopNode, "ε, ε → $");

    drawSelfLoop(loopNode, "Rules / Match");

    drawArrow(loopNode, acceptNode, "$, $ → ε");
}

void PDAVisualizer::updateVisualization(const QString& currentState, const QString& inputSymbol, 
                                       const QString& stackTop, const QString& action) {
    nodes["q0"]->setHighlighted(false);
    nodes["q1"]->setHighlighted(false);
    nodes["q2"]->setHighlighted(false);

    if (nodes.contains(currentState)) {
        nodes[currentState]->setHighlighted(true);
    }

    if (stackTop == "" && action.contains("pop S")) {
         nodes["q2"]->setHighlighted(true);
    }

    scene->update();
}


void PDAVisualizer::stepAnimation(const QString& action, const QString& stackTop, const QString& input) {
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        it.value()->setHighlighted(false);
    }

    if (action.contains("START")) {
        nodes["q0"]->setHighlighted(true);
    } else if (action.contains("ACCEPT")) {
        nodes["q2"]->setHighlighted(true);
    } else {
        nodes["q1"]->setHighlighted(true);
    }

    scene->update();
}


void PDAVisualizer::drawArrow(PDAStateNode* from, PDAStateNode* to, QString label) {
    QGraphicsLineItem* line = new QGraphicsLineItem(from->x() + 25, from->y(), to->x() - 25, to->y());
    line->setPen(QPen(Qt::black, 2));
    scene->addItem(line);

    // Simple arrow head
    QPolygonF head;
    head << QPointF(to->x() - 25, to->y()) << QPointF(to->x() - 35, to->y() - 5) << QPointF(to->x() - 35, to->y() + 5);
    scene->addPolygon(head, QPen(Qt::black), QBrush(Qt::black));

    QGraphicsTextItem* text = scene->addText(label);
    text->setPos((from->x() + to->x())/2 - 30, from->y() - 30);
}


void PDAVisualizer::drawSelfLoop(PDAStateNode* node, QString label) {
    const double R = 25.0; 
    const double LOOP_HEIGHT_REL = 3.0;
    QPointF sourcePos = node->pos();
    QPen pen(Qt::black, 2);

    auto polarToCartesian = [R](double angleDeg, const QPointF& center) -> QPointF {
        double angleRad = angleDeg * M_PI / 180.0;
        return center + QPointF(R * cos(angleRad), R * sin(angleRad)); 
    };

    const double START_ANGLE_DEG = -60.0; 
    const double END_ANGLE_DEG   = -120.0;   
    
    QPointF startPoint = polarToCartesian(START_ANGLE_DEG, sourcePos);
    QPointF endPoint   = polarToCartesian(END_ANGLE_DEG, sourcePos);
    
    QPointF controlPoint = sourcePos + QPointF(0, -R * LOOP_HEIGHT_REL);

    QPainterPath path;
    path.moveTo(startPoint);
    path.quadTo(controlPoint, endPoint); 

    QGraphicsPathItem* pathItem = new QGraphicsPathItem(path);
    pathItem->setPen(pen);
    scene->addItem(pathItem);

    QPointF arrowDirection = endPoint - controlPoint;
    double angle = std::atan2(arrowDirection.y(), arrowDirection.x());

    QPolygonF arrowHead;
    arrowHead << endPoint 
              << QPointF(endPoint.x() - 10 * std::cos(angle - M_PI/6), endPoint.y() - 10 * std::sin(angle - M_PI/6))
              << QPointF(endPoint.x() - 10 * std::cos(angle + M_PI/6), endPoint.y() - 10 * std::sin(angle + M_PI/6));

    scene->addPolygon(arrowHead, QPen(Qt::black), QBrush(Qt::black));

    QGraphicsTextItem* textItem = new QGraphicsTextItem(label);
    textItem->setFont(QFont("Arial", 10));
    
    QPointF labelPos = controlPoint + QPointF(-textItem->boundingRect().width() / 2, -10);
    textItem->setPos(labelPos);
    scene->addItem(textItem);
}


