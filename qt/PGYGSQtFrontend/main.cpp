#include "YGSWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    YGSWindow w;
    w.show();
    return a.exec();
}
