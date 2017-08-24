// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2015 NVIDIA Corporation. All rights reserved.

// This file was generated by NvParameterized/scripts/GenParameterized.pl


#include "ConvexHullParameters_0p0.h"
#include <string.h>
#include <stdlib.h>

using namespace NvParameterized;

namespace nvidia
{
namespace parameterized
{

using namespace ConvexHullParameters_0p0NS;

const char* const ConvexHullParameters_0p0Factory::vptr =
    NvParameterized::getVptr<ConvexHullParameters_0p0, ConvexHullParameters_0p0::ClassAlignment>();

const uint32_t NumParamDefs = 15;
static NvParameterized::DefinitionImpl* ParamDefTable; // now allocated in buildTree [NumParamDefs];


static const size_t ParamLookupChildrenTable[] =
{
	1, 3, 7, 9, 11, 12, 13, 14, 2, 4, 5, 6, 8, 10,
};

#define TENUM(type) nvidia::##type
#define CHILDREN(index) &ParamLookupChildrenTable[index]
static const NvParameterized::ParamLookupNode ParamLookupTable[NumParamDefs] =
{
	{ TYPE_STRUCT, false, 0, CHILDREN(0), 8 },
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->vertices), CHILDREN(8), 1 }, // vertices
	{ TYPE_VEC3, false, 1 * sizeof(physx::PxVec3), NULL, 0 }, // vertices[]
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->uniquePlanes), CHILDREN(9), 1 }, // uniquePlanes
	{ TYPE_STRUCT, false, 1 * sizeof(Plane_Type), CHILDREN(10), 2 }, // uniquePlanes[]
	{ TYPE_VEC3, false, (size_t)(&((Plane_Type*)0)->normal), NULL, 0 }, // uniquePlanes[].normal
	{ TYPE_F32, false, (size_t)(&((Plane_Type*)0)->d), NULL, 0 }, // uniquePlanes[].d
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->widths), CHILDREN(12), 1 }, // widths
	{ TYPE_F32, false, 1 * sizeof(float), NULL, 0 }, // widths[]
	{ TYPE_ARRAY, true, (size_t)(&((ParametersStruct*)0)->edges), CHILDREN(13), 1 }, // edges
	{ TYPE_U32, false, 1 * sizeof(uint32_t), NULL, 0 }, // edges[]
	{ TYPE_BOUNDS3, false, (size_t)(&((ParametersStruct*)0)->bounds), NULL, 0 }, // bounds
	{ TYPE_F32, false, (size_t)(&((ParametersStruct*)0)->volume), NULL, 0 }, // volume
	{ TYPE_U32, false, (size_t)(&((ParametersStruct*)0)->uniqueEdgeDirectionCount), NULL, 0 }, // uniqueEdgeDirectionCount
	{ TYPE_U32, false, (size_t)(&((ParametersStruct*)0)->planeCount), NULL, 0 }, // planeCount
};


bool ConvexHullParameters_0p0::mBuiltFlag = false;
NvParameterized::MutexType ConvexHullParameters_0p0::mBuiltFlagMutex;

ConvexHullParameters_0p0::ConvexHullParameters_0p0(NvParameterized::Traits* traits, void* buf, int32_t* refCount) :
	NvParameters(traits, buf, refCount)
{
	//mParameterizedTraits->registerFactory(className(), &ConvexHullParameters_0p0FactoryInst);

	if (!buf) //Do not init data if it is inplace-deserialized
	{
		initDynamicArrays();
		initStrings();
		initReferences();
		initDefaults();
	}
}

ConvexHullParameters_0p0::~ConvexHullParameters_0p0()
{
	freeStrings();
	freeReferences();
	freeDynamicArrays();
}

void ConvexHullParameters_0p0::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	NvParameterized::Traits* traits = mParameterizedTraits;
	int32_t* refCount = mRefCount;
	void* buf = mBuffer;

	this->~ConvexHullParameters_0p0();

	NvParameters::destroy(this, traits, doDeallocateSelf, refCount, buf);
}

const NvParameterized::DefinitionImpl* ConvexHullParameters_0p0::getParameterDefinitionTree(void)
{
	if (!mBuiltFlag) // Double-checked lock
	{
		NvParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);
		if (!mBuiltFlag)
		{
			buildTree();
		}
	}

	return(&ParamDefTable[0]);
}

