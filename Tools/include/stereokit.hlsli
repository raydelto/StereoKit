///////////////////////////////////////////

cbuffer StereoKitBuffer : register(b1) {
	float4x4 sk_view       [2];
	float4x4 sk_proj       [2];
	float4x4 sk_proj_inv   [2];
	float4x4 sk_viewproj   [2];
	float4   sk_lighting_sh[9];
	float4   sk_camera_pos [2];
	float4   sk_camera_dir [2];
	float4   sk_fingertip  [2];
	float4   sk_cubemap_i;
	float    sk_time;
	uint     sk_view_count;
};
struct Inst {
	float4x4 world;
	float4   color;
};
cbuffer TransformBuffer : register(b2) {
	Inst sk_inst[819]; // 819 is UINT16_MAX / sizeof(Inst)
};
TextureCube  sk_cubemap   : register(t11);
SamplerState sk_cubemap_s : register(s11);

///////////////////////////////////////////

// A spherical harmonics lighting lookup!
// Some calculations have been offloaded to 'sh_to_fast'
// in StereoKitC
float3 Lighting(float3 normal) {
	// Band 0
	float3 result = sk_lighting_sh[0].xyz;

	// Band 1
	result += sk_lighting_sh[1].xyz * normal.y;
	result += sk_lighting_sh[2].xyz * normal.z;
	result += sk_lighting_sh[3].xyz * normal.x;

	// Band 2
	float3 n  = normal.xyz * normal.yzx;
	float3 n2 = normal * normal;
	result += sk_lighting_sh[4].xyz * n.x;
	result += sk_lighting_sh[5].xyz * n.y;
	result += sk_lighting_sh[6].xyz * (3.0f * n2.z - 1.0f);
	result += sk_lighting_sh[7].xyz * n.z;
	result += sk_lighting_sh[8].xyz * (n2.x - n2.y);
	return result;
}