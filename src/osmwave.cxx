/*

  This is a small tool that counts the number of nodes, ways, and relations in
  the input file.

  The code in this example file is released into the Public Domain.

*/

#include <cstdint>
#include <iostream>
#include <memory>

#include <osmium/index/map/dummy.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>

#include <osmium/handler/node_locations_for_ways.hpp>

#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <proj_api.h>
#include "ObjWriter.hxx"

using namespace std;
using namespace osmwave;

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_type;
typedef osmium::handler::NodeLocationsForWays<index_type> location_handler_type;

projPJ wgs84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

class ObjHandler : public osmium::handler::Handler {
    projPJ proj;
    ObjWriter writer;
    vector<double> wayCoords;

public:
    ObjHandler(projPJ p, ObjWriter writer) : proj(p), writer(writer) {}

    void way(osmium::Way& way) {
        const osmium::TagList& tags = way.tags();
        const char* building = tags.get_value_by_key("building");
        const char* highway = tags.get_value_by_key("highway");

        if (!building /*&& !highway*/) {
            return;
        }

        osmium::WayNodeList& nodes = way.nodes();
        int nNodes = nodes.size();

        if (wayCoords.capacity() < nNodes) {
            wayCoords.reserve(nNodes * 3);
            cerr << "Resized buffers to " << nodes.size() << '\n';
        }

        for (auto& nr : nodes) {
            wayCoords.push_back(nr.lon() * DEG_TO_RAD);
            wayCoords.push_back(nr.lat() * DEG_TO_RAD);
            wayCoords.push_back(0);
        }

        pj_transform(wgs84, proj, nodes.size(), 3, wayCoords.data(), wayCoords.data() + 1, nullptr);

        footprintVolume(wayCoords);

        wayCoords.clear();
    }

private:

    void footprintVolume(vector<double> wayCoords) {
        int nVerts = wayCoords.size() / 3;
        int vertexCount = 0;
        writer.checkpoint();
        for (vector<double>::iterator i = wayCoords.begin(); i != wayCoords.end(); i += 3) {
            writer.vertex(*(i + 1), *(i + 2), *i);
            writer.vertex(*(i + 1), *(i + 2) + 8, *i);

            if (vertexCount) {
                writer.beginFace();
                writer << (vertexCount - 2) << (vertexCount) << (vertexCount + 1) << (vertexCount - 1);
                writer.endFace();
            }

            vertexCount += 2;
        }

        writer.beginFace();
        for (int i = 0; i < nVerts; i++) {
            writer << (i * 2 + 1);
        }
        writer.endFace();
    }
};

projPJ get_proj(osmium::io::Header& header) {
    auto& box = header.boxes()[0];
    float clat = (box.bottom_left().lat() + box.top_right().lat()) / 2;
    float clon = (box.bottom_left().lon() + box.top_right().lon()) / 2;

    ostringstream stream;
    stream << "+proj=tmerc +lat_0=" << clat << " +lon_0=" << clon << " +k=1.000000 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs";
    string projDef = stream.str();

    return pj_init_plus(projDef.c_str());
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " OSM_FILE\n";
        return 1;
    }

    string input_filename(argv[1]);
    osmium::io::Reader reader(input_filename, osmium::osm_entity_bits::node | osmium::osm_entity_bits::way);
    osmium::io::Header header = reader.header();
    projPJ proj = get_proj(header);

    const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, osmium::Location>::instance();
    unique_ptr<index_type> index = map_factory.create_map("sparse_mem_array");
    location_handler_type location_handler(*index);
    location_handler.ignore_errors();

    ObjHandler handler(proj, ObjWriter(cout));
    osmium::apply(reader, location_handler, handler);
    reader.close();
}

