#pragma once

#include "model.h"

namespace RGL
{
    class GeomPrimitive
    {
    public:
        GeomPrimitive() = delete;
        ~GeomPrimitive() = delete;

        friend class Model;

    private:
        static void genCube       (VertexBuffers & buffers, float radius);
        static void genCubeMap    (VertexBuffers & buffers, float radius);
        static void genTorus      (VertexBuffers & buffers, float innerRadius, float outerRadius, unsigned int slices, unsigned int stacks);
        static void genCylinder   (VertexBuffers & buffers, float height, float radius, unsigned int slices);
        static void genCone       (VertexBuffers & buffers, float height, float radius, unsigned int slices, unsigned int stacks);
        static void genQuad       (VertexBuffers & buffers, float width, float height);
        static void genPlane      (VertexBuffers & buffers, float width, float height, unsigned int slices, unsigned int stacks);
        static void genSphere     (VertexBuffers & buffers, float radius, unsigned int slices);
        static void genTrefoilKnot(VertexBuffers & buffers, unsigned int slices, unsigned int stacks);
        static void genPQTorusKnot(VertexBuffers& buffers, unsigned int slices, unsigned int stacks, int p, int q, float knot_r, float tube_r);
    };
}

