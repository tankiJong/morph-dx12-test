#define IDENTITY_MATRIX float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

float4x4 inverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}


struct PSInput {
	float4 position: SV_POSITION;
	float2 uv: UV;
};

PSInput VSMain(uint id: SV_VertexID)
{
	PSInput input;

	input.uv = float2((id << 1) & 2, id & 2);
	input.position = float4(input.uv * float2(2, -2) + float2(-1, 1), 0, 1);
	return input;
}

struct ssao_param_t {
	float4 kernels[32];
};

cbuffer cCamera : register(b0) {
  float4x4 projection;
  float4x4 view;
};
cbuffer cSSAOParam: register(b1) {
	ssao_param_t ssaoParam;
}

Texture2D gTexDepth : register(t0);
Texture2D gTexNormal : register(t1);
Texture2D gTexScene: register(t2);
SamplerState gSampler : register(s0);


struct PSOutput {
	float4 color: SV_TARGET0;
	float4 occlusion: SV_TARGET1;
};
PSOutput PSMain(PSInput input) : SV_TARGET 
{
	// get world position, normal, tbn transform

	float4 ndc = float4(input.uv, gTexDepth.Sample(gSampler, input.uv).r, 1.f);
	float4 almostWorldPosition = mul(inverse(mul(projection, view)), ndc);
	float3 worldPosition = almostWorldPosition.xyz / almostWorldPosition.w;

	float3 normal = normalize(gTexNormal.Sample(gSampler, input.uv) * 2.f - 1.f);

	float3 RIGHT = float3(1.f, 0.f, 0.f);
	float3 bitan = normalize(cross(RIGHT, normal));
	float3 tan = cross(bitan, normal);
	float3x3 tbn = float3x3(tan, bitan, normal);

	float occlusion = 0.f;

	for(uint i = 0; i < 32; i++) {
		
		float3 sample = mul(tbn, ssaoParam.kernels[i].xyz);
		sample += worldPosition;

		float4 screen = float4(sample, 1.f);
		screen = mul(mul(projection, view), screen);
		float3 uvd = screen.xyz / screen.w;
		
		float depth = gTexDepth.Sample(gSampler, uvd.xy).r;

		occlusion += depth > uvd.z ? 0.f : 1.f;
	}

	occlusion = 1.f - occlusion / 32.f;
	
	float4 color = gTexScene.Sample(gSampler, input.uv);

	PSOutput output;
	output.color = occlusion * color;
	output.occlusion = float4(occlusion, 0.f, 0.f, 1.f);

	return output;
}