#include "PDAView.h"
#include <QPen>
#include <QBrush>
#include <QPainter>
#include <cmath>
#include <QTabWidget>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QFont>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QAbstractItemView>
#include <QColor>
#include <QTextEdit>
#include <QRegularExpression>


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

void PDAStateNode::setTextPos(qreal x, qreal y) {
    if (text) text->setPos(x, y);
}

PDAVisualizer::PDAVisualizer(QWidget *parent) : QWidget(parent) {
    auto* tabWidget = new QTabWidget(this);



    // Tab 1: PDA Graph
    auto* graphView = new QGraphicsView();
    scene = new QGraphicsScene(graphView);
    graphView->setScene(scene);
    graphView->setRenderHint(QPainter::Antialiasing);
    setupGraph();
    tabWidget->addTab(graphView, "PDA Graph");

    // Tab 2: Grammar Display
    grammarText = new QTextEdit();
    displayGrammar();
    tabWidget->addTab(grammarText, "Grammar");

    // Tab 3: NEW LL(1) Parsing Table 
    parsingTable = new QTableWidget();
    setupParsingTable(); 
    tabWidget->addTab(parsingTable, "Predictive Parsing Table");

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tabWidget);
    setLayout(layout);
}

QString PDAVisualizer::makeEdgeId(const QString& base) {
    QString s = base;
    s = s.simplified();
    s.replace(" ", "_");
    // remove potentially problematic characters
    s.remove(QRegularExpression("[^A-Za-z0-9_'-]"));
    return QString("%1_%2").arg(s).arg(edgeCounter++);
}


void PDAVisualizer::updateVisualization(const QString& currentState, const QString& inputSymbol, 
                                        const QString& stackTop, const QString& action) {
    // 1. Reset all highlights for nodes and edges
    for (auto& node : nodes) node->setHighlighted(false);

    for (auto* label : transitionLabelsById) {
        label->setDefaultTextColor(Qt::black);
        QFont f = label->font();
        f.setBold(false);
        label->setFont(f);
    }

    for (auto* path : transitionPathsById) path->setPen(QPen(Qt::black, 1));

    // 2. Highlight current state
    if (nodes.contains(currentState)) nodes[currentState]->setHighlighted(true);

    QString rawAction = action.trimmed();

    if (rawAction == "ε, ε → $" || rawAction == "ε, $ → S" || rawAction == "match $ → pop") {
            
            QString targetLabel = rawAction;
            // Map the "match" text back to the physical arrow label
            if (rawAction == "match $ → pop") targetLabel = "ε, $ → ε";

            if (labelTextToIds.contains(targetLabel) && !labelTextToIds[targetLabel].isEmpty()) {
                highlightEdge(labelTextToIds[targetLabel].first());
            }
            scene->update();
            return;
        }

    // 2. Normalize the action string to match the PDA's short labels
    QString normalizedAction = rawAction;
    normalizedAction.replace("IDENTIFIER", "ID");
    normalizedAction.replace("NUMBER", "NUM");
    normalizedAction.replace("Expr'", "E'"); 
    normalizedAction.replace("Expr", "E");
    normalizedAction.replace("Term'", "T'");
    normalizedAction.replace("Term", "T");
    normalizedAction.replace("Factor", "F");
    normalizedAction.replace("print", "PRINT");
    normalizedAction.replace("FUNCTION", "FUNC");
    normalizedAction.replace("→", "->"); // Match your map's "->"
    normalizedAction.replace("ε", "e");   // Match your map's "e"

    if (normalizedAction.startsWith("Expand ")) {
        normalizedAction = normalizedAction.mid(7).trimmed();
    }

    // 3. Handle "expand" actions (non-terminal expansions)
    if (rawAction.startsWith("Expand", Qt::CaseInsensitive)) {
        pendingEdges.clear();
        pendingEdgeIndex = 0;

        if (productionEdges.contains(normalizedAction)) {
            pendingEdges = productionEdges[normalizedAction];
        } else {
            qDebug() << "LOOKUP FAILED. Action from parser:" << normalizedAction;
            qDebug() << "Available keys in map:" << productionEdges.keys();
            return;

        }

        // Highlight first micro-step immediately
        if (!pendingEdges.isEmpty())
            highlightEdge(pendingEdges[pendingEdgeIndex++]);

        scene->update();
        return;
    }

    // 4. Handle "match" actions (terminal matches)
    if (rawAction.startsWith("match")) {
        QString sym = action.section(' ', 1, 1);
        if (sym == "IDENTIFIER") sym = "ID";
        else if (sym == "NUMBER") sym = "NUM";
        else if (sym == "print") sym = "PRINT";
        else if (sym == "FUNCTION") sym = "FUNC";

        QString label = QString("%1, %1 → ε").arg(sym);
        if (labelTextToIds.contains(label) && !labelTextToIds[label].isEmpty())
            highlightEdge(labelTextToIds[label].first());
    }

    scene->update();
}

