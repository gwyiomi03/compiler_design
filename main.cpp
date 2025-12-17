#include <QApplication>
#include "gui/mainwindow.h" 

int main(int argc, char *argv[])
{
    // Create the QApplication object
    QApplication a(argc, argv);

    // Create an instance of the main window containing both visualizers in tabs
    MainWindow w;

    // Set a good initial size for the combined application window
    w.resize(1200, 800);
    w.show();

    // Start the Qt event loop
    return a.exec();
}