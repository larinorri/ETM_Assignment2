cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float2 time;
	float2 padding;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
	// must be passed for older shader models
	float3 posC : POSITION0;
};

#define R_FACTOR 1.5f
#define T_FACTOR 1.5f

float4 main(PixelShaderInput input) : SV_TARGET
{
	//return float4(input.color,1.0f);
	float3x3 clrMat = {
		cos(time.x * R_FACTOR), -sin(time.x * R_FACTOR), 0,
		sin(time.x * R_FACTOR),  cos(time.x * R_FACTOR), 0,
		0, 0, 1
	};
	float3 seedClr = float3(cos(time.x + input.posC.x * T_FACTOR),
							sin(time.x + input.posC.y * T_FACTOR),
							cos(time.x + input.posC.y * T_FACTOR));

	return 1-float4(mul(seedClr, clrMat), 1);
}
