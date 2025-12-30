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



void PDAVisualizer::updateVisualization(const QString& currentState, const QString& inputSymbol, 
                                        const QString& stackTop, const QString& action) {
for (auto& node : nodes) node->setHighlighted(false);

    // 2. Reset all Labels and Paths to default (Black/Thin)
    for (auto* label : transitionLabels) {
        label->setDefaultTextColor(Qt::black);
        label->setZValue(1);
        QFont f = label->font(); f.setBold(false); label->setFont(f);
    }
    for (auto* path : transitionPaths) {
        path->setPen(QPen(Qt::black, 1)); 
        path->setZValue(0);
    }

    // 3. Highlight Node
    if (nodes.contains(currentState)) nodes[currentState]->setHighlighted(true);
    
    // 4. Highlight the specific Edge and Label
    QString lookupAction = action;
    if (action.startsWith("Match ")) {
        QString sym = action.mid(6);
        lookupAction = sym + ", " + sym + " → ε";
    }

    if (transitionLabels.contains(lookupAction)) {
        // Highlight Label (Red and Bold)
        QGraphicsTextItem* lbl = transitionLabels[lookupAction];
        lbl->setDefaultTextColor(Qt::red);
        lbl->setZValue(10);
        QFont f = lbl->font(); f.setBold(true); lbl->setFont(f);

        // Highlight Arc (Red and Thick)
        if (transitionPaths.contains(lookupAction)) {
            QGraphicsPathItem* pth = transitionPaths[lookupAction];
            pth->setPen(QPen(Qt::red, 3)); 
            pth->setZValue(5);
        }
    }
    scene->update();
}

void PDAVisualizer::setupGraph() {
    scene->clear();
    nodes.clear();

    // --- Create Nodes ---
    auto* q0 = new PDAStateNode("q0");          
    auto* q1 = new PDAStateNode("q1");          
    auto* q2 = new PDAStateNode("q2");          
    auto* q3 = new PDAStateNode("q3", true);          
    


    // --- Set Sizes ---
    q0->setRect(-25, -25, 50, 50);
    q1->setRect(-25, -25, 50, 50);
    q3->setRect(-25, -25, 50, 50);
    q2->setRect(-75, -75, 150, 150);  
    q2->setTextPos(-15, -15);

    // --- Set Positions ---
    q0->setPos(100, 100);
    q1->setPos(250, 100);
    q2->setPos(450, 100);
    q3->setPos(700, 100);

    // --- Store Nodes ---
    nodes["q0"] = q0;
    nodes["q1"] = q1;
    nodes["q2"] = q2;
    nodes["q3"] = q3;


    // --- Add to Scene ---
    scene->addItem(q0);
    scene->addItem(q1);
    scene->addItem(q2);
    scene->addItem(q3);


    // --- Draw Linear Arrows ---
    drawArrow(q0, q1, "ε, ε → $");
    drawArrow(q1, q2, "ε, $ → S");
    drawArrow(q2, q3, "ε, $ → ε");

    // --- Draw Self-Loops on q4 ---

    // 1. Terminals above
    QStringList terminals = {
        "IDENTIFIER, IDENTIFIER → ε",
        "NUMBER, NUMBER → ε",
        "+, + → ε",
        "-, - → ε",
        "*, * → ε",
        "/, / → ε",
        "=, = → ε",
        "(, ( → ε",
        "), ) → ε"
    };
    drawTerminalLoop(q2, terminals);

    // 2. Nonterminal groups below q4
    drawNonTerminalLoop(q2, {"ε, StmtList → Stmt StmtList", "ε, StmtList → ε"}, 70);
    drawNonTerminalLoop(q2, {"ε, Stmt → AssignStmt ", "ε, Stmt → PrintStmt"}, 170);
    drawNonTerminalLoop(q2, {"ε, AssignStmt → IDENTIFIER = Expr"}, 290);
    drawNonTerminalLoop(q2, {"ε, PrintStmt → print ( Expr )"}, 370);
    drawNonTerminalLoop(q2, {"ε, Expr → Term ExprPrime"}, 450);
    drawNonTerminalLoop(q2, {"ε, ExprPrime → + Term ExprPrime", "ε, ExprPrime → - Term ExprPrime", "ε, ExprPrime → ε"}, 570);
    drawNonTerminalLoop(q2, {"ε, Term → Factor TermPrime"}, 710);
    drawNonTerminalLoop(q2, {"ε, TermPrime → * Factor TermPrime", "ε, TermPrime → / Factor TermPrime", "ε, TermPrime → ε"}, 850);
    drawNonTerminalLoop(q2, {"ε, Factor → Number", "ε, Factor → IDENTIFIER", "ε, Factor → FUNCTION ( Expr )", "ε, Factor → ( Expr )"}, 1000);


}


