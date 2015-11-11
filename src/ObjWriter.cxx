#include "ObjWriter.hxx"

namespace osmwave {
    ObjWriter::ObjWriter(std::ostream& stream) : stream(stream), vertIndex(1) {
    }

    void ObjWriter::materialLibrary(const std::string& path) {
        this->stream << "mtllib" << path << std::endl;
    }

    void ObjWriter::material(const std::string& material) {
        this->stream << "mtl" << material << std::endl;
    }

    template<class VertIterator, class FaceIterator> 
    int ObjWriter::write(VertIterator vertices, FaceIterator faces) {
        int startVertIndex = this->vertIndex;
        for (auto v : vertices) {
          this->stream << "v " << v.getX() << ' ' << v.getY() << ' ' << v.getZ() << std::endl;
          this->vertIndex++;
        }

        for (auto f : faces) {
            this->stream << 'f';
            for (auto vertIndex : vertices) {
                this->stream << ' ' << (vertIndex + startVertIndex);
            }
        }

        return vertIndex;
    }
}
