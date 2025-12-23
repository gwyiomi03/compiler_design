#include "NFADiagramView.h"
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <cmath>

NFADiagramView::NFADiagramView(QWidget *parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("<b>NFA DIAGRAM (Thompson Construction)</b>");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; color: #2c3e50; margin: 10px;");
    layout->addWidget(titleLabel);

    view = new QGraphicsView(this);
    scene = new QGraphicsScene(this);
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);
    
    layout->addWidget(view);
    setupNFAGraph();
}


void NFADiagramView::setupNFAGraph() {
    scene->clear();
    
    // 1. The only "Global" start state
    auto* start = drawState(50, 500, "START");

    // --- ID BRANCH ---
    // createIDNFA returns its first state (e.g., id0)
    auto* id_first_state = createIDNFA(300, 100); 
    drawTransition(start, id_first_state, "ε", true);

    // --- OPERATORS (Using a simple loop to keep code clean) ---
    struct Symbol { QString label; QString prefix; int y; };
    QList<Symbol> symbols = {
        {"=", "eq", 200}, {"+", "pl", 300}, {"-", "mn", 400},
        {"*", "mt", 500}, {"/", "dv", 600}, {"(", "lp", 700},
        {")", "rp", 800}
    };

    for (const auto& sym : symbols) {
        auto* sub_start = createSingleCharNFA(300, sym.y, sym.label, sym.prefix);
        drawTransition(start, sub_start, "ε", (sym.y != 500)); 
    }

    // --- NUMBER BRANCH ---
    auto* num_first_state = createNumberNFA(300, 950);
    drawTransition(start, num_first_state, "ε", true);

    scene->setSceneRect(0, 0, 1500, 1200);
}

QGraphicsEllipseItem* NFADiagramView::createIDNFA(int x, int y) {
    // 1. Mandatory Letter [a-zA-Z]
    auto* id_s1 = drawState(x, y, "id1");
    auto* id_s2 = drawState(x+150, 100, "id2");
    drawTransition(id_s1, id_s2, "a-zA-Z_", false);

    // Thompson Loop for [a-zA-Z0-9]*
    auto* id_loop_entry = drawState(x+300, 100, "id3");
    auto* id_loop_match = drawState(x+450, 100, "id4");
    auto* id_acc     = drawState(x+550, 100, "id_acc", true);

    drawTransition(id_s2, id_loop_entry, "ε", false);
    drawTransition(id_loop_entry, id_loop_match, "a-zA-Z0-9_", false); 
    drawTransition(id_loop_match, id_loop_entry, "ε,", true);      
    drawTransition(id_loop_match, id_acc, "ε",false);           
    drawTransition(id_s2, id_acc, "ε", true);      

    return id_s1;
}


QGraphicsEllipseItem* NFADiagramView::createNumberNFA(int x, int y) {
    auto* num_s1 = drawState(x, y, "n1");
    auto* num_s2 = drawState(x+100, y, "n2");
    drawTransition(num_s1, num_s2, "0-9", false);

    // Thompson Loop for the '+' (at least one)
    auto* num_loop_entry = drawState(x+200, y, "n3");
    auto* num_loop_match = drawState(x+300, y, "n4");
    auto* n5 = drawState(x+400, y, "n5");
    auto* n6 = drawState(x+500, y-50, "n6");
    auto* n7 = drawState(x+1000, y-50, "n7");
    auto* n8 = drawState(x+500, y+50, "n8");
    auto* n9 = drawState(x+600, y+50, "n9");
    auto* n10 = drawState(x+700, y+50, "n10");
    auto* n11 = drawState(x+800, y+50, "n11");
    auto* n12 = drawState(x+900, y+50, "n12");
    auto* n13 = drawState(x+1000, y+50, "n13");
    auto* n_acc = drawState(x+1100, y, "n14", true);


    drawTransition(num_s2, num_loop_entry, "ε", false);
    drawTransition(num_loop_entry, num_loop_match, "0-9",false);
    drawTransition(num_loop_match, num_loop_entry, "ε",true); // Repeat
    drawTransition(num_loop_match, n5, "ε", false);     // Exit
    drawTransition(num_s2, n5, "ε", true);
    
    //(\.[0-9]+)?
    drawTransition(n5, n6, "ε", false); 
    drawTransition(n6, n7, "ε", false);
    drawTransition(n7, n_acc, "ε", false);
    drawTransition(n5, n8, "ε", false);
    drawTransition(n8, n9, ".", false); 
    drawTransition(n9, n10, "0-9", false);    
    drawTransition(n10, n11, "ε", false);
    drawTransition(n11, n12, "0-9", false);
    drawTransition(n12, n11, "ε", true);
    drawTransition(n10, n13, "ε", true);
    drawTransition(n12, n13, "ε", false);
    drawTransition(n13, n_acc, "ε", false);

    return num_s1;
}


