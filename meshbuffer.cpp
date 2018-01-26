#include "meshbuffer.h"
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include "vertexattributeindices.h"

MeshBuffer::MeshBuffer()
    : UsesNormals(false)
    , UsesUVs(false)
    , UsesIndices(false)
    , UsesGenerics(5, false)
    , VertCnt(0)
    , IdxCnt(0)
    , Generics(5)
{

}

MeshBuffer::~MeshBuffer()
{
    cleanUp();
}

MeshBuffer::MeshBuffer(const MeshBuffer & ref)
    : UsesNormals(false)
    , UsesUVs(false)
    , UsesIndices(false)
    , VertCnt(0)
    , IdxCnt(0)
{
    this->operator =(ref);
}

MeshBuffer& MeshBuffer::operator=(const MeshBuffer & ref)
{
    if (this == &ref) return *this;
    cleanUp();

    setVerts(ref.VertCnt, (float*)&ref.Verts[0]);
    setNorms(ref.VertCnt, (float*)&ref.Norms[0]);
    setTexCoords(0, ref.VertCnt, (float*)&ref.TexCoords[0]);

    for (size_t i=0; i<5; ++i)
        if (ref.UsesGenerics[i])
            setGenerics(i, ref.getGenerics(i));

    setIndices(ref.IdxCnt, (uint32_t*)&ref.Indices[0]);
    return *this;
}

void MeshBuffer::loadFileObj(const char * fileName)
{
  // get the mesh
    #ifdef HIDEME
    AttributeContainer<Attributes::Position> constPosition = mesh->GetAttributes<Attributes::Position>( size_t(0u) );
    AttributeContainer<Attributes::Position>::const_iterator pos_itr = constPosition.begin();

    AttributeContainer<Attributes::Normal> constNormal = mesh->GetAttributes<Attributes::Normal>( size_t(0u) );
    AttributeContainer<Attributes::Normal>::const_iterator norm_itr = constNormal.begin();

    AttributeContainer<Attributes::TexCoord> constTexCoord = mesh->GetAttributes<Attributes::TexCoord>( size_t(0u) );
    AttributeContainer<Attributes::TexCoord>::const_iterator tex_itr = constTexCoord.begin();

    FaceContainer faces = mesh->GetFaces();
    FaceContainer::const_iterator face_itr = faces.begin();

    /* The Obj file format description
    # comments

    # positions
    v f f f

    # texture coords
    vt f f

    # normals
    vn f f f

    # faces (faces start at 1, not 0)
    # faces with positions only
    f 1 2 3
    # faces with positions and norms
    f 1//1 2//2 3//3
    # faces with positions and textures
    f 1/1 2/2 3/3

    # faces with positions, norms and textures
    f 1/1/1 2/2/2 3/3/3
    */

    std::ofstream out_file(filename.c_str());

    out_file << "# Medical Simulation Corparation" << std::endl;
    // send in the positions
    for (; pos_itr != constPosition.end(); ++pos_itr)
    {                
        out_file << "v " << (*pos_itr)[0] << " " << (*pos_itr)[1] << " " << (*pos_itr)[2] << "\n";
    }
    out_file << std::endl;

    
    // send in the texCoords
    for (; tex_itr != constTexCoord.end(); ++tex_itr)
    {                
        out_file << "vt " << tex_itr->u << " " << tex_itr->v << "\n";
    }
    out_file << std::endl;

    // and now the normals
    for (; norm_itr != constNormal.end(); ++norm_itr)
    {                
        out_file << "vn " << (*norm_itr)[0] << " " << (*norm_itr)[1] << " " << (*norm_itr)[2] << "\n";
    }
    out_file << std::endl;

    // set up the faces
    // Our meshes are kept interleaved, so the relationship between vert attributes are on a 1 to 1 basis
    if (!constNormal.empty() && !constTexCoord.empty())
    {
        for (; face_itr != faces.end(); ++face_itr)
        {
            unsigned short a = (*face_itr)[0]+1;
            unsigned short b = (*face_itr)[1]+1;
            unsigned short c = (*face_itr)[2]+1;
            out_file << "f " << a << "/" << a << "/" << a << " " 
                             << b << "/" << b << "/" << b << " " 
                             << c << "/" << c << "/" << c << "\n";
        }
    }
    else if (!constTexCoord.empty())
    {
        for (; face_itr != faces.end(); ++face_itr)
        {
            unsigned short a = (*face_itr)[0]+1;
            unsigned short b = (*face_itr)[1]+1;
            unsigned short c = (*face_itr)[2]+1;
            out_file << "f " << a << "/" << a << " "
                             << b << "/" << b << " "
                             << c << "/" << c << "\n";
        }
    }
    else if (!constNormal.empty())
    {
        for (; face_itr != faces.end(); ++face_itr)
        {
            unsigned short a = (*face_itr)[0]+1;
            unsigned short b = (*face_itr)[1]+1;
            unsigned short c = (*face_itr)[2]+1;
            out_file << "f " << a << "//" << a << " "
                             << b << "//" << b << " "
                             << c << "//" << c << "\n";
        }
    }
    else
    {
        for (; face_itr != faces.end(); ++face_itr)
        {                                
            out_file << "f " << (*face_itr)[0] << " " << (*face_itr)[1] << " " << (*face_itr)[2] << "\n";
        }
    }

    out_file << std::endl;
    out_file.close();
    #endif
}

