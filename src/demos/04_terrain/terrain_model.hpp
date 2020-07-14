#pragma once

#include <model.h>
#include "util.h"

class TerrainModel : public RapidGL::Model
{
public:
    TerrainModel(const std::string & heightmap_filename, float size = 200.0f, float max_height = 100.0f);
    ~TerrainModel();

    float getHeightOfTerrain(float world_x, float world_z, float terrain_world_x, float terrain_world_z);

protected:
    void genTerrainVertices(const std::string & heightmap_filename);
    float getHeight(int x, int z, unsigned char* heightmap_data, RapidGL::ImageData & heightmap_metadata);
    glm::vec3 calculateNormal(int x, int z, unsigned char* heightmap_data, RapidGL::ImageData& heightmap_metadata);

    float barycentricHeight(const glm::vec3 & p1, const glm::vec3 & p2, const glm::vec3 & p3, const glm::vec2 & p);

    std::vector<std::vector<float>> m_heights;

    const float M_SIZE;
    const float M_MAX_HEIGHT;
    const float M_MAX_PIXEL_COLOR = 255.0;
};