QGraphicsEllipseItem* NFADiagramView::createSingleCharNFA(int x, int y, QString label, QString prefix) {
    // 1. Create the start state for this specific character
    auto* s_start = drawState(x, y, prefix + "_1");
    auto* s_end = drawState(x + 150, y, prefix + "_acc", true);
    
    drawTransition(s_start, s_end, label, false);
    
    return s_start;
}




QGraphicsEllipseItem* NFADiagramView::drawState(int x, int y, QString label, bool isAccepting) {
    auto* ellipse = scene->addEllipse(x, y, 50, 50, QPen(Qt::black, 2), QBrush(Qt::white));
    auto* text = scene->addText(label);
    text->setPos(x + 5, y + 12);
    
    if (isAccepting) {
        // Double circle for accepting states
        scene->addEllipse(x + 5, y + 5, 40, 40, QPen(Qt::black, 1));
    }
    return ellipse;
}



void NFADiagramView::drawTransition(QGraphicsEllipseItem* from, QGraphicsEllipseItem* to, QString label, bool isJumping) {
    QPointF start = from->rect().center() + from->pos();
    QPointF end = to->rect().center() + to->pos();
    QPen pen(Qt::black, 2);
    
    if (!isJumping) {
        // --- Standard Straight Line for Adjacent States ---
        QLineF line(start, end);
        double angle = std::atan2(-line.dy(), line.dx());
        QPointF p1 = start + QPointF(25 * cos(angle), -25 * sin(angle));
        QPointF p2 = end - QPointF(25 * cos(angle), -25 * sin(angle));

        scene->addLine(QLineF(p1, p2), pen);
        drawArrowhead(p2, angle);
        drawLabel((p1 + p2) / 2, label, -25);
    } 
    else {
        // --- Curved Path for Jumping Transitions ---
        // 1. Calculate Control Point (Arched upwards)
        QPointF midPoint = (start + end) / 2.0;
        double dist = QLineF(start, end).length();
        // Offset the control point vertically based on distance to create the arc
        QPointF controlPoint = midPoint + QPointF(0, -dist * 0.4); 

        // 2. Find intersection points on circle edges
        double angleStart = std::atan2(-(controlPoint.y() - start.y()), controlPoint.x() - start.x());
        double angleEnd = std::atan2(-(end.y() - controlPoint.y()), end.x() - controlPoint.x());
        
        QPointF p1 = start + QPointF(25 * cos(angleStart), -25 * sin(angleStart));
        QPointF p2 = end - QPointF(25 * cos(angleEnd), -25 * sin(angleEnd));

        // 3. Draw Bezier Curve
        QPainterPath path;
        path.moveTo(p1);
        path.quadTo(controlPoint, p2);
        scene->addPath(path, pen);

        // 4. Add Arrowhead and Label
        drawArrowhead(p2, angleEnd);
        drawLabel(controlPoint, label, -10);
    }
}


void NFADiagramView::drawArrowhead(QPointF tip, double angle) {
    QPolygonF head;
    double size = 10.0;
    head << tip 
         << QPointF(tip.x() - size * cos(angle - M_PI/6), tip.y() + size * sin(angle - M_PI/6))
         << QPointF(tip.x() - size * cos(angle + M_PI/6), tip.y() + size * sin(angle + M_PI/6));
    scene->addPolygon(head, QPen(Qt::black), QBrush(Qt::black));
}


void NFADiagramView::drawLabel(QPointF pos, QString label, int yOffset) {
    auto* text = scene->addText(label);
    text->setPos(pos.x() - text->boundingRect().width() / 2, pos.y() + yOffset);
}


void NFADiagramView::drawSelfLoop(QGraphicsEllipseItem* node, QString label) {
    QPointF center = node->rect().center() + node->pos();
    QPainterPath path;
    path.moveTo(center.x() + 10, center.y() - 23);
    path.quadTo(center.x(), center.y() - 70, center.x() - 10, center.y() - 23);
    
    scene->addPath(path, QPen(Qt::black, 2));
    auto* text = scene->addText(label);
    text->setPos(center.x() - 20, center.y() - 95);
}

