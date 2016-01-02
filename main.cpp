#include <QApplication>
#include <QtGlobal>
#include <QDesktopWidget>
#include <QtMessageHandler>
#include <ctime>
#include <iostream>
#include <QDateTime>
#include <QDebug>

//#include "logger.h"
#include "mainwindow.h"

QFile outlog("application.log");
bool filestream;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray currtime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLocal8Bit();
    std::cerr << clock() << " " << currtime.constData() << " " << localMsg.constData() << " (" << context.file << ": " << context.line << ", " << context.function << ")\n";

    return;
    if (!filestream){

        switch (type) {
        case QtDebugMsg:
            std::cerr << "Debug: ";
            break;
        case QtWarningMsg:
            std::cerr << "Warinig: ";
            break;
        case QtCriticalMsg:
            std::cerr << "Critical: ";
            break;
        case QtFatalMsg:
            std::cerr << "Fatal: ";
            abort();
        }

        std::cerr << clock() << " " << currtime.constData() << " " << localMsg.constData() << " (" << context.file << ": " << context.line << ", " << context.function << ")\n";
    }else{
//        QTextStream out(&outlog);
//        out << clock() << " " << currtime.constData() << " " << context.file << ": "<< localMsg.constData() << " ("  << context.line << ", " << context.function << ")\n";
    }
}

int main(int argc, char *argv[])
{
//    if( !outlog.open( QIODevice::WriteOnly ) ){
//        qWarning() << "Cannot open log File";
//        filestream=false;
//    } else {
//        filestream=true;
//    }


//    qInstallMessageHandler(myMessageOutput); //переопределяем обработчик
//    QObject::connect(&app, SIGNAL(aboutToQuit()), &window, SLOT(closing()));
//    atexit(exitFunction); //сразу заводим функцию,коотрая выполнится при выходе
//    logger outlog;

    QApplication a(argc, argv);
    Q_INIT_RESOURCE(graph);
    MainWindow w;

    int i = 0;
    while (i < argc){
        if ( strcmp(argv[i],"-t")==0)
            w.trackemulation_on(argv[++i]);
        if ( strcmp(argv[i],"-n")==0)
            w.set_from_emu_pnt(QString(argv[++i]).toInt());
        i++;
    }

#ifdef Q_PROCESSOR_ARM
    QRect scr;
    scr=QApplication::desktop()->screenGeometry(-1);
    if (scr.width() > 640)
        w.show();
    else
        //    w.showMaximized();
        w.showFullScreen();
#else
    w.show();
#endif
    return a.exec();
}
