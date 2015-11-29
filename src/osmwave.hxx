#ifndef __OSMWAVE_HXX__
#define __OSMWAVE_HXX__

#include <string>

namespace osmwave {
    void osm_to_obj(const std::string& osmFile, const std::string& elevationPath, const std::string* projDef);
}

#endif
