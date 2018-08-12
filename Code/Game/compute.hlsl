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

struct light_info_t {
  float4 color;

  float3 attenuation;
  float dotInnerAngle;

  float3 specAttenuation;
  float dotOuterAngle;

  float3 position;
  float directionFactor;

  float3 direction;
  float __pad00;

  float4x4 vp;
};

struct vertex {
  float4 position;
};

cbuffer cCamera : register(b0) {
  float4x4 projection;
  float4x4 view;
};

cbuffer cLight: register(b1) {
  light_info_t light;
}

struct Ray {
	float3 position;
	float3 direction;
};


Ray GenPrimaryRay(float3 ndc) {
	Ray ray;
	
	float4x4 invView = inverse(view);
	float4 _worldCoords = mul(invView, mul(inverse(projection), float4(ndc, 1.f)));
	float3 worldCoords = _worldCoords.xyz / _worldCoords.w;

	ray.position = worldCoords;

	float3 origin = mul(invView, float4(0, 0, 0, 1.f)).xyz;
	ray.direction = normalize(worldCoords - origin);

	return ray;
}


struct Contact {
	float3 position;
	float4 t;
	bool valid;
};

Contact triIntersection(float3 a, float3 b, float3 c, Ray ray) {

	Contact contact;

	float3 ab = b - a;
	float3 ac = c - a;
	float3 normal = normalize(cross(ab, ac));

	contact.valid = dot(normal, ray.direction) > 0;

	float t = (dot(normal, a) - dot(normal, ray.position)) / dot(normal, ray.direction);
	contact.t = t;
	contact.position = ray.position + t * ray.direction;

	float3 p = contact.position;
	contact.valid = contact.valid && dot(cross(b - a, p - a), normal) >= 0;
	contact.valid = contact.valid && dot(cross(c - b, p - b), normal) >= 0;
	contact.valid = contact.valid && dot(cross(a - c, p - c), normal) >= 0;

	return contact;
}


RWTexture2D<float4> Output: register(u0);
RWStructuredBuffer<float4> cVerts: register(u1);

[numthreads(32, 32, 1)]
void main( uint3 threadId : SV_DispatchThreadID )
{

	uint2 pix = threadId.xy;
	uint2 size;
	Output.GetDimensions(size.x, size.y);

	if(pix.x >= size.x || pix.y >= size.y) return;

	float2 screen = float2(pix.x, size.y - pix.y);
	float2 ndcxy = float2(screen) / float2(size);
	ndcxy = ndcxy * 2.f - 1.f;
	float3 ndc = float3(ndcxy, 0.f);

	Ray ray = GenPrimaryRay(ndc);

	uint vertCount, stride;
	cVerts.GetDimensions(vertCount, stride);

	Contact contact;
	contact.t = 1e6;
	contact.valid = false;


	uint i = 0;
	for(; i < vertCount; i+=3) {
		Contact c = triIntersection(cVerts[i].xyz, cVerts[i+1].xyz, cVerts[i+2].xyz, ray);
		bool valid = c.valid && (c.t < contact.t);
		if(valid)	{
			contact = c;
		}
	} 

	if(contact.valid) {
		float4 world = float4(contact.position, 1.f);
		float4 clip = mul(projection, mul(view, world));

		float3 ndc = clip.xyz / clip.w;

		Output[threadId.xy] = float4(.5, .5, .5, 1.f);
	}

  // float2 cc =projection[0].xy * 0.f + light.position.xy * 0.f + cVerts[0].xy * 0.f; 
	// Output[threadId.xy] = float4(threadId.xy % 32 / 32.f + cc, 1.f, 1.f);
}

