void main( in float3 inPosition : POSITION0, in float4 inColor : COLOR0, out float4 outPosition : SV_POSITION, out float4 outColor : TEXCOORD0 ) {
	outPosition = float4(inPosition, 1.0f);
	outColor = float4(inColor);
}