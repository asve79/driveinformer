#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include "soundnotify.h"

SoundNotify::SoundNotify()
{
    playstr="";
    pcmplay = new soundplay;
}

//Проигрываие звукового файла
bool SoundNotify::play(QString sounds)
{
    playstr = sounds;
    this->start();
    return true;
}

//true если занят false если свободен
bool SoundNotify::isBusy()
{
    return(this->isRunning());
}

void SoundNotify::run()
{
    if (playstr=="")return;
    qDebug() << QDateTime::currentDateTime().toString("yyyy-mm-dd hh:mm:ss") << "Threaded " + playstr;
#if SOUND
//    system(playstr.toAscii());
    pcmplay->playstr(playstr);
    qDebug() << QDateTime::currentDateTime().toString("yyyy-mm-dd hh:mm:ss") << "Thread EOF " + playstr;
#endif
}

SoundNotify::~SoundNotify()
{
    if (pcmplay != NULL){
        pcmplay->clearpcmstorage();
        delete(pcmplay);
    }
}

//0 - rus 1 - eng
void SoundNotify::setLanguage(int langId)
{
    QString langpath="/home/asve/dev/rinf-gui/voice";
    QString soundpath="/home/asve/dev/rinf-gui/sound";
    QString langsid;

    switch (langId) {
        case 0: langsid="ru";break;
        case 1: langsid="en";break;
    default:
        langsid="ru";break;
    }

    pcmplay->clearpcmstorage();
    pcmplay->loadfromdir(langpath+"/"+langsid);
    pcmplay->loadfromdir(soundpath);
}
