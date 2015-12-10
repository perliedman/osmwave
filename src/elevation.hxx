#ifndef __ELEVATION_HXX__
#define __ELEVATION_HXX__

#include <string>

using namespace std;

namespace osmwave {
    class Elevation {
        int south;
        int west;
        int north;
        int east;
        int cols;
        int tileSize;
        int8_t** tiles;

    public:
        Elevation(int south, int west, int north, int east, const string& tilesPath);
        ~Elevation();

        double elevation(double lat, double lon);

    private:
        double getTileValue(int8_t* tile, int index);
    };
}

#endif