void PDAVisualizer::highlightEdge(const QString& edgeId) {
    // We expect edgeId to be a UNIQUE ID (e.g., "edge_12") 
    // generated during setupGraph and stored in productionEdges.

    if (!transitionLabelsById.contains(edgeId)) {
        // If the ID doesn't exist, we don't fallback to searching by text.
        // This prevents the "wrong edge" bug.
        return;
    }

    QGraphicsTextItem* lbl = transitionLabelsById.value(edgeId, nullptr);
    if (lbl) {
        lbl->setDefaultTextColor(Qt::red);
        QFont f = lbl->font(); f.setBold(true); lbl->setFont(f);
    }

    QGraphicsPathItem* path = transitionPathsById.value(edgeId, nullptr);
    if (path) {
        QPen p = path->pen();
        p.setColor(Qt::red);
        p.setWidth(3);
        path->setPen(p);
    }
}

void PDAVisualizer::stepPendingEdge() {
    if (!hasPendingEdges()) return;

    // Reset highlights
    for (auto& node : nodes) node->setHighlighted(false);
    for (auto* label : transitionLabelsById) {
        label->setDefaultTextColor(Qt::black);
        QFont f = label->font(); f.setBold(false);
        label->setFont(f);
    }
    for (auto* path : transitionPathsById) {
        path->setPen(QPen(Qt::black, 1));
    }

    highlightEdge(pendingEdges[pendingEdgeIndex++]);

    scene->update();
}

void PDAVisualizer::clearAllHighlights() {
    pendingEdges.clear();
    pendingEdgeIndex = 0;

    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        it.value()->setHighlighted(false);
    }

    for (auto* label : transitionLabelsById) {
        label->setDefaultTextColor(Qt::black);
        QFont f = label->font();
        f.setBold(false);
        label->setFont(f);
    }

    for (auto* path : transitionPathsById) {
        path->setPen(QPen(Qt::black, 1));
    }

    scene->update();
}


