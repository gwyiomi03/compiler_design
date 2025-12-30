#include "LexicalGUI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QLabel>
#include <QTextCursor>
#include <QMessageBox>
#include <cmath>
#include <QDebug>
#include <QMap>
#include <QQueue>
#include <QSet>
#include <QList>
#include <QPainterPath>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSplitter>
#include <cctype>
#include <QApplication>
#include <QStyleFactory>
#include <QGraphicsDropShadowEffect>
#include "CodeEditor.h"


// --- Constants for Drawing ---
static constexpr double STATE_RADIUS = 25.0;
static constexpr double ARROW_SIZE   = 10.0;
static constexpr double LAYER_X_GAP  = 160.0;
static constexpr double LAYER_Y_GAP  = 100.0;

// Transition colors
static const QColor TRANSITION_NORMAL_COLOR = QColor(100, 100, 100); // Softer gray
static const QColor TRANSITION_HIGHLIGHT_COLOR = QColor(255, 69, 0); // Orange-red for highlight

// State colors
static const QColor STATE_NORMAL_COLOR    = QColor(173, 216, 230); // Light blue
static const QColor STATE_ACCEPTING_COLOR = QColor(144, 238, 144); // Light green
static const QColor STATE_HIGHLIGHT_COLOR = QColor(255, 255, 102); // Light yellow
static const QColor STATE_DEAD_COLOR      = QColor(255, 182, 193); // Light red for dead state


QGraphicsItem* drawArrowHead(QGraphicsScene* scene, const QPointF& tip, const QPointF& direction) {
    QPainterPath path;
    path.moveTo(tip);

    QPointF unitDir = direction / std::sqrt(QPointF::dotProduct(direction, direction));
    QPointF perp(unitDir.y(), -unitDir.x());
    
    QPointF wing1 = tip - unitDir * ARROW_SIZE + perp * (ARROW_SIZE / 2.0);
    QPointF wing2 = tip - unitDir * ARROW_SIZE - perp * (ARROW_SIZE / 2.0);

    path.lineTo(wing1);
    path.lineTo(wing2);
    path.lineTo(tip);

   return scene->addPolygon(path.toFillPolygon(), QPen(Qt::black), QBrush(Qt::black));
}


// ------------------- StateNode -------------------
StateNode::StateNode(int id, TokenType type, bool isAccepting, bool isDead = false)
    : QGraphicsEllipseItem(-STATE_RADIUS, -STATE_RADIUS, 2 * STATE_RADIUS, 2 * STATE_RADIUS),
      stateId(id), tokenType(type), isDead(isDead) {

    setPen(QPen(Qt::black, 2));

    // Set brush color based on state type
    if (isDead) {
        setBrush(QBrush(STATE_DEAD_COLOR));
    } else {
        setBrush(QBrush(isAccepting ? STATE_ACCEPTING_COLOR : STATE_NORMAL_COLOR));
    }

    // Double circle for accepting states (not dead)
    if (isAccepting && !isDead) {
        QGraphicsEllipseItem* inner = new QGraphicsEllipseItem(
            -STATE_RADIUS + 5, -STATE_RADIUS + 5,
            2 * STATE_RADIUS - 10, 2 * STATE_RADIUS - 10,
            this
        );
        inner->setPen(QPen(Qt::black, 2));
    }

    // Label
    QString label = QString("S%1").arg(id);
    QGraphicsTextItem* labelText = new QGraphicsTextItem(label, this);
    qreal textWidth = labelText->boundingRect().width();
    qreal textHeight = labelText->boundingRect().height();
    labelText->setPos(-textWidth / 2, -textHeight / 2);
}

void StateNode::setHighlighted(bool highlight) {
    if (isDead) {
        setBrush(QBrush(STATE_DEAD_COLOR));
        return;
    }

    setBrush(QBrush(highlight ? STATE_HIGHLIGHT_COLOR
                              : (isAccepting ? STATE_ACCEPTING_COLOR : STATE_NORMAL_COLOR)));
}


