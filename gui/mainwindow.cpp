#include "MainWindow.h"
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    
    setWindowTitle("Compiler Front-End Visualizer");
    
    // Create the central Tab Widget
    tabWidget = new QTabWidget(this);
    
    // Instantiate the two visualizers
    lexicalVisualizer = new LexicalVisualizer();
    syntacticVisualizer = new SyntacticVisualizer();
    connect(lexicalVisualizer, 
        &LexicalVisualizer::tokensReady, 
        syntacticVisualizer, 
        &SyntacticVisualizer::receiveTokens);

    nfaTab = new NFADiagramView();
    
    setupTabs();
    
    setCentralWidget(tabWidget);
}

void MainWindow::setupTabs() {
    tabWidget->addTab(lexicalVisualizer, "Lexical Analysis");
    tabWidget->addTab(syntacticVisualizer, "Syntactic Analysis");
    tabWidget->addTab(nfaTab, "NFA Diagram");
    
}