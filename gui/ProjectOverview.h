#ifndef PROJECTOVERVIEW_H
#define PROJECTOVERVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QTableWidget>
#include <QHeaderView>

class ProjectOverview : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectOverview(QWidget *parent = nullptr);

private:
    void setupUI();
    void setupLookupTable();
    void setupTokenTable();

    QVBoxLayout *mainLayout;
    QTableWidget *lookupTable;
    QTableWidget *tokenTable;
};

#endif // PROJECTOVERVIEW_H