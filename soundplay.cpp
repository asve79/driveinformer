#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextCodec>
#include <QFileInfo>
#include "soundplay.h"

soundplay::soundplay()
{
    int err;
    pcmstorage=NULL;
    frame_buf_size=-1;
    numChannels=0;
    bitsPerSample=-1;

    if ((err = snd_pcm_open (&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n",
                 "default",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_malloc (&params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_any (handle, params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_hw_params_set_access (handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    qDebug() << "Handle " << handle << " params " << params;
}

soundplay::~soundplay()
{

    qDebug() << "close audio";
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    clearpcmstorage();
}

void soundplay::clearpcmstorage()
{
    pcmstorage_type *currpcm,*nextpcm;
    int i=0;
    qDebug() << "free pcm storage";
    if (pcmstorage==NULL){
        qDebug() << ".. empty";
        return;
    }
    currpcm = pcmstorage;
    while(currpcm != NULL){
        i++;
        nextpcm = currpcm->next;
        if (currpcm->buffer != NULL)
            free(currpcm->buffer);
        free(currpcm);
        currpcm = nextpcm;
    }
    pcmstorage=NULL;
    qDebug() << "...pcm storage: delete " << i << " records";
}

bool soundplay::loadfromdir(QString path)
{
    QDir cdir(path);
    QString filename;
    QStringList filters;
    if (!cdir.exists()){
        qDebug() << "Directory " << path << " not exists";
        return false;
    }

//    clearpcmstorage();

    cdir.setFilter(QDir::Files);
    filters << "*.wav" << "*.pcm";
    cdir.setNameFilters(filters);

    for (int i=0; i<cdir.entryList().count();i++){
        filename=cdir.entryList().at(i);
        if (!addpcm(path + '/' + filename))
            qDebug() << "skip";

    }
    return true;

}

bool soundplay::addpcm(QString filename)
{
    pcmstorage_type *currpcm,*prevpcm;
    FILE *infile;
    WAVType wavehead;
    QFileInfo fi;
    unsigned int res;

    //Находим последнюю запись
    currpcm=pcmstorage;
    if (currpcm != NULL)
        while (currpcm->next != NULL)
            currpcm=currpcm->next;

    qDebug() << "loading " << filename;

    if ((infile = fopen(filename.toAscii(),"rb"))==NULL){
        qDebug() << "Error open file " << filename;
        return false;
    }
    qDebug() << "Header " << sizeof(wavehead) << "bytes";
    res=fread(&wavehead,1,sizeof(wavehead),infile);
    if (res != sizeof(wavehead)){
        qDebug() << "Error load header. loaded just " << res << "bytes";
        return false;
    }

    prevpcm = currpcm;

    currpcm = new pcmstorage_type;
    currpcm->buffer= (char *)malloc(wavehead.dataSize);

    res = fread(currpcm->buffer,1,wavehead.dataSize,infile);
    if (res != wavehead.dataSize){
        qDebug() << "loaded only " << res << " bytes, it is less " << wavehead.dataSize << "bytes";
        fclose(infile);
        free(currpcm->buffer);
        free(currpcm);
        return false;
    };

    fclose(infile);

    fi = QFileInfo(filename);
    currpcm->id=fi.baseName();
    currpcm->sampleRate=wavehead.sampleRate;
    currpcm->bitsPerSample=wavehead.bitsPerSample;
    currpcm->bytesPerSecond=wavehead.bytesPerSecond;
    currpcm->numChannels=wavehead.numChannels;
    currpcm->buf_size=wavehead.dataSize;
    currpcm->next = NULL;

    if (prevpcm != NULL)
        prevpcm->next=currpcm;
    else
        pcmstorage=currpcm;
    qDebug() << "..." << currpcm->id << " complete";
    return true;
}

bool soundplay::play(QString pcmId)
{
    pcmstorage_type *currpcm;
    int rc,dir,frame_nums;
    unsigned int val,offs;
    bool reinit;

    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    if (pcmId=="")return false;
    currpcm=pcmstorage;
    reinit=false;

    qDebug() << "search PCM id " << pcmId;
    //Поиск в хранилище нужного файла
    while ((currpcm != NULL)&&(currpcm->id != pcmId))
        currpcm=currpcm->next;
    if (currpcm==NULL){
        qDebug() << pcmId << " not found in storage";
        return false;
    }

    if (bitsPerSample != currpcm->bitsPerSample)reinit=true;
    if (numChannels != currpcm->numChannels)reinit=true;
    if (sampleRate != currpcm->sampleRate)reinit=true;

    if (reinit){
        switch (currpcm->bitsPerSample) {
        case 8 :
            rc=snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_U8);
            break;
        case 16 :
            rc=snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_S16_LE);
            break;
        case 24 :
            rc=snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_S24_LE);
            break;
        case 32 :
            rc=snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_S32_LE);
            break;
        default:
            qDebug() << "Unsupported format PCM " << currpcm->bitsPerSample << " bits ";
            break;
        }
        if (rc < 0){
            qDebug() << "Cant set PCM format to " << currpcm->bitsPerSample << " bits ";
            return false;
        } else
            bitsPerSample=currpcm->bitsPerSample;
        qDebug() << "PCM format set to " << bitsPerSample << " BPS";
    }

    if (reinit){
        rc=snd_pcm_hw_params_set_channels(handle, params, currpcm->numChannels);

        if (rc < 0){
            qDebug() << "Cant set channels num to " << currpcm->numChannels;
            return false;
        }else
            numChannels=currpcm->numChannels;
        qDebug() << "Channels format set to " << numChannels;
    }

    if (reinit){
        val = currpcm->sampleRate;
        rc=snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
        if (rc < 0){
            qDebug() << "Cant set sampleRate to " << currpcm->sampleRate;
            return false;
        }else
            sampleRate=currpcm->sampleRate;
        qDebug() << "sample rate format set to " << sampleRate;
    }


//    frames = currpcm->buf_size/(currpcm->bitsPerSample/8*currpcm->numChannels);
//    qDebug() << "frames calculated to " << frames;
    frames=32;
    rc=snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
    if (rc < 0){
         qDebug() << "snd_pcm_hw_params_set_period_size_near error";
         return false;
    }

    qDebug() << "frames set to " << frames;

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        qDebug() << "unable to set hw parameters: %s\n" << snd_strerror(rc);
        return false;
    }

    rc=snd_pcm_hw_params_get_period_size(params, &frames,&dir);
    if (rc < 0){
         qDebug() << "snd_pcm_hw_params_get_period_size error";
         return false;
    }


    frame_buf_size = frames * (bitsPerSample/8) * numChannels;
    qDebug() << "frame_buf_size = " <<frame_buf_size;

    rc=snd_pcm_hw_params_get_period_time(params, &val, &dir);
    if (rc < 0){
         qDebug() << "snd_pcm_hw_params_get_period_time error";
         return false;
    }

    frame_nums=frames;

    offs=0;
    while (offs < currpcm->buf_size ){
//        qDebug() << "play step " << offs << "frames " << frame_nums;
        rc = snd_pcm_writei(handle, currpcm->buffer+offs, frame_nums);
        if (rc == -EPIPE) {
            /* EPIPE means underrun */
            qDebug() << "underrun occurred";
            snd_pcm_prepare(handle);
            return false;
        } else if (rc < 0) {
            qDebug() << "error from writei: " << snd_strerror(rc);
            return false;
        }  else if (rc != (int)frame_nums) {
            qDebug() << "short write, write " << rc << "frames (must " << frame_nums <<")";
            return false;
        }
        offs += frame_buf_size;
        if (offs+frame_buf_size > currpcm->buf_size){
            frame_nums = (currpcm->buf_size - offs)/((bitsPerSample/8) * numChannels);
        }
    }


    return true;
}

void soundplay::playstr(QString pcmStr)
{
 QStringList lst;

 lst = pcmStr.split(" ");
 for (int i=0; i<lst.count();i++)
     this->play(lst.at(i));
}