void PDAVisualizer::setupGraph() {
    scene->clear();
    nodes.clear();
    transitionLabelsById.clear();
    transitionPathsById.clear();
    labelTextToIds.clear();
    productionEdges.clear();
    edgeCounter = 0;


    auto* q2 = new PDAStateNode("q2");
    q2->setRect(-80, -80, 160, 160);
    q2->setPos(600, 450); 
    nodes["q2"] = q2;
    scene->addItem(q2);


    auto* q0 = new PDAStateNode("q0");
    auto* q1 = new PDAStateNode("q1");
    auto* q3 = new PDAStateNode("q3", true);
    q0->setPos(100, 200);
    q1->setPos(100, 350);
    q3->setPos(100, 500);
    nodes["q0"] = q0; nodes["q1"] = q1; nodes["q3"] = q3;
    scene->addItem(q0); scene->addItem(q1); scene->addItem(q3);

    drawArrow(q0, q1, "ε, ε → $", false);
    drawArrow(q1, q2, "ε, $ → S", false);
    drawArrow(q2, q3, "ε, $ → ε", false);


    QStringList terminals = {        
        "ID, ID → ε",
        "NUM, NUM → ε",
        "+, + → ε",
        "-, - → ε",
        "*, * → ε",
        "/, / → ε",
        "=, = → ε",
        "%, % → ε",
        "(, ( → ε",
        "), ) → ε",
        "PRINT, PRINT → ε",
        "FUNC, FUNC → ε",};
    drawTerminalLoop(q2, terminals); 

    struct Production { QString lhs; QString rhs; };
    QList<Production> productions = {
        {"S", "Stmt S"}, {"S", "ε"}, 
        {"Stmt", "ID = E"}, {"Stmt", "PRINT ( E )"},
        {"E", "T E'"}, {"E'", "+ T E'"}, {"E'", "- T E'"}, {"E'", "ε"},
        {"T", "F T'"}, {"T'", "* F T'"}, {"T'", "/ F T'"}, {"T'", "% F T'"}, {"T'", "ε"},
        {"F", "NUM"}, {"F", "ID"}, {"F", "FUNC ( E )"}, {"F", "( E )"}
    };

    double startAngle = -M_PI / 2.4; // Fan starts at top-right
    double endAngle = M_PI / 1.15;   // Fan ends at bottom-right
    double angleStep = (endAngle - startAngle) / (productions.size() - 1);

        for (int index = 0; index < productions.size(); ++index) {
        auto& p = productions[index];

        // --- Create a unique production ID ---
        QString prodId = QString("%1 -> %2").arg(p.lhs).arg(p.rhs);

        if (p.rhs == "ε") {
            QString id = drawArrow(q2, q2, QString("%1 -> e").arg(p.lhs), true);
            productionEdges[prodId] = { id };
            continue;
        }

        QStringList symbols = p.rhs.split(" ", Qt::SkipEmptyParts);
        std::reverse(symbols.begin(), symbols.end()); 
        QString lastSymbol = symbols.takeLast(); 

        QStringList edgesForProd;

        double angle = startAngle + (index * angleStep);
        PDAStateNode* firstInter = new PDAStateNode("");
        firstInter->setRect(-12, -12, 24, 24);

        double firstDist = 180.0;
        firstInter->setPos(q2->pos().x() + cos(angle) * firstDist, 
                           q2->pos().y() + sin(angle) * firstDist);
        scene->addItem(firstInter);

        // --- STEP 1: POP ---
        QString popLabel = QString("ε, %1 → ε").arg(p.lhs);
        QString popId = drawArrow(q2, firstInter, popLabel, true);
        edgesForProd << popId;

        PDAStateNode* prevNode = firstInter;

        // --- STEP 2: PUSH RHS ---
        for (int i = 0; i < symbols.size(); ++i) {
            QString sym = symbols[i];
            // Maintain your symbol mapping
            if (sym == "IDENTIFIER") sym = "ID";
            else if (sym == "NUMBER") sym = "NUM";
            else if (sym == "Expr") sym = "E";
            else if (sym == "Expr'") sym = "E'";
            else if (sym == "Term") sym = "T";
            else if (sym == "Term'") sym = "T'";
            else if (sym == "Factor") sym = "F";
            else if (sym == "print") sym = "PRINT";
            else if (sym == "FUNCTION") sym = "FUNC";

            PDAStateNode* nextInter = new PDAStateNode("");
            nextInter->setRect(-12, -12, 24, 24);
            double dist = firstDist + ((i + 1) * 100.0);
            nextInter->setPos(q2->pos().x() + cos(angle) * dist, 
                              q2->pos().y() + sin(angle) * dist);
            scene->addItem(nextInter);

            QString pushLabel = QString("ε, ε → %1").arg(sym);
            QString pushId = drawArrow(prevNode, nextInter, pushLabel, true);
            edgesForProd << pushId;
            prevNode = nextInter;
        }

        // --- STEP 3: FINAL PUSH (Curly Return) ---
        QString curlyId = drawCurlyReturn(prevNode, q2);

        QString returnLabel = QString("ε, ε → %1").arg(lastSymbol);
        auto* text = scene->addText(returnLabel);
        text->setDefaultTextColor(Qt::black);

        // Center and rotate text along path
        QPointF start = prevNode->pos();
        QPointF end = q2->pos();
        QPointF mid = (start + end) / 2.0;
        QPointF offset(sin(angle) * 60, -cos(angle) * 60); 
        text->setTransformOriginPoint(text->boundingRect().center());
        QLineF line(start, end);
        text->setRotation(-line.angle());
        text->setPos(mid + offset * 0.5 - text->boundingRect().center());

        transitionLabelsById[curlyId] = text;
        labelTextToIds[returnLabel].append(curlyId);

        edgesForProd << curlyId;

        // --- Register edges under unique production ID ---
        productionEdges[prodId] = edgesForProd;
    }
}


