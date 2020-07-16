#include "terrain_model.hpp"
#include <iostream>
#include <cmath>

TerrainModel::TerrainModel(const std::string& heightmap_filename, float size, float max_height)
    : M_SIZE(size),
      M_MAX_HEIGHT(max_height)
{
    genTerrainVertices(heightmap_filename);

    std::cout << "Created terrain with max height = " << M_MAX_HEIGHT << std::endl;
}

TerrainModel::~TerrainModel()
{
    std::cout << "Deleted terrain with max height = " << M_MAX_HEIGHT << std::endl;
}

float TerrainModel::getHeightOfTerrain(float world_x, float world_z, float terrain_world_x, float terrain_world_z)
{
    float terrain_x    = terrain_world_x - world_x;
    float terrain_z    = terrain_world_z - world_z;
    float aspect_ratio = float(m_heights[0].size()) / float(m_heights.size());

    float grid_square_size_x = aspect_ratio * M_SIZE / float(m_heights[0].size() - 1);
    float grid_square_size_z = M_SIZE / float(m_heights.size()    - 1);

    int grid_x = int(terrain_x / grid_square_size_x);
    int grid_z = int(terrain_z / grid_square_size_z);

    if (grid_x >= m_heights[0].size() - 1 || grid_z >= m_heights.size() - 1 || grid_x < 0 || grid_z < 0)
    {
        return 0;
    }

    float x_coord = std::fmod(terrain_x, grid_square_size_x) / grid_square_size_x;
    float z_coord = std::fmod(terrain_z, grid_square_size_z) / grid_square_size_z;

    float height;
    if (x_coord <= 1.0 - z_coord)
    {
        height = barycentricHeight(glm::vec3(0.0, m_heights[grid_z    ][grid_x    ], 0.0), 
                                   glm::vec3(1.0, m_heights[grid_z + 1][grid_x    ], 0.0),
                                   glm::vec3(0.0, m_heights[grid_z    ][grid_x + 1], 1.0),
                                   glm::vec2(z_coord, x_coord));
    }
    else
    {
        height = barycentricHeight(glm::vec3(1.0, m_heights[grid_z + 1][grid_x    ], 0.0), 
                                   glm::vec3(1.0, m_heights[grid_z + 1][grid_x + 1], 1.0),
                                   glm::vec3(0.0, m_heights[grid_z    ][grid_x + 1], 1.0),
                                   glm::vec2(z_coord, x_coord));
    }

    return height;
}

void TerrainModel::genTerrainVertices(const std::string& heightmap_filename)
{
    RapidGL::VertexBuffers buffers;
    RapidGL::ImageData heightmap_metadata;

    stbi_set_flip_vertically_on_load(1);
    auto heightmap_image = RapidGL::Util::loadTextureData(heightmap_filename, heightmap_metadata, 1);
    stbi_set_flip_vertically_on_load(0);

    if (heightmap_image)
    {
        std::cout << "Success loading heightmap\n";

        int vertex_count_width  = heightmap_metadata.width;
        int vertex_count_height = heightmap_metadata.height;

        float aspect_ratio = float(vertex_count_width) / float(vertex_count_height);

        m_heights = std::vector<std::vector<float>>(vertex_count_height /* rows */, std::vector<float>(vertex_count_width /* cols */));

        for (unsigned int j = 0; j < vertex_count_height; ++j)
        {
            for (unsigned int i = 0; i < vertex_count_width; ++i)
            {
                RapidGL::VertexBuffers::Vertex v;
                m_heights[j][i] = getHeight(i, j, heightmap_image, heightmap_metadata);

                v.m_position = glm::vec3(-float(i) / float(vertex_count_width - 1) * M_SIZE * aspect_ratio,
                                          m_heights[j][i], 
                                         -float(j) / float(vertex_count_height - 1) * M_SIZE);
                v.m_normal   = calculateNormal(i, j, heightmap_image, heightmap_metadata);
                v.m_texcoord = glm::vec3((1.0 - float(i) / float(vertex_count_width - 1)),
                                         1.0 - float(j) / float(vertex_count_height - 1),
                                         0.0f);
                v.m_tangent  = glm::vec3(0.0f);

                buffers.m_vertices.push_back(v);
            }
        }

        for (unsigned int j = 0; j < vertex_count_height - 1; ++j)
        {
            for (unsigned int i = 0; i < vertex_count_width - 1; ++i)
            {
                int top_left     = j * vertex_count_width + i;
                int top_right    = top_left + 1;
                int bottom_left  = (j + 1) * vertex_count_width + i;
                int bottom_right = bottom_left + 1;

                buffers.m_indices.push_back(top_left);
                buffers.m_indices.push_back(bottom_left);
                buffers.m_indices.push_back(top_right);

                buffers.m_indices.push_back(top_right);
                buffers.m_indices.push_back(bottom_left);
                buffers.m_indices.push_back(bottom_right);
            }
        }

        genPrimitive(buffers);
    }
    else
    {
        std::cout << "Failed during loading heightmap\n";
    }

    stbi_image_free(heightmap_image);
}

float TerrainModel::getHeight(int x, int z, unsigned char* heightmap_data, RapidGL::ImageData & heightmap_metadata)
{
    x = std::clamp(x, 0, int(heightmap_metadata.width  - 1));
    z = std::clamp(z, 0, int(heightmap_metadata.height - 1));

    float height = heightmap_data[z * heightmap_metadata.width + (heightmap_metadata.width - x - 1)];

    height /= M_MAX_PIXEL_COLOR;
    height = (height - 0.5f) * 2.0f;
    height *= M_MAX_HEIGHT;

    return height;
}

glm::vec3 TerrainModel::calculateNormal(int x, int z, unsigned char* heightmap_data, RapidGL::ImageData& heightmap_metadata)
{
    float height_left  = getHeight(x - 1, z,     heightmap_data, heightmap_metadata);
    float height_right = getHeight(x + 1, z,     heightmap_data, heightmap_metadata);
    float height_down  = getHeight(x,     z - 1, heightmap_data, heightmap_metadata);
    float height_up    = getHeight(x,     z + 1, heightmap_data, heightmap_metadata);

    auto normal = glm::vec3(height_left - height_right, 2.0f, height_down - height_up);
    normal      = glm::normalize(normal);
    
    return normal;
}

float TerrainModel::barycentricHeight(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec2& p)
{
    float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);

    float l1 = ((p2.z - p3.z) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.z)) / det;
    float l2 = ((p3.z - p1.z) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.z)) / det;
    float l3 = 1.0 - l1 - l2;

    return l1 * p1.y + l2 * p2.y + l3 * p3.y;
}