// --- LexicalVisualizer Implementation ---

LexicalVisualizer::LexicalVisualizer(QWidget *parent) : QWidget(parent) {
    // 1. IDENTICAL STYLESHEET (Copied from Syntactic)
    setStyleSheet(
        "QWidget { font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #e3f2fd, stop:1 rgba(248, 247, 206, 1)); "
        "color: #000000; }"
        
        "QLabel { font-weight: bold; font-size: 11pt; background: transparent; }"
        
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4caf50, stop:1 #388e3c); "
        "color: #ffffff; border: none; padding: 10px 20px; border-radius: 8px; font-weight: bold; font-size: 11pt; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66bb6a, stop:1 #4caf50); }"
        "QPushButton:disabled { background: #cccccc; color: #666666; }"

        "QTextEdit { border: 2px solid #1f2020; border-radius: 8px; padding: 6px; background-color: #ffffff; color: #000000; }"
        "QTableWidget { border: 2px solid #2196f3; border-radius: 8px; background-color: #ffffff; color: #000000; }"
        "QHeaderView::section { background: #1976d2; color: #ffffff; font-weight: bold; }"
        
        "QGraphicsView { border: 2px solid #2196f3; border-radius: 8px; background-color: #ffffff; }"
    );

    // 2. APPLY IDENTICAL EFFECTS (Fusion Style & Shadows)
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    setAttribute(Qt::WA_StyledBackground, true);
    
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(2, 2);
    setGraphicsEffect(shadow);

    // 3. LAYOUT (Matching Syntactic's Splitter structure)
    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(15, 15, 15, 15);
    rootLayout->setSpacing(15);

    QSplitter* mainHorizontalSplitter = new QSplitter(Qt::Horizontal);

    // --- LEFT COLUMN ---
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);

    QLabel* inputLabel = new QLabel("Input String:");
    inputEditor = new CodeEditor(this);
    inputEditor->setPlaceholderText("e.g. x = 42 + y;");
    inputEditor->setMaximumHeight(120);

    inputEditor->setStyleSheet(
        "CodeEditor {"
        "  background-color: white;"
        "  border: 2px solid #1E88E5;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
        "CodeEditor:focus {"
        "  border: 2px solid #1565C0;"
        "}"
    );

    QLabel* tokenLabel = new QLabel("Token List:");
    tokenTableWidget = new QTableWidget(0, 2);
    tokenTableWidget->setHorizontalHeaderLabels({"Lexeme", "Token Type"});
    tokenTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tokenTableWidget->setAlternatingRowColors(true);


    QHBoxLayout* buttonLayout = new QHBoxLayout();
    tokenizeButton = new QPushButton("Start Tokenization");
    playPauseButton = new QPushButton("Play");
    resetButton = new QPushButton("Reset");
    buttonLayout->addWidget(tokenizeButton);
    buttonLayout->addWidget(playPauseButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->setSpacing(15);

    leftLayout->addWidget(inputLabel);
    leftLayout->addWidget(inputEditor);
    leftLayout->addLayout(buttonLayout); 
    leftLayout->addWidget(tokenLabel);
    leftLayout->addWidget(tokenTableWidget);

    // --- RIGHT COLUMN ---
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(15);

    dfaScene = new QGraphicsScene(this);
    dfaView = new QGraphicsView(dfaScene);
    dfaView->setRenderHint(QPainter::Antialiasing);



    rightLayout->addWidget(new QLabel("DFA State Diagram:"));
    rightLayout->addWidget(dfaView);

    // 4. FINALIZE
    mainHorizontalSplitter->addWidget(leftWidget);
    mainHorizontalSplitter->addWidget(rightWidget);
    mainHorizontalSplitter->setStretchFactor(1, 2);
    rootLayout->addWidget(mainHorizontalSplitter);

    setupDFA();
    traversalTimer = new QTimer(this);
    traversalTimer->setInterval(400); 
    
    currentHighlightedTransition = nullptr; 
    traversalIndex = 0;
    isTraversing = false;

    connect(traversalTimer, &QTimer::timeout, this, &LexicalVisualizer::autoTraverse);
    connect(tokenizeButton, &QPushButton::clicked, this, &LexicalVisualizer::tokenizeClicked);
    connect(playPauseButton, &QPushButton::clicked, this, &LexicalVisualizer::playPauseClicked);
    connect(resetButton, &QPushButton::clicked, this, &LexicalVisualizer::resetClicked);
    connect(inputEditor, &CodeEditor::textChanged, this, &LexicalVisualizer::inputTextChanged);


    drawDFA();
    if (!dfa.allStates.empty()) highlightDFAState(dfa.start->id);
}


