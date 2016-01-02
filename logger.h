#ifndef LOGGER_H
#define LOGGER_H

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <QCoreApplication>
#include <QtDebug>
#include <QtGlobal>

class logger: public QObject {
    Q_OBJECT

public:
    explicit logger();
    ~logger();

private:
    FILE *logFile;
    char * currentTime();
//    void loggerHandler(QtMsgType type, const char *msg);
    void loggerHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void exitFunction();

};

#endif // LOGGER_H

