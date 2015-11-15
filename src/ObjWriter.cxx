#include "ObjWriter.hxx"
#include <iostream>

using namespace std;

namespace osmwave {
    ObjWriter::ObjWriter(std::ostream& stream) : stream(stream), vertIndex(1), offset(0) {
    }

    void ObjWriter::materialLibrary(const std::string& path) {
        stream << "mtllib" << path << std::endl;
    }

    void ObjWriter::material(const std::string& material) {
        stream << "mtl" << material << std::endl;
    }

    void ObjWriter::checkpoint() {
        offset = vertIndex;
    }

    int ObjWriter::vertex(double x, double y, double z) {
        stream << "v " << x << ' ' << y << ' ' << z << std::endl;
        vertIndex++;
    }

    void ObjWriter::beginFace() {
        stream << "f ";
    }

    void ObjWriter::endFace() {
        stream << '\n';
    }

    ObjWriter& ObjWriter::operator << (int index) {
        stream << (index + offset) << ' ';
        return *this;
    }
}
