#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <math.h>
#include "elevation.hxx"

using namespace std;

namespace osmwave {
    Elevation::Elevation(int south, int west, int north, int east, const string& tilesPath) : 
    south(south), west(west), north(north), east(east), cols(east - west + 1), tileSize(0) {
        tiles = new uint8_t*[(north - south + 1) * cols];

        int i = 0;
        for (int lat = south; lat <= north; lat++) {
            for (int lon = west; lon <= east; lon++) {
                ostringstream ss;
                ss << tilesPath << "/" << (lat >= 0 ? 'N' : 'S') << setw(2) << setfill('0') << abs(lat) <<
                    (lon >= 0 ? 'E' : 'W') << setw(3) << abs(lon) << ".hgt";

                string filePath = ss.str();

                ifstream file(filePath.c_str(), ios::in | ios::binary | ios::ate);
                if (file.is_open()) {
                    int size = file.tellg();
                    int currTileSize;
                    switch (size) {
                    case 2884802:
                        currTileSize = 1201;
                        break;
                    case 25934402:
                        currTileSize = 3601;
                        break;
                    default:
                        cerr << "Unknown tile resolution in tile " << filePath << '\n';
                    }

                    if (tileSize != 0 && tileSize != currTileSize) {
                        cerr << "Warning! Tiles of different resolutions detected. This will not work.\n";
                    } else {
                        tileSize = currTileSize;
                    }

                    tiles[i] = new uint8_t[size];
                    file.seekg(0, ios::beg);
                    file.read((char*)tiles[i], size);

                    if (!file) {
                        cerr << "Read " << file.gcount() << " of expected " << size << " bytes from " << filePath << "\n";
                    }

                    //cerr << "Read " << file.gcount() << " bytes from " << filePath << '\n';
                    file.close();

                    i++;
                } else {
                    cerr << "Unable to open file " << ss.str() << '\n';
                }
            }
        }
    }

    Elevation::~Elevation() {
        int i = 0;
        for (int lat = south; lat <= north; lat++) {
            for (int lon = west; lon <= east; lon++) {
                delete tiles[i++];
            }
        }

        delete tiles;
    }

    double Elevation::getTileValue(uint8_t* tile, int index) {
        return tile[index] << 8 | tile[index + 1];
    }

    double Elevation::elevation(double lat, double lon) {
        double fLat = floor(lat);
        double fLon = floor(lon);
        int tileRow = (int)fLat - south;
        int tileCol = (int)fLon - west;
        uint8_t* tile = tiles[tileRow * cols + tileCol];

        double row = (lat - fLat) * tileSize;
        double col = (lon - fLon) * tileSize;

        int rowI = floor(row);
        int colI = floor(col);
        int index = ((tileSize - rowI - 1) * tileSize + colI) * 2;
        double rowFrac = row - rowI;
        double colFrac = col - colI;
        double v00 = getTileValue(tile, index);
        double v10 = getTileValue(tile, index + 2);
        double v11 = getTileValue(tile, index + tileSize*2 + 2);
        double v01 = getTileValue(tile, index + tileSize*2);
        double v1 = v00 + (v10 - v00) * colFrac;
        double v2 = v01 + (v11 - v01) * colFrac;

/*
        cerr << "lat=" << lat << ", lon=" << lon << ", tileRow=" << tileRow <<
            ", tileCol=" << tileCol << ", row=" << row << ", col=" << col << 
            ", near=" << v00 << ", output=" << (v1 + (v2 - v1) * rowFrac) << '\n';
*/
        return v1 + (v2 - v1) * rowFrac;
    }
}
