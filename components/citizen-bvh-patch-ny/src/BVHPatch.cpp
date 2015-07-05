#include "StdInc.h"
#include "Hooking.h"

struct phBoundBVH
{
	char pad[224];
	void* bvh;

	void CalculateBVH();
};

void WRAPPER phBoundBVH::CalculateBVH() { EAXJMP(0x6C0630); }

static void* ModifyRelocatedBVH(phBoundBVH* bvhBound, void* bm)
{
	// if no BVH is set, calculate it
	if (!bvhBound->bvh)
	{
		bvhBound->CalculateBVH();
	}

	return bvhBound;
}

static void WRAPPER RelocateBVH()
{
	__asm
	{
		// store register arguments
		push eax
		push ecx
		
		// call original function
		mov edx, 6C1360h
		call edx

		// modify BVH if needed
		call ModifyRelocatedBVH

		// remove arguments from stack
		add esp, 8h

		// return to caller
		retn
	}
}

static HookFunction hookFunction([]()
{
	// wrap BVH relocation so empty BVHs mean they get generated
	hook::call(0x686911, RelocateBVH);
});