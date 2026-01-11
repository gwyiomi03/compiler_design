#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QTableWidget>
#include <QHeaderView>
#include <QColor>
#include <QFont>
#include <QScrollArea>
#include <QAbstractTextDocumentLayout>
#include "ProjectOverview.h"

ProjectOverview::ProjectOverview(QWidget *parent) : QWidget(parent) {
    // 1. Create a top-level layout for this widget
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // 2. Create the Scroll Area
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    
    // 3. Create a container widget to hold all the actual content
    QWidget *contentWidget = new QWidget();
    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);

    // 4. Set up the main layout on the contentWidget
    mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(25);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    setupUI();
}

void ProjectOverview::setupUI() {
    // --- 1. Project Members ---
    QVBoxLayout *membersLayout = new QVBoxLayout();
    QLabel *membersTitle = new QLabel("Members:");
    membersTitle->setStyleSheet("font-size: 13pt; font-weight: bold; color: #333;");
    membersLayout->addWidget(membersTitle);

    QStringList members = {
        "Gwynette Galleros",
        "Yasser Tomawis",
        "Ayyah Ampuan",
        "Nomeben Frietz Clarin"
    };

    for (const QString &member : members) {
        QLabel *memberLabel = new QLabel(member);
        memberLabel->setStyleSheet("font-size: 11pt; color: #555; margin-left: 15px;");
        membersLayout->addWidget(memberLabel);
    }
    mainLayout->addLayout(membersLayout);

    // --- 2. Project Title ---
    QLabel *titleLabel = new QLabel("Python-based Simple Calculator Language");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 32px; font-weight: bold; color: #3776ab;"
        "background-color: #f0f7ff; padding: 20px;"
        "border-radius: 12px; border: 2px solid #3776ab;"
    );
    mainLayout->addWidget(titleLabel);

    // --- 3. Project Description ---
    QTextBrowser *description = new QTextBrowser();
    description->setFrameStyle(QFrame::NoFrame);
    // Disable internal scrolling so it expands to its content
    description->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    description->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    description->setHtml(R"(
        <div style='background:#ffffff; padding:20px; border-radius:10px; border:1px solid #e0e0e0;'>
            <p style='font-size:17px; line-height:1.8; text-indent:40px; color:#333;'>
                This project involves the design and implementation of a
                <b>Python-based Simple Calculator Language</b> that demonstrates the
                <b>fundamental concepts of front-end compiler design</b>. The language is inspired by
                <b>Python’s simplicity and readability</b>, focusing on arithmetic computation,
                variable assignment, and mathematical function evaluation.
            </p>
            <p style='font-size:17px; line-height:1.8; text-indent:40px; margin-top:15px; color:#333;'>
                The compiler begins with a <b>Lexical Analyzer</b>, implemented using
                <b>regular expressions</b>. These expressions are converted into a
                <b>Non-deterministic Finite Automaton (NFA)</b> using
                <b>Thompson’s Construction</b>, and then transformed into a
                <b>Deterministic Finite Automaton (DFA)</b> using
                <b>Subset Construction</b> for efficient token recognition.
                A <b>lookup table</b> distinguishes reserved keywords such as
                <b>print</b> and predefined mathematical functions from user-defined identifiers.
            </p>
            <p style='font-size:17px; line-height:1.8; text-indent:40px; margin-top:15px; color:#333;'>
                For syntactic analysis, a <b>Pushdown Automaton (PDA)</b> is implemented to
                recognize the <b>context-free grammar</b> of the language. The PDA ensures
                correct syntactic structure, including <b>balanced parentheses</b> and
                <b>nested expressions</b>. Parsing follows a <b>top-down approach</b> using
                the <b>LL(1) parsing algorithm</b>, guided by a
                <b>predictive parsing table</b>.
            </p>
            <p style='font-size:17px; line-height:1.8; text-indent:40px; margin-top:15px; color:#333;'>
                Overall, the project demonstrates how <b>regular languages</b> are handled
                through automata theory in lexical analysis, while
                <b>context-free languages</b> are processed using pushdown automata and
                <b>LL(1) parsing techniques</b>, providing a complete illustration of a
                <b>compiler front-end</b>.
            </p>
        </div>
    )");

    // We use documentHeight to ensure the browser doesn't cut off text
    description->document()->setTextWidth(description->viewport()->width());
    int docHeight = static_cast<int>(description->document()->documentLayout()->documentSize().height());
    description->setFixedHeight(docHeight-300);
    mainLayout->addWidget(description);

    // --- 4. Tables Section ---
    QLabel *tablesHeader = new QLabel("Language Reference Tables");
    tablesHeader->setStyleSheet("font-size: 24px; font-weight: bold; color: #444; margin-top: 10px;");
    mainLayout->addWidget(tablesHeader);

    QHBoxLayout *tableLayout = new QHBoxLayout();
    tableLayout->setSpacing(40);

    // Lookup Table
    QVBoxLayout *lookupLayout = new QVBoxLayout();
    QLabel *lookupLabel = new QLabel("Print & Functions");
    lookupLabel->setStyleSheet("font-size:18px; font-weight:bold; color: #555;");
    lookupLayout->addWidget(lookupLabel);

    lookupTable = new QTableWidget();
    setupLookupTable();
    lookupLayout->addWidget(lookupTable);
    tableLayout->addLayout(lookupLayout);

    // Token Table
    QVBoxLayout *tokenLayout = new QVBoxLayout();
    QLabel *tokenLabel = new QLabel("Base Token Types");
    tokenLabel->setStyleSheet("font-size:18px; font-weight:bold; color: #555;");
    tokenLayout->addWidget(tokenLabel);

    tokenTable = new QTableWidget();
    setupTokenTable();
    tokenLayout->addWidget(tokenTable);
    tableLayout->addLayout(tokenLayout);

    mainLayout->addLayout(tableLayout);
    
    // Add a spacer at the bottom to keep everything pushed up
}

