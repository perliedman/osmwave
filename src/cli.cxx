#include <iostream>
#include <boost/program_options.hpp>
#include <string>
#include "osmwave.hxx"

using namespace std;

int main(int argc, char* argv[]) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("elevation_dir,e", po::value<string>()->required(), "Set directory containing elevation data")
        ("proj,p", po::value<string>(), "Projection definition")
        ("osm_file", po::value<string>()->required(), "Input OSM data file");
    po::positional_options_description positionOptions;
    positionOptions.add("osm_file", 1);

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

    const string& input_filename = vm["osm_file"].as<string>();
    const string& elevPath(vm["elevation_dir"].as<string>());
    const string* projDef = nullptr;

    if (vm.count("proj")) {
        projDef = &vm["proj"].as<string>();
    }

    osmwave::osm_to_obj(input_filename, elevPath, projDef);

    return 0;
}

