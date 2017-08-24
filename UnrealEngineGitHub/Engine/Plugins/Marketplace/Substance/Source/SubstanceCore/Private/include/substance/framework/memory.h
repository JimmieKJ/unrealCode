//! @file memory.h
//! @author Josh Coyne - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.
#ifndef _SUBSTANCE_AIR_FRAMEWORK_MEMORY_H
#define _SUBSTANCE_AIR_FRAMEWORK_MEMORY_H

#if !defined(AIR_DEFAULT_ALIGNMENT)
	#define AIR_DEFAULT_ALIGNMENT 16
#endif

namespace SubstanceAir
{
	
void* alignedMalloc(size_t bytesCount, size_t alignment);
void alignedFree(void* bufferPtr);

//! @brief substance air C++ allocator
template<class T, int ALIGNMENT> struct aligned_allocator : std::allocator<T>
{
	typedef std::allocator<T> base;
	typedef typename base::pointer pointer;
	typedef typename base::size_type size_type;

	template<class U> struct rebind { typedef aligned_allocator<U,ALIGNMENT> other; };

	aligned_allocator() 
		: std::allocator<T>()
	{
	}

	template<class U>
	aligned_allocator(const aligned_allocator<U,ALIGNMENT>& u)
		: std::allocator<T>(u)
	{
	}

	aligned_allocator(const aligned_allocator<T,ALIGNMENT>& u)
		: std::allocator<T>(u)
	{
	}

	pointer allocate(size_type n)
	{
		return (pointer)alignedMalloc(n * sizeof(T), ALIGNMENT);
	}

	pointer allocate(size_type n, void const*)
	{
		return this->allocate(n);
	}

	void deallocate(pointer p, size_type)
	{
		alignedFree(p);
	}
};

//! @brief substance air new/delete macros
template<typename T> void deleteObject(T* ptr)
{
	ptr->~T();
	alignedFree(ptr);
}

template<typename T> T* newObjectArray(size_t count)
{
	size_t* ptr = (size_t*)SubstanceAir::alignedMalloc(sizeof(size_t) + (sizeof(T) * count), AIR_DEFAULT_ALIGNMENT);
	*ptr = count;
	T* objArray = reinterpret_cast<T*>(ptr+1);
	for (size_t i = 0;i < count;i++)
		new (objArray+i) T;
	return objArray;
}

template<typename T> void deleteObjectArray(T* ptr)
{
	size_t* countPtr = reinterpret_cast<size_t*>(ptr) - 1;
	size_t count = *(countPtr);
	for (size_t i = 0;i < count;i++)
		(ptr + i)->~T();
	alignedFree(countPtr);
}

//! @brief deleter
template<typename T> struct deleter
{
	void operator()(T* ptr)
	{
		deleteObject(ptr);
	}
};

}

//used to ensure only our macros use our global new/delete operators
struct SubstanceAirMallocDummy { int unused; };

inline void* operator new (size_t size, SubstanceAirMallocDummy*, const std::nothrow_t&)
{
	return SubstanceAir::alignedMalloc(size, AIR_DEFAULT_ALIGNMENT);
}

//compiler complains if we don't have this, but do NOT use it!
inline void operator delete(void*, SubstanceAirMallocDummy*, const std::nothrow_t&) { assert(false); }

#define AIR_NEW(type) new ((SubstanceAirMallocDummy*)0, std::nothrow) type
#define AIR_DELETE(ptr) deleteObject(ptr)

#define AIR_NEW_ARRAY(type,count) newObjectArray<type>(count)
#define AIR_DELETE_ARRAY(ptr) deleteObjectArray(ptr)

//! @brief if AIR_OVERRIDE_GLOBAL_NEW is set
//!	we override the global new operators.
//! Be careful enabling this as it will override
//! operator new in any linked static libraries you compile against!
#if defined(AIR_OVERRIDE_GLOBAL_NEW)
void* operator	new (size_t);
void* operator	new(size_t, std::nothrow_t&);
void* operator	new [](size_t);
void* operator	new [](size_t, std::nothrow_t&);
void operator	delete(void*);
void operator	delete(void*, std::nothrow_t&);
void operator	delete [](void*);
void operator	delete [](void*, std::nothrow_t&);
#endif

#endif //_SUBSTANCE_AIR_FRAMEWORK_MEMORY_H
