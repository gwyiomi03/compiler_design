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
#include <QRegularExpression>

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
    QString drawArrow(PDAStateNode* from, PDAStateNode* to, const QString& label, bool labelAlongLine = false);
    QString drawSelfLoop(PDAStateNode* node, QString label);
    void drawNonTerminalLoop(PDAStateNode* node, const QStringList& productions, double baseLoopHeight, double loopSpacing);
    void drawTerminalLoop(PDAStateNode* node, const QStringList& terminals);
    QString drawCurlyReturn(PDAStateNode* from, PDAStateNode* to);
    void highlightEdge(const QString& labelKey);
    
    bool hasPendingEdges() const {
        return pendingEdgeIndex < pendingEdges.size();
    }

    void stepPendingEdge();
    void clearAllHighlights();

    
    
                               


private:
    void setupGraph();
    void displayGrammar();
    void setupParsingTable();


    QMap<QString, PDAStateNode*> nodes;
    QMap<QString, QGraphicsItem*> transitions; 
    QGraphicsScene* scene;
    QTextEdit* grammarText; 
    QTableWidget* parsingTable;
    PDAStateNode* createNodeWithLabel(const QString& label, double x, double y, double w = 50, double h = 50);
    QMap<PDAStateNode*, QString> nodeLabels;
    QStringList pendingEdges;
    int pendingEdgeIndex = 0;

    // Edge id counter for deterministic ids
    int edgeCounter = 0;
    QString makeEdgeId(const QString& base);


    // Mapping by unique edge ID
    QMap<QString, QGraphicsTextItem*> transitionLabelsById;
    QMap<QString, QGraphicsPathItem*> transitionPathsById; 

    // Map from visible label text to list of edge IDs that carry that label
    QMap<QString, QStringList> labelTextToIds;

    // Map from parser "Expand ..." action string to ordered list of edge IDs (micro-steps)
    QMap<QString, QStringList> productionEdges;


};

#endif
