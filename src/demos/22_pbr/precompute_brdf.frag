#version 450
#define PI 3.141592653589793238462643

const uint SAMPLE_COUNT = 1024;

in vec2 texcoord;
out vec2 frag_color;

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
float geometrySchlickGGX(float NdotV, float roughness)
{
	//Note: different k for the IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 n, vec3 wo, vec3 wi, float roughness)
{
    float NdotV = max(dot(n, wo), 0.0);
    float NdotL = max(dot(n, wi), 0.0);

    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec2 integrateBRDF(float NdotV, float roughness)
{
    vec3 wo;
    wo.x = sqrt(1.0 - NdotV*NdotV);
    wo.y = 0.0;
    wo.z = NdotV;

    float A = 0.0;
    float B = 0.0; 

    vec3 N = vec3(0.0, 0.0, 1.0);

    for(uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSampleGGX(Xi, N, roughness);
        vec3 wi = normalize(2.0 * dot(wo, H) * H - wo);

        float NdotL = max(wi.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(wo, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = geometrySmith(N, wo, wi, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}
// ----------------------------------------------------------------------------
void main() 
{
    vec2 integrated_brdf = integrateBRDF(texcoord.x, texcoord.y);
    frag_color = integrated_brdf;
}