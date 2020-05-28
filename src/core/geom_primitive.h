#pragma once

#include "model.h"

namespace RapidGL
{
    class GeomPrimitive
    {
    public:
        GeomPrimitive() = delete;
        ~GeomPrimitive() = delete;

        friend class Model;

    private:
        static void genCube    (VertexBuffers & buffers, float radius);
        static void genCubeMap (VertexBuffers & buffers, float radius);
        static void genTorus   (VertexBuffers & buffers, float innerRadius, float outerRadius, unsigned int slices, unsigned int stacks);
        static void genCylinder(VertexBuffers & buffers, float height, float radius, unsigned int slices);
        static void genCone    (VertexBuffers & buffers, float height, float radius, unsigned int slices, unsigned int stacks);
        static void genQuad    (VertexBuffers & buffers, float width, float height);
        static void genPlane   (VertexBuffers & buffers, float width, float height, unsigned int slices, unsigned int stacks);
        static void genSphere  (VertexBuffers & buffers, float radius, unsigned int slices);
    };
}

