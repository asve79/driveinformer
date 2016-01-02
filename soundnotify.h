#ifndef SOUNDNOTIFY_H
#define SOUNDNOTIFY_H

#include <QString>
#include <QThread>
#include <QDir>
#include <soundplay.h>

#define SOUND 1

class SoundNotify : public QObject {
    Q_OBJECT

private:
    time_t endtime;
    QString playstr;
    QStringList langslist;
    int langidx;
    soundplay *pcmplay;
    int *ledstate;
    QThread* playthread;
    QString langpath;
    QString soundpath;

public:
    void run();
    SoundNotify();
    ~SoundNotify();
    bool play(QString sounds);
    bool isBusy();
    void setLanguage(QString lastlang);
    void setsoundled(int *ledstate);
    void nextLang();
    QString getLangSid();

};

#endif // SOUNDNOTIFY_H
