#pragma once
#include <vector>
#include <tuple>
#include <numeric>
#include "glad/glad.h"
#include "vec3.hpp"
#include "vec2.hpp"
#include "mat4x4.hpp"
#include "bvh.h"
#include "split_bvh.h"
#include "Util/Types.h"

struct Mesh
{
    Mesh() 
    {
        bvh = new RadeonRays::SplitBvh(2.0f, 64, 0, 0.001f, 0);
    }
    ~Mesh() 
    {
        if(bvh)
        {
            delete bvh;
            bvh = nullptr;
        }
    }

    void BuildBvh()
    {
        int numTri = static_cast<int>(vertices.size() / 3);
        std::vector<RadeonRays::bbox> bounds(numTri);

#pragma omp parallel for
        for(int i = 0; i < numTri; ++i)
        {
            bounds[i].grow(vertices[3 * i]);
            bounds[i].grow(vertices[3 * i + 1]);
            bounds[i].grow(vertices[3 * i + 2]);
        }

        bvh->Build(&bounds[0], numTri);
    }

    std::vector<glm::vec3> vertices;
    std::vector<Uint32> indices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    RadeonRays::Bvh* bvh;
};

struct MeshInstance
{
    MeshInstance() {}
    MeshInstance(Uint32 _meshID, Uint32 _materialID, glm::mat4 _transform)
        : meshID(_meshID), materialID(_materialID), transform(_transform)
    {
    }
    ~MeshInstance() {}

    Uint32 meshID = -1;
    Uint32 materialID = -1;
    glm::mat4 transform;
};

struct MeshBatch
{
    friend class Scene;
public:
    MeshBatch()
    {
        instanceCount = 0;
        bDrawElement = false;
    }
    ~MeshBatch()
    {
        glDeleteVertexArrays(1, &vao);
        for(Int i = 0; i < vbos.size(); i++)
        {
            glDeleteBuffers(1, &vbos[i]);
        }
        glDeleteBuffers(1, &ebo);
    }

    void Build()
    {
        if(vertices.size() == 0)
        {
            return;
        }
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        std::vector<std::tuple<void*, Int, Int>> vbosArray;
        vbosArray.push_back(std::make_tuple((void*)vertices.data(), sizeof(glm::vec3), vertices.size()));
        if(normals.size() > 0)
        {
            vbosArray.push_back(std::make_tuple((void*)normals.data(), sizeof(glm::vec3), normals.size()));
        }
        if(uvs.size() > 0)
        {
            vbosArray.push_back(std::make_tuple((void*)uvs.data(), sizeof(glm::vec2), uvs.size()));
        }
        vbos.resize(vbosArray.size() + 1);
        for(Int i = 0; i < vbosArray.size(); i++)
        {
            glGenBuffers(1, &vbos[i]);
            glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
            glBufferData(GL_ARRAY_BUFFER, std::get<2>(vbosArray[i]) * std::get<1>(vbosArray[i]), std::get<0>(vbosArray[i]), GL_STATIC_DRAW);
            glVertexAttribPointer(i, std::get<1>(vbosArray[i]) / sizeof(Float), GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(i);
        }

        glGenBuffers(1, &vbos[vbosArray.size()]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[vbosArray.size()]);
        glBufferData(GL_ARRAY_BUFFER, materialIDs.size() * sizeof(Uint32), materialIDs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(3);

        // if(indices.size() == 0)
        // {
        //     indices.resize(vertices.size());
        //     std::iota(indices.begin(), indices.end(), 0);
        // }

        if(indices.size() > 0)
        {
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(Uint32), indices.data(), GL_STATIC_DRAW);
            bDrawElement = true;
        }

        glBindVertexArray(0);
    }

    void Bind()
    {
        glBindVertexArray(vao);
    }

    void UnBind()
    {
        glBindVertexArray(0);
    }

    Uint GetInstanceCount() {return instanceCount;}
    Uint GetNumPerMeshBatch() {return indices.size();}
    Uint GetNumVertices() const {return vertices.size();}
    Bool GetDrawElement() const {return bDrawElement;}

private:
    std::vector<glm::vec3> vertices;
    std::vector<Uint32> indices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<Uint32> materialIDs;
    Uint instanceCount;
    Bool bDrawElement;

private:
    GLuint vao;
    std::vector<GLuint> vbos;
    GLuint ebo;
};