void LexicalVisualizer::setupDFA() {
    // 1. Create NFAs for all token types
    std::vector<NFA> nfas;
    nfas.push_back(createIdentifierNFA());
    nfas.push_back(createNumberNFA());
    nfas.push_back(createSingleCharNFA('+', PLUS));
    nfas.push_back(createSingleCharNFA('-', MINUS));
    nfas.push_back(createSingleCharNFA('*', MULTIPLY));
    nfas.push_back(createSingleCharNFA('/', DIVIDE));
    nfas.push_back(createSingleCharNFA('=', ASSIGN));
    nfas.push_back(createSingleCharNFA('(', LPAREN));
    nfas.push_back(createSingleCharNFA(')', RPAREN));
    
    // 2. Combine all NFAs into one master NFA
    NFA masterNFA = combineNFAs(nfas);

    // 3. Convert master NFA to DFA
    dfa = convertNFAtoDFA(masterNFA);
}


QGraphicsItemGroup* LexicalVisualizer::drawDFATransition(DFAState* source, DFAState* target, const QString& labelText, 
                                          const QPointF& sourcePos, const QPointF& targetPos, 
                                          int offsetIndex, bool isLoop) {
    QPen pen(TRANSITION_NORMAL_COLOR, 2);
    QGraphicsItemGroup* group = new QGraphicsItemGroup();

    if (isLoop) {
        // --- 1. Define Loop Geometry (Omitted setup code) ---
        const double R = STATE_RADIUS;
        const double LOOP_HEIGHT_REL = 2.0;
        
        auto polarToCartesian = [R](double angleDeg, const QPointF& center) -> QPointF {
            double angleRad = angleDeg * M_PI / 180.0;
            return center + QPointF(R * cos(angleRad), R * sin(angleRad)); 
        };
        
        const double START_ANGLE_DEG = -45.0; 
        const double END_ANGLE_DEG   = 45.0;   
        
        QPointF startPoint = polarToCartesian(START_ANGLE_DEG, sourcePos);
        QPointF endPoint   = polarToCartesian(END_ANGLE_DEG, sourcePos);
        
        QPointF controlPoint = sourcePos + QPointF(R * LOOP_HEIGHT_REL, -R * LOOP_HEIGHT_REL);
        
        // --- 2. Draw the Path (Quadratic Bezier Curve) ---
        QPainterPath path;
        path.moveTo(startPoint);
        path.quadTo(controlPoint, endPoint); 
        
        QGraphicsPathItem* pathItem = new QGraphicsPathItem(path); // FIX: Use new
        pathItem->setPen(pen);
        group->addToGroup(pathItem);

        // --- 3. Draw Arrowhead at the End Point ---
        QPointF arrowDirection = endPoint - controlPoint;
        
        QGraphicsItem* arrowItem = createArrowHeadItem(endPoint, arrowDirection); // FIX: Use creator helper
        group->addToGroup(arrowItem);

        // --- 4. Draw Label ---
        QPointF labelPos = controlPoint + QPointF(3.0, -3.0); 
        QGraphicsTextItem* textItem = new QGraphicsTextItem(labelText, nullptr); // FIX: Use new
        textItem->setFont(QFont("Arial", 10)); // Set font manually
        
        textItem->setPos(labelPos - QPointF(textItem->boundingRect().width() / 2, textItem->boundingRect().height() / 2));
        group->addToGroup(textItem);
        
        dfaScene->addItem(group);
        return group;
    }

    // Curved transition (non-loop)
    QPointF start = sourcePos;
    QPointF end = targetPos;
    QPointF dirVec = end - start;
    double length = std::sqrt(QPointF::dotProduct(dirVec, dirVec));
    if (length == 0.0) { delete group; return nullptr; }

    QPointF unitDir = dirVec / length;
    QPointF perp(unitDir.y(), -unitDir.x());
    double curveOffset = (offsetIndex % 2 == 0 ? 1 : -1) * (offsetIndex/2 + 1) * 20.0; 
    start += unitDir * STATE_RADIUS;
    end -= unitDir * STATE_RADIUS;
    QPointF midPoint = (start + end)/2.0;
    QPointF controlPoint = midPoint + perp * curveOffset;

    QPainterPath path;
    path.moveTo(start);
    path.quadTo(controlPoint, end);
    
    QGraphicsPathItem* pathItem_nonLoop = new QGraphicsPathItem(path); 
    pathItem_nonLoop->setPen(pen);
    group->addToGroup(pathItem_nonLoop);

    QGraphicsItem* arrowItem_nonLoop = createArrowHeadItem(end, end - controlPoint);
    if (arrowItem_nonLoop) group->addToGroup(arrowItem_nonLoop);
    
    QPointF labelPos = controlPoint + perp * 2;
    QGraphicsTextItem* label = new QGraphicsTextItem(labelText, nullptr); // FIX: Use new
    label->setFont(QFont("Arial", 10)); 
    label->setPos(labelPos.x() - label->boundingRect().width()/2,
                  labelPos.y() - label->boundingRect().height()/2);
    group->addToGroup(label);
    
    dfaScene->addItem(group);
    return group;
}


