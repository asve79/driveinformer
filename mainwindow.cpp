#include <QPainter>
#include <QRect>
#include <QPen>
#include <QBrush>
#include <QList>
#include <QDir>
#include <QFontMetrics>
#include <QDataStream>
#include <QDateTime>
#include <QTextCodec>
#include <QKeyEvent>
#include <QDebug>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QFileInfoList>
#include <math.h>
#include <cstdlib>
#include "mainwindow.h"
#include "gps-func.h"
#include "radarobjects.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QSettings settings("rinf.ini",QSettings::IniFormat);
//    double my_xd,my_yd;

//    ui->setupUi(this);


    this->setMinimumSize(640,480);
    gtimerwork=false;
    vtimerwork=false;
    drawing=false;
    gpsd_connected=false;   //признак того, что мы подключены к GPSD
    satellitesignal=false;  //Признак что нет сигнала со спутников
    gpsd_mode=0;            //Режим 0, для инициализации GPS
    active_scr=settings.value("active_scr",0).toInt();           //номер активного экрана
    cam_distance=-1;
    my_speed=-1;
    id_lang=settings.value("id_lang",0).toInt(); //0-русский
    snotify = new SoundNotify;
    snotify->setLanguage(id_lang);
    poipath=settings.value("poi","poi").toString(); //каталог с POI (в т.ч. с камерами)
    trackpath=settings.value("tracks","tracks").toString(); //каталог с записанными треками
    gpspausetill=time(NULL);
    last_speed_cat=0;
    last_say_unit=0;
    cam_notified_voice=false;
    cam_notified_beep=false;
    cam_notify_on=settings.value("cam_notify",true).toBool();
    speed_notify_on=settings.value("speed_notify",true).toBool();
    said_less_low_limit=true; //true сразу чтобы не говорила при включении что скорость меньше LESS_SPEED_LIMIT
    speed_over_delta=10; //15 для россии, где можно ставить и 20

    autorecordtrack=settings.value("autorecordtrack",true).toBool();;
    recordingtrack=false;
    trackemulator=false;
    lw_width=-1;
    lw_height=-1;
    fontspeed = QFont("Tahoma", 0, 99);
    fontcamera = QFont("Tahoma", 0, 99);
    fontkmh = QFont("Tahoma", 0, 99);
    fontspeedlimit = QFont("Tahoma", 0, QFont::Bold);
    trackf=NULL;
    vid_radar_distance=FULL_RADAR_DICTANCE;
    radar_distance=FULL_RADAR_DICTANCE;

    gpsi = new gps;
//    objects_loaded = gpsi->loadobjects(&object_file_name);

    loadpoi(poipath);
    my_lat = 55.6573;
    my_lon = 37.8182;

    my_x=gpsi->lontometer(my_lat,my_lon);
    my_y=gpsi->lattometer(my_lat);
    my_azimuth=90;
    my_gmt=4;
    my_time=-1;
    vis_azimuth=90;
    l_azimut=-1;
    waitpointer=0;
    from_emu_pnt=0;
    emu_pnt_num=0;

    noradarpic = new QImage(":noradar","png");
    nosignalpic = new QImage(":nosignal","png");
    mutepic = new QImage(":mute","png");
    trackrecordpic = new QImage(":trackrecord","png");

    gtimer = new QTimer(this);
    connect(gtimer,SIGNAL(timeout()),SLOT(gtimer_event()));
    gtimer->start(500);
    vtimer = new QTimer(this);
    connect(vtimer,SIGNAL(timeout()),SLOT(vtimer_event()));
    vtimer->start(200);

#ifdef QT_ARCH_ARM //Если приложение под ARM-ом, то убрать курсор мыши
    QCursor oCursor(Qt::BlankCursor);
    oCursor.setPos(-100, -100);
    setCursor(oCursor);
#endif

//    qDebug() << gpsi->getangle(1,1,3,3);
//    qDebug() << gpsi->getangle(1,1,3,1);
//    qDebug() << gpsi->getangle(1,3,3,1);
//    qDebug() << gpsi->getangle(3,3,1,1);
//    if (!trackemulator) recordingtrack = createtracklog();
}

MainWindow::~MainWindow()
{

    gtimer->stop();
    vtimer->stop();

    writesetting("poi",poipath);
    writesetting("tracks",trackpath);

    if (recordingtrack)closetracklog();

    if (!radar_obj_list.isEmpty())radar_obj_list.clear();
    if (!wayloghist.isEmpty())wayloghist.clear();

    if (gpsd_connected)gpsi->freegps();

    while (snotify->isBusy())usleep(1000);
    delete snotify;

    delete noradarpic;
    delete nosignalpic;
    delete mutepic;
    delete gtimer;
    delete gpsi;
}


void MainWindow::paintEvent(QPaintEvent *){
    int view_scr; //тотбражаемый экран

    if (drawing) return;
    drawing=true;

    view_scr=active_scr;
    if (active_scr==1){
        if((cam_notify_on)&&(cam_distance>0)){
            lw_height=-1;
            lw_width=-1;
            view_scr=0;
        }
        else
        {
            view_scr=1;
            lw_height=-1;
            lw_width=-1;
        }
    }

    switch (view_scr){
        case 0: drawScreen0(); break;
    default:
        drawScreen1();
    };

    drawing=false;
}

