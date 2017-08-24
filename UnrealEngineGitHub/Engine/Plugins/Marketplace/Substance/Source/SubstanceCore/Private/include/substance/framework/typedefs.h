//! @file typedefs.h
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#ifndef _SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H
#define _SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H

#include <assert.h>
#include <cstddef>

// Detect C++11 support
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1900)	//C++11 isn't fully supported until MSVC 2015
	#define AIR_CPP11_SUPPORTED
	#define AIR_CPP11_CONSTEXPR constexpr
	#define AIR_CPP11_EXPLICIT  explicit
	#define AIR_CPP11_NULLPTR   std::nullptr_t
#else
	#define AIR_CPP11_CONSTEXPR
	#define AIR_CPP11_EXPLICIT
	#define AIR_CPP11_NULLPTR   const void*
#endif

// VS2010/VS2012/VS2013 and 'typename' do not always mix
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#define AIR_CPP11_TYPENAME
#else
#define AIR_CPP11_TYPENAME typename
#endif

// Detect TR1 support
#if defined(AIR_CPP11_SUPPORTED) || defined(__EMSCRIPTEN__) || defined(__GXX_EXPERIMENTAL_CXX0X__)
	#include <memory>               // C++0x support
	#define SBS_TR1_NAMESPACE std
#elif defined (__GNUC__) && __GNUC__ >= 4 && defined (__GLIBCXX__)
	#include <tr1/memory>           // GCC 4.0 TR1
	#define SBS_TR1_NAMESPACE std::tr1
#elif defined (_MSC_VER) && _MSC_FULL_VER>=150030729
	#include <memory>               // VS 2008 SP1 and above TR1
	#define SBS_TR1_NAMESPACE std::tr1
#else
	#include <boost/tr1/memory.hpp> // Boost fallback
	#define SBS_TR1_NAMESPACE std::tr1
#endif

#include <deque>
#include <map>
#include <new>
#include <string>
#include <sstream>
#include <vector>

#include <stdlib.h>

#ifdef __linux__
#include <string.h>
#endif

#include "memory.h"

