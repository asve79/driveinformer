#include <QDebug>
#include <QDateTime>
#include <QObject>
#include <math.h>
#include <gps.h>
#include "gpsmath.h"
#include "gps-func.h"
#include "radarobjects.h"

gps::gps(){
    this->mapObjects=NULL;
    last_epx=-1;
    last_epy=-1;
    l_azimuth=-1;
}

gps::~gps(){
    if (mapObjects != NULL) unload_list_obj(mapObjects);

}

//Освободить память выделенную для объектов
void gps::unload_list_obj(struct mapobject *firstMO){
    struct mapobject *currMO, *nextMO;
    int i=0;

    currMO = firstMO;
    while (currMO != NULL){
        nextMO = currMO->next;
        free(currMO);
        currMO = nextMO;
        i++;
    }
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "release " << i << " records from memory";
}

//Градусы в радианы
double gps::degtorad(qreal degrees){
    return degrees * (M_PI / 180);
}

//радианы в градусы
int gps::radtodeg(qreal radians){
    qreal degrees;
    degrees=radians * (180 / M_PI);
    if (degrees>=0) return (int)degrees;
        else
    return (int)(360+degrees);
}

//Широту в метры
double gps::lattometer(double lat){
    return lat*Lekv/360;
}

//Долготу в метры
double gps::lontometer(double lat, double lon){
    return lon*cos(degtorad(lat))*Lmer/360;
}

//Находим угол наклона направления движения по координатам
int gps::getangle(double x1, double y1, double x2, double y2){
//    return(radtodeg(atan2((y2-y1),(x2-x1))));

    qreal radangle, tmpa;

    radangle=atan2((y2-y1),(x2-x1));
    tmpa=radtodeg(radangle);
    if (tmpa < 0){
        qDebug() << "ERROR ANGLE x1:" << x1 << " y1:" << y1 << " x2:" << x2 << " y2:" << y2;
    }

    tmpa=360-tmpa+90;
    if (tmpa>360) tmpa -= 360;
    return (int)tmpa;
}


//Загрузка данных из файла
mapObjectType* gps::load_info(QString *filename){

    FILE *in;
    int i;
    mapRecordType mpTmp;
    struct mapobject *firstMapObject, *currMapObject, *newMapObject;

    if ((in = fopen(filename->toAscii(),"rb"))==NULL){
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Error opening file " << filename;
        return NULL;
    }

    currMapObject = NULL;
    firstMapObject = NULL;
    newMapObject = NULL;
    i = 0;

    while(!feof(in)){
        fread(&mpTmp,sizeof(mpTmp),1,in);
        if (mpTmp.type>5) continue;                 //Берем пока информацию только о камерах
        if (!(newMapObject = (struct mapobject *)malloc(sizeof(struct mapobject)))){
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Memory allocation error";
            fclose(in);
            return firstMapObject;
        }

        newMapObject->idx = mpTmp.idx;
        newMapObject->x = mpTmp.x;
        newMapObject->y = mpTmp.y;
        newMapObject->lon = (int)mpTmp.x;
        newMapObject->lon2 = (int)((mpTmp.x-newMapObject->lon)*10000);
        newMapObject->epx = (int)lontometer(mpTmp.y,mpTmp.x); //!
        newMapObject->lat = (int)mpTmp.y;
        newMapObject->lat2 = (int)((mpTmp.y-newMapObject->lat)*10000);

//        GPS_Math_WGS84LatLonH_To_XYZ(mpTmp.y,mpTmp.x,0,&epx,&epy,&zero);
//        GPS_Math_WGS84_To_ICS_EN(mpTmp.y,mpTmp.x,&epx,&epy);
//        newMapObject->epx = epx;
//        newMapObject->epy = epy;
        newMapObject->epy = (int)lattometer(mpTmp.y); //!!
        newMapObject->speed = mpTmp.speed;
        //Поворачиваем азимут камеры на 90 градусов, а затем отражаем
        //Это нужно чтобы углы камер совпадали с координатрными углами движения
//        newMapObject->angle = (mpTmp.angle-90) < 0 ? 270+mpTmp.angle : mpTmp.angle-90;
//        newMapObject->angle = 360 - newMapObject->angle;
        newMapObject->angle = mpTmp.angle;
        newMapObject->dirType = mpTmp.dirType;
        newMapObject->camType = mpTmp.type;

        if (firstMapObject == NULL){
//            printf("!!!");
            firstMapObject = newMapObject;
        }else{
//             printf("*");
             currMapObject->next = newMapObject;
        }
/*		if (i  != 0){
            printf("cur addr: %p next addr: %p\n",currMapObject, currMapObject->next);
        } */
        currMapObject = newMapObject;
        i++;

    }


    fclose(in);
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " Loaded " << i << " records";

    return firstMapObject;
}