void MainWindow::adjustFontSize( QFont &font, int width, const QString &text )
{
    QFontMetrics fm( font );

    int fontSize = -1;

    do {
        ++fontSize;
        font.setPointSize( fontSize + 1 );
        fm = QFontMetrics( font );
    } while( fm.width( text ) <= width );

    font.setPointSize( fontSize );
}

QPoint MainWindow::RotateVector(int tx, int ty, QPoint center, int angle){

//    QPoint res;
    double fangle=(360-angle)*(double)GPS_PI/(double)180.;
//    double fangle=angle*(double)GPS_PI/(double)180.;
    double nx,ny,x0,y0;

    x0=center.x();
    y0=center.y();

    nx = x0 + (tx - x0) * cos(fangle) - (ty - y0) * sin(fangle);
    ny = y0 + (ty - y0) * cos(fangle) + (tx - x0) * sin(fangle);

//    res.setX((int)(tx + (tx-center.x())*cos(fangle)-(ty-center.y())*sin(fangle)));
//    res.setY((int)(ty + (ty-center.y())*cos(fangle)+(tx-center.x())*sin(fangle)));
//    return(res);
    return(QPoint(nx,ny));
}


void MainWindow::drawScreen0(){
    QPainter paint(this);
    QPoint pnt,rpnt,dpnt,apnt;
    QRect cam_rec,rec,speed_rec,kmh_rec,time_rec;
    QFont font("Tahoma", 0, 99);
    QBrush brush;
    qreal scale_f,obj_x,obj_y,obj_angle;
    char waitptr[4] = { '|', '/', '-', '\\' };
    int way_x, way_y, max_x, max_y, min_x, min_y,cam_x,cam_y;
    int l,h,rc, s_azimuth;
    int radar_full, radar_med, radar_warn, radar_fix, o_h;
    int pic_x=0;

    s_azimuth=360-vis_azimuth;
    h = this->height();
    l = this->width();
    o_h = h/6;
    rc = (int)h/2;
    pnt = QPoint(rc,rc);
    rec = QRect(0,0,l,h);

    radar_full=rc-5;

    scale_f = radar_full/(qreal)vid_radar_distance;
    min_x = my_x-radar_full/scale_f;     //Экран в координатах
    max_x = my_x+radar_full/scale_f;
    min_y = my_y-radar_full/scale_f;
    max_y = my_y+radar_full/scale_f;

    radar_med=(int)(scale_f*(qreal)MED_RADAR_DICTANCE);
    radar_warn=(int)(scale_f*(qreal)WARN_RADAR_DICTANCE);
    radar_fix=(int)(scale_f*(qreal)FIX_RADAR_DICTANCE);

    cam_rec=QRect((int)l/2,2,(int)l/2,(int)h/2);
    speed_rec=QRect((int)l/2,h-(h/2),(int)l/2,(int)h/2);
    kmh_rec=QRect((int)(l-l/8),(int)h-h/10,(int)l/8,o_h);
    time_rec=QRect(kmh_rec.left()-l/4,h-(h/9),l/4,h/10);

    if ((lw_height != h)||(lw_width != l)){
        adjustFontSize( fontspeed, speed_rec.width(), "000" );
        adjustFontSize( fontcamera, cam_rec.width(), "0000 m" );
        adjustFontSize( fontkmh, kmh_rec.width(), "km/h" );
        adjustFontSize( fontspeedlimit, o_h*2 ,"000" );
        adjustFontSize( fontgpstime, time_rec.width() ," YYYY/MM/DD " );
        lw_height=h;
        lw_width=l;
    }

    brush = QBrush(Qt::black, Qt::SolidPattern);
    paint.setPen(QPen(Qt::black,1));
    paint.setBrush(brush);
    paint.drawRect(rec);

    brush = QBrush(Qt::blue, Qt::NoBrush);
    paint.setBrush(brush);
    paint.setPen(QPen(Qt::yellow,5));
    paint.drawEllipse(pnt,radar_full,radar_full);

    if (radar_med<radar_full){
        paint.setPen(QPen(Qt::blue,4));
        paint.drawEllipse(pnt,radar_med,radar_med);
    }

    if (radar_med<radar_warn){
        paint.setPen(QPen(Qt::blue,3));
        paint.drawEllipse(pnt,radar_warn,radar_warn);
    }

    paint.setPen(QPen(Qt::red,3));
    paint.drawEllipse(pnt,radar_fix,radar_fix);

    //Берем ближайшие камеры
//    radar_obj_list = gpsi->get_objects_detected(my_lat,my_lon,FULL_RADAR_DICTANCE);
//    make_obj_list(my_lat,my_lon,FULL_RADAR_DICTANCE,FIX_RADAR_DICTANCE);

    font.setPixelSize(10);
    paint.setFont(font);
    paint.save();
    paint.translate(pnt);

    paint.setPen(QPen(Qt::red,5));
    paint.drawPoint(0,0);
//    paint.drawText(0,0,QString::number(my_x)+ " " + QString::number(my_y));
    font.setPixelSize(20);
    paint.setFont(font);

    paint.scale(1,-1);
    rpnt=RotateVector(0,radar_full,QPoint(0,0),s_azimuth); //Указатель где север
    paint.drawText(rpnt,"*");
    font.setPixelSize(10);
    paint.setFont(font);

//    paint.drawPoint(20,20);

/*-----------------------------------------------------------------------------------------
* Отрисовка положения камер и их направления
*/

    for(int i=0;i<radar_obj_list.count();i++){
        cam_x=radar_obj_list.at(i).x;
        cam_y=radar_obj_list.at(i).y;
        if ((cam_x>min_x)&&(cam_x<max_x)&&(cam_y>min_y)&&(cam_y<max_y)){
            obj_x = scale_f*(cam_x-my_x);
            obj_y = scale_f*(cam_y-my_y);
            rpnt = RotateVector((int)obj_x,(int)obj_y,QPoint(0,0),s_azimuth); //Вращение точки
            paint.setPen(QPen(Qt::green,5));
            paint.drawPoint(rpnt);

            if (radar_obj_list.at(i).roundangle)
                paint.drawEllipse(rpnt,radar_fix,radar_fix);
            else{

                obj_angle=radar_obj_list.at(i).angle+(s_azimuth);        //Прибавляем к углу камеры азимут
                if (obj_angle > 360) obj_angle = obj_angle-360;       //Если больше 360 градусов
            //          dpnt=RotateVector(rpnt.x()+radar_fix,rpnt.y(),rpnt,obj_angle);  //Вращение направления камеры
                dpnt=RotateVector(rpnt.x(),rpnt.y()+radar_fix,rpnt,obj_angle);  //Вращение направления камеры
                paint.setPen(QPen(Qt::green,3));
                paint.drawLine(rpnt,dpnt);
            }

            if (radar_obj_list.at(i).nearless){
                if (radar_obj_list.at(i).nearless_warn)
                    paint.setPen(QPen(Qt::red,2));
                else
                    paint.setPen(QPen(Qt::darkCyan,1));
                paint.drawLine(rpnt,QPoint(0,0));
                paint.setPen(QPen(Qt::green,1));
            }

            paint.save();
            paint.translate(rpnt);
            paint.scale(1,-1);
            paint.setPen(QPen(Qt::yellow,1));
            paint.drawText(0,0,QString::number(radar_obj_list.at(i).speedlimit));
            //        paint.drawText(0,0,QString::number(radar_obj_list.at(i).id));
            //        paint.drawText(0,0,QString::number(radar_obj_list.at(i).x)+ " " + QString::number(radar_obj_list.at(i).y));
            paint.restore();
        }
    }

/* Отрисовка пройденного пути */
    paint.setPen(QPen(Qt::blue,3));
    int cntway=wayloghist.count();
    for(int i=0; i<cntway;i++){
        way_x=wayloghist.at(i).x;
        way_y=wayloghist.at(i).y;
        if ((way_x> min_x)&&(way_x < max_x)&&(way_y > min_y)&&(way_y < max_y)){
            obj_x = scale_f*(way_x-my_x);
            obj_y = scale_f*(way_y-my_y);
            rpnt = RotateVector((int)obj_x,(int)obj_y,QPoint(0,0),s_azimuth);
            paint.drawPoint(rpnt);
        }
    }

    paint.restore();
    paint.save();

/* Прорисовка азимутов реального и видимого */
//    apnt=QPoint(l-radar_fix-20,(int)h/2);
//    apnt=QPoint(h,(int)(h-(radar_fix*2+10)));
    apnt=QPoint(o_h+1,(int)(h-(o_h+1)));
    paint.translate(apnt);
    paint.scale(1,-1);

//Рисуем азимут реальный
    brush = QBrush(Qt::black, Qt::SolidPattern);
    paint.setPen(QPen(Qt::black,1));
    paint.setBrush(brush);

    pnt = QPoint(0,0);
    paint.setPen(QPen(Qt::yellow,3));
    paint.drawPoint(pnt);
    paint.drawEllipse(pnt,o_h+2,o_h+2);

    paint.setPen(QPen(Qt::yellow,2));
    rpnt = RotateVector(pnt.x(),pnt.y()+o_h-5,pnt,my_azimuth);
    paint.drawLine(pnt,rpnt);
//Рисуем азимут для карты, который "доворачивает"
    paint.setPen(QPen(Qt::green,1));
    rpnt = RotateVector(pnt.x(),pnt.y()+o_h-5,pnt,vis_azimuth);
    paint.drawLine(pnt,rpnt);

    paint.restore();
//подписываем реальный азимут
    paint.setPen(QPen(Qt::yellow,1));
    paint.drawText(apnt,QString::number(my_azimuth));

    paint.save();
//Знак ограничения скорости
    if (cam_distance != -1){
        apnt=QPoint(l-o_h+5,(int)h/3+h/10);
        paint.translate(apnt);
        paint.scale(1,-1);
        brush = QBrush(Qt::white, Qt::SolidPattern);
        paint.setPen(QPen(Qt::red,1));
        paint.setBrush(brush);
        pnt = QPoint(0,0);
        paint.setPen(QPen(Qt::red,(int)o_h/6));
        paint.drawPoint(pnt);
        paint.drawEllipse(pnt,o_h+2,o_h+2);
        paint.setFont(fontspeedlimit);
        paint.scale(1,-1);
        paint.drawText( QRect(-o_h,-o_h,o_h*2,o_h*2), Qt::AlignCenter, QString::number(cam_speed));
    }
    paint.restore();

    //Время с GPS
    paint.setFont(fontgpstime);
    paint.setPen(QPen(Qt::white,3));
//    paint.drawRect(time_rec);
    if (my_time <= 0)
        paint.drawText( time_rec, Qt::AlignCenter, "----/--/--\n--:--:--");
    else
        paint.drawText( time_rec, Qt::AlignCenter, QDateTime::fromTime_t(my_time).toString("yyyy/MM/dd\nhh:mm:ss"));

    //Скорость текущая
    paint.setFont(fontspeed);
//    paint.drawRect(speed_rec);
    paint.setPen(QPen(Qt::white,3));
    if (my_speed > -1){
        paint.drawText( speed_rec, Qt::AlignRight, QString::number(my_speed));
        paint.setFont(fontkmh);
        paint.drawText( kmh_rec, Qt::AlignRight, "km/h");
    }
    else
    {
        paint.drawText( speed_rec, Qt::AlignRight, QString(waitptr[waitpointer++]));
        if (waitpointer >=4) waitpointer=0;
    }

//Расстояние до ближайшей камеры, если нужно
    if (cam_distance != -1){
        paint.setFont(fontcamera);
        paint.setPen(QPen(Qt::red,1));
        paint.drawText( cam_rec, Qt::AlignRight, QString::number(cam_distance) + "m" );
        if (cam_distance > FIX_RADAR_DICTANCE)
            radar_distance = cam_distance + 20;
        else
            radar_distance=FIX_RADAR_DICTANCE+10;
    } else radar_distance = FULL_RADAR_DICTANCE;

    if (!cam_notify_on){
        paint.drawImage(QPoint(pic_x,0),*noradarpic);
        pic_x +=35;
    }
    if (!speed_notify_on){
        paint.drawImage(QPoint(pic_x,0),*mutepic);
        pic_x +=35;
    }
    if (!satellitesignal){
        paint.drawImage(QPoint(pic_x,0),*nosignalpic);
        pic_x +=35;
    }
    if (recordingtrack){
        paint.drawImage(QPoint(pic_x,0),*trackrecordpic);
        pic_x +=35;
    }

    paint.end();
}


