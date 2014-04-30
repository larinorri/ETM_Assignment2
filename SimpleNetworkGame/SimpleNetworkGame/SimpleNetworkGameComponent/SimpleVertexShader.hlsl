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
	float3 nrm : NORMAL;
	float3 uvw : TEXCOORD;
};

struct VertexShaderOutput
{
	float4 pos : SV_POSITION;
	float3 uvw : TEXCOORD;
	float3 nrm : NORMAL;
	// must be passed for older shader models
	float3 posC : POSITION0;
	float3 posW : POSITION1;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	float4 pos = float4(input.pos, 1.0f);

	// Transform the vertex position into projected space.
	// these should all be identity for the menu
	pos = mul(pos, model);
	output.posW = pos.xyz;

	pos = mul(pos, view);
	pos = mul(pos, projection);

	output.pos = pos;
	output.posC = pos.xyz / pos.w;

	// world space normal
	output.nrm = mul(input.nrm, model);
	
	// Pass through the uvw without modification.
	output.uvw = input.uvw;
	
	return output;
}
