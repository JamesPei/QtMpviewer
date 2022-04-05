QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
INCLUDEPATH += /usr/local/include/glm /usr/local/boost_1_73_0
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    GraphicObject.cpp \
    camera.cpp \
    main.cpp \
    mainwindow.cpp \
    molviewer.cpp \
    sphere.cpp

HEADERS += \
    GraphicObject.h \
    camera.h \
    color_table.h \
    config.h \
    mainwindow.h \
    molviewer.h \
    sphere.h


FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    mpviewer.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/release/ -lFileParsers
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/debug/ -lFileParsers
else:unix: LIBS += -L$$PWD/../../../../usr/local/lib/ -lFileParsers

INCLUDEPATH += $$PWD/../../../../usr/local/include/FileParsers
DEPENDPATH += $$PWD/../../../../usr/local/include/FileParsers

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/release/ -lGraphMol
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/debug/ -lGraphMol
else:unix: LIBS += -L$$PWD/../../../../usr/local/lib/ -lGraphMol

INCLUDEPATH += $$PWD/../../../../usr/local/include/GraphMol
DEPENDPATH += $$PWD/../../../../usr/local/include/GraphMol

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/release/ -lRDGeneral
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/local/lib/debug/ -lRDGeneral
else:unix: LIBS += -L$$PWD/../../../../usr/local/lib/ -lRDGeneral

INCLUDEPATH += $$PWD/../../../../usr/local/include/RDGeneral
DEPENDPATH += $$PWD/../../../../usr/local/include/RDGeneral
