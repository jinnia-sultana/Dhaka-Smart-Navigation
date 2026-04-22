#include "mode.h"

const double EARTH_RADIUS_KM = 6371.0;
const double PI = 3.14159265358979323846;

const char* getModeName(Mode mode) {
    switch(mode) {
        case MODE_WALK: return "Walk";
        case MODE_METRO: return "Metro";
        case MODE_BIKE: return "Bike";
        case MODE_RICKSHAW: return "Rickshaw";
        case MODE_BIKOLPO: return "Bikolpo Bus";
        case MODE_UTTARA: return "Uttara Bus";
        default: return "Unknown";
    }
}