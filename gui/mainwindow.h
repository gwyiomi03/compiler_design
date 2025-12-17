#pragma once

#include <QMainWindow>
#include <QTabWidget>

// Include the headers for the two visualizer modules
// Assuming they are located in the same include path or 'gui/'
#include "LexicalGUI.h"
#include "SyntacticGUI.h"
#include "NFADiagramView.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QTabWidget* tabWidget;
    LexicalVisualizer* lexicalVisualizer;
    SyntacticVisualizer* syntacticVisualizer;
    NFADiagramView* nfaTab;

    void setupTabs();
};