#ifndef SOUNDPLAY_H
#define SOUNDPLAY_H

#include <alsa/asoundlib.h>

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

typedef struct tpcmstorage
{
    QString id;
    int numChannels;
    int sampleRate;
    int bytesPerSecond;
    int bitsPerSample;
    unsigned int buf_size;
    char *buffer;
    struct tpcmstorage *next;
} pcmstorage_type;

class soundplay
{
private:
    int frame_buf_size;
    int numChannels;
    int bitsPerSample;
    int sampleRate;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;

#pragma pack(push,1)
    struct WAVType
    {
        char    chunkId[4];            //RIFF
        quint32 chunkSize;    //8
        char    format[4];             //WAVE
        char    subChunkId[4];         /* Wave chunks (секции WAV-файла). Chunk ID (fmt) */
        quint32 subChunkSize; /* 16 + extra format bytes */
        quint16 audioFormat; /* Код сжатия (Compression Code)  1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM*/
        quint16 numChannels; /*Количество каналов (Number of Channels)  1=Mono 2=Sterio */
        quint32 sampleRate; /* Скорость выборок (Sample Rate) Число выборок аудиосигнала, приходящихся на секунду. */
        quint32 bytesPerSecond; /* Среднее количество байт в секунду (Average Bytes Per Second) */
        quint16 blockAlign; /* Выравнивание блока (Block Align) 2=16-bit mono, 4=16-bit stereo */
        quint16 bitsPerSample; /* Количество используемых бит на выборку (Significant Bits Per Sample) 8 бит или 16 бит*/
        char dataChunkId[4]; /*[Секция данных - "data"] */
        quint32 dataSize; /* Размер данных */
    };
#pragma pack(pop)

    pcmstorage_type *pcmstorage;

public:
    soundplay();
    ~soundplay();
    bool loadfromdir(QString path);
    bool addpcm(QString filename);
    void clearpcmstorage();
    bool play(QString pcmId);
    void playstr(QString pcmStr);
};

#endif // SOUNDPLAY_H
