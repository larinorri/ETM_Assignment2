cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float2 time;
	float2 padding;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 color : COLOR0;
};

struct VertexShaderOutput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
	// must be passed for older shader models
	float3 posC : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	float4 pos = float4(input.pos, 1.0f);

	// Transform the vertex position into projected space.
	//pos = mul(pos, model);
	//pos = mul(pos, view);
	//pos = mul(pos, projection);
	output.pos = pos;
	output.posC = pos.xyz / pos.w;

	// Pass through the color without modification.
	output.color = input.color;

	return output;
}
