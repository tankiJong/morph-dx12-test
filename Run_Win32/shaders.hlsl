//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

cbuffer Tint : register(b0) {
  float4 cTintColor;
};

struct PSInput {
  float4 position : SV_POSITION;
  float4 color : COLOR;
  float2 uv: UV;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float3 position : POSITION, float4 color : COLOR, float2 uv: UV) {
  PSInput result;

  result.position = float4(position, 1.f);
  result.color = color;
  result.uv = uv;
  return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  return g_texture.Sample(g_sampler, input.uv) * cTintColor;
}
