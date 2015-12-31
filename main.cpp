#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Q_INIT_RESOURCE(graph);
    MainWindow w;

    int i = 0;
    while (i < a.argc()){
        if ( strcmp(a.argv()[i],"-t")==0)
            w.trackemulation_on(a.argv()[++i]);
        if ( strcmp(a.argv()[i],"-n")==0)
            w.set_from_emu_pnt(QString(a.argv()[++i]).toInt());
        i++;
    }

#ifdef QT_ARCH_ARM
    w.showFullScreen();
#else
//    w.showMaximized();
    w.show();
#endif
    return a.exec();
}
