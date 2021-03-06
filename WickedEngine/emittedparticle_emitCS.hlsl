#include "globals.hlsli"
#include "ShaderInterop_EmittedParticle.h"

RWSTRUCTUREDBUFFER(particleBuffer, Particle, 0);
RWSTRUCTUREDBUFFER(aliveBuffer_CURRENT, uint, 1);
RWSTRUCTUREDBUFFER(aliveBuffer_NEW, uint, 2);
RWSTRUCTUREDBUFFER(deadBuffer, uint, 3);
RWSTRUCTUREDBUFFER(counterBuffer, ParticleCounters, 4);

TEXTURE2D(randomTex, float4, TEXSLOT_ONDEMAND0);
TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
RAWBUFFER(meshVertexBuffer_NOR, TEXSLOT_ONDEMAND3);


[numthreads(THREADCOUNT_EMIT, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint emitCount = counterBuffer[0].realEmitCount;

	if(DTid.x < emitCount)
	{
		// we can emit:

		const float3 randoms = randomTex.SampleLevel(sampler_linear_wrap, float2((float)DTid.x / (float)THREADCOUNT_EMIT, g_xFrame_Time + xEmitterRandomness), 0).rgb;
		
		// random triangle on emitter surface:
		uint tri = (uint)(((xEmitterMeshIndexCount - 1) / 3) * randoms.x);

		// load indices of triangle from index buffer
		uint i0 = meshIndexBuffer[tri * 3 + 0];
		uint i1 = meshIndexBuffer[tri * 3 + 1];
		uint i2 = meshIndexBuffer[tri * 3 + 2];

		// load vertices of triangle from vertex buffer:
		float3 pos0 = asfloat(meshVertexBuffer_POS.Load3(i0 * xEmitterMeshVertexPositionStride));
		float3 pos1 = asfloat(meshVertexBuffer_POS.Load3(i1 * xEmitterMeshVertexPositionStride));
		float3 pos2 = asfloat(meshVertexBuffer_POS.Load3(i2 * xEmitterMeshVertexPositionStride));

		uint nor_u = meshVertexBuffer_NOR.Load(i0 * xEmitterMeshVertexNormalStride);
		float3 nor0;
		{
			nor0.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}
		nor_u = meshVertexBuffer_NOR.Load(i1 * xEmitterMeshVertexNormalStride);
		float3 nor1;
		{
			nor1.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}
		nor_u = meshVertexBuffer_NOR.Load(i2 * xEmitterMeshVertexNormalStride);
		float3 nor2;
		{
			nor2.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}

		// random barycentric coords:
		float f = randoms.x;
		float g = randoms.y;
		[flatten]
		if (f + g > 1)
		{
			f = 1 - f;
			g = 1 - g;
		}

		// compute final surface position on triangle from barycentric coords:
		float3 pos = pos0 + f * (pos1 - pos0) + g * (pos2 - pos0);
		float3 nor = nor0 + f * (nor1 - nor0) + g * (nor2 - nor0);
		pos = mul(xEmitterWorld, float4(pos, 1)).xyz;
		nor = normalize(mul((float3x3)xEmitterWorld, nor));


		// create new particle:
		Particle particle;
		particle.position = pos;
		particle.size = xParticleSize + xParticleSize * (randoms.y - 0.5f) * xParticleRandomFactor;
		particle.rotation = xParticleRotation * (randoms.z - 0.5f) * xParticleRandomFactor;
		particle.velocity = (nor + (randoms.xyz - 0.5f) * xParticleRandomFactor) * xParticleNormalFactor;
		particle.rotationalVelocity = particle.rotation;
		particle.maxLife = xParticleLifeSpan + xParticleLifeSpan * (randoms.x - 0.5f) * xParticleLifeSpanRandomness;
		particle.life = particle.maxLife;
		particle.opacity = 1;
		particle.sizeBeginEnd = float2(particle.size, particle.size * xParticleScaling);
		particle.mirror = float2(randoms.x > 0.5f, randoms.y < 0.5f);


		// new particle index retrieved from dead list (pop):
		uint deadCount;
		InterlockedAdd(counterBuffer[0].deadCount, -1, deadCount);
		uint newParticleIndex = deadBuffer[deadCount - 1];

		// write out the new particle:
		particleBuffer[newParticleIndex] = particle;

		// and add index to the alive list (push):
		uint aliveCount;
		InterlockedAdd(counterBuffer[0].aliveCount, 1, aliveCount);
		aliveBuffer_CURRENT[aliveCount] = newParticleIndex;
	}
}
