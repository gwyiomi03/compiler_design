#pragma once

#include <QMainWindow>
#include <QTabWidget>

// Include the headers for the two visualizer modules
// Assuming they are located in the same include path or 'gui/'
#include "LexicalGUI.h"
#include "SyntacticGUI.h"
#include "NFADiagramView.h"
#include "ProjectOverview.h"

// Forward-declare to avoid any potential name lookup issues
class ProjectOverview;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QTabWidget* tabWidget;
    LexicalVisualizer* lexicalVisualizer;
    SyntacticVisualizer* syntacticVisualizer;
    NFADiagramView* nfaTab;
    ProjectOverview* projectOverview;

    void setupTabs();
};