//void MainWindow::on_horizontalSlider_sliderMoved(int position)
//{
//    my_azimuth=position;
////    this->update();
//}

void MainWindow::gtimer_event()
{
//    srand( time(NULL) );
//    my_speed=rand();
    double tmy_lat, tmy_lon, tmy_time;
    int tmy_x,tmy_y,tmy_speed, tmy_azimuth, gres;

    if (gtimerwork)return;
    gtimerwork=true;


    if (difftime(gpspausetill,time(NULL))<=0) //Если ненадо ждать паузу в работе с GPS
        switch (gpsd_mode) {
        case 0:             // Режим 0, инифиализируем
            if ((gpsd_connected = gpsi->initgps())==true)
                gpsd_mode++;
            if (autorecordtrack&&(!recordingtrack)){
                recordingtrack=createtracklog();
                if (!recordingtrack) {
                    autorecordtrack=false;
                    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " Cant start autorecord track";
                }
            }
            else
                gpspausetill=time(NULL)+10; //Установим паузу в 10 секунд
            break;
        case 1:             //Запрашиваем данные с приемника
//            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " Read GPS data";
            gres = gpsi->getgpsdata(&tmy_time, &tmy_lat,&tmy_lon,&tmy_x,&tmy_y,&tmy_speed,&tmy_azimuth);
            switch (gres) {
            case -1 : gpsd_mode--;
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " Error read GPS data";
                break; //read error
            case 0  : gtimerwork=false; return; break;  //no new data
            case 2  : //no satelites
                my_time = tmy_time;
                my_speed = -1; //для графического экрана - если скорость -1, значит нет приема
                if (satellitesignal){
                    if (!snotify->isBusy()){
                        satellitesignal=false;
                        snotify->play("gps_signal_lost");
                    }
                }
                gtimerwork=false;
                return;
                break;
            case 1  :   //if have data
                //Если есть прием
                my_time=tmy_time;
                my_lat=tmy_lat;
                my_lon=tmy_lon;
                my_x=tmy_x;
                my_y=tmy_y;
                my_speed=tmy_speed;
                my_azimuth=tmy_azimuth;
                //                    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "set az " << my_azimuth;
                waypointlog();
                if (!satellitesignal){
                    if (!snotify->isBusy()){
                        satellitesignal=true;
                        snotify->play("gps_signal_online");
                    }
                }
                //Берем ближайшие камеры
                make_obj_list(my_lat,my_lon,FULL_RADAR_DICTANCE,FIX_RADAR_DICTANCE);

                if (recordingtrack)writetracklog(); //Если ведем запись трека, то выозвим ф-цию
                if (cam_notify_on)checkandnotifycam();
                if (speed_notify_on)checkandsayspeed();
                gtimerwork=false;
                //                qDebug() << "End of mode one";
                return;
                break;
            default:
                qDebug() << "Unknown return gps parameter " << gres;
                break;
            }
            break;
        case 3:             //Запрашиваем данные из файла (эмуляция трека)
//            qDebug() << "Read emulation data gpsd_mode " << gpsd_mode;
            if (getemudata(&tmy_time, &tmy_lat,&tmy_lon,&tmy_x,&tmy_y,&tmy_speed,&tmy_azimuth)){
                //Если есть данные
                my_time=tmy_time;
                my_lat=tmy_lat;
                my_lon=tmy_lon;
                my_x=tmy_x;
                my_y=tmy_y;
                my_speed=tmy_speed;
                my_azimuth=tmy_azimuth;
                waypointlog();
                make_obj_list(my_lat,my_lon,FULL_RADAR_DICTANCE,FIX_RADAR_DICTANCE);
            } else {
                qDebug() << "No more emulation data";
                trackemulation_off();
                trackemulator=false;
            }
            break;
        default:
            qDebug() << "Unknown work gps parameter";
            break;
        }

//    this->update(); //Пока уберем, чтобы отрисовываться по таймеру отрисовки
    if (cam_notify_on)checkandnotifycam();
    if (speed_notify_on)checkandsayspeed();
    gtimerwork=false;
}

