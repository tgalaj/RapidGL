#version 460 core
#define PI 3.141592653589793238462643

const uint SAMPLE_COUNT = 1024;

out vec4 frag_color;

layout (location = 0) in vec3 in_world_pos;

layout (binding = 1) uniform samplerCube environment_map;
uniform float u_roughness;

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
          denom = PI * denom * denom;

    return nom / denom;
}

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float radicalInverseVdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), radicalInverseVdC(i));
}
// ----------------------------------------------------------------------------
vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;
	
	float phi      = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
// ----------------------------------------------------------------------------

void main()
{		
    vec3 N = normalize(in_world_pos);
    
    // make the simplyfying assumption that V equals R equals the normal 
    vec3 R = N;
    vec3 V = R;

    vec3 prefiltered_color = vec3(0.0);
    float total_weight = 0.0;
    
    for(uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSampleGGX(Xi, N, u_roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            // sample from the environment's mip level based on roughness/pdf
            float D     = distributionGGX(N, H, u_roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf   = D * NdotH / (4.0 * HdotV) + 0.0001; 

            float resolution = textureSize(environment_map, 0 /*lod*/).x; // resolution of source cubemap (per face)
            float sa_texel    = 4.0 * PI / (6.0 * resolution * resolution);
            float sa_sample   = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mip_level = u_roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel); 
            
            prefiltered_color += textureLod(environment_map, L, mip_level).rgb * NdotL;
            total_weight      += NdotL;
        }
    }

    prefiltered_color = prefiltered_color / total_weight;

    frag_color = vec4(prefiltered_color, 1.0);
}