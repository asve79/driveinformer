#ifndef RADAROBJECTS_H
#define RADAROBJECTS_H

typedef struct radarobjects {
    int x;
    int y;
    double lat;
    double lon;
    int angle;
    int id;
    int speedlimit;
    int cur_distance;
    bool roundangle; //камера 360 градусов
    bool nearless;
    bool nearless_warn;
}radar_objects_type;

typedef struct tracklog {
    time_t datetime;
    double lat;
    double lon;
    int speed;
} track_log_type;

typedef struct waylog {
    time_t fixtime;
    double x;
    double y;
    int speed;
}way_log_type;

#endif // RADAROBJECTS_H
