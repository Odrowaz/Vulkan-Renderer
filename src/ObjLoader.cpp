#include "ObjLoader.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <tiny_obj_loader.h>
#include <vector>

ModelData LoadObj(const std::string& InPath)
{
    tinyobj::attrib_t Attrib;
    std::vector<tinyobj::shape_t> Shapes;
    std::vector<tinyobj::material_t> Materials;

    std::string Warn, Err;

    bool Ret = tinyobj::LoadObj(
        &Attrib,
        &Shapes,
        &Materials,
        &Warn,
        &Err,
        InPath.c_str());

    if (!Warn.empty())
        std::cerr << Warn << '\n';

    if (!Err.empty())
        std::cerr << Err << '\n';

    if (!Ret)
        throw std::runtime_error("Failed to load OBJ: " + InPath);


    std::vector<float> Data;
    std::vector<uint32_t> Indices;

    uint32_t CurrentIndex = 0;

    for(const auto& Shape : Shapes)
    {
        for(const auto& Idx : Shape.mesh.indices)
        {
            // Position
            Data.push_back(
                Attrib.vertices[3 * Idx.vertex_index + 0]);

            Data.push_back(
                Attrib.vertices[3 * Idx.vertex_index + 1]);

            Data.push_back(
                Attrib.vertices[3 * Idx.vertex_index + 2]);


            // Normal
            if(Idx.normal_index >= 0)
            {
                Data.push_back(
                    Attrib.normals[3 * Idx.normal_index + 0]);

                Data.push_back(
                    Attrib.normals[3 * Idx.normal_index + 1]);

                Data.push_back(
                    Attrib.normals[3 * Idx.normal_index + 2]);
            }
            else
            {
                Data.push_back(0.f);
                Data.push_back(0.f);
                Data.push_back(0.f);
            }


            // UV
            if(Idx.texcoord_index >= 0)
            {
                Data.push_back(
                    Attrib.texcoords[2 * Idx.texcoord_index + 0]);

                Data.push_back(
                    1.0f - Attrib.texcoords[2 * Idx.texcoord_index + 1]);
            }
            else
            {
                Data.push_back(0.f);
                Data.push_back(0.f);
            }


            Indices.push_back(CurrentIndex++);
        }
    }


    ModelData Result{
        .Vertices = Data,
        .Indices = Indices,
        .VertexCount = CurrentIndex,
        .IndexCount = static_cast<uint32_t>(Indices.size())
    };

    return Result;
}