void PDAVisualizer::drawArrow(PDAStateNode* from, PDAStateNode* to, const QString& label) {
    QPointF fromCenter = from->pos();
    QPointF toCenter   = to->pos();
    QPointF vec = toCenter - fromCenter;
    double length = std::sqrt(vec.x()*vec.x() + vec.y()*vec.y());
    if (length == 0) return;

    QPointF unitVec = vec / length;

    double fromRadius = from->rect().width() / 2.0;
    double toRadius   = to->rect().width() / 2.0;

    // Start and end points exactly at the edge of nodes
    QPointF startPoint = fromCenter + unitVec * fromRadius;
    QPointF endPoint   = toCenter - unitVec * toRadius;  // no extra subtraction

    // Draw line
    QGraphicsLineItem* line = new QGraphicsLineItem(QLineF(startPoint, endPoint));
    line->setPen(QPen(Qt::black, 2));
    scene->addItem(line);

    QPainterPath arrowPath;
    arrowPath.moveTo(startPoint);
    arrowPath.lineTo(endPoint);
    QGraphicsPathItem* pathItem = scene->addPath(arrowPath, QPen(Qt::black, 2));


    // Draw arrowhead
    double arrowSize = 10.0;
    double angle = std::atan2(endPoint.y() - startPoint.y(), endPoint.x() - startPoint.x());

    QPolygonF arrowHead;
    arrowHead << endPoint
              << QPointF(endPoint.x() - arrowSize * std::cos(angle - M_PI/6),
                         endPoint.y() - arrowSize * std::sin(angle - M_PI/6))
              << QPointF(endPoint.x() - arrowSize * std::cos(angle + M_PI/6),
                         endPoint.y() - arrowSize * std::sin(angle + M_PI/6));
    scene->addPolygon(arrowHead, QPen(Qt::black), QBrush(Qt::black));

    // Draw label at midpoint above the line
    QPointF labelPos = (startPoint + endPoint) / 2 - QPointF(0, 20);
    QGraphicsTextItem* textItem = scene->addText(label);
    textItem->setFont(QFont("Arial", 10));
    textItem->setPos(labelPos - QPointF(textItem->boundingRect().width()/2, 0));

    transitionPaths[label] = pathItem;
    transitionLabels[label] = textItem;
}


void PDAVisualizer::drawTerminalLoop(PDAStateNode* node, const QStringList& terminals){
if (terminals.isEmpty()) return;

    QPointF center = node->pos(); 
    double radius = node->rect().width() / 2.0; // 50 for q4
    double loopHeight = 50.0; 

    // Calculate circumference points: 240 deg (top-left) and 300 deg (top-right)
    QPointF startPoint(center.x() + radius * std::cos(240 * M_PI / 180.0), 
                       center.y() + radius * std::sin(240 * M_PI / 180.0));
    QPointF endPoint(center.x() + radius * std::cos(300 * M_PI / 180.0), 
                     center.y() + radius * std::sin(300 * M_PI / 180.0));
    
    // Control point strictly above the node center
    QPointF controlPoint(center.x(), center.y() - radius - loopHeight);

    QPainterPath path;
    path.moveTo(startPoint);
    path.quadTo(controlPoint, endPoint);

    QGraphicsPathItem* pathItem = scene->addPath(path, QPen(Qt::black, 2));
    pathItem->setZValue(0);

    // Arrowhead at the end point on the circumference
    double angle = std::atan2(endPoint.y() - controlPoint.y(), endPoint.x() - controlPoint.x());
    QPolygonF arrowHead;
    arrowHead << endPoint 
              << QPointF(endPoint.x() - 10 * cos(angle - M_PI/6), endPoint.y() - 10 * sin(angle - M_PI/6))
              << QPointF(endPoint.x() - 10 * cos(angle + M_PI/6), endPoint.y() - 10 * sin(angle + M_PI/6));
    scene->addPolygon(arrowHead, QPen(Qt::black), QBrush(Qt::black));

    // Stacked labels above the loop
    for (int i = 0; i < terminals.size(); ++i) {
        QString labelText = terminals[i];
        auto* text = scene->addText(terminals[i]);
        text->setPos(center.x() - (text->boundingRect().width() / 2), 
                     controlPoint.y() - ((terminals.size() - i) * 15));

        transitionLabels[labelText] = text;
        transitionPaths[labelText] = pathItem;            
    }


}


