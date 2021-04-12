#version 450

in vec2 texcoord;
out vec4 fragColor;

layout(binding = 0) uniform sampler2D filterTexture;

subroutine vec4 ps_filter();
layout(location = 0) subroutine uniform ps_filter ps_filter_func;

layout(index = 0) subroutine(ps_filter) vec4 no_filter()
{
	return texture(filterTexture, texcoord);
}

layout(index = 1) subroutine(ps_filter) vec4 negative()
{
	return 1 - texture(filterTexture, texcoord);
}

float luminance(vec3 color)
{
	return dot(vec3(0.2126, 0.7152, 0.0722), color);
}

layout(index = 2) subroutine(ps_filter) vec4 edge_detection()
{
	/* Sobel operator */
	const float edge_threshold = 0.05;
	ivec2 pix = ivec2(gl_FragCoord.xy);
	
	float s00 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(-1,1)).rgb);
	float s10 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(-1,0)).rgb);
	float s20 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(-1,-1)).rgb);
	float s01 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(0,1)).rgb);
	float s21 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(0,-1)).rgb);
	float s02 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(1,1)).rgb);
	float s12 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(1,0)).rgb);
	float s22 = luminance(texelFetchOffset(filterTexture, pix, 0, ivec2(1,-1)).rgb);

	float sx = s00 + 2 * s10 + s20 - (s02 + 2 * s12 + s22);
	float sy = s00 + 2 * s01 + s02 - (s20 + 2 * s21 + s22);
	float g = sx * sx + sy * sy;
	
	if(g > edge_threshold) 
	{
		return vec4(1.0);
	}
	else 
	{
		return vec4(0, 0, 0, 1);
	}
}

float normpdf(in float x, in float sigma)
{
	return 0.39894 * exp(-0.5*x*x/(sigma*sigma)) / sigma;
}

layout(index = 3) subroutine(ps_filter) vec4 gaussian_blur()
{
	/* Gaussian blur based on: https://www.shadertoy.com/view/XdfGDH */
	vec2 texture_dims = textureSize(filterTexture, 0);
	vec3 color = vec3(0.0);

    //declare stuff
	const int mSize = 11;
	const int kSize = (mSize-1)/2;
	float kernel[mSize];
	
	//create the 1-D kernel
	float sigma = 7.0;
	float Z = 0.0;
	for (int j = 0; j <= kSize; ++j)
	{
		kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), sigma);
	}
	
	//get the normalization factor (as the gaussian has been clamped)
	for (int j = 0; j < mSize; ++j)
	{
		Z += kernel[j];
	}
	
	//read out the texels
	for (int i = -kSize; i <= kSize; ++i)
	{
		for (int j = -kSize; j <= kSize; ++j)
		{
			color += kernel[kSize + j] * kernel[kSize + i] * texture(filterTexture, (gl_FragCoord.xy + vec2(float(i),float(j))) / texture_dims).rgb;
		}
	}

	return vec4(color / (Z * Z), 1.0);
}

void main()
{
	fragColor = ps_filter_func();
}