QString PDAVisualizer::drawCurlyReturn(PDAStateNode* from, PDAStateNode* to) {
    QPointF start = from->pos();
    QPointF end = to->pos();
    
    QPointF mid = (start + end) / 2.0;
    QPointF vec = end - start;
    QPointF normal(-vec.y(), vec.x()); 
    double length = std::sqrt(vec.x()*vec.x() + vec.y()*vec.y());
    if (length > 0) normal /= length;

    // Offset the control point by 50 units to create the curve
    QPointF control = mid + normal * 45.0; 

    QPainterPath path;
    path.moveTo(start);
    path.quadTo(control, end);

    QGraphicsPathItem* pathItem = scene->addPath(path, QPen(Qt::black, 1, Qt::DashLine));
    pathItem->setZValue(-1);

    // Add arrowhead at the end
    // Compute tangent vector at the end of the quadratic Bezier curve: 2*(end - control)
    QPointF tangent = 1.5 * (end - control);
    double tanLen = std::sqrt(tangent.x() * tangent.x() + tangent.y() * tangent.y());
    if (tanLen > 0) tangent /= tanLen;

    // Arrowhead parameters
    double arrowLength = 10.0;
    double arrowWidth = 5.0;

    // Perpendicular vector for arrow width
    QPointF perp(-tangent.y(), tangent.x());

    // Arrow points: tip at end, base points offset back along tangent
    QPointF arrowTip = end;
    QPointF arrowBase1 = end - tangent * arrowLength + perp * arrowWidth;
    QPointF arrowBase2 = end - tangent * arrowLength - perp * arrowWidth;

    QPolygonF arrowPoly;
    arrowPoly << arrowTip << arrowBase1 << arrowBase2;

    // Add the arrowhead as a filled polygon
    QGraphicsPolygonItem* arrowItem = scene->addPolygon(arrowPoly, QPen(Qt::black), QBrush(Qt::black));
    arrowItem->setZValue(-1); // Above the path

    QString edgeId = makeEdgeId("curly");
    transitionPathsById[edgeId] = pathItem;
    return edgeId;
}