void PDAVisualizer::drawNonTerminalLoop(PDAStateNode* node, const QStringList& group, double loopHeight)
{
    if (group.isEmpty()) return;

    QPointF center = node->pos();
    double radius = node->rect().width() / 2.0; 

    // Calculate circumference points: 120 deg (bottom-left) and 60 deg (bottom-right)
    QPointF startPoint(center.x() + radius * std::cos(140 * M_PI / 180.0), 
                       center.y() + radius * std::sin(140 * M_PI / 180.0));
    QPointF endPoint(center.x() + radius * std::cos(30 * M_PI / 180.0), 
                     center.y() + radius * std::sin(30 * M_PI / 180.0));
    
    QPointF controlPoint(center.x(), center.y() + radius + loopHeight);

    QPainterPath path;
    path.moveTo(startPoint);
    path.quadTo(controlPoint, endPoint);

    QGraphicsPathItem* pathItem = scene->addPath(path, QPen(Qt::black, 1));
    pathItem->setZValue(0);

    // Arrowhead logic
    double angle = std::atan2(endPoint.y() - controlPoint.y(), endPoint.x() - controlPoint.x());
    QPolygonF arrowHead;
    arrowHead << endPoint 
              << QPointF(endPoint.x() - 10 * cos(angle - M_PI/6), endPoint.y() - 10 * sin(angle - M_PI/6))
              << QPointF(endPoint.x() - 10 * cos(angle + M_PI/6), endPoint.y() - 10 * sin(angle + M_PI/6));
    scene->addPolygon(arrowHead, QPen(Qt::black), QBrush(Qt::black));

    // Labels positioned next to the bottom loop
    double padding = 12.0; 
    for (int i = 0; i < group.size(); ++i) {
        QString labelText = group[i];
        auto* text = scene->addText(group[i]);
        text->setFont(QFont("Arial", 9));

        // - move left, + move right
        double xPos = center.x() - 10;

        double arcMidPointY = (center.y() + radius) + (loopHeight / 2.0);
        double blockCenterOffset = (group.size() * padding) / 2.0;
        
        double yPos = arcMidPointY - blockCenterOffset + (i * padding);

        text
        ->setPos(xPos, yPos);
        transitionLabels[labelText] = text;
        transitionPaths[labelText] = pathItem;
    }
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


void PDAVisualizer::displayGrammar() {
    // We use HTML to create a "Code Editor" look within the QTextEdit
    QString html = R"(
        <div style="background-color: #1e1e1e; padding: 20px; border-radius: 10px; font-family: 'Consolas', 'Courier New', monospace;">
            <h2 style="color: #4fc3f7; margin-top: 0; border-bottom: 1px solid #333; padding-bottom: 10px;">Context-Free Grammar</h2>
            <table style="color: #d4d4d4; font-size: 13pt; line-height: 1.6;">
                <tr><td style="color: #ce9178; padding-right: 20px;">S</td><td style="color: #569cd6;">&rarr;</td><td>StmtList</td></tr>
                <tr><td style="color: #ce9178;">StmtList</td><td style="color: #569cd6;">&rarr;</td><td>Stmt StmtList <span style="color: #b5cea8;">|</span> &epsilon;</td></tr>
                <tr><td style="color: #ce9178;">Stmt</td><td style="color: #569cd6;">&rarr;</td><td>AssignStmt <span style="color: #b5cea8;">|</span> PrintStmt</td></tr>
                <tr><td style="color: #ce9178;">AssignStmt</td><td style="color: #569cd6;">&rarr;</td><td><span style="color: #4ec9b0;">IDENTIFIER</span> <span style="color: #d4d4d4;">=</span> Expr</td></tr>
                <tr><td style="color: #ce9178;">PrintStmt</td><td style="color: #569cd6;">&rarr;</td><td><span style="color: #9cdcfe;">print</span> ( Expr )</td></tr>
                <tr><td style="color: #ce9178;">Expr</td><td style="color: #569cd6;">&rarr;</td><td>Term ExprPrime</td></tr>
                <tr><td style="color: #ce9178;">ExprPrime</td><td style="color: #569cd6;">&rarr;</td><td><span style="color: #d4d4d4;">+</span> Term ExprPrime <span style="color: #b5cea8;">|</span> <span style="color: #d4d4d4;">-</span> Term ExprPrime <span style="color: #b5cea8;">|</span> &epsilon;</td></tr>
                <tr><td style="color: #ce9178;">Term</td><td style="color: #569cd6;">&rarr;</td><td>Factor TermPrime</td></tr>
                <tr><td style="color: #ce9178;">TermPrime</td><td style="color: #569cd6;">&rarr;</td><td><span style="color: #d4d4d4;">*</span> Factor TermPrime <span style="color: #b5cea8;">|</span> <span style="color: #d4d4d4;">/</span> Factor TermPrime <span style="color: #b5cea8;">|</span> &epsilon;</td></tr>
                <tr><td style="color: #ce9178;">Factor</td><td style="color: #569cd6;">&rarr;</td><td><span style="color: #b5cea8;">NUMBER</span> <span style="color: #b5cea8;">|</span> <span style="color: #4ec9b0;">IDENTIFIER</span> <span style="color: #b5cea8;">|</span> <span style="color: #c9a24eff;">FUNCTION</span> ( Expr ) <span style="color: #b5cea8;">|</span> ( Expr )</td></tr>
            </table>
        </div>
    )";

    grammarText->setHtml(html);
    grammarText->setReadOnly(true);
    
    // Set a dark background for the widget itself to match the HTML block
    grammarText->setStyleSheet("QTextEdit { background-color: #1e1e1e; border: 2px solid #3f72af; border-radius: 8px; }");
}


