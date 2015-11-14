#ifndef _OBJEWRITER_HXX_
#define _OBJEWRITER_HXX_

#include <ostream>
#include <vector>
#include "vec3.hxx"

using namespace std;

namespace osmwave {
    class ObjWriter {
        std::ostream& stream;
        int vertIndex;

    public:
        ObjWriter(std::ostream& stream);

        void materialLibrary(const std::string& path);

        void material(const std::string& materialName);

        int write(vector<Vec3<double>>& vertices, vector<vector<int>>& faces);
    };
}

#endif
