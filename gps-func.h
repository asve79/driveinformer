#include <QString>
#include <QApplication>
#include <gps.h>
#include "radarobjects.h"

#ifndef GPSFUNC_H
#define GPSFUNC_H

#define Lekv 40075700
#define Lmer 40008550

#define FIX_AZIMUTH_SPEED 5
#define FIX_AZIMUTH_METERS 10
#define SYSGPSSLEEP 50

#define GPSD_ADDR "localhost"
//#define GPSD_ADDR "192.168.10.39"

//Определение структуры хранения данных
typedef struct mapRecord {
        int idx;    //Номер объекта
        float x;    //Координата X (lon)
        float y;    //Координата Y (lat)
        int type;   //Тип камеры
        int speed;  //Ограничение скорости
        int dirType;//Тип направленности камеры
        int angle;  //Урол (азимут) камеры
    } mapRecordType;

//База данных объектов
typedef struct mapobject {
        int idx;
        float x;
        float y;
        int epx;
        int epy;
        int lat;
        int lat2;
        int lon;
        int lon2;
        int camType;
        int speed;
        int dirType;
        int angle;
        struct mapobject *next;
    } mapObjectType;

class gps {

private:

    mapObjectType *mapObjects;

    void unload_list_obj(struct mapobject *firstMO);
    mapObjectType* load_info(QString *filename);
    struct gps_data_t gpsdata; //текущая позиция
    struct gps_data_t last_fix_gpsdata; //Предыдущая координата, для фиксации азимута
    int last_epx, last_epy; //Переситанные координаты предыдущего положения

    double degtorad(qreal degrees);
    int radtodeg(qreal radians);

    double GPS_Math_Deg_To_Rad(double v);
    double GPS_Math_Rad_To_Deg(double v);

    void GPS_Math_LatLonH_To_XYZ(double phi, double lambda, double H,
                     double *x, double *y, double *z,
                     double a, double b);
    void GPS_Math_XYZ_To_LatLonH(double *phi, double *lambda, double *H,
                     double x, double y, double z,
                     double a, double b);

    void GPS_Math_WGS84LatLonH_To_XYZ(double phi, double lambda, double H,
                      double *x, double *y, double *z);
    void GPS_Math_XYZ_To_WGS84LatLonH(double *phi, double *lambda, double *H,
                      double x, double y, double z);


public:
    gps();
    ~gps();
    bool loadobjects(QString filename);
    bool appendobjects(QString filename);
    QList<radar_objects_type> get_objects_detected(double my_lat, double my_lon, int area);
    double lattometer(double lat);
    double lontometer(double lat, double lon);
    int l_azimuth;
    int getangle(double x1, double y1, double x2, double y2);
    void WGStoGK(double iLat, double iLon, double *x, double *y);
    bool initgps();
    int getgpsdata(double *time, double *lat, double *lon, int *x, int *y, int *speed, int *azimuth, int *num_satelites);
    void freegps();
    double getDistance(double lat1, double lon1, double lat2, double lon2);

};

#endif // GPSFUNC_H
