OSM Wave
========

Convert OpenStreetMap buildings and roads into Wavefront OBJ 3D models.

## Dependencies

* C++11
* boost
* libosmium

## Building

```sh
mkdir build
cd build
cmake
make
```

## Running

To run, you need OSM data in PBF or XML format as well as HGT files (elevation data)
for the area your working with.

The resulting model is written to standard out.

```sh
./osmwave -e ELEVATION_DIRECTORY OSM_DATA_FILE >model.obj
```