static constexpr double MIN_NODE_SPACING_Y = 90.0;
static constexpr double MIN_NODE_SPACING_X = 180.0;
static constexpr double DEAD_STATE_OFFSET_Y = 80.0;

void LexicalVisualizer::drawDFA() {
    dfaScene->clear();
    stateNodes.clear();

    if (dfa.allStates.empty() || !dfa.start) return;

    DFAState* deadStatePtr = nullptr;
    for (DFAState* s : dfa.allStates) {
        if (s->id == 15) {  // or some marker for dead state
            deadStatePtr = s;
            break;
        }
    }

    // 1. BFS to assign layers
    QMap<DFAState*, QPointF> positions;
    QMap<int, QList<DFAState*>> statesByLayer;
    QQueue<DFAState*> queue;
    QSet<DFAState*> visited;

    if (dfa.start && dfa.start != deadStatePtr) {
        queue.enqueue(dfa.start);
        visited.insert(dfa.start);
        statesByLayer[0].append(dfa.start);
    }

    int maxLayer = 0;
    while (!queue.isEmpty()) {
        DFAState* current = queue.dequeue();
        int currentLayer = 0;
        for (auto it = statesByLayer.begin(); it != statesByLayer.end(); ++it) {
            if (it.value().contains(current)) {
                currentLayer = it.key();
                break;
            }
        }
        maxLayer = qMax(maxLayer, currentLayer);

        for (auto const& [symbol, target] : current->transitions) {
            if (target == deadStatePtr) continue; // skip dead state here
            if (!visited.contains(target)) {
                visited.insert(target);
                statesByLayer[currentLayer + 1].append(target);
                queue.enqueue(target);
            }
        }
    }

    // 2. Position normal states
    for (int layer : statesByLayer.keys()) {
        QList<DFAState*>& layerStates = statesByLayer[layer];
        double startY = -(layerStates.size() - 1) * MIN_NODE_SPACING_Y / 2.0;
        for (int i = 0; i < layerStates.size(); ++i) {
            DFAState* state = layerStates[i];
            double x = layer * MIN_NODE_SPACING_X;
            double y = startY + i * MIN_NODE_SPACING_Y;
            positions[state] = QPointF(x, y);

            StateNode* node = new StateNode(
                state->id,
                state->tokenType,
                state->isAccepting,
                false
            );
            node->setPos(positions[state]);
            dfaScene->addItem(node);
            stateNodes[state->id] = node;
        }
    }

    // 3. Position dead state far away (only once)
    if (deadStatePtr) {
        double deadX = (maxLayer + 6) * MIN_NODE_SPACING_X; // far to the right
        double deadY = DEAD_STATE_OFFSET_Y;

        positions[deadStatePtr] = QPointF(deadX, deadY);

        StateNode* node = new StateNode(
            deadStatePtr->id,
            deadStatePtr->tokenType,
            false,   // dead state is never accepting
            true     // mark as dead
        );

        node->setPos(positions[deadStatePtr]);
        dfaScene->addItem(node);
        stateNodes[deadStatePtr->id] = node;
    }


    // 5. Build grouped transition labels
    QMap<QPair<int, int>, QString> labels;

    for (DFAState* source : dfa.allStates) {
        for (auto const& [symbol, target] : source->transitions) {
            QPair<int, int> key(source->id, target->id);
            QString& labelStr = labels[key];

            if (isdigit(static_cast<unsigned char>(symbol))) {
                if (!labelStr.contains("[0-9]"))
                    labelStr += (labelStr.isEmpty() ? "" : ",") + QString("[0-9]");
            }
            else if (isalpha(static_cast<unsigned char>(symbol)) || symbol == '_') {
                if (!labelStr.contains("[a-zA-Z_]"))
                    labelStr += (labelStr.isEmpty() ? "" : ",") + QString("[a-zA-Z_]");
            }
            else {
                QString symStr = QString(QChar(symbol));
                if (!labelStr.contains(symStr))
                    labelStr += (labelStr.isEmpty() ? "" : ",") + symStr;
            }
        }
    }


    // 6. Draw transitions
    transitionGroups.clear();
    QMap<QPair<int, int>, int> drawIndex;

    for (DFAState* source : dfa.allStates) {
        if (!positions.contains(source)) continue;

        for (auto const& [symbol, target] : source->transitions) {
            if (source == dfa.start && target == deadStatePtr) continue;
            QPair<int, int> key(source->id, target->id);
            if (!positions.contains(target) || drawIndex.contains(key)) continue;

            bool isLoop = (source == target);

            QGraphicsItemGroup* group = drawDFATransition(
                source,
                target,
                labels[key],
                positions[source],
                positions[target],
                drawIndex[key],
                isLoop
            );

            if (group)
                transitionGroups[key] = group;

            drawIndex[key]++;
        }
    }

    // -------------------------------
    // 7. Draw start arrow
    // -------------------------------
    QPointF startNodePos = positions[dfa.start];
    QPointF startArrowTip = startNodePos + QPointF(-STATE_RADIUS, 0);
    QPointF startArrowTail = startNodePos + QPointF(-STATE_RADIUS - 40, 0);

    dfaScene->addLine(
        QLineF(startArrowTail, startArrowTip),
        QPen(QColor(0, 123, 255), 3)
    );

    drawArrowHead(dfaScene, startArrowTip, QPointF(1, 0));
}