void PDAVisualizer::setupParsingTable() {
    QStringList nonTerminals = { "S", "StmtList", "Stmt", "AssignStmt", "PrintStmt", "Expr", "ExprPrime", "Term", "TermPrime", "Factor" };
    QStringList terminals = { "IDENTIFIER", "NUMBER", "print", "FUNCTION", "=", "+", "-", "*", "/", "(", ")", "$" };

    parsingTable->clear();
    parsingTable->setRowCount(nonTerminals.size());
    parsingTable->setColumnCount(terminals.size());

    parsingTable->setVerticalHeaderLabels(nonTerminals);
    parsingTable->setHorizontalHeaderLabels(terminals);

    parsingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    parsingTable->setSelectionMode(QAbstractItemView::NoSelection);

    // Enable wrapping & auto sizing
    parsingTable->setWordWrap(true);
    parsingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    parsingTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    parsingTable->horizontalHeader()->setStretchLastSection(true);
    parsingTable->setStyleSheet(
        "QTableWidget { background-color: #ffffff; gridline-color: #dcdcdc; font-size: 10pt; }"
        "QHeaderView::section { background-color: #3f72af; color: white; font-weight: bold; padding: 4px; }"
    );

    // Lambda to helper fill the table
    auto setRule = [&](const QString& nt, const QString& t, const QString& rule) {
        int r = nonTerminals.indexOf(nt);
        int c = terminals.indexOf(t);

        if (r < 0 || c < 0) return;

        QTableWidgetItem* item = new QTableWidgetItem(rule);
        item->setTextAlignment(Qt::AlignCenter | Qt::TextWordWrap);
        item->setForeground(QBrush(QColor("#2c3e50")));
        item->setFlags(Qt::ItemIsEnabled);

        parsingTable->setItem(r, c, item);
    };

    // --- Fill Table Rules (Based on CFG in PDAVIEW.cpp) ---
    // S -> StmtList
    for(const QString& t : {"IDENTIFIER", "print", "$"}) setRule("S", t, "S → StmtList");

    // StmtList -> Stmt StmtList | ε
    setRule("StmtList", "IDENTIFIER", "StmtList → Stmt StmtList");
    setRule("StmtList", "print", "StmtList → Stmt StmtList");
    setRule("StmtList", "$", "StmtList → ε");

    // Stmt -> AssignStmt | PrintStmt
    setRule("Stmt", "IDENTIFIER", "Stmt → AssignStmt");
    setRule("Stmt", "print", "Stmt → PrintStmt");

    // AssignStmt -> IDENTIFIER = Expr
    setRule("AssignStmt", "IDENTIFIER", "AssignStmt → ID = Expr");

    // PrintStmt -> print ( Expr )
    setRule("PrintStmt", "print", "PrintStmt → print(Expr)");

    // Expr -> Term ExprPrime
    for(const QString& t : {"IDENTIFIER", "NUMBER", "FUNCTION", "("}) setRule("Expr", t, "Expr → Term ExprP");

    // ExprPrime -> + Term ExprPrime | - Term ExprPrime | ε
    setRule("ExprPrime", "+", "ExprP → + Term ExprP");
    setRule("ExprPrime", "-", "ExprP → - Term ExprP");
    for(const QString& t : {")", "$"}) setRule("ExprPrime", t, "ExprP → ε");

    // Term -> Factor TermPrime
    for(const QString& t : {"IDENTIFIER", "NUMBER", "FUNCTION", "("}) setRule("Term", t, "Term → Factor TermP");

    // TermPrime -> * Factor TermPrime | / Factor TermPrime | ε
    setRule("TermPrime", "*", "TermP → * Factor TermP");
    setRule("TermPrime", "/", "TermP → / Factor TermP");
    for(const QString& t : {"+", "-", ")", "$"}) setRule("TermPrime", t, "TermP → ε");

    // Factor -> NUMBER | IDENTIFIER | FUNCTION(Expr) | (Expr)
    setRule("Factor", "NUMBER", "Factor → NUMBER");
    setRule("Factor", "IDENTIFIER", "Factor → ID");
    setRule("Factor", "FUNCTION", "Factor → FUNC(Expr)");
    setRule("Factor", "(", "Factor → (Expr)");
    
    parsingTable->resizeRowsToContents();
    parsingTable->resizeColumnsToContents();
}