//Загрузка данных
bool gps::loadobjects(QString filename){
    if ((this->mapObjects=load_info(&filename))==NULL)
        return false;
     else
        return true;
}

//Загрузка данных и присоединение к загруженному списку.
bool gps::appendobjects(QString filename){
    mapobject *newobj,*cobj;
    if ((newobj=load_info(&filename))!=NULL){
        cobj=mapObjects;
        while (cobj->next != NULL)
            cobj=cobj->next;
        cobj->next = newobj;
        return true;
    }
     else
        return false;
}

/* @func GPS_Math_Deg_To_Rad *******************************************
**
** Convert degrees to radians
**
** @param [r] v [double] degrees
**
** @return [double] radians
************************************************************************/

double gps::GPS_Math_Deg_To_Rad(double v)
{
    return v*(double)((double)GPS_PI/(double)180.);
}

/* @func GPS_Math_Rad_To_Deg *******************************************
**
** Convert radians to degrees
**
** @param [r] v [double] radians
**
** @return [double] degrees
************************************************************************/

double gps::GPS_Math_Rad_To_Deg(double v)
{
    return v*(double)((double)180./(double)GPS_PI);
}


/* @func GPS_Math_LatLonH_To_XYZ **********************************
**
** Convert latitude and longitude and ellipsoidal height to
** X, Y & Z coordinates
**
** @param [r] phi [double] latitude (deg)
** @param [r] lambda [double] longitude (deg)
** @param [r] H [double] ellipsoidal height (metres)
** @param [w] x [double *] X
** @param [w] y [double *] Y
** @param [w] z [double *] Z
** @param [r] a [double] semi-major axis (metres)
** @param [r] b [double] semi-minor axis (metres)
**
** @return [void]
************************************************************************/
void gps::GPS_Math_LatLonH_To_XYZ(double phi, double lambda, double H,
                 double *x, double *y, double *z,
                 double a, double b)
{
    double esq;
    double nu;

    phi    = GPS_Math_Deg_To_Rad(phi);
    lambda = GPS_Math_Deg_To_Rad(lambda);


    esq   = ((a*a)-(b*b)) / (a*a);

    nu    = a / pow(((double)1.0-esq*sin(phi)*sin(phi)),(double)0.5);
    *x    = (nu+H) * cos(phi) * cos(lambda);
    *y    = (nu+H) * cos(phi) * sin(lambda);
    *z    = (((double)1.0-esq)*nu+H) * sin(phi);

    return;
}


