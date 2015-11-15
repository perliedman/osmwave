#ifndef _OBJEWRITER_HXX_
#define _OBJEWRITER_HXX_

#include <ostream>

using namespace std;

namespace osmwave {
    class ObjWriter {
        std::ostream& stream;
        int vertIndex;
        int offset;

    public:
        ObjWriter(std::ostream& stream);

        void materialLibrary(const std::string& path);

        void material(const std::string& materialName);

        void checkpoint();

        int vertex(double x, double y, double z);
        void beginFace();
        ObjWriter& operator << (int vertex);
        void endFace();
    };
}

#endif
