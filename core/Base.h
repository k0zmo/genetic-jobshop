#pragma once

#include "Prerequisites.h"
#include <stdexcept>

namespace core
{
	// Zamiana danych.
	void swapMemory(uint8* s1, uint8* s2, const uint32 size);
	// Odbija segment danych (pikseli) wzgledem osi X.
	void flipVertical(uint8* ptr, uint32 width, uint32 height, uint32 bpp);

	// Zamienia wszystkie znaki na male.
	void toLower(String& str);
	// Zamienia wszystkie znaki na wielkie.
	void toUpper(String& str);
	// Rozbija string na tokeny
	StringVector tokenize(const String& str, char delim, char group = 0);
	// Obcina z podanej sciezki nazwe pliku.
	String stripFileName(const String& path);
	// Obcina biale znaki.
	String& trim(String& str, bool left = true, bool right = true);
	// Zamienia znaki '\t' na spacje.
	void convertTabs(String& str, uint32 tabWidth);
	// Wyszukuje pierwszego wystepienia CIAGU w lancuchu.
	uint32 find(const String& str, const char* sz);

	// Konwersja String na bool.
	bool makeBool(const String& s);
	// Konwersja String na int.
	int makeInt(const String& s);
	// Konwersja String na uint32.
	uint32 makeUInt(const String& s);
	// Konwersja String na float.
	float makeFloat(const String& s, char _separator = '.');

	// Konwersja bool na String.
	String makeString(bool value);
	// Konwersja int na String.
	String makeString(int value);
	// Konwersja uint32 na String.
	String makeString(uint32 value);
	// Konwersja float na String.
	String makeString(float value, short prec = -1);

	// Real-Time Stamp Counter
	void getrdtsc(uint32* lo, uint32* hi);


	// Struktura pomocnicza do konwersji dowolnego typu na String.
	template<typename T>
	struct VarToStr_t
	{
		String operator()(const T& var){ return String("Typecast not allowed for this type"); }
	};

	// Struktura pomocnicza do konwersji String na dowolny typ.
	template<typename T>
	struct StrToVar_t
	{
		T operator()(const T& str){ return String("Typecast not allowed for this type"); }
	};

	template<>
	struct VarToStr_t<bool>
	{
		String operator()(const bool& var){ return makeString(var); }
	};

	template<>
	struct VarToStr_t<int>
	{
		String operator()(const int& var){ return makeString(var); }
	};

	template<>
	struct VarToStr_t<uint32>
	{
		String operator()(const uint32& var){ return makeString(var);	}
	};

	template<>
	struct VarToStr_t<float>
	{
		String operator()(const float& var){ return makeString(var);	}
	};

	template<>
	struct VarToStr_t<double>
	{
		String operator()(const double& var){ return makeString(static_cast<float>(var)); }
	};

	template<>
	struct StrToVar_t<bool>
	{
		bool operator()(const String& str){	return makeBool(str); }
	};

	template<>
	struct StrToVar_t<int>
	{
		int operator()(const String& str){ return makeInt(str); }
	};

	template<>
	struct StrToVar_t<uint32>
	{
		uint32 operator()(const String& str){ return makeUInt(str); }
	};

	template<>
	struct StrToVar_t<float>
	{
		float operator()(const String& str){ return makeFloat(str); }
	};

	template<>
	struct StrToVar_t<double>
	{
		double operator()(const String& str){ return static_cast<double>(makeFloat(str)); }
	};

	// Operator konwersji dowolnego typu na String.
	template<typename T>
	String VarToStr(const T& var)
	{
		return VarToStr_t<T>()(var);
	}

	// Operator konwersji String na dowolny typ.
	template<typename T>
	T StrToVar(const String& str)
	{
		return StrToVar_t<T>()(str);
	}

}
