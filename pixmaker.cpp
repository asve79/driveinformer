#include "pixmaker.h"

pixmaker::pixmaker()
{
    speed_sign_ptr=NULL;

    for (int i=0;i<=200;i++)
    {
        speed_big[i] = NULL ;
        speed_big2[i] = NULL;
        speed_small[i] = NULL;
    }

}

pixmaker::~pixmaker()
{
    speed_sign_type *c_ssign, *n_ssign;

    for (int i=0;i<=200;i++){
        delete speed_big[i] ;
        delete speed_big2[i];
        delete speed_small[i];
    }

    //Чистим массив знаков
    c_ssign=speed_sign_ptr;
    while (c_ssign != NULL){
        qDebug() << "relese " << c_ssign->speed << "sign";
        n_ssign=c_ssign->next;
        delete c_ssign->graph;
        delete c_ssign;
//        if (n_ssign != NULL)
        c_ssign = n_ssign;
    }

}


int pixmaker::adjustFontSize( QFont &font, int width, const QString &text )
{
    QFontMetrics fm( font );

    int fontSize = -1;

    do {
        ++fontSize;
        font.setPointSize( fontSize + 1 );
        fm = QFontMetrics( font );
    } while( fm.width( text ) <= width );

    font.setPointSize( fontSize );
    return fontSize;
}

void pixmaker::init(int win_w, int win_h)
{
    scr_h=win_h;
    scr_w=win_w;
    init_speedbig_small();
    init_speed_sign();;
}

void pixmaker::init_speedbig_small()
{
    QFont fsize;
    QFont fntSmallSpeed;
    QFont fntKm;
    QPainter drawpix;
    QBrush brush;

    //Инициализация параметров
    fsize=QFont("Tahoma", 0, QFont::Bold);
    fsize.setWordSpacing(1);
    fsize.setLetterSpacing(QFont::AbsoluteSpacing,1);
    fntSmallSpeed=QFont("Tahoma", 0, QFont::Bold);
    fntSmallSpeed.setWordSpacing(1);
    fntSmallSpeed.setLetterSpacing(QFont::AbsoluteSpacing,1);
    fntKm=QFont("Tahoma", 0, QFont::Bold);
    fntKm.setWordSpacing(1);
    fntKm.setLetterSpacing(QFont::AbsoluteSpacing,1);
    int h=adjustFontSize(fsize,scr_w-(scr_w/4),"200")+80;
    int h_s=adjustFontSize(fntSmallSpeed,SPEED_NUM_SMALL_W,"200")+80;
    int h_k=adjustFontSize(fntKm,scr_w/5,"km/h")+20;

    //Вводим ограничение, что скорость не может быть больше 200 км/ч
    for(int i=0;i<200;i++){
        //СОздаем графические объекты
        speed_big[i] = new QImage(scr_w,scr_h,QImage::Format_RGB16);
        speed_big2[i] = new QPixmap(scr_w,scr_h/2);
        speed_small[i] = new QPixmap(SPEED_NUM_SMALL_W,SPEED_NUM_SMALL_H);

        //Большие цифры
        brush = QBrush(Qt::black, Qt::SolidPattern);
        drawpix.begin(speed_big2[i]);
        drawpix.setPen(QPen(Qt::black,1));
        drawpix.setBrush(brush);
        drawpix.setPen(QPen(Qt::white,3));
        drawpix.setFont(fsize);
        drawpix.drawText(QRect(0,-90,scr_w,h+140),Qt::AlignCenter,QString::number(i));
        drawpix.setFont(fntKm);
        drawpix.setPen(QPen(Qt::blue,2));
        drawpix.drawText(QRect(500,190,120,h_k),Qt::AlignCenter,"km/h");
        drawpix.end();

        //Маленькие цифры
        drawpix.begin(speed_small[i]);
        drawpix.setPen(QPen(Qt::black,1));
        drawpix.setBrush(brush);
        drawpix.setPen(QPen(Qt::white,3));
        drawpix.setFont(fsize);
        drawpix.drawText(QRect(0,0,SPEED_NUM_SMALL_W,h_s+10),Qt::AlignCenter,QString::number(i));
        drawpix.end();

    }
    //в 200-й ячейке храним символ отображаемый при отсутсвии спутников
    speed_big[200] = new QImage(scr_w-2,scr_h-2,QImage::QImage::Format_RGB32);
    speed_big2[200] = new QPixmap(scr_w,scr_h/2);
    speed_small[200] = new QPixmap(scr_w/4,h_s);

    //проверки когда нет коннекта большие
    brush = QBrush(Qt::black, Qt::SolidPattern);
    drawpix.begin(speed_big2[200]);
    drawpix.setPen(QPen(Qt::black,1));
    drawpix.setBrush(brush);
    drawpix.drawRect(QRect(0,0,scr_w,scr_h));
    drawpix.setPen(QPen(Qt::white,3));
    drawpix.setFont(fsize);
    drawpix.drawText(QRect(0,-90,scr_w,h+140),Qt::AlignCenter,"---");
    drawpix.end();

    //Прочерки когда нет коннекта маленькие
    drawpix.begin(speed_small[200]);
    drawpix.setPen(QPen(Qt::black,1));
    drawpix.setBrush(brush);
    drawpix.drawRect(QRect(0,0,scr_w,scr_h));
    drawpix.setPen(QPen(Qt::white,3));
    drawpix.setFont(fsize);
    drawpix.drawText(QRect(0,0,scr_w/4,h_s+10),Qt::AlignCenter,"---");
    drawpix.end();

}