void LexicalVisualizer::highlightDFAState(int id) {
    for (auto node : stateNodes) {
        node->setHighlighted(node->stateId == id);
    }
}

// --- Slots/Event Handlers ---
void LexicalVisualizer::inputTextChanged() {
    tokenizeButton->setEnabled(true);
    playPauseButton->setEnabled(false);
}

void LexicalVisualizer::highlightInput(size_t start, size_t end) {
    QList<QTextEdit::ExtraSelection> selections;

    if (start < end) {
        QTextEdit::ExtraSelection selection;
        QTextCursor cursor(inputEditor->document());

        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);

        selection.cursor = cursor;
        selection.format.setBackground(QColor(255, 255, 153, 180)); // Softer yellow highlight

        selections.append(selection);
    }

    inputEditor->setExtraSelections(selections);
}


void LexicalVisualizer::updateTokenList(const Token& token) {
    // Get the table and current row count
    int row = tokenTableWidget->rowCount();
    tokenTableWidget->insertRow(row);

    std::string lexemeStr = token.value;
    lexemeStr.erase(lexemeStr.begin(),
        std::find_if(lexemeStr.begin(), lexemeStr.end(),
                     [](unsigned char ch){ return !std::isspace(ch); }));

    // Column 0: Lexeme
    QTableWidgetItem* lexemeItem = new QTableWidgetItem(QString::fromStdString(token.value));
    tokenTableWidget->setItem(row, 0, lexemeItem);

    // Column 1: Token Type
    QString tokenName = QString::fromStdString(getTokenName(token.type));
    QTableWidgetItem* typeItem = new QTableWidgetItem(tokenName);
    tokenTableWidget->setItem(row, 1, typeItem);

    qDebug() << "Token" << token.lexeme << "is on line" << token.line;

    // Clear previous highlight
    finalTokens.push_back(token);
}


