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

using namespace std;

typedef osmium::index::map::Map<osmium::unsigned_object_id_type, osmium::Location> index_type;
typedef osmium::handler::NodeLocationsForWays<index_type> location_handler_type;

projPJ wgs84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

class ObjHandler : public osmium::handler::Handler {
    projPJ proj;

public:
    ObjHandler(projPJ p) : proj(p) {}

    void way(osmium::Way& way) {
        for (auto& nr : way.nodes()) {
            double y = nr.lat() * DEG_TO_RAD;
            double x = nr.lon() * DEG_TO_RAD;
            pj_transform(wgs84, this->proj, 1, 0, &x, &y, nullptr);

            cout << x << ", " << y << "\n";
        }
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

    ObjHandler handler(proj);
    osmium::apply(reader, location_handler, handler);
    reader.close();
}

