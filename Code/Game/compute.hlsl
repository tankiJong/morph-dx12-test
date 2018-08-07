

RWTexture2D<float4> Output: register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadId : SV_DispatchThreadID )
{
	Output[threadId.xy] = float4(threadId.xy % 32 / 32.f, 1.f, 1.f);
}