void LexicalVisualizer::resetClicked(){
    // Stop the timer
    if (traversalTimer) traversalTimer->stop(); 

    tokenTableWidget->setRowCount(0);

    currentScanPos = 0;
    traversalIndex = 0;
    isTraversing = false;
    currentline = 1;


    // Highlight start state
    if (dfa.start) highlightDFAState(dfa.start->id);
    highlightDFATransition(-1, -1);

    playPauseButton->setEnabled(false);
    playPauseButton->setText("Play");
    
    tokenizeButton->setEnabled(true);
    highlightInput(0, 0);
}


void LexicalVisualizer::tokenizeClicked(){
    QString input = inputEditor->toPlainText();

    for(int i = 0; i < input.length(); ++i) {
        if(input[i] == '\n') qDebug() << "Found newline at index" << i;
    }


    if (input.isEmpty()) return;
    rawInputString = input;

    currentScanPos = 0;
    traversalIndex = 0;
    isTraversing = false;
    currentline = 1;


    tokenizeButton->setEnabled(false);
    playPauseButton->setEnabled(true);
    playPauseButton->setText("Pause");// Starts in play mode
    
    tokenTableWidget->setRowCount(0);
    finalTokens.clear();

    tokenTableWidget->setRowCount(0);
    highlightDFAState(dfa.start->id);
    highlightDFATransition(-1, -1);
    highlightInput(0, 0);
    
    // START THE TIMER
    traversalTimer->start();
    autoTraverse(); 
}


void LexicalVisualizer::playPauseClicked() {
    if (traversalTimer->isActive()) {
        traversalTimer->stop();
        playPauseButton->setText("Play");
    } else {
        if (currentScanPos >= inputEditor->toPlainText().size()) {
             resetClicked();
             if (inputEditor->toPlainText().isEmpty()) return;
        }
        
        traversalTimer->start();
        playPauseButton->setText("Pause");
        playPauseButton->setEnabled(true);
        
        if (!isTraversing) {
             autoTraverse();
        }
    }
}