void PDAVisualizer::drawTerminalLoop(PDAStateNode* node, const QStringList& terminals){
    if (terminals.isEmpty()) return;

    QPointF center = node->pos(); 
    double radius = node->rect().width() / 2.0; 
    double loopHeight = 50.0; 

    QPointF startPoint(center.x() + radius * std::cos(220 * M_PI / 180.0), 
                       center.y() + radius * std::sin(220 * M_PI / 180.0));
    QPointF endPoint(center.x() + radius * std::cos(250 * M_PI / 180.0), 
                     center.y() + radius * std::sin(250 * M_PI / 180.0));
    
    QPointF controlPoint(center.x(), center.y() - radius - loopHeight);

    QPainterPath path;
    path.moveTo(startPoint);
    path.quadTo(controlPoint, endPoint);

    QGraphicsPathItem* pathItem = scene->addPath(path, QPen(Qt::black, 2));
    pathItem->setZValue(0);

    double arrowSize = 10.0;
    double angle = std::atan2(endPoint.y() - controlPoint.y(), endPoint.x() - controlPoint.x());
    QPolygonF arrowHead;
    arrowHead << endPoint 
              << QPointF(endPoint.x() - arrowSize * std::cos(angle - M_PI/6), endPoint.y() - arrowSize * std::sin(angle - M_PI/6))
              << QPointF(endPoint.x() - arrowSize * std::cos(angle + M_PI/6), endPoint.y() - arrowSize * std::sin(angle + M_PI/6));
    scene->addPolygon(arrowHead, QPen(Qt::black), QBrush(Qt::black));

    for (int i = 0; i < terminals.size(); ++i) {
        QString labelText = terminals[i];
        auto* text = scene->addText(terminals[i]);
        double textWidth = text->boundingRect().width();
        double horizontalMargin = 15.0;

        text->setPos(controlPoint.x() - textWidth - horizontalMargin, 
                     controlPoint.y() + (i * 15) - (terminals.size() * 15));

        QString id = makeEdgeId(labelText);
        transitionLabelsById[id] = text;
        transitionPathsById[id] = pathItem;
        labelTextToIds[labelText].append(id);
    }
}



QString PDAVisualizer::drawArrow(PDAStateNode* from, PDAStateNode* to, const QString& label, bool labelAlongLine) {
    if (!from || !to) return QString();

    QPointF fromCenter = from->pos();
    QPointF toCenter = to->pos();
    
    QPointF vec = toCenter - fromCenter;
    double length = std::sqrt(vec.x() * vec.x() + vec.y() * vec.y());
    
    if (length == 0) return QString();

    QPointF unitVec = vec / length;

    // Calculate surface points based on the radius of the nodes
    double fromRadius = from->rect().width() / 2.0;
    double toRadius = to->rect().width() / 2.0;

    QPointF startPoint = fromCenter + unitVec * fromRadius;
    QPointF endPoint = toCenter - unitVec * toRadius;

    QPainterPath path;
    path.moveTo(startPoint);
    path.lineTo(endPoint);

    QGraphicsPathItem* pathItem = new QGraphicsPathItem(path);
    pathItem->setPen(QPen(Qt::black, 1));
    scene->addItem(pathItem);

    // Arrowhead logic at the destination surface
    double arrowSize = 10.0;
    double angle = std::atan2(endPoint.y() - startPoint.y(), endPoint.x() - startPoint.x());
    QPolygonF arrowHead;
    arrowHead << endPoint 
              << QPointF(endPoint.x() - arrowSize * std::cos(angle - M_PI/6),
                         endPoint.y() - arrowSize * std::sin(angle - M_PI/6)) 
              << QPointF(endPoint.x() - arrowSize * std::cos(angle + M_PI/6),
                         endPoint.y() - arrowSize * std::sin(angle + M_PI/6));
    scene->addPolygon(arrowHead, QPen(Qt::black), QBrush(Qt::black));

    QString edgeId = makeEdgeId(label.isEmpty() ? "edge" : label);

    if (!label.isEmpty()) {
        QGraphicsTextItem* textItem = scene->addText(label, QFont("Arial", 9));
        
        if (labelAlongLine) {
            // Place non-terminal label EXACTLY along the line
            QLineF line(startPoint, endPoint);
            QPointF center = line.pointAt(0.5);
            textItem->setPos(center.x() - textItem->boundingRect().width()/2, 
                             center.y() - textItem->boundingRect().height()/2);
        } else {
            // Standard placement: above the line depending on direction
            QPointF midPoint = (startPoint + endPoint) / 2.0;
            
            // Determine if arrow is mostly horizontal or vertical
            if (std::abs(vec.x()) >= std::abs(vec.y())) {
                // Horizontal arrow: place label above (y - 20)
                textItem->setPos(midPoint.x() - textItem->boundingRect().width()/2, 
                                 midPoint.y() - 20 - textItem->boundingRect().height());
            } else {
                // Vertical arrow: place label to the right (x + 20)
                textItem->setPos(midPoint.x() + 20, 
                                 midPoint.y() - textItem->boundingRect().height()/2);
            }
        }
        
        transitionLabelsById[edgeId] = textItem;
        labelTextToIds[label].append(edgeId);
    }
    transitionPathsById[edgeId] = pathItem;

    return edgeId;
}