void MeshBuffer::loadFileStl(const char * fileName)
{
    std::ifstream in_file(filename, std::ifstream::binary);

    /*  
        The Stl file format description
        From: https://en.wikipedia.org/wiki/STL_(file_format)
        UINT8[80] – Header, ignored by most applications
        UINT32 – Number of triangles

        foreach triangle
        REAL32[3] – Normal vector, if the normal is (0,0,0) most applications will generate the facet normal
        REAL32[3] – Vertex 1
        REAL32[3] – Vertex 2
        REAL32[3] – Vertex 3
        UINT16 – Attribute byte count, almost no applications use this and should be 0
        end
    */


    /*
        unsigned int VertCnt;
        unsigned int IdxCnt;

        std::vector<glm::vec3> Verts;
        std::vector<glm::vec3> Norms;
        std::vector<glm::vec2> TexCoords;

        std::vector<uint32_t>  Indices;
    */

    uint8_t header[80] = { 0 };
    uint32_t num_triangles = 0;
    in_file.read((const char*)&header[0], 80);
    in_file.read((const char*)&num_triangles, sizeof(uint32_t));

    VertCnt = num_triangles / 3;
    IdxCnt = ;
    
    for (uint32_t i = 0; i < num_triangles; ++i)
    {
        glm::vec3 verta, vertb, vertc;
        glm::vec3 normal;
        in_file.write((const char*)&normal, sizeof(glm::vec3));
        in_file.write((const char*)&verta, sizeof(glm::vec3));
        in_file.write((const char*)&vertb, sizeof(glm::vec3));
        in_file.write((const char*)&vertc, sizeof(glm::vec3));

        // move to the next bytes.
        uint16_t null;
        in_file.write((const char*)&null, sizeof(uint16_t));

    }
    in_file.close();
}

void MeshBuffer::setVerts(unsigned int count, const float* verts)
{
    if (!verts) return;

    VertCnt = count;
    Verts.resize(VertCnt);
    int src_num_componets = 3;

    for (unsigned int i=0; i<VertCnt; ++i)
    {
        glm::vec3 vec;
        vec[0] = verts[i * src_num_componets + 0];
        vec[1] = verts[i * src_num_componets + 1];
        vec[2] = verts[i * src_num_componets + 2];
        Verts[i] = vec;
    }
}

void MeshBuffer::setNorms(unsigned int count, const float* normals)
{
    if (count != VertCnt)
    {
        std::cout << "Vert count does not match the number normals to create" << std::endl;
        exit(1);
    }

    if (!normals) return;
    UsesNormals = true;

    Norms.clear();
    Norms.resize(count);
    int src_num_componets = 3;

    for (unsigned int i=0; i<VertCnt; ++i)
    {
        glm::vec3 vec;
        vec[0] = normals[i * src_num_componets + 0];
        vec[1] = normals[i * src_num_componets + 1];
        vec[2] = normals[i * src_num_componets + 2];

        Norms[i] = vec;
    }
}