/* @func GPS_Math_XYX_To_LatLonH ***************************************
**
** Convert XYZ coordinates to latitude and longitude and ellipsoidal height
**
** @param [w] phi [double] latitude (deg)
** @param [w] lambda [double] longitude (deg)
** @param [w] H [double] ellipsoidal height (metres)
** @param [r] x [double *] X
** @param [r] y [double *] Y
** @param [r] z [double *] Z
** @param [r] a [double] semi-major axis (metres)
** @param [r] b [double] semi-minor axis (metres)
**
** @return [void]
************************************************************************/
void gps::GPS_Math_XYZ_To_LatLonH(double *phi, double *lambda, double *H,
                 double x, double y, double z,
                 double a, double b)
{
    double esq;
    double nu=0.0;
    double phix;
    double nphi;
    double rho;
    unsigned int    t1=0;
    unsigned int    t2=0;

    if(x<(double)0 && y>=(double)0) t1=1;
    if(x<(double)0 && y<(double)0)  t2=1;

    rho  = pow(((x*x)+(y*y)),(double)0.5);
    esq  = ((a*a)-(b*b)) / (a*a);
    phix = atan(z/(((double)1.0 - esq) * rho));
    nphi = (double)1e20;

    while(fabs(phix-nphi)>(double)0.00000000001)
    {
    nphi  = phix;
    nu    = a / pow(((double)1.0-esq*sin(nphi)*sin(nphi)),(double)0.5);
    phix  = atan((z+esq*nu*sin(nphi))/rho);
    }

    *phi    = GPS_Math_Rad_To_Deg(phix);
    *H      = (rho / cos(phix)) - nu;
    *lambda = GPS_Math_Rad_To_Deg(atan(y/x));

    if(t1) *lambda += (double)180.0;
    if(t2) *lambda -= (double)180.0;

    return;
}


/* @func GPS_Math_WGS84LatLonH_To_XYZ **********************************
**
** Convert WGS84 latitude and longitude and ellipsoidal height to
** X, Y & Z coordinates
**
** @param [r] phi [double] latitude (deg)
** @param [r] lambda [double] longitude (deg)
** @param [r] H [double] ellipsoidal height (metres)
** @param [w] x [double *] X
** @param [w] y [double *] Y
** @param [w] z [double *] Z
**
** @return [void]
************************************************************************/
void gps::GPS_Math_WGS84LatLonH_To_XYZ(double phi, double lambda, double H,
                  double *x, double *y, double *z)
{
    double a = 6378137.000;
    double b = 6356752.3141;

    GPS_Math_LatLonH_To_XYZ(phi,lambda,H,x,y,z,a,b);

    return;
}


/* @func GPS_Math_XYZ_To_WGS84LatLonH **********************************
**
** Convert XYZ to WGS84 latitude and longitude and ellipsoidal height
**
** @param [r] phi [double] latitude (deg)
** @param [r] lambda [double] longitude (deg)
** @param [r] H [double] ellipsoidal height (metres)
** @param [w] x [double *] X
** @param [w] y [double *] Y
** @param [w] z [double *] Z
**
** @return [void]
************************************************************************/
void gps::GPS_Math_XYZ_To_WGS84LatLonH(double *phi, double *lambda, double *H,
                  double x, double y, double z)
{
    double a = 6378137.000;
    double b = 66356752.3141;

    GPS_Math_XYZ_To_LatLonH(phi,lambda,H,x,y,z,a,b);

    return;
}

// Точное расстояние в метрах между двумя точками
double gps::getDistance(double lat1, double lon1, double lat2, double lon2) {
    double d_lon, slat1, slat2, clat1,clat2,sdelt,cdelt,x,y;

    lat1 *= M_PI / 180;
    lat2 *= M_PI / 180;
    lon1 *= M_PI / 180;
    lon2 *= M_PI / 180;

    d_lon = lon1 - lon2;

    slat1 = sin(lat1);
    slat2 = sin(lat2);
    clat1 = cos(lat1);
    clat2 = cos(lat2);
    sdelt = sin(d_lon);
    cdelt = cos(d_lon);

    y = pow(clat2 * sdelt, 2) + pow(clat1 * slat2 - slat1 * clat2 * cdelt, 2);
    x = slat1 * slat2 + clat1 * clat2 * cdelt;

    return atan2(sqrt(y), x) * 6372795;
}

