#include "objectHF.hlsli"



float4 main(PixelInputType input) : SV_TARGET
{
	OBJECT_PS_MAKE

	return float4(0, 0, 0, 1);
}