namespace SubstanceAir
{

//! @brief basic types used in the integration framework
typedef unsigned int UInt;
typedef unsigned long long UInt64;

// Shared Pointer namespace
namespace Tr1 = SBS_TR1_NAMESPACE;

//! @brief shared_ptr implementation
template<typename T> class shared_ptr : private Tr1::shared_ptr<T>
{
public:
	AIR_CPP11_CONSTEXPR shared_ptr()
		: Tr1::shared_ptr<T>()
	{
	}
	AIR_CPP11_CONSTEXPR shared_ptr(AIR_CPP11_NULLPTR null)
		: Tr1::shared_ptr<T>(null)
	{
	}
	shared_ptr(T* ptr)
	{
		reset<T>(ptr);
	}
	template<typename Y> shared_ptr(Y* ptr)
	{
		reset<Y>(ptr);
	}
	inline T* operator->() const
	{
		return get();
	}
	inline T* get() const
	{
		return Tr1::shared_ptr<T>::get();
	}
	inline void reset()
	{
		Tr1::shared_ptr<T>::reset();
	}
	inline void reset(T* ptr)
	{
		reset<T>(ptr);
	}
	template<class Y> inline void reset(Y* ptr)
	{
		Tr1::shared_ptr<T>::reset(ptr, deleter<Y>(), aligned_allocator<Y, AIR_DEFAULT_ALIGNMENT>());
	}
	AIR_CPP11_EXPLICIT inline operator bool() const
	{
		return (get() != NULL) ? true : false;
	}
};

template < class T, class U > bool operator==( const shared_ptr<T>& lhs, const shared_ptr<U>& rhs )
	{ return lhs.get() == rhs.get(); }
template< class T > bool operator==( const shared_ptr<T>& lhs, AIR_CPP11_NULLPTR rhs )
	{ return lhs.get() == rhs; }
template< class T > bool operator==( AIR_CPP11_NULLPTR lhs, const shared_ptr<T>& rhs )
	{ return lhs == rhs.get(); }
template< class T, class U > bool operator!=( const shared_ptr<T>& lhs, const shared_ptr<U>& rhs )
	{ return lhs.get() != rhs.get(); }
template< class T > bool operator!=( const shared_ptr<T>& lhs, AIR_CPP11_NULLPTR rhs )
	{ return lhs.get() != rhs; }
template< class T > bool operator!=( AIR_CPP11_NULLPTR lhs, const shared_ptr<T>& rhs )
	{ return lhs != rhs.get(); }

#if defined(AIR_CPP11_SUPPORTED)
template<typename T> using unique_ptr = std::unique_ptr<T, deleter<T> >;
#else
template<typename T> class unique_ptr : private std::unique_ptr<T, deleter<T> > 
{
public:
	AIR_CPP11_CONSTEXPR unique_ptr() : std::unique_ptr<T, deleter<T> >() {}
	AIR_CPP11_CONSTEXPR unique_ptr(AIR_CPP11_NULLPTR null) : std::unique_ptr<T, deleter<T> >(null) {}
	explicit unique_ptr(typename unique_ptr::pointer p) : std::unique_ptr<T, deleter<T> >(p) {}
	unique_ptr(unique_ptr&& p) : std::unique_ptr<T, deleter<T> >(p.release()) {}

	typename unique_ptr::pointer get() const { return std::unique_ptr<T, deleter<T> >::get(); }
	typename unique_ptr::pointer release() { return std::unique_ptr<T, deleter<T> >::release(); }
	void reset(typename unique_ptr::pointer ptr = AIR_CPP11_TYPENAME unique_ptr::pointer()) { std::unique_ptr<T, deleter<T> >::reset(ptr); }
	void reset(AIR_CPP11_NULLPTR p) { std::unique_ptr<T, deleter<T> >::reset(p); }

	typename unique_ptr::pointer operator->() const { return std::unique_ptr<T, deleter<T> >::operator ->(); }

	unique_ptr& operator=(unique_ptr&& r) { reset(r.release()); return *this; }
};
#endif

//! @brief STL aliases mapped to substance air allocators
#if defined(AIR_CPP11_SUPPORTED)
template<typename T>             using deque  = std::deque<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >;
template<typename K, typename V> using map    = std::map<K,V,std::less<K>,aligned_allocator<std::pair<const K, V>, AIR_DEFAULT_ALIGNMENT> >;
template<typename T>             using vector = std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >;
#else
template<typename T> class deque : public std::deque<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> > {};
template<typename K, typename V> class map : public std::map<K,V,std::less<K>,aligned_allocator<std::pair<const K, V>, AIR_DEFAULT_ALIGNMENT> > {};

template<typename T> class vector : public std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> > 
{
public:
	explicit vector() : std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >()
	{
	}
	template< class InputIt >
	vector(InputIt first, InputIt last)
		: std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >(first, last)
	{
	}
};
#endif

typedef std::basic_string<char, std::char_traits<char>, aligned_allocator<char, AIR_DEFAULT_ALIGNMENT> >			
	string;
typedef std::basic_stringstream<char, std::char_traits<char>, aligned_allocator<char, AIR_DEFAULT_ALIGNMENT> >	
	stringstream;

static inline string to_string(std::string& str)
{
	return string(str.c_str());
}

static inline string to_string(const std::string& str)
{
	return string(str.c_str());
}

static inline string to_string(const char* str)
{
	return string(str);
}

static inline std::string to_std_string(string& str)
{
	return std::string(str.c_str());
}

static inline std::string to_std_string(const string& str)
{
	return std::string(str.c_str());
}

}  // namespace SubstanceAir

#include "vector.h"

namespace SubstanceAir
{
	//! @brief containers used in the integration framework
	typedef vector<unsigned char> BinaryData;
	typedef vector<UInt> Uids;

	// Vectors
	typedef Math::Vector2<float> Vec2Float;
	typedef Math::Vector3<float> Vec3Float;
	typedef Math::Vector4<float> Vec4Float;
	typedef Math::Vector2<int> Vec2Int;
	typedef Math::Vector3<int> Vec3Int;
	typedef Math::Vector4<int> Vec4Int;
}

#endif //_SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H