QList<radar_objects_type> gps::get_objects_detected(double my_lat, double my_lon, int area){

    QList<radar_objects_type> summary;
    struct mapobject *currMO, *nextMO;

    radar_objects_type rto;

    int distance;

    currMO = mapObjects ;
    int i=0;
    while (currMO != NULL){
        nextMO = currMO->next;

        distance=getDistance(my_lat,my_lon,currMO->y,currMO->x);
        if (distance <= area){
            rto.id = currMO->idx;
            rto.lat= currMO->y;
            rto.lon= currMO->x;
            rto.x= currMO->epx;
            rto.y= currMO->epy;
            rto.speedlimit= currMO->speed;
            rto.angle= currMO->angle;
            rto.cur_distance=distance;
            rto.nearless = false;
            rto.nearless_warn = false;
            if (currMO->dirType==0)
                rto.roundangle=true;
            else
                rto.roundangle=false;
            summary.push_back(rto);
            i++;

//            if ((rto.id==272828)||(rto.id==293578))
//                qDebug() << rto.id << " " << rto.lat << " " << rto.lon << " x:" << rto.x << " y:" << rto.y;

        }

        currMO = nextMO;
    }

//    qDebug() << "My:   " << " " << my_lat << " " << my_lon << " x:" << (int)lontometer(my_lat,my_lon) << " y:" << (int)lattometer(my_lat);

//    qDebug()<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") <<" Found " << i << " cams over " << area << " meters";

    return(summary);
}


// Easting,
// E =  FE + k0*nu[A + (1 - T + C)A^3/6 + (5 - 18T + T^2 + 72C - 58e'sq)A^5/120]
//
// Northing,
// N =  FN + k0{M - M0 + nu*tan(lat)[A^2/2 + (5 - T + 9C + 4C^2)A^4/24 +
//             (61 - 58T + T^2 + 600C - 330e'sq)A^6/720]}
// where
//   T = tan^2(lat)
//   nu = a /(1 - esq*sin^2(lat))^0.5
//   C = esq*cos^2(lat)/(1 - esq)
//   A = (lon - lon0)cos(lat), with lon and lon0 in radians.
//   M = a[(1 - esq/4 - 3e^4/64 - 5e^6/256 -....)lat -
//         (3esq/8 + 3e^4/32 + 45e^6/1024+....)sin(2*lat) +
//         (15e^4/256 + 45e^6/1024 +.....)sin(4*lat) -
//         (35e^6/3072 + ....)sin(6*lat) + .....]
//

void gps::WGStoGK(double iLat, double iLon, double *x, double *y)
{
//    double A, a, T, nu, e, esq, e_sq, E4;
//    double E6, C, Lon0, Lat0, M, M0, K0;
//    double alpha, FN, FE, Lat, Lon, Zone;

    double FN    = 0;
    double FE    = 500000;
    double a     = 6378245;
    double alpha = 1 / 298.3; // сжатие Земли
    double K0    = 0.9996013;
    double Zone  = 39;

    double Lat = iLat / 57.29577951;
    double Lon = iLon / 57.29577951;
    double Lat0 = 0;// / 57.29577951;
    double Lon0 = Zone / 57.29577951;

    double esq = alpha * 2 - pow(alpha,2);
    double e   = sqrt(esq);
    double C   = esq * pow(cos(Lat),2) / (1 - esq);
    double nu  = a / sqrt(1 - esq * pow(sin(Lat),2));
    double A   = (Lon - Lon0) * cos(Lat);
    double T   = pow(tan(Lat),2);

    double E4 = pow(e,4);
    double E6 = pow(e,6);

    double M   = a*(
                (1 - esq/4   -  3*E4/64 -  5*E6/256 )*Lat-
                (  3*esq/8   +  3*E4/32 + 45*E6/1024)*sin(2*Lat)+
                (  15*E4/256 + 45*E6/1024           )*sin(4*Lat)-
                (  35*E6/3072                       )*sin(6*Lat)
                );
    double M0  = a*(
                (1 - esq/4   -  3*E4/64 -  5*E6/256 )*Lat0-
                (  3*esq/8   +  3*E4/32 + 45*E6/1024)*sin(2*Lat0)+
                (  15*E4/256 + 45*E6/1024           )*sin(4*Lat0)-
                (  35*E6/3072                       )*sin(6*Lat0)
                );


    *x = FE + K0 * nu * (A + (1 - T + C) * pow(A,3) / 6 + (5 - 18 * T + pow(T,2) + 72 * C - 58 * esq) * pow(A,5) / 120);
    *y = FN + K0 * (M - M0 + nu * tan(Lat) *
                    (pow(A,2) / 2 + (5 - T + 9 * C + 4 * pow(C,2)) *
                     pow(A,4) / 24 + (61 - 58 * T + pow(T,2) + 600 * C - 330 * esq) *
                     pow(A,6) / 720));

}