void ProjectOverview::setupLookupTable() {
    QStringList lexemes = {"print","sin","cos","tan","sqrt","abs","ceil","floor"};
    QStringList types   = {"PRINT","FUNCTION","FUNCTION","FUNCTION","FUNCTION","FUNCTION","FUNCTION","FUNCTION"};

    lookupTable->setColumnCount(2);
    lookupTable->setRowCount(lexemes.size());
    lookupTable->setHorizontalHeaderLabels({"Lexeme","Token Type"});
    lookupTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    lookupTable->verticalHeader()->setVisible(false);
    lookupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lookupTable->setSelectionMode(QAbstractItemView::NoSelection);
    lookupTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lookupTable->setStyleSheet("QTableWidget { background-color: white; border-radius: 5px; }");

    for (int i = 0; i < lexemes.size(); ++i) {
        lookupTable->setItem(i,0,new QTableWidgetItem(lexemes[i]));
        lookupTable->setItem(i,1,new QTableWidgetItem(types[i]));
        lookupTable->setRowHeight(i,35);
    }

    lookupTable->setFixedHeight(lookupTable->horizontalHeader()->height() + (lexemes.size() * 35) + 5);
}

void ProjectOverview::setupTokenTable() {
    QStringList tokens = {"identifier","number","+","-","*","/", "=" , "%", "(",")"};
    QStringList desc = {"variable name","numeric value","addition","subtraction","multiplication","division", "assigment", "modulo", "open parens","close parens"};

    tokenTable->setColumnCount(2);
    tokenTable->setRowCount(tokens.size());
    tokenTable->setHorizontalHeaderLabels({"Token","Description"});
    tokenTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tokenTable->verticalHeader()->setVisible(false);
    tokenTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tokenTable->setSelectionMode(QAbstractItemView::NoSelection);
    tokenTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tokenTable->setStyleSheet("QTableWidget { background-color: white; border-radius: 5px; }");

    for (int i = 0; i < tokens.size(); ++i) {
        tokenTable->setItem(i,0,new QTableWidgetItem(tokens[i]));
        tokenTable->setItem(i,1,new QTableWidgetItem(desc[i]));
        tokenTable->setRowHeight(i,35);
    }

    tokenTable->setFixedHeight(tokenTable->horizontalHeader()->height() + (tokens.size() * 35) + 5);
}
