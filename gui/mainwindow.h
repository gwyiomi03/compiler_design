#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QTreeWidget> // New include for tree visualization
#include "../parser.h"    // Required for ParseNode structure

// Forward declaration for the global analysis function
void performAnalysis(const QString& inputCode, QString& tokenOutput, QString& consoleOutput, QTreeWidget* parseTreeWidget);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void analyzeButtonClicked();

private:
    // UI elements
    QTextEdit *codeInput;
    QTextBrowser *tokenOutput;
    QTextBrowser *consoleOutput; // For capturing and showing console prints/errors
    QTreeWidget *parseTreeOutput; // For showing the structured Parse Tree
    QPushButton *analyzeButton;

    // Helper to set up the layout
    void setupUi();
};

#endif // MAINWINDOW_H