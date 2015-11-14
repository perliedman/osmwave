#include "ObjWriter.hxx"

using namespace std;

namespace osmwave {
    ObjWriter::ObjWriter(std::ostream& stream) : stream(stream), vertIndex(1) {
    }

    void ObjWriter::materialLibrary(const std::string& path) {
        this->stream << "mtllib" << path << std::endl;
    }

    void ObjWriter::material(const std::string& material) {
        this->stream << "mtl" << material << std::endl;
    }

    int ObjWriter::write(vector<Vec3<double>>& vertices, vector<vector<int>>& faces) {
        int startVertIndex = this->vertIndex;
        for (auto& v : vertices) {
          this->stream << "v " << v.getX() << ' ' << v.getY() << ' ' << v.getZ() << std::endl;
          this->vertIndex++;
        }

        for (auto& f : faces) {
            this->stream << 'f';
            for (auto& vertIndex : f) {
                this->stream << ' ' << (vertIndex + startVertIndex);
            }
        }

        return vertIndex;
    }
}
