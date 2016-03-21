OSM Wave
========

Convert OpenStreetMap buildings and roads into Wavefront OBJ 3D models.

![hildedal-south-20160321](https://cloud.githubusercontent.com/assets/1246614/13912511/05fb6ad4-ef3e-11e5-8dd6-d2211af36297.png)


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
