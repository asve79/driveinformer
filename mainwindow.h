#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QTimer>
#include <QFile>
#include <QDataStream>
#include <QKeyEvent>
#include <QImage>
#include "gps-func.h"
#include "radarobjects.h"
#include "soundnotify.h"

#define LOW_SPEED_LIMIT_NOTIFY 6
#define FULL_RADAR_DICTANCE 2000
#define MED_RADAR_DICTANCE 1000
#define WARN_RADAR_DICTANCE 600
#define FIX_RADAR_DICTANCE 400
#define NO_OUTPUT ">/dev/null 2>/dev/null"
#define SLOW_BOOST_FACTOR 3.3 //Замедление (обратно ускорению) на 10 км/ч за 3 секунды


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void trackemulation_on(char *filename);
    void set_from_emu_pnt(qint64 from_pnt);

private slots:
//    void on_pushButton_clicked();
//    void on_horizontalSlider_sliderMoved(int position);
    void gtimer_event();
    void vtimer_event();

private:
    MainWindow *ui;
    gps *gpsi;
    SoundNotify *snotify;
    QTimer *gtimer; //Для работы с GPS данными
    QTimer *vtimer; //Для перерисовки
    QImage *mutepic;
    QImage *noradarpic;
    QImage *nosignalpic;
    QImage *trackrecordpic;

    void paintEvent(QPaintEvent *);
    void keyPressEvent(QKeyEvent *event);
    void adjustFontSize( QFont &font, int width, const QString &text );
    bool gtimerwork; //true когда работает таймер, обрабатываюищй информацию с GPS
    bool vtimerwork; //true когда работает таймер, redraw
    bool satellitesignal; //true если есть сигнал со спутников, false если нет
//    QDataStream *trackds;
    int my_speed;
    int cam_distance;
    int cam_id;
    bool cam_id_fixed; //true если дистанция до камеры равна оповещаемой. Нужно для того, чтобы не если например мы притормозили перед камерой и дистанция для оповещения сократилась, не терять камеру из "вида"
    int cam_speed;
    int last_cam_id;
    int my_x;
    int my_y;
    int my_azimuth;  //Наш азимут, к которому будет поворачиваться видимый
    int vis_azimuth; //Видимый азимут
    int l_azimut; //Предыдущее значение азимута (для расчета при эмитации трека)
    double l_lat;
    double l_lon;
    int last_speed_cat; //Предыдущая скорость деленая на 10 (для отслеживания изменения скорости в пределах 10 км/ч)
    double my_lat;
    double my_lon;
    double my_time;
    int my_gmt;
    QString poipath;
    QString trackpath;
    qreal add_slow_meters(int curr_speed, int speed_lim);
    void writesetting(QString varname, QVariant vol);
    void loadpoi(QString path);

    /* Работа с о звуком */
    QString voice_lang;
    int id_lang; //0 - русский, 1 - английский
    QString voice_files_path;
    QString sound_files_path;
    int get_last_word_idx(int num);
    QString say_current_speed(int speed, bool camspeed);
    void checkandsayspeed();
    int last_say_unit; // 1 - if last say km/h (speed)  2 - if last was meters (mesuare)
    bool said_less_low_limit; // true if speed less one XX km/h
    QString speech_numbers(int num);
    bool cam_notified_voice;    //true если дано начальное сообщение о камере голосом
    bool cam_notified_beep;    //true если дано начальное сообщение о камере сигналом
    bool cam_notify_on;         //true если включено оповещение о камерах
    bool speed_notify_on;       //true если включено оповещение о скорости
    int speed_over_delta;       //Допустимое приращение к скоростному пределу, когда говориться о камере голосом

    void checkandnotifycam();
    QString say_current_meters_ptr(int meters);

    int gpsd_mode; //Режим работы с GPS
    bool gpsd_connected;
    bool objects_loaded;
    time_t gpspausetill; //Время, до которого не опрашивать gps;
    void make_obj_list(double my_lat, double my_lon, int visible_distance, int fix_distance);

    /* Вывод на экран */
    QList<radar_objects_type> radar_obj_list;
    bool drawing; //true когда выполняется прорисовка. Чтобы избежать пересечений.
    int active_scr;
    void drawScreen0();
    void drawScreen1();
    QPoint RotateVector(int tx, int ty, QPoint center, int angle);
    int anglediff(int angle_one, int angle_two );
    QFont fontspeed;
    QFont fontcamera;
    QFont fontkmh;
    QFont fontspeedlimit;
    QFont fontgpstime;
    int lw_width;
    int lw_height;
    int fs_speed;
    int fs_kmh;
    int vid_radar_distance;
    int radar_distance;
    int waitpointer;

    /* Работа с треком - запись/эмуляция */
    QFile *trackf;
    QList<way_log_type> wayloghist;
    qint64 emu_pnt_num;
    qint64 from_emu_pnt;
    bool recordingtrack; //true если идет запись трека
    bool trackemulator; //true если используем режим эмуляции поездки
    bool autorecordtrack;
    bool createtracklog();
    bool writetracklog();
    void closetracklog();
    void waypointlog();
    void trackemulation_off();
    bool getemudata(double *time, double *lat, double *lon, int *x, int *y, int *speed, int *azimuth);

};

#endif // MAINWINDOW_H