//Инициализируем подключеие к GPS
bool gps::initgps()
{
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") <<"Connect to GPSd";
    for (;;){
//        if(gps_open("192.168.10.39", "2947", &gpsdata)<0){
        if(gps_open("localhost", "2947", &gpsdata)<0){
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Could not connect to GPSd. Try to coccnect after 10 seconds...";
            return false;
        } else {
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "done";
            if(gps_stream(&gpsdata, WATCH_ENABLE | WATCH_JSON, NULL) != 0){
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Error opening GPS stream.";
                gps_close(&gpsdata);
                return false;
            }
            return true;
        }
    }
}

//Ждем данных
bool gps::waitgpsdata()
{

    while(!gps_waiting(&gpsdata,1));
    return gps_waiting(&gpsdata,1);

}

//Возвращаем данные
//return code:
//-1 error; 0 - no data 1 - success 2 - no satelites
int gps::getgpsdata(double *time, double *lat, double *lon, int *x, int *y, int *speed, int *azimuth)
{

    int c_epx, c_epy;

    if (!gps_waiting(&gpsdata,1)){
//        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "no data";
        return 0;
    }

    while (gps_waiting(&gpsdata,1))
        if(gps_read(&gpsdata)==-1){
            qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") <<"GPSd waiting data error";
            gps_stream(&gpsdata, WATCH_DISABLE, NULL);
           gps_close(&gpsdata);
           return -1;
    }

    if ((gpsdata.fix.mode == MODE_2D)||(gpsdata.fix.mode == MODE_3D)){
        *time=gpsdata.fix.time;
        *lat=gpsdata.fix.latitude;
        *lon=gpsdata.fix.longitude;
        *speed=gpsdata.fix.speed*3.6; //Переводим метры в секунду в километры в час.;
        *x=(int)lontometer(*lat,*lon);
        *y=(int)lattometer(*lat);

        if ((l_azimuth != -1)&&
                (*speed >= FIX_AZIMUTH_SPEED) &&
                (getDistance(gpsdata.fix.latitude,gpsdata.fix.longitude,last_fix_gpsdata.fix.latitude,last_fix_gpsdata.fix.longitude) >= FIX_AZIMUTH_METERS)
                ){
            if ((gpsdata.fix.latitude==last_fix_gpsdata.fix.latitude)&&
                 gpsdata.fix.longitude==last_fix_gpsdata.fix.longitude){
                *azimuth=l_azimuth;
//                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "coords equal, last az fix";
            }else{
                c_epx=*x;
                c_epy=*y;
                *azimuth = getangle(last_epx,last_epy,c_epx,c_epy);
                last_fix_gpsdata=gpsdata;
                last_epx=c_epx;
                last_epy=c_epy;
                l_azimuth=*azimuth;
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "Normal fix" << *azimuth;
            }
        }else{
            if (l_azimuth == -1){        //Если первая фиксация
                qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "First fix";
                last_fix_gpsdata=gpsdata;
                last_epx=lontometer(gpsdata.fix.latitude,gpsdata.fix.longitude);
                last_epy=lattometer(gpsdata.fix.latitude);
                *azimuth=0;             //Пока вернем 0, но можно возвращать -1 что еще нет значения
                l_azimuth=0;
            }else{
                *azimuth=l_azimuth;
            }
        }

        return 1;
    }else {
        qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " No GPS Signal";
        return 2;
    }
    }

void gps::freegps()
{
    qDebug() << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "GPSd free";
    gps_stream(&gpsdata, WATCH_DISABLE, NULL);
    gps_close(&gpsdata);

}
