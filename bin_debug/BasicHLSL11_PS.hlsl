void main(  in float4 inPosition : SV_POSITION, in float4 inColor : TEXCOORD0, out float4 Out : SV_TARGET ) {
	Out = inColor;
}