QString PDAVisualizer::drawSelfLoop(PDAStateNode* node, QString label) {
    const double R = 25.0; 
    const double LOOP_HEIGHT_REL = 3.0;
    QPointF sourcePos = node->pos();
    QPen pen(Qt::black, 2);

    auto polarToCartesian = [R](double angleDeg, const QPointF& center) -> QPointF {
        double angleRad = angleDeg * M_PI / 180.0;
        return center + QPointF(R * cos(angleRad), R * sin(angleRad)); 
    };

    const double START_ANGLE_DEG = -50.0; 
    const double END_ANGLE_DEG   = -110.0;   
    
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
    
    QPointF labelPos = controlPoint + QPointF(-textItem->boundingRect().width() / 2, -15);
    textItem->setPos(labelPos);
    scene->addItem(textItem);

    QString id = makeEdgeId(label);
    transitionLabelsById[id] = textItem;
    transitionPathsById[id] = pathItem;
    labelTextToIds[label].append(id);
    return id;
}


void PDAVisualizer::displayGrammar() {
    // We use HTML to create a "Code Editor" look within the QTextEdit
    QString html = R"(
        <div style="background-color: #1e1e1e; padding: 20px; border-radius: 10px; font-family: 'Consolas', 'Courier New', monospace;">
            <h2 style="color: #4fc3f7; margin-top: 0; border-bottom: 1px solid #333; padding-bottom: 10px;">
                Context-Free Grammar
            </h2>
            <table style="color: #d4d4d4; font-size: 13pt; line-height: 1.6;">

                <tr>
                    <td style="color: #ce9178; padding-right: 20px;">S</td>
                    <td style="color: #569cd6;">&rarr;</td>
                    <td>Stmt S <span style="color: #b5cea8;">|</span> &epsilon;</td></td>
                </tr>

                <tr>
                    <td style="color: #ce9178;">Stmt</td>
                    <td style="color: #569cd6;">&rarr;</td>
                    <td>
                        <span style="color: #4ec9b0;">IDENTIFIER</span>
                        <span style="color: #d4d4d4;">=</span>
                        Expr
                        <span style="color: #b5cea8;">|</span>
                        <span style="color: #9cdcfe;">print</span> ( Expr )
                    </td>
                </tr>

                <tr>
                    <td style="color: #ce9178;">Expr</td>
                    <td style="color: #569cd6;">&rarr;</td>
                    <td>Term Expr'</td>
                </tr>

                <tr>
                    <td style="color: #ce9178;">Expr'</td>
                    <td style="color: #569cd6;">&rarr;</td>
                    <td>
                        <span style="color: #d4d4d4;">+</span> Term Expr'
                        <span style="color: #b5cea8;">|</span>
                        <span style="color: #d4d4d4;">-</span> Term Expr'
                        <span style="color: #b5cea8;">|</span>
                        &epsilon;
                    </td>
                </tr>

                <tr>
                    <td style="color: #ce9178;">Term</td>
                    <td style="color: #569cd6;">&rarr;</td>
                    <td>Factor Term'</td>
                </tr>

                <tr>
                    <td style="color: #ce9178;">Term'</td>
                    <td style="color: #569cd6;">&rarr;</td>
                    <td>
                        <span style="color: #d4d4d4;">*</span> Factor Term'
                        <span style="color: #b5cea8;">|</span>
                        <span style="color: #d4d4d4;">/</span> Factor Term'
                        <span style="color: #b5cea8;">|</span>
                        <span style="color: #d4d4d4;">%</span> Factor Term'
                        <span style="color: #b5cea8;">|</span>
                        &epsilon;
                    </td>
                </tr>

                <tr>
                    <td style="color: #ce9178;">Factor</td>
                    <td style="color: #569cd6;">&rarr;</td>
                    <td>
                        <span style="color: #b5cea8;">NUMBER</span>
                        <span style="color: #b5cea8;">|</span>
                        <span style="color: #4ec9b0;">IDENTIFIER</span>
                        <span style="color: #b5cea8;">|</span>
                        <span style="color: #c9a24eff;">FUNCTION</span> ( Expr )
                        <span style="color: #b5cea8;">|</span>
                        ( Expr )
                    </td>
                </tr>

            </table>
        </div>


    )";

    grammarText->setHtml(html);
    grammarText->setReadOnly(true);
    
    // Set a dark background for the widget itself to match the HTML block
    grammarText->setStyleSheet("QTextEdit { background-color: #1e1e1e; border: 2px solid #3f72af; border-radius: 8px; }");
}


