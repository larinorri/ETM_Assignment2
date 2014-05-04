// texture objects
texture2D surfaceColor : register(t0);
sampler wrapSampler : register(s0);

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float2 time;
	uint shading;
	uint padding;
};

struct SpotLight
{
	float3	coneDir;
	float	innerCone;
	float3	lightPos;
	float	outerCone;
	float3	lightColor;
	float	radius;
};

struct PointLight
{
	float3	lightPos;
	float	radius;
	float4	lightColor;
};

cbuffer LightDataConstantBuffer : register(b1)
{
	SpotLight	lanterns[2];
	PointLight	ambiance[2];
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uvw : TEXCOORD;
	float3 nrm : NORMAL;
	// must be passed for older shader models
	float3 posC : POSITION0;
	float3 posW : POSITION1;
};

#define R_FACTOR 1.5f
#define T_FACTOR 1.5f

// types of shading available (stored in "shading")
#define MENU		0
#define TEXTURED	1
#define LIGHTING	2


float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 final = { 1, 1, 1, 1 };

	if (shading == MENU)
	{
		float3x3 clrMat = {
			cos(time.x * R_FACTOR), -sin(time.x * R_FACTOR), 0,
			sin(time.x * R_FACTOR), cos(time.x * R_FACTOR), 0,
			0, 0, 1
		};
		float3 seedClr = float3(cos(time.x + input.posC.x * T_FACTOR),
			sin(time.x + input.posC.y * T_FACTOR),
			cos(time.x + input.posC.y * T_FACTOR));
		final = 1 - float4(mul(seedClr, clrMat), 1);
	}
	if (shading >= TEXTURED)
	{
		final = surfaceColor.Sample(wrapSampler, input.uvw.xy);
		clip(final.a - 0.9f); // remove alpha pixels
	}
	if (shading == LIGHTING)
	{
		float3 totalLight = { 0, 0, 0 };
		// compute spotlights
		for (uint i = 0; i < 2; ++i)
		{
			float3 lightDir = lanterns[i].lightPos - input.posW;
			float3 lightDirN = normalize(lightDir);
			float surfaceRatio = saturate(dot(-lightDirN, lanterns[i].coneDir));
			float lightAmount = saturate(dot(lightDirN, normalize(input.nrm)));
			lightAmount *= 1 - saturate(length(lightDir) / lanterns[i].radius);
			lightAmount *= 1 - saturate((lanterns[i].innerCone - surfaceRatio) /
										(lanterns[i].innerCone - lanterns[i].outerCone));
			totalLight += lightAmount * lanterns[i].lightColor;
		}
		final.rgb *= totalLight;
	}
	// final fragment color
	return final;
}
/*
LIGHTDIR = NORMALIZE( LIGHTPOS – SURFACEPOS ) )
SURFACERATIO = CLAMP( DOT( -LIGHTDIR, CONEDIR ) )
LIGHTRATIO = CLAMP( DOT( LIGHTDIR, SURFACENORMAL ) )
RESULT = SPOTFACTOR * LIGHTRATIO * LIGHTCOLOR * SURFACECOLOR

ATTENUATION = 1.0 – CLAMP( MAGNITUDE( LIGHTPOS – SURFACEPOS ) / LIGHTRADIUS )

ATTENUATION = 1.0 – CLAMP( ( INNERCONERATIO - SURFACERATIO ) / ( INNERCONERATIO – OUTERCONERATIO ) )

*/