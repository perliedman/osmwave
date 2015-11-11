#ifndef _OBJEWRITER_HXX_
#define _OBJEWRITER_HXX_

#include <ostream>
#include <iterator>

namespace osmwave {
    class ObjWriter {
        std::ostream& stream;
        int vertIndex;

    public:
        ObjWriter(std::ostream& stream);

        void materialLibrary(const std::string& path);

        void material(const std::string& materialName);

        template<class VertIterator, class FaceIterator> 
        int write(VertIterator vertices, FaceIterator faces);
    };
}

#endif
