#ifndef PIXMAKER_H
#define PIXMAKER_H

#define SPEED_NUM_SMALL_W   260
#define SPEED_NUM_SMALL_H   220
#define SPEED_NUM_BIG_W     640
#define SPEED_NUM_BIG_H     240
#define SPEED_SIGN          150

#include <QImage>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QRect>
#include <QPixmap>
#include <QDebug>

//База со знаками ограничения скорости
typedef struct speed_sign{
    int speed;      //скорость
    QPixmap *graph; //Изображение
    struct speed_sign *next;
}speed_sign_type;

typedef struct symbols{
    QString id; //элемент
    QPixmap *graph; //Изображение
    struct symbols *next;
}symbols_type;

class pixmaker
{
private:
    int  adjustFontSize( QFont &font, int width, const QString &text );
    void init_speedbig_small();
    void init_speed_sign();

public:
    void init(int win_w, int win_h);
    QPixmap* getImgSign(int speed);
    pixmaker();
    ~pixmaker();

    speed_sign_type *speed_sign_ptr; //Указатель на массив знаков
    QImage *speed_big[201];
    QPixmap *speed_big2[201];   //Цифры скорости большие
    QPixmap *speed_small[201];  //Цифры скорости маленькие
    QPixmap *nums[100];         //Цифры информационные
    int scr_h;
    int scr_w;
};

#endif // PIXMAKER_H
