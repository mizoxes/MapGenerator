struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
};

float4 PS(PS_INPUT input) : SV_TARGET
{
	float3 lightdir = normalize(float3(-1.0f, 1.0f, -1.0f));

	float diffuse = max(dot(lightdir, input.Normal), 0.2f);

	float4 color = diffuse * float4(0.5f, 0.8f, 0.4f, 1.0f);
	color.a = 1;

	return color;
}