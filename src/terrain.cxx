#include <boost/program_options.hpp>
#include <string>
#include <math.h>
#include <proj_api.h>
#include "elevation.hxx"
#include "ObjWriter.hxx"
#include "Delaunay.h"

using namespace std;
using namespace osmwave;

static projPJ wgs84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

// Note: in place addition
static void vecAdd(XYZ& a, const XYZ& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}

static void vecSub(const XYZ& a, const XYZ& b, XYZ& result) {
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
}

static void triangleNormal(const XYZ* coords, const ITRIANGLE& tri, XYZ& normal) {
    XYZ u;
    XYZ v;

    vecSub(coords[tri.p2], coords[tri.p1], u);
    vecSub(coords[tri.p3], coords[tri.p1], v);

    normal.x = u.y*v.z - u.z*v.y;
    normal.y = u.z*v.x - u.x*v.z;
    normal.z = u.x*v.y - u.y*v.x;

    double l = sqrt(normal.x*normal.x+normal.y*normal.y+normal.z*normal.z);

    normal.x = normal.x / l;
    normal.y = normal.y / l;
    normal.z = normal.z / l;
}

static double findNearHeight(int rows, int cols, int index, int dx, int dy, XYZ* verts) {
    int s = index;
    do {
        s += dx * rows + dy;
    } while (std::isnan(verts[s].z));

    return verts[s].z;
}

static int thin(int rows, int cols, XYZ* verts, double tolerance) {
    int index = 0;
    int nonEmpty = 0;

    for (int i = 1; i < cols - 1; i++) {
        for (int j = 1; j < rows - 1; j++) {
            if (!std::isnan(verts[index].z)) {
                double e1 = verts[index].z;
                double e2 = findNearHeight(rows, cols, index, 1, -1, verts);
                double e3 = findNearHeight(rows, cols, index, 1, 0, verts);
                double e4 = findNearHeight(rows, cols, index, 1, 1, verts);
                double d2 = abs(e1 - e2);
                double d3 = abs(e1 - e3);
                double d4 = abs(e1 - e4);

                if (d2 <= tolerance &&
                    d3 <= tolerance &&
                    d4 <= tolerance) {
                    verts[index].z = NAN;
                } else {
                    nonEmpty++;
                }
            }

            index++;
        }
    }

    return nonEmpty;
}

void terrain_to_obj(const std::string& elevationPath, const std::string& projDef, double x1, double y1, double x2, double y2) {
    Elevation elevation(floor(y1), floor(x1), ceil(y2), ceil(x2), elevationPath);
    ObjWriter writer(cout);
    projPJ proj = pj_init_plus(projDef.c_str());
    double step = 1.0 / 3600;
    int rows = (int)floor((y2 - y1) / step + 1);
    int cols = (int)floor((x2 - x1) / step + 1);
    double bounds[] = {x1*DEG_TO_RAD, y1*DEG_TO_RAD, x2*DEG_TO_RAD, y2*DEG_TO_RAD};

    pj_transform(wgs84, proj, 2, 2, (double*)&bounds, (double*)&bounds + 1, nullptr);

    cerr << "rows: " << rows << ", cols: " << cols << endl;
    cerr << "bounds: " << bounds[0] << ", " << bounds[1] << " - " << bounds[2] << ", " << bounds[3] << endl;

    cerr << "Calculating vertices..." << endl;
    XYZ* coords = new XYZ[rows * cols + 3];
    XYZ* normals = new XYZ[rows * cols];

    int i = 0;
    // Having columns as outer loop ensures x will be growing,
    // which is a requirement for the triangulation algorithm,
    // as long as projection is west to east.
    for (int c = 0; c < cols; c++) {
        double x = (bounds[2] - bounds[0]) * c / cols;
        for (int r = 0; r < rows; r++) {
            double y = (bounds[3] - bounds[1]) * r / rows;
            double ll[2] = {x, y};
            pj_transform(proj, wgs84, 1, 2, (double*)&ll, (double*)&ll + 1, nullptr);

            XYZ *coord = &coords[i++];
            coord->x = x;
            coord->y = y;
            coord->z = elevation.elevation(ll[1]*RAD_TO_DEG, ll[0]*RAD_TO_DEG);
        }
    }

    cerr << "Thinning..." << endl;
    int startCount = rows * cols,
        lastCount = 0,
        count = -1;
    while (lastCount != count) {
        lastCount = count;
        count = thin(rows, cols, coords, 2);
    }

    int j = 0;
    for (int i = 0; i < rows * cols; i++) {
        if (!std::isnan(coords[i].z)) {
            coords[j++] = coords[i];
        }
    }
    cerr << "Thinned to " << j << " vertices" << endl;

    cerr << "Triangulating..." << endl;
    ITRIANGLE *tris = new ITRIANGLE[3 * rows * cols];
    int numTriangles;
    Triangulate(j, coords, tris, numTriangles);
    cerr << numTriangles << " triangles" << endl;

    memset((void*)normals, 0, sizeof(XYZ) * j);
    for (int i = 0; i < numTriangles; i++) {
        XYZ normal;
        triangleNormal(coords, tris[i], normal);
        vecAdd(normals[tris[i].p1], normal);
        vecAdd(normals[tris[i].p2], normal);
        vecAdd(normals[tris[i].p3], normal);
    }

    writer.checkpoint();
    for (int i = 0; i < j; i++) {
        writer.vertex(coords[i].x, coords[i].z, coords[i].y, normals[i].x, normals[i].y, normals[i].z);
    }

    for (int i = 0; i < numTriangles; i++) {
        writer.beginFace();
        writer << tris[i].p1 << tris[i].p2 << tris[i].p3;
        writer.endFace();
    }

    delete coords;
    delete normals;
    delete tris;
}

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("elevation_dir,e", po::value<string>()->required(), "Set directory containing elevation data")
        ("proj,p", po::value<string>(), "Projection definition")
        ("x1", po::value<double>()->required(), "X1")
        ("y1", po::value<double>()->required(), "Y1")
        ("x2", po::value<double>()->required(), "X2")
        ("y2", po::value<double>()->required(), "Y2");
    po::positional_options_description positionOptions;
    positionOptions.add("x1", 1);
    positionOptions.add("y1", 1);
    positionOptions.add("x2", 1);
    positionOptions.add("y2", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv)
            .options(desc)
            .positional(positionOptions)
            .run(), vm);

        po::notify(vm);
    } catch (po::error& e) {
        cerr << "Error " << e.what() << endl << endl;
        cerr << desc << endl;
        return 1;
    }

    const string& elevPath(vm["elevation_dir"].as<string>());
    const string* projDef = nullptr;
    const double x1 = vm["x1"].as<double>();
    const double y1 = vm["y1"].as<double>();
    const double x2 = vm["x2"].as<double>();
    const double y2 = vm["y2"].as<double>();

    if (vm.count("proj")) {
        projDef = &vm["proj"].as<string>();
    } else {
        ostringstream stream;
        stream << "+proj=tmerc +lat_0=" << ((y1 + y2) / 2) << " +lon_0=" << ((x1 + x2) / 2) << " +k=1.000000 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
        projDef = new string(stream.str());
    }

    terrain_to_obj(elevPath, *projDef, x1, y1, x2, y2);

    return 0;
}

