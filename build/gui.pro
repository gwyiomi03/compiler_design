# Project configuration for qmake

# Specify the type of template: app for an application
TEMPLATE = app

# Name the executable file
TARGET = compiler_gui

# Use C++17 standard
CONFIG += c++17

# List the required Qt modules
QT += core gui widgets

# Use '../' to point to the parent directory for core files
SOURCES += \
    ../main.cpp \
    ../lexical.cpp \
    ../parser.cpp \
    ../gui/mainwindow.cpp  # mainwindow.cpp is in D:\...\test\gui

# Use '../' to point to the parent directory for core headers
HEADERS += \
    ../lexical.h \
    ../parser.h \
    ../gui/mainwindow.h    # mainwindow.h is in D:\...\test\gui

# Specify the directory containing custom headers (used for #include directives)
# The gui directory is relative to the parent, so we need the full path
# Or, relative to the project file: ../gui
INCLUDEPATH += ../gui