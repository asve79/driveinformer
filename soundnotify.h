#ifndef SOUNDNOTIFY_H
#define SOUNDNOTIFY_H

#include <QString>
#include <QThread>
#include <soundplay.h>

#define SOUND 1

class SoundNotify : public QThread
{
private:
    time_t endtime;
    QString playstr;
    soundplay *pcmplay;

public:
    void run();
    SoundNotify();
    ~SoundNotify();
    bool play(QString sounds);
    bool isBusy();
    void setLanguage(int langId);
};

#endif // SOUNDNOTIFY_H
