#include <cstdint>
#include <iostream>
#include <memory>
#include <algorithm>
#include <boost/regex.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>

#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>

#include <osmium/handler/node_locations_for_ways.hpp>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <proj_api.h>
//#include "earcut.hxx"
#include "ObjWriter.hxx"
#include "elevation.hxx"

using namespace std;
using namespace boost;
using namespace osmwave;

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_type;
typedef osmium::handler::NodeLocationsForWays<index_type> location_handler_type;

projPJ wgs84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
const double METERS_PER_LEVEL = 3.0;

class ObjHandler : public osmium::handler::Handler {
    projPJ proj;
    ObjWriter writer;
    vector<double> wayCoords;
    Elevation& elevation;
    double defaultBuildingHeight;

public:
    ObjHandler(projPJ p, ObjWriter writer, Elevation& elevation, double defaultBuildingHeight = 8) : 
        proj(p), writer(writer), elevation(elevation), defaultBuildingHeight(defaultBuildingHeight) {}

    void area(osmium::Area& area) {
        const osmium::TagList& tags = area.tags();
        const char* building = tags.get_value_by_key("building");
        const char* highway = tags.get_value_by_key("highway");

        if (!building /*&& !highway*/) {
            return;
        }

        double height = getBuildingHeight(tags, defaultBuildingHeight, "height", "building:levels");
        double baseHeight = getBuildingHeight(tags, 0, "min_height", "building:min_level");

        for (auto oit = area.cbegin<osmium::OuterRing>(); oit != area.cend<osmium::OuterRing>(); ++oit) {
            const osmium::NodeRefList& nodes = *oit;
            int nNodes = nodes.size();

            if (wayCoords.capacity() < nNodes) {
                wayCoords.reserve(nNodes * 2);
            }

            double minElevation = numeric_limits<double>::max();
            for (auto& nr : nodes) {
                double lon = nr.lon();
                double lat = nr.lat();
                wayCoords.push_back(lon * DEG_TO_RAD);
                wayCoords.push_back(lat * DEG_TO_RAD);
                minElevation = min(minElevation, elevation.elevation(lat, lon));
            }

            pj_transform(wgs84, proj, nodes.size(), 2, wayCoords.data(), wayCoords.data() + 1, nullptr);

            ringWalls(wayCoords, minElevation + baseHeight, height - baseHeight);
            flatRoof(nNodes);

            wayCoords.clear();
        }
    }

private:
    void ringWalls(vector<double> wayCoords, double elevation, double height) {
        int nVerts = wayCoords.size() / 2;
        int vertexCount = 0;
        writer.checkpoint();
        for (vector<double>::iterator i = wayCoords.begin(); i != wayCoords.end(); i += 2) {
            writer.vertex(*(i + 1), elevation, *i);
            writer.vertex(*(i + 1), elevation + height, *i);

            if (vertexCount) {
                writer.beginFace();
                writer << (vertexCount - 2) << (vertexCount) << (vertexCount + 1) << (vertexCount - 1);
                writer.endFace();
            }

            vertexCount += 2;
        }
    }

    void flatRoof(int nVerts) {
        writer.beginFace();
        for (int i = 0; i < nVerts; i++) {
            writer << (i * 2 + 1);
        }
        writer.endFace();
    }

    double getBuildingHeight(const osmium::TagList& tags, double defaultHeight, const char* heightTagName, const char* levelTagName) {
        regex heightexpr("^\\s*([0-9\\.]+)\\s*(\\S*)\\s*$");
        const char* heightTag = tags[heightTagName];
        double height = defaultHeight;

        if (heightTag) {
            string heightStr(heightTag);
            auto match_begin = 
                sregex_iterator(heightStr.begin(), heightStr.end(), heightexpr);
            auto match_end = sregex_iterator();

            if (match_begin != match_end) {
                smatch match = *match_begin;
                height = stod(match[1]);
            }
        } else {
            const char* levelsTag = tags[levelTagName];
            if (levelsTag) {
                string levelsStr(levelsTag);
                try {
                    double levels = stod(levelsStr);
                    height = levels * METERS_PER_LEVEL;
                } catch (const invalid_argument& ia) {
                    cerr << "Unparseable value for \"building:levels\" tag: \"" << levelsStr << "\"" << endl; 
                }
            }
        }

        return height;
    }
};