void MeshBuffer::setTexCoords(unsigned int layer, unsigned int count, const float* coords)
{
    if (count != VertCnt)
    {
        std::cout << "Vert count does not match the number uvs to create" << std::endl;
        exit(1);
    }

    if (!coords) return;
    UsesUVs = true;

    TexCoords.clear();
    TexCoords.resize(count);
    int src_num_componets = 2;

    for (unsigned int i=0; i<VertCnt; ++i)
    {
        glm::vec2 vec;
        vec[0] = coords[i * src_num_componets + 0];
        vec[1] = coords[i * src_num_componets + 1];

        TexCoords[i] = vec;
    }
}

void MeshBuffer::setGenerics(unsigned int index, const std::vector<glm::vec4>& values)
{
    if (values.size() != VertCnt)
    {
        std::cout << "Vert count does not match the number generic values to add to vbo" << std::endl;
        exit(1);
    }

    if (index >= (unsigned int)UsesGenerics.size())
    {
        std::cout << "setGenerics index is not within the valid range of [0-4]" << std::endl;
        exit(1);
    }
    UsesGenerics[index] = true;

    Generics[index].clear();
    Generics[index].resize(values.size());
    Generics[index] = values;
}

void MeshBuffer::setIndices(unsigned int count, const unsigned int * indices)
{
    if (!indices) return;
    UsesIndices = true;

    IdxCnt = count;
    Indices.clear();
    Indices.resize(IdxCnt);
    for (unsigned int i=0; i<IdxCnt; ++i)
    {
        Indices[i] = indices[i];
    }
}

const std::vector<glm::vec3>& MeshBuffer::getVerts() const
{
    return Verts;
}

const std::vector<glm::vec3>& MeshBuffer::getNorms() const
{
    return Norms;
}

const std::vector<glm::vec2>& MeshBuffer::getTexCoords(unsigned int layer) const
{
    return TexCoords;
}

const std::vector<glm::vec4>& MeshBuffer::getGenerics(unsigned int index) const
{
    return Generics[index];
}

const std::vector<uint32_t>& MeshBuffer::getIndices() const
{
    return Indices;
}

void MeshBuffer::generateFaceNormals()
{
    assert(IdxCnt);

    UsesNormals = true;
    Norms.clear();
    Norms.resize(VertCnt);
    for (unsigned int i=0; i<IdxCnt; i+=3){
        unsigned int idx_a = i+0;
        unsigned int idx_b = i+1;
        unsigned int idx_c = i+2;

        unsigned int vert_idx_a = Indices[idx_a];
        unsigned int vert_idx_b = Indices[idx_b];
        unsigned int vert_idx_c = Indices[idx_c];

        glm::vec3 vec_a = Verts[vert_idx_a];
        glm::vec3 vec_b = Verts[vert_idx_b];
        glm::vec3 vec_c = Verts[vert_idx_c];

        glm::vec3 edge_ab = vec_b - vec_a;
        glm::vec3 edge_ac = vec_c - vec_a;

        glm::vec3 norm = glm::normalize( glm::cross( edge_ab, edge_ac ) );
        Norms[vert_idx_a] = norm;
        Norms[vert_idx_b] = norm;
        Norms[vert_idx_c] = norm;
    }
}

unsigned int MeshBuffer::getVertCnt() const
{
    return VertCnt;
}

unsigned int MeshBuffer::getIdxCnt() const
{
    return IdxCnt;
}

void MeshBuffer::cleanUp()
{
    UsesNormals = false;
    UsesUVs     = false;
    UsesIndices = false;
    VertCnt     = 0;
    IdxCnt      = 0;

    Verts.clear();
    Norms.clear();
    TexCoords.clear();
    for (auto& gen: Generics)
        gen.clear();
    Generics.clear();

    Indices.clear();
}
