#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QStyleFactory>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Compiler Visualizer");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    setStyleSheet(
        "QMainWindow { background-color: #f0f0f0; }"
        "QTabWidget::pane { border: 1px solid #ccc; border-radius: 4px; background-color: #ffffff; }"
        "QTabBar::tab { background-color: #e0e0e0; padding: 10px 20px; border: 1px solid #ccc; border-bottom: none; border-radius: 4px 4px 0 0; font-weight: bold; color: #333; }"
        "QTabBar::tab:selected { background-color: #ffffff; color: #000; }"
        "QTabBar::tab:hover { background-color: #d0d0d0; }"
        "QWidget { font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; }"
    );


    QApplication::setStyle(QStyleFactory::create("Fusion"));


    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);
    setupTabs();
}

void MainWindow::setupTabs() {
    projectOverview = new ProjectOverview(this);
    tabWidget->addTab(projectOverview, "Project Overview");

    nfaTab = new NFADiagramView(this);
    tabWidget->addTab(nfaTab, "NFA Diagram");

    lexicalVisualizer = new LexicalVisualizer(this);
    tabWidget->addTab(lexicalVisualizer, "Lexical Analysis");

    syntacticVisualizer = new SyntacticVisualizer(this);
    tabWidget->addTab(syntacticVisualizer, "Syntactic Analysis");


    connect(lexicalVisualizer, 
        &LexicalVisualizer::tokensReady, 
        syntacticVisualizer, 
        &SyntacticVisualizer::receiveTokens);

}