void LexicalVisualizer::autoTraverse() {
    const QString input = inputEditor->toPlainText(); 
    string text = inputEditor->toPlainText().toStdString();

    while (currentScanPos < text.size() && std::isspace(static_cast<unsigned char>(text[currentScanPos]))) {
        if (input[currentScanPos] == '\n') {
            currentline++;
        }
        currentScanPos++;
    }


    if (currentScanPos >= input.size()) {
        traversalTimer->stop();
        playPauseButton->setText("Play");
        playPauseButton->setEnabled(false);
        highlightDFAState(-1);
        highlightDFATransition(-1, -1);

        QMessageBox::information(this, "Tokenization Complete", 
            QString("Tokenization Complete!\nTotal tokens found: %1")
            .arg(finalTokens.size()));

        emit tokensReady(finalTokens, rawInputString);
        return;
    }


    if (!isTraversing) {
        // --- PHASE 1: START SCAN FOR NEXT TOKEN ---
        
        // 1. Scan for the next token and reset traversal index
        currentResult = scanNextToken(dfa, input.toStdString(), currentScanPos, currentline);
        traversalIndex = 0;
        isTraversing = true;

        // 2. Setup for traversal
        highlightDFATransition(-1, -1);
        highlightDFAState(dfa.start->id);
        
        // Handle case where a token is found immediately or not at all (no path)
        if (currentResult.traversalPath.empty()) {
            isTraversing = false; 
        } else {
            return; 
        }
    }

    if (isTraversing) {
        // --- PHASE 2: CONTINUOUS TRAVERSAL STEP ---
        if (traversalIndex < currentResult.traversalPath.size()) {
            const auto& trace = currentResult.traversalPath[traversalIndex];
            highlightDFATransition(trace.sourceId, trace.targetId);
            highlightDFAState(trace.targetId); 

            traversalIndex++;

            return; 
        } 
  
        isTraversing = false;
    }

    highlightDFATransition(-1, -1); 
    if (currentResult.foundToken) {
        updateTokenList(currentResult.token);
        highlightInput(currentScanPos, currentResult.newPosition);
        currentScanPos = currentResult.newPosition;
    } else {
        // Error or UNKNOWN token handling
        size_t unknownEnd = currentResult.newPosition;
        Token errorToken = {
            UNKNOWN,
            input.mid(currentScanPos, unknownEnd - currentScanPos).toStdString(),
            currentline
        };
        updateTokenList(errorToken);
        highlightInput(currentScanPos, unknownEnd);
        currentScanPos = unknownEnd;
        
        highlightDFAState(-1); 
    }
}


void LexicalVisualizer::highlightDFATransition(int sourceId, int targetId) {
    if (currentHighlightedTransition) {
        setTransitionGroupColor(currentHighlightedTransition, TRANSITION_NORMAL_COLOR);
        currentHighlightedTransition = nullptr;
    }

    QPair<int, int> key(sourceId, targetId);
    if (transitionGroups.contains(key)) {
        currentHighlightedTransition = transitionGroups[key];
        setTransitionGroupColor(currentHighlightedTransition, TRANSITION_HIGHLIGHT_COLOR);
    }
}


void LexicalVisualizer::setTransitionGroupColor(QGraphicsItemGroup* group, const QColor& color) {
    QPen pen(color, 2);
    QBrush brush(color);
    for (QGraphicsItem* item : group->childItems()) {
        if (QGraphicsPathItem* pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
            pathItem->setPen(pen);
        } else if (QGraphicsPolygonItem* polyItem = dynamic_cast<QGraphicsPolygonItem*>(item)) {
            polyItem->setPen(pen);
            polyItem->setBrush(brush);
        }
    }
}


QGraphicsPolygonItem* createArrowHeadItem(const QPointF& tip, const QPointF& direction) {
    QPainterPath path;
    path.moveTo(tip);

    QPointF unitDir = direction / std::sqrt(QPointF::dotProduct(direction, direction));
    QPointF perp(unitDir.y(), -unitDir.x());
    
    QPointF wing1 = tip - unitDir * ARROW_SIZE + perp * (ARROW_SIZE / 2.0);
    QPointF wing2 = tip - unitDir * ARROW_SIZE - perp * (ARROW_SIZE / 2.0);

    path.lineTo(wing1);
    path.lineTo(wing2);
    path.lineTo(tip);

    QGraphicsPolygonItem* item = new QGraphicsPolygonItem(path.toFillPolygon());
    item->setPen(QPen(TRANSITION_NORMAL_COLOR));
    item->setBrush(QBrush(TRANSITION_NORMAL_COLOR));
    
    return item;
}