void MainWindow::vtimer_event()
{
    int angle, step;
    if (vtimerwork)return;
    vtimerwork=true;

    if (my_azimuth != vis_azimuth){

        angle = 360-my_azimuth+vis_azimuth;
        while (angle > 360) angle-=360;

        if ((angle == 360)||(angle <=0 )) {
            vis_azimuth = my_azimuth;
            vtimerwork=false;
            return;
        }

        if ((angle < 6)||(angle >354)) step=1;
        else
        {
//            step=10;
//            step = int(angle/2);
            if (angle<180) step=int(angle/2);
            else
                step=int((360-angle)/2);
        }

        if (angle > 180) vis_azimuth+=step;
        else
            vis_azimuth-=step;
    }

    if (vis_azimuth < 0)vis_azimuth = my_azimuth;
//    vis_azimuth = my_azimuth;

    if (vid_radar_distance != radar_distance){
        if (vid_radar_distance < radar_distance){
            step=(radar_distance-vid_radar_distance)/2;
            if (step<5)step=1;
            vid_radar_distance += step;
        }else{
            step=(vid_radar_distance-radar_distance)/2;
            if (step<5)step=1;
            vid_radar_distance -= step;
        }
    }

    this->update();
    vtimerwork=false;
}

bool MainWindow::createtracklog()
{
    QString filename;
//    QDir tpatch(trackpath);

//    if (!tpatch.exists(trackpath)){
//        if (!tpatch.mkdir(trackpath))
//        {
//            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Cant create directory " << trackpath;
//            return false;
//        }
//    }

    filename= trackpath + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".track";

    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Open trackfile for write " << filename;
    if (trackf != NULL) delete trackf;
    trackf = new QFile(filename);
    if((trackf->open(QIODevice::WriteOnly))==false)
    {
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Error create file for write";
        if (trackf != NULL) delete trackf;
        return false;
    }
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "done";
    snotify->play("track_logger_on");
    return true;
}