void PDAVisualizer::setupParsingTable() {
    QStringList nonTerminals = {
        "S", "Stmt", "Expr", "Expr'", "Term", "Term'", "Factor"
    };

    QStringList terminals = {
        "IDENTIFIER", "NUMBER", "print", "FUNCTION",
        "=", "+", "-", "*", "/", "%", "(", ")", "$"
    };

    parsingTable->clear();
    parsingTable->setRowCount(nonTerminals.size());
    parsingTable->setColumnCount(terminals.size());

    parsingTable->setVerticalHeaderLabels(nonTerminals);
    parsingTable->setHorizontalHeaderLabels(terminals);

    parsingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    parsingTable->setSelectionMode(QAbstractItemView::NoSelection);
    parsingTable->setWordWrap(true);

    parsingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    parsingTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    auto setRule = [&](const QString& nt, const QString& t, const QString& rule) {
        int r = nonTerminals.indexOf(nt);
        int c = terminals.indexOf(t);
        if (r < 0 || c < 0) return;

        QTableWidgetItem* item = new QTableWidgetItem(rule);
        item->setTextAlignment(Qt::AlignCenter | Qt::TextWordWrap);
        item->setFlags(Qt::ItemIsEnabled);
        parsingTable->setItem(r, c, item);
    };

    setRule("S", "IDENTIFIER", "S → Stmt S");
    setRule("S", "print", "S → Stmt S");
    setRule("S", "$", "S → ε");

    setRule("Stmt", "IDENTIFIER", "Stmt → IDENTIFIER = Expr");
    setRule("Stmt", "print", "Stmt → print ( Expr )");

    for (const QString& t : {"IDENTIFIER", "NUMBER", "FUNCTION", "("})
        setRule("Expr", t, "Expr → Term Expr'");

    setRule("Expr'", "+", "Expr' → + Term Expr'");
    setRule("Expr'", "-", "Expr' → - Term Expr'");
    for (const QString& t : {")", "$"})
        setRule("Expr'", t, "Expr' → ε");

    for (const QString& t : {"IDENTIFIER", "NUMBER", "FUNCTION", "("})
        setRule("Term", t, "Term → Factor Term'");

    setRule("Term'", "*", "Term' → * Factor Term'");
    setRule("Term'", "/", "Term' → / Factor Term'");
    setRule("Term'", "%", "Term' → % Factor Term'");
    for (const QString& t : {"+", "-", ")", "$"})
        setRule("Term'", t, "Term' → ε");

    setRule("Factor", "NUMBER", "Factor → NUMBER");
    setRule("Factor", "IDENTIFIER", "Factor → IDENTIFIER");
    setRule("Factor", "FUNCTION", "Factor → FUNCTION ( Expr )");
    setRule("Factor", "(", "Factor → ( Expr )");

    parsingTable->resizeRowsToContents();
    parsingTable->resizeColumnsToContents();
}