string* get_proj(osmium::io::Header& header) {
    auto& box = header.boxes()[0];
    float clat = (box.bottom_left().lat() + box.top_right().lat()) / 2;
    float clon = (box.bottom_left().lon() + box.top_right().lon()) / 2;

    ostringstream stream;
    stream << "+proj=tmerc +lat_0=" << clat << " +lon_0=" << clon << " +k=1.000000 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
    return new string(stream.str());
}

namespace osmwave {
    void write_obj_header(ObjWriter& objWriter, const string& osmFile, const osmium::Location& sw, const osmium::Location& ne, projPJ proj) {
        ostringstream c;
        c.precision(7);

        objWriter.comment("Created with OSMWAVE");
        objWriter.comment("");

        c << "Input file: " << osmFile;
        objWriter.comment(c.str());
        cerr << c.str() << endl;

        c.str("");
        c << "Lat/lng bounds: (" << sw.lat() << ", " << sw.lon() << ") - (" << ne.lat() << ", " << ne.lon() << ")";
        objWriter.comment(c.str());
        cerr << c.str() << endl;

        c.str("");
        c << "Projection: " << pj_get_def(proj, 0);
        objWriter.comment(c.str());
        cerr << c.str() << endl;

        double coord[2];
        c.str("");
        c << "Projected bounds: (";
        coord[0] = sw.lon() * DEG_TO_RAD;
        coord[1] = sw.lat() * DEG_TO_RAD;
        pj_transform(wgs84, proj, 1, 2, coord, coord + 1, nullptr);
        c << coord[0] << ", " << coord[1];
        c << ") - (";
        coord[0] = ne.lon() * DEG_TO_RAD;
        coord[1] = ne.lat() * DEG_TO_RAD;
        pj_transform(wgs84, proj, 1, 2, coord, coord + 1, nullptr);
        c << coord[0] << ", " << coord[1];
        c << ")";
        objWriter.comment(c.str());
        cerr << c.str() << endl;
    }

    void osm_to_obj(const std::string& osmFile, const std::string& elevationPath, const std::string* projDef) {
        ObjWriter objWriter(cout);

        osmium::io::File infile(osmFile);
        osmium::area::Assembler::config_type assembler_config;
        osmium::area::MultipolygonCollector<osmium::area::Assembler> collector(assembler_config);

        osmium::io::Reader reader1(infile, osmium::osm_entity_bits::relation);
        collector.read_relations(reader1);
        reader1.close();

        osmium::io::Reader reader2(osmFile);
        osmium::io::Header header = reader2.header();
        projPJ proj;

        auto& box = header.boxes()[0];
        auto& sw = box.bottom_left();
        auto& ne = box.top_right();
        float clat = (box.bottom_left().lat() + box.top_right().lat()) / 2;
        float clon = (box.bottom_left().lon() + box.top_right().lon()) / 2;
        if (!projDef) {
            projDef = get_proj(header);
            proj = pj_init_plus(projDef->c_str());
            delete projDef;
        } else {
            proj = pj_init_plus(projDef->c_str());
        }

        write_obj_header(objWriter, osmFile, sw, ne, proj);

        const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
        unique_ptr<index_type> index = map_factory.create_map("sparse_mem_array");
        location_handler_type location_handler(*index);
        location_handler.ignore_errors();

        Elevation elevation((int)floor(sw.lat()), (int)floor(sw.lon()), (int)floor(ne.lat()), (int)floor(ne.lon()), elevationPath);
        ObjHandler handler(proj, objWriter, elevation);
        osmium::apply(reader2, location_handler, collector.handler([&handler](osmium::memory::Buffer&& buffer) {
            osmium::apply(buffer, handler);
        }));
        reader2.close();
    }
}

