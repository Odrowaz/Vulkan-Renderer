#include "ObjLoader.h"
#include "Common.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <tuple>
#include <map>

ModelData LoadObj(const std::string &InPath) {
  tinyobj::attrib_t Attrib;
  std::vector<tinyobj::shape_t> Shapes;
  std::vector<tinyobj::material_t> Materials;
  std::string Warn, Err;

  bool Result = tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warn, &Err,
                                 InPath.c_str());

  if (!Warn.empty()) {
    std::cout << "TinyObj Warn: " << Warn;
  }

  if (!Err.empty()) {
    std::cout << "TinyObj Err: " << Err;
  }

  CHECK_DIE(Result, "Failed to load OBJ file");

  ModelData Data;

  using VertexKey = std::tuple<int, int, int>;

  std::map<VertexKey, uint32_t> UniqueIndexes;

  for (const auto &Shape : Shapes) {

    for (const auto &Index : Shape.mesh.indices) {
      VertexKey Key{Index.vertex_index, Index.normal_index,
                    Index.texcoord_index};

      if (UniqueIndexes.contains(Key)) {
        Data.Indices.push_back(UniqueIndexes.at(Key));
        continue;
      }

      Data.Vertices.push_back(Attrib.vertices[3 * Index.vertex_index + 0]);
      Data.Vertices.push_back(Attrib.vertices[3 * Index.vertex_index + 1]);
      Data.Vertices.push_back(Attrib.vertices[3 * Index.vertex_index + 2]);

      if (Index.normal_index != -1) {
        Data.Vertices.push_back(Attrib.normals[3 * Index.normal_index + 0]);
        Data.Vertices.push_back(Attrib.normals[3 * Index.normal_index + 1]);
        Data.Vertices.push_back(Attrib.normals[3 * Index.normal_index + 2]);
      } else {
        Data.Vertices.push_back(0.f);
        Data.Vertices.push_back(0.f);
        Data.Vertices.push_back(0.f);
      }

      if (Index.texcoord_index != -1) {
        Data.Vertices.push_back(Attrib.texcoords[2 * Index.texcoord_index + 0]);
        Data.Vertices.push_back(1.0f - Attrib.texcoords[2 * Index.texcoord_index + 1]);
      } else {
        Data.Vertices.push_back(0.f);
        Data.Vertices.push_back(0.f);
      }

      uint32_t NewIndex = static_cast<uint32_t>(UniqueIndexes.size());
      Data.Indices.push_back(NewIndex);

      UniqueIndexes.insert({Key, NewIndex});
    }
  }

  Data.IndexCount = static_cast<uint32_t>(Data.Indices.size());
  Data.VertexCount = static_cast<uint32_t>(Data.Vertices.size());

  return Data;
}
