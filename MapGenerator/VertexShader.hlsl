cbuffer ConstantBuffer : register(b0)
{
	float4x4 World;
	float4x4 View;
	float4x4 Projection;
};

struct VS_INPUT
{
	float3 Pos : POSITION;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
};

VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	output.Pos = mul(float4(input.Pos, 1.0f), World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Normal = normalize(mul(float4(input.Normal, 1.0f), World).xyz);

	return output;
}