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

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uvw : TEXCOORD;
	// must be passed for older shader models
	float3 posC : POSITION0;
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
	if (shading == TEXTURED)
	{
		final = surfaceColor.Sample(wrapSampler, input.uvw.xy);
	}
	
	return final;
}
