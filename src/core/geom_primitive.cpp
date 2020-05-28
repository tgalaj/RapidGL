#include "geom_primitive.h"

#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

namespace RapidGL
{
    void GeomPrimitive::genCube(VertexBuffers & buffers, float radius)
    {
        float r2 = radius * 0.5f;

        std::vector<glm::vec3> positions = 
        {
            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, -r2, -r2),

            glm::vec3(-r2, +r2, -r2),
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, +r2, -r2),

            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, +r2, -r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, -r2, -r2),

            glm::vec3(-r2, -r2, +r2),
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, -r2, +r2),

            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(-r2, +r2, -r2),

            glm::vec3(+r2, -r2, -r2),
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, +r2, -r2) 
        };

        std::vector<glm::vec3> normals =
       {
           glm::vec3(0.0f, -1.0f, 0.0f),
           glm::vec3(0.0f, -1.0f, 0.0f),
           glm::vec3(0.0f, -1.0f, 0.0f),
           glm::vec3(0.0f, -1.0f, 0.0f),
           
           glm::vec3(0.0f, +1.0f, 0.0f),
           glm::vec3(0.0f, +1.0f, 0.0f),
           glm::vec3(0.0f, +1.0f, 0.0f),
           glm::vec3(0.0f, +1.0f, 0.0f),
           
           glm::vec3(0.0f, 0.0f, -1.0f),
           glm::vec3(0.0f, 0.0f, -1.0f),
           glm::vec3(0.0f, 0.0f, -1.0f),
           glm::vec3(0.0f, 0.0f, -1.0f),
           
           glm::vec3(0.0f, 0.0f, +1.0f),
           glm::vec3(0.0f, 0.0f, +1.0f),
           glm::vec3(0.0f, 0.0f, +1.0f),
           glm::vec3(0.0f, 0.0f, +1.0f),
           
           glm::vec3(-1.0f, 0.0f, 0.0f),
           glm::vec3(-1.0f, 0.0f, 0.0f),
           glm::vec3(-1.0f, 0.0f, 0.0f),
           glm::vec3(-1.0f, 0.0f, 0.0f),
           
           glm::vec3(+1.0f, 0.0f, 0.0f),
           glm::vec3(+1.0f, 0.0f, 0.0f),
           glm::vec3(+1.0f, 0.0f, 0.0f),
           glm::vec3(+1.0f, 0.0f, 0.0f)
        };

        std::vector<glm::vec3> texcoords =
        {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
                    
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
                    
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),

            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),

            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),

            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f)
        };

        for(unsigned i = 0; i < positions.size(); ++i)
        {
            VertexBuffers::Vertex vertex;
            vertex.m_position = positions[i];
            vertex.m_normal   = normals[i];
            vertex.m_texcoord = texcoords[i];
            vertex.m_tangent  = glm::vec3(0.0f);

            buffers.m_vertices.push_back(vertex);
        }

        buffers.m_indices = 
        {
            0,  2,  1,  0,  3,  2,
            4,  5,  6,  4,  6,  7,
            8,  9,  10, 8,  10, 11,
            12, 15, 14, 12, 14, 13,
            16, 17, 18, 16, 18, 19,
            20, 23, 22, 20, 22, 21
        };
    }

    void GeomPrimitive::genCubeMap(VertexBuffers & buffers, float radius)
    {
        float r2 = radius * 0.5f;

        std::vector<glm::vec3> positions = 
        {
            // Front
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(-r2, +r2, +r2),
            // Right
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, -r2, -r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, +r2, +r2),
            // Back
            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, +r2, -r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, -r2, -r2),
            // Left
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(-r2, +r2, -r2),
            glm::vec3(-r2, -r2, -r2),
            // Bottom
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(-r2, -r2, -r2),
            glm::vec3(+r2, -r2, -r2),
            glm::vec3(+r2, -r2, +r2),
            // Top
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, -r2, +r2)
        };

        for(unsigned i = 0; i < positions.size(); ++i)
        {
            VertexBuffers::Vertex vertex;
            vertex.m_position = positions[i];
            vertex.m_normal   = glm::vec3(0.0f);
            vertex.m_texcoord = glm::vec3(0.0f);
            vertex.m_tangent  = glm::vec3(0.0f);

            buffers.m_vertices.push_back(vertex);
        }

        buffers.m_indices = 
        {
            0,  2,  1,  0,  3,  2,
            4,  6,  5,  4,  7,  6,
            8,  10, 9,  8,  11, 10,
            12, 14, 13, 12, 15, 14,
            16, 18, 17, 16, 19, 18,
            20, 22, 21, 20, 23, 22
        };
    }

    void GeomPrimitive::genTorus(VertexBuffers & buffers, float innerRadius, float outerRadius, unsigned int slices, unsigned int stacks)
    {
        float phi = 0.0f;
        float theta = 0.0f;

        float cos2PIp = 0.0f;
        float sin2PIp = 0.0f;
        float cos2PIt = 0.0f;
        float sin2PIt = 0.0f;

        float torusRadius = (outerRadius - innerRadius) * 0.5f;
        float centerRadius = outerRadius - torusRadius;

        float phiInc = 1.0f / float(slices);
        float thetaInc = 1.0f / float(stacks);

        buffers.m_vertices.reserve((stacks + 1) * (slices + 1));
        buffers.m_indices.reserve(stacks * slices * 2 * 3);

        for (unsigned int sideCount = 0; sideCount <= slices; ++sideCount, phi += phiInc)
        {
            cos2PIp = glm::cos(glm::two_pi<float>() * phi);
            sin2PIp = glm::sin(glm::two_pi<float>() * phi);

            theta = 0.0f;
            for (unsigned int faceCount = 0; faceCount <= stacks; ++faceCount, theta += thetaInc)
            {
                cos2PIt = glm::cos(glm::two_pi<float>() * theta);
                sin2PIt = glm::sin(glm::two_pi<float>() * theta);

                VertexBuffers::Vertex vertex;
                vertex.m_position = glm::vec3((centerRadius + torusRadius * cos2PIt) * cos2PIp,
                                              (centerRadius + torusRadius * cos2PIt) * sin2PIp,
                                               torusRadius * sin2PIt);

                vertex.m_normal = glm::vec3(cos2PIp * cos2PIt,
                                            sin2PIp * cos2PIt,
                                            sin2PIt);

                vertex.m_texcoord = glm::vec3(phi, theta, 0.0f);

                vertex.m_tangent = glm::vec3(0.0f);

                buffers.m_vertices.push_back(vertex);
            }
        }

        for (unsigned int sideCount = 0; sideCount < slices; ++sideCount)
        {
            for (unsigned int faceCount = 0; faceCount < stacks; ++faceCount)
            {
                GLushort v0 = sideCount      * (stacks + 1) + faceCount;
                GLushort v1 = (sideCount + 1) * (stacks + 1) + faceCount;
                GLushort v2 = (sideCount + 1) * (stacks + 1) + (faceCount + 1);
                GLushort v3 = sideCount      * (stacks + 1) + (faceCount + 1);

                buffers.m_indices.push_back(v0);
                buffers.m_indices.push_back(v1);
                buffers.m_indices.push_back(v2);

                buffers.m_indices.push_back(v0);
                buffers.m_indices.push_back(v2);
                buffers.m_indices.push_back(v3);
            }
        }
    }

    void GeomPrimitive::genCylinder(VertexBuffers & buffers, float height, float r, unsigned int slices)
    {
        float halfHeight = height * 0.5f;
        glm::vec3 p1 = glm::vec3(0.0f, halfHeight, 0.0f);
        glm::vec3 p2 = -p1;

        float thetaInc = glm::two_pi<float>() / float(slices);
        float theta = 0.0f;
        float sign = -1.0f;

        VertexBuffers::Vertex vertex;

        /* Center bottom */
        vertex.m_position = p2;
        vertex.m_normal   = glm::vec3(0.0f, -1.0f, 0.0f);
        vertex.m_texcoord = glm::vec3(0.5f, 0.5f, 0.0f);
        vertex.m_tangent  = glm::vec3(0.0f);
        buffers.m_vertices.push_back(vertex);

        /* Bottom */
        for (unsigned int sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            VertexBuffers::Vertex v;

            v.m_position = glm::vec3(glm::cos(theta) * r, -halfHeight, -glm::sin(theta) * r);
            v.m_normal   = glm::vec3(0.0f, -1.0f, 0.0f);
            v.m_texcoord = glm::vec3(glm::cos(theta) * 0.5f + 0.5f, glm::sin(theta) * 0.5f + 0.5f, 0.0f);
            v.m_tangent  = glm::vec3(0.0f);
            buffers.m_vertices.push_back(v);
        }

        /* Center top */
        vertex.m_position = p1;
        vertex.m_normal   = glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.m_texcoord = glm::vec3(0.5f, 0.5f, 0.0f);
        vertex.m_tangent  = glm::vec3(0.0f);
        buffers.m_vertices.push_back(vertex);

        /* Top */
        for (unsigned int sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            VertexBuffers::Vertex v;

            v.m_position = glm::vec3(glm::cos(theta) * r, halfHeight, -glm::sin(theta) * r);
            v.m_normal   = glm::vec3(0.0f, 1.0f, 0.0f);
            v.m_texcoord = glm::vec3(glm::cos(theta) * 0.5f + 0.5f, glm::sin(theta) * 0.5f + 0.5f, 0.0f);
            v.m_tangent  = glm::vec3(0.0f);
            buffers.m_vertices.push_back(v);
        }

        /* Sides */
        for (unsigned int sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            sign = -1.0f;

            for (int i = 0; i < 2; ++i)
            {
                VertexBuffers::Vertex v;

                v.m_position = glm::vec3(glm::cos(theta) * r, halfHeight * sign, -glm::sin(theta) * r);
                v.m_normal   = glm::vec3(glm::cos(theta), 0.0f, -glm::sin(theta));
                v.m_texcoord = glm::vec3(sideCount / (float)slices, (sign + 1.0f) * 0.5f, 0.0f);
                v.m_tangent  = glm::vec3(0.0f);
                buffers.m_vertices.push_back(v);

                sign = 1.0f;
            }
        }

        GLushort centerIdx = 0;
        GLushort idx = 1;

        /* Indices Bottom */
        for (unsigned int sideCount = 0; sideCount < slices; ++sideCount)
        {
            buffers.m_indices.push_back(centerIdx);
            buffers.m_indices.push_back(idx + 1);
            buffers.m_indices.push_back(idx);

            ++idx;
        }
        ++idx;

        /* Indices Top */
        centerIdx = idx;
        ++idx;

        for (unsigned int sideCount = 0; sideCount < slices; ++sideCount)
        {
            buffers.m_indices.push_back(centerIdx);
            buffers.m_indices.push_back(idx);
            buffers.m_indices.push_back(idx + 1);

            ++idx;
        }
        ++idx;

        /* Indices Sides */
        for (unsigned int sideCount = 0; sideCount < slices; ++sideCount)
        {
            buffers.m_indices.push_back(idx);
            buffers.m_indices.push_back(idx + 2);
            buffers.m_indices.push_back(idx + 1);

            buffers.m_indices.push_back(idx + 2);
            buffers.m_indices.push_back(idx + 3);
            buffers.m_indices.push_back(idx + 1);

            idx += 2;
        }
    }

    void GeomPrimitive::genCone(VertexBuffers & buffers, float height, float r, unsigned int slices, unsigned int stacks)
    {
        float thetaInc = glm::two_pi<float>() / float(slices);
        float theta = 0.0f;

        VertexBuffers::Vertex vertex;

        /* Center bottom */
        glm::vec3 p = glm::vec3(0.0f, height, 0.0f);

        vertex.m_position = -p;
        vertex.m_normal   = glm::vec3(0.0f, -1.0f, 0.0f);
        vertex.m_texcoord = glm::vec3(0.0f, 0.0f, 0.0f);
        vertex.m_tangent  = glm::vec3(0.0f);
        buffers.m_vertices.push_back(vertex);

        /* Bottom */
        for (unsigned int sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            VertexBuffers::Vertex v;
            v.m_position = glm::vec3(glm::cos(theta) * r, -height, -glm::sin(theta) * r);
            v.m_normal   = glm::vec3(0.0f, -1.0f, 0.0f);
            v.m_texcoord = glm::vec3(0.0f, 0.0f, 0.0f);
            v.m_tangent  = glm::vec3(0.0f);

            buffers.m_vertices.push_back(v);
        }

        /* Sides */
        float l = glm::sqrt(height * height + r * r);

        for (unsigned int stackCount = 0; stackCount <= stacks; ++stackCount)
        {
            float level = stackCount / float(stacks);

            for (unsigned int sliceCount = 0; sliceCount <= slices; ++sliceCount, theta += thetaInc)
            {
                VertexBuffers::Vertex v;
                v.m_position = glm::vec3(glm::cos(theta) * r * (1.0f - level),
                                         -height + height * level,
                                         -glm::sin(theta) * r * (1.0f - level));
                v.m_normal   = glm::vec3(glm::cos(theta) * height / l, r / l, -glm::sin(theta) * height / l);
                v.m_texcoord = glm::vec3(sliceCount / float(slices), level, 0.0f);
                v.m_tangent  = glm::vec3(0.0f);

                buffers.m_vertices.push_back(v);
            }
        }

        GLushort centerIdx = 0;
        GLushort idx = 1;

        /* Indices Bottom */
        for (unsigned int sliceCount = 0; sliceCount < slices; ++sliceCount)
        {
            buffers.m_indices.push_back(centerIdx);
            buffers.m_indices.push_back(idx + 1);
            buffers.m_indices.push_back(idx);

            ++idx;
        }
        ++idx;

        /* Indices Sides */
        for (unsigned int stackCount = 0; stackCount < stacks; ++stackCount)
        {
            for (unsigned int sliceCount = 0; sliceCount < slices; ++sliceCount)
            {
                buffers.m_indices.push_back(idx);
                buffers.m_indices.push_back(idx + 1);
                buffers.m_indices.push_back(idx + slices + 1);

                buffers.m_indices.push_back(idx + 1);
                buffers.m_indices.push_back(idx + slices + 2);
                buffers.m_indices.push_back(idx + slices + 1);

                ++idx;
            }

            ++idx;
        }
    }

    void GeomPrimitive::genQuad(VertexBuffers & buffers, float width, float height)
    {
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;

        std::vector<glm::vec3> positions, normals, texcoords;

        positions.push_back(glm::vec3(-halfWidth, 0.0f, -halfHeight));
        positions.push_back(glm::vec3(-halfWidth, 0.0f, halfHeight));
        positions.push_back(glm::vec3(halfWidth, 0.0f, -halfHeight));
        positions.push_back(glm::vec3(halfWidth, 0.0f, halfHeight));

        normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));

        texcoords.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        texcoords.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
        texcoords.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
        texcoords.push_back(glm::vec3(1.0f, 0.0f, 0.0f));

        for(unsigned i = 0; i < positions.size(); ++i)
        {
            VertexBuffers::Vertex v;
            v.m_position = positions[i];
            v.m_normal   = normals[i];
            v.m_texcoord = texcoords[i];
            v.m_tangent  = glm::vec3(0.0f);

            buffers.m_vertices.push_back(v);
        }

        buffers.m_indices.push_back(0);
        buffers.m_indices.push_back(1);
        buffers.m_indices.push_back(2);
        buffers.m_indices.push_back(3);
    }

    void GeomPrimitive::genPlane(VertexBuffers & buffers, float width, float height, unsigned int slices, unsigned int stacks)
    {
        float widthInc  = width  / float(slices);
        float heightInc = height / float(stacks);

        float w = -width * 0.5f;
        float h = -height * 0.5f;

        for (unsigned int j = 0; j <= stacks; ++j, h += heightInc)
        {
            for (unsigned int i = 0; i <= slices; ++i, w += widthInc)
            {
                //buffers.m_texcoords.push_back(glm::vec2(i / (float) slices, j / (float) stacks));

                VertexBuffers::Vertex v;
                v.m_position = glm::vec3(w, 0.0f, h);
                v.m_normal   = glm::vec3(0.0f, 1.0f, 0.0f);
                v.m_texcoord = glm::vec3(i, j, 0.0f);
                v.m_tangent  = glm::vec3(0.0f);

                buffers.m_vertices.push_back(v);
            }
            w = -width * 0.5f;
        }

        GLushort idx = 0;

        for (unsigned int j = 0; j < stacks; ++j)
        {
            for (unsigned int i = 0; i < slices; ++i)
            {
                buffers.m_indices.push_back(idx);
                buffers.m_indices.push_back(idx + slices + 1);
                buffers.m_indices.push_back(idx + 1);

                buffers.m_indices.push_back(idx + 1);
                buffers.m_indices.push_back(idx + slices + 1);
                buffers.m_indices.push_back(idx + slices + 2);

                ++idx;
            }

            ++idx;
        }
    }

    void GeomPrimitive::genSphere(VertexBuffers & buffers, float r, unsigned int slices)
    {
        float deltaPhi = glm::two_pi<float>() / static_cast<float>(slices);

        unsigned int parallels = static_cast<unsigned int>(slices * 0.5f);

        for (unsigned int i = 0; i <= parallels; ++i)
        {
            for (unsigned int j = 0; j <= slices; ++j)
            {
                VertexBuffers::Vertex v;
                v.m_position = glm::vec3(r * glm::sin(deltaPhi * i) * glm::sin(deltaPhi * j),
                                         r * glm::cos(deltaPhi * i),
                                         r * glm::sin(deltaPhi * i) * glm::cos(deltaPhi * j));
                v.m_normal = glm::vec3(r * glm::sin(deltaPhi * i) * glm::sin(deltaPhi * j) / r,
                                       r * glm::cos(deltaPhi * i) / r,
                                       r * glm::sin(deltaPhi * i) * glm::cos(deltaPhi * j) / r);
                v.m_texcoord = glm::vec3(j / static_cast<float>(slices),
                                         1.0f - i / static_cast<float>(parallels), 0.0f);
                v.m_tangent = glm::vec3(0.0f);

                buffers.m_vertices.push_back(v);
            }
        }

        for (unsigned int i = 0; i < parallels; ++i)
        {
            for (unsigned int j = 0; j < slices; ++j)
            {
                buffers.m_indices.push_back(i      * (slices + 1) + j);
                buffers.m_indices.push_back((i + 1) * (slices + 1) + j);
                buffers.m_indices.push_back((i + 1) * (slices + 1) + (j + 1));

                buffers.m_indices.push_back(i      * (slices + 1) + j);
                buffers.m_indices.push_back((i + 1) * (slices + 1) + (j + 1));
                buffers.m_indices.push_back(i      * (slices + 1) + (j + 1));
            }
        }
    }
}