//Подготавливаем изображения знаков ограничения скорости
void pixmaker::init_speed_sign()
{
    QFont fontspeedlimit;
    QPainter paint;
    QBrush brush;
    int o_h=SPEED_SIGN;
    fontspeedlimit=QFont("Tahoma", 0, QFont::Bold);
    fontspeedlimit.setWordSpacing(1);
    fontspeedlimit.setLetterSpacing(QFont::AbsoluteSpacing,1);
    adjustFontSize( fontspeedlimit, o_h ,"0000" );
    speed_sign_type *c_ssign, *p_ssign;

    p_ssign=NULL;
    int cam_speed=0;
    while (cam_speed <= 200){
        c_ssign = new speed_sign_type;
        c_ssign->next=NULL;
        if (p_ssign != NULL)
             p_ssign->next = c_ssign;
        if (cam_speed == 0){
            speed_sign_ptr = c_ssign;
        }
        c_ssign->speed=cam_speed;
        c_ssign->graph = new QPixmap(o_h,o_h);
        paint.begin(c_ssign->graph);

        brush = QBrush(Qt::white, Qt::SolidPattern);
        paint.setBrush(brush);
        QPoint pnt = QPoint(SPEED_SIGN/2,SPEED_SIGN/2);
        paint.setPen(QPen(Qt::red,10));
        paint.drawPoint(pnt);
        paint.drawEllipse(pnt,(o_h-10)/2,(o_h-10)/2);
        paint.setFont(fontspeedlimit);
//        paint.scale(1,-1);
        paint.setPen(QPen(Qt::red,1));
        if (cam_speed > 0)
            paint.drawText( QRect(0,0,o_h,o_h), Qt::AlignCenter, QString::number(cam_speed));
        else
            paint.drawText( QRect(0,0,o_h,o_h), Qt::AlignCenter, "N/A");
        paint.end();
        cam_speed += 10;
        p_ssign=c_ssign;
    }
}

QPixmap* pixmaker::getImgSign(int speed)
{
    speed_sign_type *curr;
    curr=speed_sign_ptr;
    while (curr !=NULL){
        if ( curr->speed == speed )
            return(curr->graph);
        curr=curr->next;
    }
    return(speed_sign_ptr->graph);
}
