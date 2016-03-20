OSM Wave
========

Convert OpenStreetMap buildings and roads into Wavefront OBJ 3D models.

![gbg-2016-03-21 00 36 02](https://cloud.githubusercontent.com/assets/1246614/13908163/9036b17c-eefd-11e5-8384-e76a1a91e631.png)


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
