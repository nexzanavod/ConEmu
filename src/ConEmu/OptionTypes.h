﻿
/*
Copyright (c) 2016 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#pragma once

#include <windows.h>
#include "../common/Common.h"
#include "../common/kl_parts.h"
#include "../common/CEStr.h"
#include "../common/MArray.h"


#if 0
enum CEOptionType
{
	eot_Undefined = 0, // CEOption - must not be
	eot_Simple,
	eot_Array,
	eot_Composite,
};

enum CEOptionDataType
{
	eodt_Undefined = 0, // must be defined!
	eodt_Bool,    // Simple `bool`
	eodt_String,  // has MaxLength template property
	eodt_Byte,    // Simple 'u8' ('BYTE'), may be subclassed as `enum`?
	eodt_Int,     // Simple 'i32' ('int'), stored as decimal signed integer
	eodt_UInt,    // Simple 'u32' ('UINT'), stored as decimal unsigned integer
	eodt_DWORD,   // Simple 'DWORD', stored as hexadecimal unsigned integer
};
#endif



// This does not ‘fixate’ the value, only check it
template <typename T, const T val_min, const T val_max>
bool validate_within(T& value)
{
	if ((value >= val_min) && (value <= val_max))
		return true;
	return false;
};

// Make value ‘not greater than’
template <typename T, const T val_max>
bool validate_max(T& value)
{
	if (value > val_max)
		value = val_max;
	return true;
};

// Make value ‘in range’
template <typename T, const T val_min, const T val_max>
bool validate_minmax(T& value)
{
	if (value < val_min)
		value = val_min;
	else if (value > val_max)
		value = val_max;
	return true;
};


class CEOptionBase;

class CEOptionList
{
public:
	MArray<CEOptionBase*> m_List;
public:
	CEOptionList() {};
	~CEOptionList() {};
	void Register(CEOptionBase* pOption)
	{
		_ASSERTE(this!=NULL);
		m_List.push_back(pOption);
	};
};

extern CEOptionList* gpOptionList;


// For arrays declarations
template <typename T, int MaxSize>
struct CEOptionArrayType
{
	const int max_size = MaxSize;
	typedef T type[MaxSize];
};


// Prototype
class CEOptionBase
{
protected:
	CEStr ms_Name;
public:
	CEOptionBase(LPCWSTR OptionName)
		: ms_Name(OptionName)
	{
		// #pragma warning(disable: 4355)
		if (gpOptionList)
			gpOptionList->Register(this);
		// #pragma warning(default: 4355)
	};
	virtual ~CEOptionBase()
	{
	};

	virtual bool Load(SettingsBase* reg) = 0;
	virtual bool Save(SettingsBase* reg) const = 0;
	virtual void Reset() = 0;

	virtual LPCWSTR Name()
	{
		return ms_Name;
	};
};



// Base class
template <typename T>
class CEOptionBaseImpl : public CEOptionBase
{
public:
	T m_Value;
	const T m_Default;
protected:
	bool (*validate_fn)(T& value);
public:
	CEOptionBaseImpl(LPCWSTR OptionName, const T& Default = T(), bool (*_validate_fn)(T& value) = NULL)
		: CEOptionBase(OptionName)
		, m_Value()
		, m_Default(Default)
		, ms_Name(OptionName)
		, validate_fn(_validate_fn)
	{
		Reset();
	};

	virtual ~CEOptionBaseImpl()
	{
	};

	virtual bool Load(SettingsBase* reg) override
	{
		if (!reg)
			return false;
		T value;
		if (!reg->Load(ms_Name, &value))
			return false;
		return Set(value);
	};

	virtual bool Save(SettingsBase* reg) const override
	{
		if (!reg)
			return false;
		// #OPT_TODO Serializators doesn't return error codes
		reg->Save(ms_Name, m_Value);
		return true;
	};

	operator const T&() const
	{
		return m_Value;
	};

	virtual bool Set(T RVAL_REF value)
	{
		if (validate_fn)
		{
			if (!validate_fn(value))
				return false;
		}
		m_Value = value;
		return true;
	};

	virtual void Reset() override
	{
		m_Value = m_Default;
	}

};




// Simple 'wchar_t[NN]' property
template <typename T, int MaxSize>
class CEOptionArray : public CEOptionBase
{
public:
	typedef typename CEOptionArrayType<T,MaxSize>::type arr_type;
	arr_type m_Value;
protected:
	LPCWSTR (*get_name)(int index);
	void (*set_default)(arr_type& value);
public:
	CEOptionArray(LPCWSTR OptionName, LPCWSTR (*_get_name)(int index) = NULL, void (*_set_default)(arr_type& value) = NULL)
		: CEOptionBase(OptionName)
		, get_name(_get_name)
		, set_default(_set_default)
	{
		_ASSERTE(MaxSize > 0);
		Reset();
	};

	virtual ~CEOptionArray()
	{
	};

	virtual bool Load(SettingsBase* reg) override
	{
		if (!reg || !get_name)
			return false;
		bool bAllOk = true;
		for (int i = 0; i < MaxSize; i++)
		{
			LPCWSTR pszName = get_name(i);
			if (!reg->Load(pszName, m_Value[i]))
				bAllOk = false;
		}
		return bAllOk;
	};

	virtual bool Save(SettingsBase* reg) const override
	{
		if (!reg || !get_name)
			return false;
		bool bAllOk = true;
		for (int i = 0; i < MaxSize; i++)
		{
			LPCWSTR pszName = get_name(i);
			// #OPT_TODO Serializators doesn't return error codes
			reg->Save(pszName, m_Value[i]);
			// if (was error) bAllOk = false;
		}
		return bAllOk;
	};

	virtual bool Set(arr_type RVAL_REF value)
	{
		memmove(m_Value, value, sizeof(m_Value));
		return true;
	};

	virtual void Reset() override
	{
		if (set_default)
			set_default(m_Value);
		else
			memset(m_Value, 0, sizeof(m_Value));
	};

	const T & operator[](int index) const
	{
		_ASSERTE(index>=0 && index<MaxSize);
		return m_Value[index];
	};
	T & operator[](int index)
	{
		_ASSERTE(index>=0 && index<MaxSize);
		return m_Value[index];
	};
};




// Simple `bool`
class CEOptionBool : public CEOptionBaseImpl<bool>
{
public:
	CEOptionBool(LPCWSTR OptionName, const bool Default = false, bool (*_validate_fn)(bool& value) = NULL)
		: CEOptionBaseImpl(OptionName, Default, _validate_fn)
	{
	};

	virtual ~CEOptionBool()
	{
	};
};

// Simple 'wchar_t*' property
class CEOptionString : public CEOptionBase
{
public:
	wchar_t* m_Value;
	CEStr m_Default;
public:
	CEOptionString(LPCWSTR OptionName, LPCWSTR Default = NULL)
		: CEOptionBase(OptionName)
		, m_Value()
		, m_Default(Default)
	{
		Reset();
	};

	virtual ~CEOptionString()
	{
		SafeFree(m_Value);
	};

	virtual bool Load(SettingsBase* reg) override
	{
		if (!reg)
			return false;
		wchar_t* value = NULL;
		if (!reg->Load(ms_Name, &value))
			return false;
		return Set(STD_MOVE(value));
	};

	virtual bool Save(SettingsBase* reg) const override
	{
		if (!reg)
			return false;
		// #OPT_TODO Serializators doesn't return error codes
		reg->Save(ms_Name, m_Value);
		return true;
	};

	// It ‘captures’ the `value`. Sort of `std::move` from C++11.
	virtual bool Set(wchar_t* RVAL_REF value)
	{
		if (m_Value != value)
		{
			if (!value)
				value = lstrdup(L"");
			LPVOID p = InterlockedExchangePointer((PVOID*)&m_Value, value);
			SafeFree(p);
			value = NULL; // capturing
		}
		return true;
	};

	virtual void Reset() override
	{
		if (m_Value && m_Default.ms_Val)
		{
			if (wcscmp(m_Value, m_Default.ms_Val) == 0)
				return;
		}
		SafeFree(m_Value);
		Set(lstrdup(m_Default));
		_ASSERTE(m_Value != NULL);
	};
};


// #NEW_OPTIONS  finished here

// Simple 'wchar_t[NN]' property
template <LPCWSTR OptionName, int MaxSize, LPCWSTR Default = L"">
class CEOptionStringFixed : public CEOptionBaseImpl<typename CEOptionArrayType<wchar_t,MaxSize>::type, OptionName>
{
public:
	CEOptionStringFixed()
	{
		_ASSERTE(MaxSize > 0);
		Reset();
	};

	virtual ~CEOptionStringFixed()
	{
	};

	virtual bool Load(SettingsBase* reg) override
	{
		if (!reg)
			return false;
		if (!reg->Load(ms_Name, m_Value, MaxSize))
			return false;
		return true;
	};

	virtual bool Set(LPCWSTR value)
	{
		_ASSERTE(validate_fn==NULL);
		lstrcpyn(m_Value, value, MaxSize);
		return true;
	};

	virtual bool Set(typename CEOptionArrayType<wchar_t,MaxSize>::type RVAL_REF value)
	{
		return Set((LPCWSTR)value);
	};

	virtual void Reset() override
	{
		Set(Default);
	};
};



// #OPT_TODO Replace with "CEOptionStringArray"
template <LPCWSTR OptionName, LPCWSTR Default = NULL>
class CEOptionStringMSZ : public CEOptionBaseImpl<wchar_t*, OptionName>
{
};

// #OPT_TODO Required for "DefaultTerminalApps"
template <LPCWSTR OptionName, LPCWSTR Default = NULL>
class CEOptionStringDelim : public CEOptionBaseImpl<wchar_t*, OptionName>
{
};


// Simple 'u8' ('BYTE')
template <u8 Default = 0, bool (*validate_fn)(u8& value)=NULL>
class CEOptionByte : public CEOptionBaseImpl<u8, validate_fn>
{
public:
	CEOptionByte(LPCWSTR OptionName)
		: CEOptionBaseImpl(OptionName)
	{
		Reset();
	};

	virtual ~CEOptionByte()
	{
	};

	virtual void Reset() override
	{
		Set(Default);
	};
};

// Enum, stored as simple 'u8' ('BYTE')
template <typename T, const T Default = (const T)0, bool (*validate_fn)(T& value)=NULL>
class CEOptionByteEnum : public CEOptionBaseImpl<T, validate_fn>
{
public:
	CEOptionByteEnum(LPCWSTR OptionName)
		: CEOptionBaseImpl(OptionName)
	{
		Reset();
	};

	virtual ~CEOptionByteEnum()
	{
	};

	virtual bool Load(SettingsBase* reg) override
	{
		if (!reg)
			return false;
		u8 value;
		if (!reg->Load(ms_Name, &value))
			return false;
		return Set(value);
	};

	virtual bool Save(SettingsBase* reg) const override
	{
		if (!reg)
			return false;
		// #OPT_TODO Serializators doesn't return error codes
		reg->Save(ms_Name, (u8)m_Value);
		return true;
	};

	virtual void Reset() override
	{
		Set(Default);
	};
};

// Simple 'i32' ('int'), stored as decimal signed integer
template <i32 Default = 0, bool (*validate_fn)(i32& value)=NULL>
class CEOptionInt : public CEOptionBaseImpl<i32, validate_fn>
{
public:
	CEOptionInt(LPCWSTR OptionName)
		: CEOptionBaseImpl(OptionName)
	{
		Reset();
	};

	virtual ~CEOptionInt()
	{
	};

	virtual void Reset() override
	{
		Set(Default);
	};
};

// Simple 'u32' ('UINT'), stored as decimal unsigned integer
template <u32 Default = 0, u32 (*get_default)()=NULL, bool (*validate_fn)(u32& value)=NULL>
class CEOptionUInt : public CEOptionBaseImpl<u32, validate_fn>
{
public:
	CEOptionUInt(LPCWSTR OptionName)
		: CEOptionBaseImpl(OptionName)
	{
		Reset();
	};

	virtual ~CEOptionUInt()
	{
	};

	virtual void Reset() override
	{
		Set(get_default ? get_default() : Default);
	};
};

// Simple 'DWORD', stored as hexadecimal unsigned integer
template <DWORD Default = 0, bool (*validate_fn)(DWORD& value)=NULL>
class CEOptionDWORD : public CEOptionBaseImpl<DWORD, validate_fn>
{
public:
	CEOptionDWORD(LPCWSTR OptionName)
		: CEOptionBaseImpl(OptionName)
	{
		Reset();
	};

	virtual ~CEOptionDWORD()
	{
	};

	virtual void Reset() override
	{
		Set(Default);
	};
};


// CESize
// #OPT_TODO CESize - add to inherited classes?
template <DWORD Default = 0, bool (*validate_fn)(DWORD& value)=NULL>
class CEOptionSize : public CEOptionBaseImpl<DWORD, validate_fn>
{
public:
	CEOptionSize(LPCWSTR OptionName)
		: CEOptionBaseImpl(OptionName)
	{
	};

	virtual ~CEOptionSize()
	{
	};
};



// ComSpec
// class CEOptionComSpec : public CEOptionBase, public ConEmuComspec
#include "OptTypeComSpec.h"