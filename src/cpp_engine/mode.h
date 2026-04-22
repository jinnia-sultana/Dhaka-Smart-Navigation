#ifndef MODE_H
#define MODE_H


extern const double EARTH_RADIUS_KM;
extern const double PI;

typedef enum {
    MODE_WALK,
    MODE_METRO,
    MODE_BIKE,
    MODE_RICKSHAW,     
    MODE_BIKOLPO,
    MODE_UTTARA
} Mode;

const char* getModeName(Mode mode);

#endif