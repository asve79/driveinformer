#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QObject>
#include "soundnotify.h"

SoundNotify::SoundNotify()
{
    playstr="";
    langidx=0;
    langpath="/home/asve/dev/rinf-gui/voice";
    soundpath="/home/asve/dev/rinf-gui/sound";

    QDir dir(langpath);
    dir.setFilter(QDir::Dirs | QDir::NoDot | QDir::NoDotDot);
    langslist = dir.entryList();

    pcmplay = new soundplay;
    //pcmplay->playque="";
    playthread = new QThread;
    pcmplay->moveToThread(playthread);
    connect(playthread, SIGNAL(started()), pcmplay, SLOT(play()));
    connect(pcmplay, SIGNAL(finished()), playthread, SLOT(quit()));
}

//Проигрываие звукового файла
bool SoundNotify::play(QString sounds)
{
//    pcmplay->playque=sounds;

    if (pcmplay->add(sounds)){
        if (!isBusy()&&(pcmplay->getquecount()==1)){
            playthread->start();
        }
        return true;
    }
    return false;
}

//true если занят false если свободен
bool SoundNotify::isBusy()
{
    //return(this->isRunning());
    return(pcmplay->playing);
}

void SoundNotify::run()
{
    if (playstr=="")return;
    qDebug() << "Threaded " + playstr;
#if SOUND
    qDebug() << "Thread EOF " + playstr;
#endif
}

SoundNotify::~SoundNotify()
{
    if (playthread != NULL)
        delete(playthread);
    if (pcmplay != NULL){
        pcmplay->clearpcmstorage();
        delete(pcmplay);
    }
}

\
//Переключение языка на следующий
void SoundNotify::nextLang()
{
    if (langslist.count() > 0){
        langidx++;
        if (langidx >= langslist.count())
            langidx=0;
        pcmplay->clearpcmstorage();
        pcmplay->loadfromdir(langpath+"/"+langslist.at(langidx));
        pcmplay->loadfromdir(soundpath);
    }
}

//Вернуть название ярлыка
QString SoundNotify::getLangSid()
{
    if (langslist.count() > 0){
        return(langslist.at(langidx));
    }else{
        return("en");
    }

}

//Установим текущий IDX для языка
void SoundNotify::setLanguage(QString lastlang)
{
    QString langsid;
    langidx = -1;

    if(langslist.count() > 0){
        for (int i=0;i<langslist.count();i++){
            if (langslist.at(i) == lastlang){
                langidx=i;
                langsid=lastlang;
                break;
            }
        }

        if (langidx == -1){
            langidx=0;
            langsid=langslist.at(0);
        }

        pcmplay->clearpcmstorage();
        pcmplay->loadfromdir(langpath+"/"+langsid);
        pcmplay->loadfromdir(soundpath);
    } else {
        pcmplay->clearpcmstorage();
    }
}

void SoundNotify::setsoundled(int *ledstate)
{
    pcmplay->soundled=ledstate;
}