void MainWindow::closetracklog()
{
    if (trackf != NULL) {
        delete trackf;
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Close writing track file";
        snotify->play("track_logger_off");
        trackf=NULL;
    }
}

bool MainWindow::writetracklog()
{
    QDataStream out(trackf);
    track_log_type trackdata;

    if (trackf == NULL){
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "No stream found";
        return false;
    }
    if (!gpsd_connected) {
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Nothing to write - no GPS connected";
        return true;  //Если GPSd не подсоединен

    }
    trackdata.datetime=my_time;
    trackdata.lat=my_lat;
    trackdata.lon=my_lon;
    trackdata.speed=my_speed;

//    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Write data";
    out.setVersion(QDataStream::Qt_4_8);
    out << (unsigned int)trackdata.datetime << trackdata.lat << trackdata.lon << trackdata.speed;
    if (out.status() == QDataStream::Ok){
//        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "done";
        return true;
    }else{
//        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "error";
        return false;}
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()){
        case Qt::Key_S:     //Запуск записи трека (кнопка S)
            if (trackemulator)
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Cant start write track becose track emulation is on";
            else
                if (!recordingtrack) recordingtrack = createtracklog();
            break;
        case Qt::Key_X:     //Остановка записи трека (кнопка X)
            if (trackemulator){
                qDebug() << "Stopping emulation process";
                trackemulation_off();
            }
            else{
                if (!recordingtrack)break;
                if (autorecordtrack)autorecordtrack=false;  //Отключение старта автозаписи
                closetracklog();
                recordingtrack=false;
            }
            break;
        case Qt::Key_0:
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") <<  "reset azimuth";
            my_azimuth=0;
            break;
        case Qt::Key_L:
            if (++id_lang >1) id_lang=0;
            writesetting("id_lang",id_lang);
            snotify->setLanguage(id_lang);
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Set " + voice_lang + " language";
            snotify->play("language");
//            snotify->run();
            break;
        case Qt::Key_Minus:
            radar_distance += 10;
            break;
        case Qt::Key_Plus:
            radar_distance -= 10;
            if (vid_radar_distance < FIX_RADAR_DICTANCE)vid_radar_distance=FIX_RADAR_DICTANCE;
            break;
        case Qt::Key_Left:
            my_azimuth += 10;
            if (my_azimuth>=360)my_azimuth=0;
            break;
        case Qt::Key_Right:
            my_azimuth -= 10;
            if (my_azimuth<0)my_azimuth=350;
            break;
        case Qt::Key_Space:
            active_scr = active_scr < 1 ? active_scr+1 : 0;
            lw_height=-1;
            lw_width=-1;
            writesetting("active_scr",active_scr);
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Set screen " + active_scr;
            break;
        case Qt::Key_1:
            if (cam_notify_on){
                cam_notify_on=false;
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Speed cam notification OFF";
                snotify->play("cam_notify_off");
            } else {
                cam_notify_on=true;
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Speed cam notification ON";
                snotify->play("cam_notify_on");
            }
            writesetting("cam_notify",cam_notify_on);
            break;
        case Qt::Key_2:
            if (speed_notify_on){
                speed_notify_on=false;
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Speed notification OFF";
                snotify->play("speed_notify_off");
            } else {
                speed_notify_on=true;
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Speed notification ON";
                snotify->play("speed_notify_on");
            }
            writesetting("speed_notify",speed_notify_on);
            break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MainWindow::checkandsayspeed()
{
    if (snotify->isBusy())return;

    int speed_cat=(int)my_speed/10;
    if (speed_cat >= LOW_SPEED_LIMIT_NOTIFY){ //Если скорость больше или равно LOW_SPEED_LIMIT_NOTIFY*10 км/ч
        if (last_speed_cat != speed_cat){
            said_less_low_limit=false;
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Say speed " << my_speed;
            snotify->play(say_current_speed(my_speed,false));
        }
    }else
    if(!said_less_low_limit){
        said_less_low_limit=true;
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Say less one " << speed_cat;
        snotify->play("less" + QString::number(speed_cat+1));
    }

    last_speed_cat=speed_cat;
}

//На выходе даем строку с именами файлов, которые нужно воспроизвести последовательно, проговария число
QString MainWindow::speech_numbers(int num){
    int i,j, k,l;
    char tbuf[100];
    char tbuf2[100];
    QString outstr;

    sprintf(tbuf,"%d",num);

    k=0;
    l=strlen(tbuf);
    for(i=0;i<l;i++){
        if (tbuf[i] != '0'){
            //        if (((l-i)==1)&&(tbuf[i]=='0'))break;
            tbuf2[k++]=tbuf[i];
            if (((l-i) == 2)&&(tbuf[i]  == '1')){
                tbuf2[k++]=tbuf[++i];
            }else{
                for (j=0;j<l-i-1;j++){
                    tbuf2[k++] = '0';
                }
            }
            tbuf2[k++] = ' ';
        }
    }

    tbuf2[k] = '\0';

//    printf("bufer %s\n",tbuf2);
    outstr = tbuf2;
    return(outstr);
}

//Вернет 0 1 или 2 в зависимости от окончания.
//Для использования как индекс массива с окончаниями 0-[] 1-[a] 2-[ов]
int MainWindow::get_last_word_idx(int num){
    char buf[100];
    int result;
    int i,l;

    sprintf(buf,"%d",num);
    l = strlen(buf);
    i = 0;
    while(i<=2){
        if (i==0){
            switch(buf[l-i-1]){
                case '2' : result=1;break;
                case '3' : result=1;break;
                case '4' : result=1;break;
                case '1' : result=0;break;
            default: result=2;
            }
        } else {
            if(buf[l-i-1] == '1'){ result=2; };
        }
        i++;
    }

    return(result);
}

//Говорит скорость
QString MainWindow::say_current_speed(int speed, bool camspeed){
    QString buf1;
    char speed_speech[2][100]={
//        {"speed.wav"},
//        {" "}
        {""},
        {""}
    };
    char kmh_speech[3][100]={
        {"kmph1"}, //[]
        {"kmph2"}, //[а]
        {"kmph3"}  //[ов]
    };
    int i,j;
    j = get_last_word_idx(speed);

    srand(time(NULL));
    i = (int)rand()/RAND_MAX*2;
    if (i>1){ i = 1; }; //Заглушка

//    sprintf(buf2,"cd /home/asve/dev/rinf/sound/voice;aplay %s %s %s >/dev/null 2>/dev/null &",speed_speech[i],buf1,kmh_speech[j]);

    if (last_say_unit != 1) {
        if (camspeed)
            buf1=speech_numbers(speed) + " " + (QString)kmh_speech[j];
        else {
            buf1=(QString)speed_speech[i] + " " + speech_numbers(speed) + " " + (QString)kmh_speech[j];
            last_say_unit=1;
        }
    }else{
        buf1 = speech_numbers(speed);
    }

    return buf1;
}

void MainWindow::trackemulation_on(char *filename)
{
    emu_pnt_num=0;
    trackf = new QFile(filename);
    if((trackf->open(QIODevice::ReadOnly))==false)
    {
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Error open emulation track file " << filename << "for read";
        return;
     }
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Open emulation file " << filename;
    trackemulator=true;
    gpsd_mode=3;
}

void MainWindow::trackemulation_off()
{
    if (trackf != NULL) {
        delete trackf;
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Clase emulator track file";
        trackemulator=false;
        gpsd_mode=0;
        trackf=NULL;
    }
}

bool MainWindow::getemudata(double *time, double *lat, double *lon, int *x, int *y, int *speed, int *azimuth)
{
    QDataStream in(trackf);
    track_log_type trackdata;
    int tmp,ta;

    if (!trackemulator) return false;  //Если не ведется эмуляция

    in.setVersion(QDataStream::Qt_4_8);
    while (from_emu_pnt > emu_pnt_num){
        in >> tmp;
        in >> trackdata.lat;
        in >> trackdata.lon;
        in >> trackdata.speed;
        emu_pnt_num++;
    }
    in >> tmp;
    in >> trackdata.lat;
    in >> trackdata.lon;
    in >> trackdata.speed;
    emu_pnt_num++;

    if (in.status() == QDataStream::Ok){
        *time=tmp;
        *lat=trackdata.lat;
        *lon=trackdata.lon;
        *speed=trackdata.speed;
        *x = gpsi->lontometer(trackdata.lat,trackdata.lon);
        *y = gpsi->lattometer(trackdata.lat);        

//        if(l_azimut != -1){
//            lx = gpsi->lontometer(l_lat,l_lon);
//            ly = gpsi->lattometer(l_lat);
//        }

        if ((l_azimut != -1)&&(*speed>FIX_AZIMUTH_SPEED)&&(gpsi->getDistance(l_lat,l_lon,*lat,*lon)>FIX_AZIMUTH_METERS))
        {
            if ((l_lat==*lat)&&(l_lon==*lon)){
                *azimuth=l_azimut;
            }else{
                ta=gpsi->getangle(l_lon,l_lat,*lon, *lat);
                if (ta < 0) qDebug() << "ERR TA!";
                *azimuth=ta;
                l_lat = *lat;
                l_lon = *lon;
                l_azimut=*azimuth;
            }
        }
        else
        if (l_azimut == -1){
            *azimuth=0;
            l_lat = *lat;
            l_lon = *lon;
            l_azimut = 0;
        } else {
            *azimuth=l_azimut;
        }

        if ((*azimuth >= 0)&&(*azimuth <= 360))
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << emu_pnt_num << "time: " << tmp << " Readed lat:" << *lat << " lon:" << *lon << " speed:" << *speed;
        else
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << emu_pnt_num << "time: " << tmp <<  "Readed lat:" << *lat << " lon:" << *lon << " speed:" << *speed << " az: " << *azimuth;

        return true;
    }
    else
        return false;

}

void MainWindow::waypointlog()
{
    way_log_type cpoint;

    if (my_speed<3) return; //Если почти стоим то не пишем

    cpoint.x = my_x;
    cpoint.y = my_y;
    cpoint.speed = my_speed;
    cpoint.fixtime = my_time;

    wayloghist.append(cpoint);
    if (wayloghist.count() > 999)
        wayloghist.removeFirst();
}

int MainWindow::anglediff(int angle_one, int angle_two )
{
//    int angle = angle_two-(360-angle_one);
//    if (angle < 0) angle= abs(angle);

//    if (angle > 180)
//        angle = 360-angle;

    int angle;

    angle = angle_one>angle_two ? angle_one-angle_two : angle_two-angle_one;

    if (angle > 180) angle = 360-angle;

//    if (angle > 180){
//        if (angle_one > 180) angle_one -= 360;
//        if (angle_two > 180) angle_two -= 360;
//    }

//    angle_one>angle_two ? angle=angle_one-angle_two : angle=angle_two-angle_one;

    return angle;
}

void MainWindow::make_obj_list(double my_lat, double my_lon, int visible_distance, int fix_distance)
{
    int cntobj, lcam_id=-1, lcam_distance=-1, ad1,ad2,ad3,ad4,ad5, tcid;
    bool fixcam;
    radar_objects_type radarobj;

    //Берем ближайшие камеры
    radar_obj_list = gpsi->get_objects_detected(my_lat,my_lon,visible_distance);
    cam_distance=-1;

    //Если нет камер
    if (radar_obj_list.isEmpty()){
        cam_id=-1;
        cam_id_fixed=false;
        return;
    }

    //Теперь пробежимся по списку
    cntobj = radar_obj_list.count();
    for (int i=0; i<cntobj;i++){
        fixcam=false;
        if (radar_obj_list.at(i).roundangle) fixcam=true; //Если круговая камера
        else {
            ad1=anglediff(my_azimuth, radar_obj_list.at(i).angle); //Разница между углом камеры и азимутом
            ad3=gpsi->getangle(radar_obj_list.at(i).x,radar_obj_list.at(i).y, my_x, my_y);
            ad2=anglediff(ad3, radar_obj_list.at(i).angle); //Разница между углом камеры и углом от камеры к машине
            ad4=gpsi->getangle(my_x, my_y,radar_obj_list.at(i).x,radar_obj_list.at(i).y);
            ad5=anglediff(my_azimuth, ad4); //Разница между направлением движения и углом к камере
            if (    //Проверим что угол камеры и движения больше 90, значит направлены друг на друга
                //а также проверим что угол камеры и вектор камера->машина меньше 90
                    ((ad1 > 150)&&(ad1<210))&&
                    ((ad2 < 90)||(ad2>270))&&
                    (ad5<35)
            ) fixcam=true;
        }
        if (fixcam){
            //Если условия совпали, то выделим ближайшую подходящую камеру
            if (lcam_id==-1){
                lcam_id=i;
                lcam_distance=radar_obj_list.at(i).cur_distance;
            } else
                if (lcam_distance > radar_obj_list.at(i).cur_distance){
                    lcam_id=i;
                    lcam_distance = radar_obj_list.at(i).cur_distance;
                }
        }
    }

   if (lcam_id == -1) { //Если ниодной опасной камеры то выйдим
       cam_id=-1;
       cam_id_fixed=false;
       return;
   }

   tcid=radar_obj_list.at(lcam_id).id;
   if (tcid != cam_id)          //Если смотрим на новую камеру
       cam_id_fixed=false;
   cam_id=tcid;
   cam_speed=radar_obj_list.at(lcam_id).speedlimit;
   radarobj=radar_obj_list.at(lcam_id);
   radarobj.nearless=true;                  //Говорим что она ближайшая

   if ((!cam_id_fixed)&&(lcam_distance < fix_distance+(int)add_slow_meters(my_speed,radar_obj_list.at(lcam_id).speedlimit))){
       qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "FIX cam at"+QString::number(fix_distance+(int)add_slow_meters(my_speed,radar_obj_list.at(lcam_id).speedlimit))+"meters";
       cam_id_fixed=true;
   }

   if (cam_id_fixed){
       radarobj.nearless_warn=true;
       cam_distance=lcam_distance;
   }
       else cam_distance = -1;

   radar_obj_list.append(radarobj);         //говорим что в зоне действия

}

//Говорит расстояние
QString MainWindow::say_current_meters_ptr(int meters){
    QString buf = "";

    char kmeter_speech[3][100]={
        {"kmeter1"}, //[]
        {"kmeter2"}, //[а]
        {"kmeter3"}  //[ов]
    };
    char meters_speech[3][100]={
        {"meter1"}, //[]
        {"meter2"}, //[а]
        {"meter3"}  //[ов]
    };
    int i=-1;

    if (meters>999){ //если больше 1000, значит сказать N километров перед метрами
        i = get_last_word_idx((int)meters/1000);
        buf=speech_numbers((int)meters/1000) + QString(kmeter_speech[i]);
        meters = meters-((int)meters/1000)*1000;
    };

    i = get_last_word_idx(meters);
    buf = buf + " " + speech_numbers(meters) + QString(meters_speech[i]);

//    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "PLAY " + buf;

    return(buf);

}

void MainWindow::checkandnotifycam()
{
    QString speechcam;

    if (cam_distance ==-1){
        if ((cam_notified_beep)||(cam_notified_voice)) {
            cam_notified_beep=false;
            cam_notified_voice=false;
            last_cam_id=-1;
        //Если проехали камеру
            snotify->play("cam_away");
        }
        return;
    }

    if ((cam_distance != -1)&&(cam_id != last_cam_id)){
        cam_notified_beep=false;
        cam_notified_voice=false;
    }

    if ((my_speed <= cam_speed)&&(!cam_notified_beep)){
        cam_notified_beep=true;
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "New cam detected";
        snotify->play("sonar");
        last_cam_id=cam_id;
        return;
    }

    if ((my_speed > (cam_speed+speed_over_delta))&&(!cam_notified_voice)){
        if (!snotify->isBusy()){
            cam_notified_voice=true;
            last_cam_id=cam_id;

            speechcam= "speed_control limit " + say_current_speed(cam_speed,true) + " distance " + say_current_meters_ptr(cam_distance);
            snotify->play(speechcam);
        }
    }

}

//Расстояние, необходимое для снижения скорости, с учетом потери за 3 секунды скорости на 10 км/ч
qreal MainWindow::add_slow_meters(int curr_speed, int speed_lim){
    qreal retw;
    if (curr_speed < speed_lim) return(0);
    retw=(pow(curr_speed,2) - pow(speed_lim,2))/(SLOW_BOOST_FACTOR*2);
    return (retw);
}

void MainWindow::drawScreen1()
{
    QPainter paint(this);
    QRect rec,speed_rec,kmh_rec;
    QBrush brush;
    char waitptr[4] = { '|', '/', '-', '\\' };
    int l,h;
    int pic_x=0;

    h = this->height();
    l = this->width();
    rec = QRect(0,0,l,h);

    if ((lw_height != h)||(lw_width != l)){
        adjustFontSize( fontspeed, l-l/8, "000" );
        adjustFontSize( fontkmh, l/8, "km/h" );
        fs_speed = QFontMetrics(fontspeed).height();
        fs_kmh = QFontMetrics(fontkmh).height();
        lw_height=h;
        lw_width=l;
    }

    speed_rec=QRect(0,h-(h/2)-(fs_speed/2),l-l/8,fs_speed);
    kmh_rec=QRect((int)(l-l/8),h-(h/2)-(fs_kmh/2),(int)l/8,fs_kmh);

    brush = QBrush(Qt::black, Qt::SolidPattern);
    paint.setPen(QPen(Qt::black,1));
    paint.setBrush(brush);
    paint.drawRect(rec);

    //Скорость текущая
    paint.setFont(fontspeed);
    paint.setPen(QPen(Qt::white,3));
    if (my_speed > -1){
        paint.drawText( speed_rec, Qt::AlignRight, QString::number(my_speed));
        paint.setFont(fontkmh);
        paint.drawText( kmh_rec, Qt::AlignRight, "km/h");
    }
    else
    {
        paint.drawText( speed_rec, Qt::AlignRight, QString(waitptr[waitpointer++]));
        if (waitpointer >=4) waitpointer=0;
    }

    if (!cam_notify_on){
        paint.drawImage(QPoint(pic_x,0),*noradarpic);
        pic_x +=35;
    }
    if (!speed_notify_on){
        paint.drawImage(QPoint(pic_x,0),*mutepic);
        pic_x +=35;
    }
    if (!satellitesignal){
        paint.drawImage(QPoint(pic_x,0),*nosignalpic);
        pic_x +=35;
    }
    if (recordingtrack){
        paint.drawImage(QPoint(pic_x,0),*trackrecordpic);
        pic_x +=35;
    }
}

void MainWindow::writesetting(QString varname, QVariant vol)
{
    QSettings settings("rinf.ini",QSettings::IniFormat);
    settings.setValue(varname,vol);

}

void MainWindow::loadpoi(QString path)
{
    QDir cdir(path);
    QString filename;
    cdir.setFilter(QDir::Files);
    cdir.setNameFilters(QStringList("*.rinf"));

    for (int i=0; i<cdir.entryList().count();i++){
        filename=cdir.entryList().at(i);
        if (i==0){
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "loading " << filename;
            gpsi->loadobjects(path + "/" + filename);
        }
        else {
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "appending " << filename;
            gpsi->appendobjects(path + "/" + filename);
        }
    }
}

void MainWindow::set_from_emu_pnt(qint64 from_pnt)
{
    from_emu_pnt = from_pnt;
}
