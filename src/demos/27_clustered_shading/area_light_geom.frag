#version 460

layout(location = 0) in vec3 light_color;
layout(location = 1) flat in uint two_sided;
layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = vec4(0.0, 0.0, 0.0, 1.0);

    if (gl_FrontFacing || two_sided == 1)
    {
        frag_color = vec4(light_color, 1.0);
    }
}