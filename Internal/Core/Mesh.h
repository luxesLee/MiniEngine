#pragma once
#include <vector>
#include <vec3.hpp>
#include <vec2.hpp>
#include <mat4x4.hpp>
#include "bvh.h"
#include "split_bvh.h"

using namespace glm;
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

    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
    RadeonRays::Bvh* bvh;
};

struct MeshInstance
{
    MeshInstance() {}
    MeshInstance(int _meshID, int _materialID, mat4 _transform)
        : meshID(_meshID), materialID(_materialID), transform(_transform)
    {
    }
    ~MeshInstance() {}

    int meshID;
    int materialID;
    mat4 transform;
};

