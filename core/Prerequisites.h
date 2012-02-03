#pragma once

// clib
#include <cassert>
#include <cstring>

// STL
#include <string>
#include <vector>
#include <map>
#include <fstream>

#ifdef _MSC_VER
	// C4996: 'func':  This function or variable may be unsafe.
	#pragma warning(disable : 4996)
	// C4482: nonstandard extension used: enum 'enum' used in qualified name
	#pragma warning(disable : 4482)
	// L4099:
	#pragma warning(disable: 4099)
#endif

namespace core
{
// MinGW
#if defined(_WIN32) && defined(__GNUC__)
	typedef char int8;
	typedef short int16;
	typedef int int32;
	typedef long long int64;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned int uint32;
	typedef unsigned long long uint64;
// MSVC
#elif defined(_WIN32)
	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;
	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;
// Linux
#else
	typedef signed char int8;
	typedef short int int16;
	typedef int int32;
	#if __WORDSIZE == 64
		typedef long int int64;
	#else
		__extension__
		typedef long long int int64;
	#endif

	typedef unsigned char uint8;
	typedef unsigned short int	uint16;
	typedef unsigned int uint32;
	#if __WORDSIZE == 64
		typedef unsigned long int uint64;
	#else
		__extension__
		typedef unsigned long long int uint64;
	#endif
#endif

	typedef std::string String;
	typedef std::vector<String> StringVector;
	typedef std::map<String, String> AttributeMap;

	const uint32 npos = 0xfffffff;

	class ConfigFile;
	//class DataStream;
	//	class MemoryDataStream;
	//	class FileStreamDataStream;
	//	class FileHandleDataStream;
	class Exception;
	class Fraction;
	class Logger;
	class Random;
	class Serializer;
	template <typename T> class SharedPtr;
	template <typename T> class Singleton;
	class Timer;
	//class XmlParser;
	//class XmlHandler;
	//class XmlAttributes;

	// Czesc matematyczna
	class Matrix3;
	class Matrix4;
	class Quaternion;
	class Vector2;
	class Vector3;
	class Vector4;

	//// Graf
	//template <typename T> class Node;
	//template <typename T> class Edge;
	//template <typename T> class Graph;
	//// Pojemniki
	//template <typename T> class List;
	//template <typename T> class Vector;
	//template <typename T> class Stack;
	//template <typename T> class Tree;
}

