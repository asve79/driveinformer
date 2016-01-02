#include "logger.h"

/*Получем текущую дату и время в строке стразу*/
char * logger::currentTime()
{
    char *tmp;
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo=localtime(&rawtime);
    tmp=asctime(timeinfo);
    tmp[strlen(tmp)-1]=32;
    return tmp;
}


logger::logger()
{
    //qInstallMessageHandler(loggerHandler);
}

logger::~logger()
{
    this->exitFunction();
}

/*Инициализация логгера*/

/*Непосредственно сам перехватчик */
//void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
void logger::loggerHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
//void logger::loggerHandler(QtMsgType type, const char *msg)
{

#ifdef QT_ARCH_ARM
    return;
#else
    logFile=fopen("application.log","a+");
    if(logFile==NULL)
    {
        qInstallMessageHandler(0);
        qWarning() << "Cannot open logFile";
    }

    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
    fclose(logFile);
    return;
#endif
}

/*Функция, которая будет вызвана при выходе из приложения.*/
void logger::exitFunction()
{
    logFile=fopen("application.log","a+");
    if(logFile==NULL)
    {
        return;
    }
    fprintf(logFile,"%s : Application closed\n",currentTime());
    fclose(logFile);
//    free(logFile);
}