const NvParameterized::DefinitionImpl* ConvexHullParameters_0p0::getParameterDefinitionTree(void) const
{
	ConvexHullParameters_0p0* tmpParam = const_cast<ConvexHullParameters_0p0*>(this);

	if (!mBuiltFlag) // Double-checked lock
	{
		NvParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);
		if (!mBuiltFlag)
		{
			tmpParam->buildTree();
		}
	}

	return(&ParamDefTable[0]);
}

NvParameterized::ErrorType ConvexHullParameters_0p0::getParameterHandle(const char* long_name, Handle& handle) const
{
	ErrorType Ret = NvParameters::getParameterHandle(long_name, handle);
	if (Ret != ERROR_NONE)
	{
		return(Ret);
	}

	size_t offset;
	void* ptr;

	getVarPtr(handle, ptr, offset);

	if (ptr == NULL)
	{
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	return(ERROR_NONE);
}

NvParameterized::ErrorType ConvexHullParameters_0p0::getParameterHandle(const char* long_name, Handle& handle)
{
	ErrorType Ret = NvParameters::getParameterHandle(long_name, handle);
	if (Ret != ERROR_NONE)
	{
		return(Ret);
	}

	size_t offset;
	void* ptr;

	getVarPtr(handle, ptr, offset);

	if (ptr == NULL)
	{
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	return(ERROR_NONE);
}

void ConvexHullParameters_0p0::getVarPtr(const Handle& handle, void*& ptr, size_t& offset) const
{
	ptr = getVarPtrHelper(&ParamLookupTable[0], const_cast<ConvexHullParameters_0p0::ParametersStruct*>(&parameters()), handle, offset);
}


/* Dynamic Handle Indices */

void ConvexHullParameters_0p0::freeParameterDefinitionTable(NvParameterized::Traits* traits)
{
	if (!traits)
	{
		return;
	}

	if (!mBuiltFlag) // Double-checked lock
	{
		return;
	}

	NvParameterized::MutexType::ScopedLock lock(mBuiltFlagMutex);

	if (!mBuiltFlag)
	{
		return;
	}

	for (uint32_t i = 0; i < NumParamDefs; ++i)
	{
		ParamDefTable[i].~DefinitionImpl();
	}

	traits->free(ParamDefTable);

	mBuiltFlag = false;
}

#define PDEF_PTR(index) (&ParamDefTable[index])

void ConvexHullParameters_0p0::buildTree(void)
{

	uint32_t allocSize = sizeof(NvParameterized::DefinitionImpl) * NumParamDefs;
	ParamDefTable = (NvParameterized::DefinitionImpl*)(mParameterizedTraits->alloc(allocSize));
	memset(ParamDefTable, 0, allocSize);

	for (uint32_t i = 0; i < NumParamDefs; ++i)
	{
		NV_PARAM_PLACEMENT_NEW(ParamDefTable + i, NvParameterized::DefinitionImpl)(*mParameterizedTraits);
	}

	// Initialize DefinitionImpl node: nodeIndex=0, longName=""
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[0];
		ParamDef->init("", TYPE_STRUCT, "STRUCT", true);






	}

	// Initialize DefinitionImpl node: nodeIndex=1, longName="vertices"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[1];
		ParamDef->init("vertices", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The vertices of a convex polytope.", true);
		HintTable[1].init("shortDescription", "Convex hull vertices", true);
		ParamDefTable[1].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=2, longName="vertices[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[2];
		ParamDef->init("vertices", TYPE_VEC3, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The vertices of a convex polytope.", true);
		HintTable[1].init("shortDescription", "Convex hull vertices", true);
		ParamDefTable[2].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=3, longName="uniquePlanes"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[3];
		ParamDef->init("uniquePlanes", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "Unique planes (neglecting parallel opposites).  That is, if two faces exist\n				on the convex hull which have opposite normals, the plane for only one of those faces is recorded.\n				The other face is implicitly recorded by a corresponding width (in the widths array).  Edges and\n				vertices are also recorded, explicitly defining all faces.", true);
		HintTable[1].init("shortDescription", "Unique planes (neglecting parallel opposites)", true);
		ParamDefTable[3].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=4, longName="uniquePlanes[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[4];
		ParamDef->init("uniquePlanes", TYPE_STRUCT, "Plane", true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "Unique planes (neglecting parallel opposites).  That is, if two faces exist\n				on the convex hull which have opposite normals, the plane for only one of those faces is recorded.\n				The other face is implicitly recorded by a corresponding width (in the widths array).  Edges and\n				vertices are also recorded, explicitly defining all faces.", true);
		HintTable[1].init("shortDescription", "Unique planes (neglecting parallel opposites)", true);
		ParamDefTable[4].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=5, longName="uniquePlanes[].normal"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[5];
		ParamDef->init("normal", TYPE_VEC3, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "A plane normal.  This plane is used to define convex volumes.", true);
		HintTable[1].init("shortDescription", "A plane normal", true);
		ParamDefTable[5].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=6, longName="uniquePlanes[].d"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[6];
		ParamDef->init("d", TYPE_F32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "A plane displacement, defined by the negative of the plane normal\n						dotted with a point in the plane.  This plane is used to define convex volumes.", true);
		HintTable[1].init("shortDescription", "A plane displacement", true);
		ParamDefTable[6].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=7, longName="widths"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[7];
		ParamDef->init("widths", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "For each unique plane (see uniquePlanes), this is the width of the convex polytope.\n				That is, if an opposing face exists, it is the distance between the faces.  If no opposing face exists,\n				it is the maximum distance below the unique plane over all vertices.", true);
		HintTable[1].init("shortDescription", "For each unique plane (see uniquePlanes), this is the width of the convex polytope", true);
		ParamDefTable[7].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=8, longName="widths[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[8];
		ParamDef->init("widths", TYPE_F32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "For each unique plane (see uniquePlanes), this is the width of the convex polytope.\n				That is, if an opposing face exists, it is the distance between the faces.  If no opposing face exists,\n				it is the maximum distance below the unique plane over all vertices.", true);
		HintTable[1].init("shortDescription", "For each unique plane (see uniquePlanes), this is the width of the convex polytope", true);
		ParamDefTable[8].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=9, longName="edges"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[9];
		ParamDef->init("edges", TYPE_ARRAY, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "All edges of the convex polytope, stored in a compressed index format.\n				Each 32-bit integer stores the indices of two endpoints, in the high and low words.  The indices\n				refer to the vertices array.\n				The edges are stored such that all unique edge directions are represented by the first\n				uniqueEdgeDirectionCount entries.", true);
		HintTable[1].init("shortDescription", "All edges of the convex polytope, stored in a compressed index format", true);
		ParamDefTable[9].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */




		ParamDef->setArraySize(-1);
	}

	// Initialize DefinitionImpl node: nodeIndex=10, longName="edges[]"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[10];
		ParamDef->init("edges", TYPE_U32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "All edges of the convex polytope, stored in a compressed index format.\n				Each 32-bit integer stores the indices of two endpoints, in the high and low words.  The indices\n				refer to the vertices array.\n				The edges are stored such that all unique edge directions are represented by the first\n				uniqueEdgeDirectionCount entries.", true);
		HintTable[1].init("shortDescription", "All edges of the convex polytope, stored in a compressed index format", true);
		ParamDefTable[10].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=11, longName="bounds"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[11];
		ParamDef->init("bounds", TYPE_BOUNDS3, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The AABB of the convex hull.", true);
		HintTable[1].init("shortDescription", "The AABB of the convex hull", true);
		ParamDefTable[11].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=12, longName="volume"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[12];
		ParamDef->init("volume", TYPE_F32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The volume of the convex hull.", true);
		HintTable[1].init("shortDescription", "The volume of the convex hull", true);
		ParamDefTable[12].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=13, longName="uniqueEdgeDirectionCount"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[13];
		ParamDef->init("uniqueEdgeDirectionCount", TYPE_U32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The number of unique edge directions.  The first uniqueEdgeDirectionCount\n				elements of the edges array represent these directions.", true);
		HintTable[1].init("shortDescription", "The number of unique edge directions", true);
		ParamDefTable[13].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// Initialize DefinitionImpl node: nodeIndex=14, longName="planeCount"
	{
		NvParameterized::DefinitionImpl* ParamDef = &ParamDefTable[14];
		ParamDef->init("planeCount", TYPE_U32, NULL, true);

#ifdef NV_PARAMETERIZED_HIDE_DESCRIPTIONS

#else

		static HintImpl HintTable[2];
		static Hint* HintPtrTable[2] = { &HintTable[0], &HintTable[1], };
		HintTable[0].init("longDescription", "The total number of faces.  This includes parallel opposite faces,\n				so may be larger than the array size of uniquePlanes.  For plane indices i less than uniquePlanes.size(),\n				simply use uniquePlanes[i] to find the corresponding plane.  For plane indicies i in the range\n				[uniquePlanes.size(), planeCount), the uniquePlanes array is arranged such that you obtain the correct plane\n				by starting with the plane p = uniquePlanes[index-uniquePlanes.size()].  Then, add widths[index-uniquePlanes.size()]\n				to the plane displacement p.d, and finally negate both the plane normal p.n and the displacement p.d.", true);
		HintTable[1].init("shortDescription", "The total number of faces", true);
		ParamDefTable[14].setHints((const NvParameterized::Hint**)HintPtrTable, 2);

#endif /* NV_PARAMETERIZED_HIDE_DESCRIPTIONS */





	}

	// SetChildren for: nodeIndex=0, longName=""
	{
		static Definition* Children[8];
		Children[0] = PDEF_PTR(1);
		Children[1] = PDEF_PTR(3);
		Children[2] = PDEF_PTR(7);
		Children[3] = PDEF_PTR(9);
		Children[4] = PDEF_PTR(11);
		Children[5] = PDEF_PTR(12);
		Children[6] = PDEF_PTR(13);
		Children[7] = PDEF_PTR(14);

		ParamDefTable[0].setChildren(Children, 8);
	}

	// SetChildren for: nodeIndex=1, longName="vertices"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(2);

		ParamDefTable[1].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=3, longName="uniquePlanes"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(4);

		ParamDefTable[3].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=4, longName="uniquePlanes[]"
	{
		static Definition* Children[2];
		Children[0] = PDEF_PTR(5);
		Children[1] = PDEF_PTR(6);

		ParamDefTable[4].setChildren(Children, 2);
	}

	// SetChildren for: nodeIndex=7, longName="widths"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(8);

		ParamDefTable[7].setChildren(Children, 1);
	}

	// SetChildren for: nodeIndex=9, longName="edges"
	{
		static Definition* Children[1];
		Children[0] = PDEF_PTR(10);

		ParamDefTable[9].setChildren(Children, 1);
	}

	mBuiltFlag = true;

}
void ConvexHullParameters_0p0::initStrings(void)
{
}

void ConvexHullParameters_0p0::initDynamicArrays(void)
{
	vertices.buf = NULL;
	vertices.isAllocated = true;
	vertices.elementSize = sizeof(physx::PxVec3);
	vertices.arraySizes[0] = 0;
	uniquePlanes.buf = NULL;
	uniquePlanes.isAllocated = true;
	uniquePlanes.elementSize = sizeof(Plane_Type);
	uniquePlanes.arraySizes[0] = 0;
	widths.buf = NULL;
	widths.isAllocated = true;
	widths.elementSize = sizeof(float);
	widths.arraySizes[0] = 0;
	edges.buf = NULL;
	edges.isAllocated = true;
	edges.elementSize = sizeof(uint32_t);
	edges.arraySizes[0] = 0;
}

void ConvexHullParameters_0p0::initDefaults(void)
{

	freeStrings();
	freeReferences();
	freeDynamicArrays();
	volume = float(0);
	uniqueEdgeDirectionCount = uint32_t(0);
	planeCount = uint32_t(0);

	initDynamicArrays();
	initStrings();
	initReferences();
}

void ConvexHullParameters_0p0::initReferences(void)
{
}

void ConvexHullParameters_0p0::freeDynamicArrays(void)
{
	if (vertices.isAllocated && vertices.buf)
	{
		mParameterizedTraits->free(vertices.buf);
	}
	if (uniquePlanes.isAllocated && uniquePlanes.buf)
	{
		mParameterizedTraits->free(uniquePlanes.buf);
	}
	if (widths.isAllocated && widths.buf)
	{
		mParameterizedTraits->free(widths.buf);
	}
	if (edges.isAllocated && edges.buf)
	{
		mParameterizedTraits->free(edges.buf);
	}
}

void ConvexHullParameters_0p0::freeStrings(void)
{
}

void ConvexHullParameters_0p0::freeReferences(void)
{
}

} // namespace parameterized
} // namespace nvidia
