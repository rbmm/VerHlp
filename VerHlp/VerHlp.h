#pragma once

#if 1
#define DBG_OPT(x) _CRT_UNPARENTHESIZE(x)
#else
#define DBG_OPT(x)
#endif

class RsrcNode
{
	struct RsrcHeader 
	{
		WORD  wLength; 
		WORD  wValueLength; 
		WORD  wType; 
		WCHAR szKey[];
	};

	C_ASSERT(sizeof(RsrcHeader) == 6);

	RsrcNode* _first, *_next;
	PCWSTR _name;
	const void* _pvValue;
	ULONG _cbValue;
	WORD  _wValueLength; 
	WORD  _wType; 

	DBG_OPT((PCSTR _prefix)); // only for debug output

public:

	bool ParseResourse(PVOID buf, ULONG size, ULONG* pLength, PCSTR prefix);

	RsrcNode(DBG_OPT((PCSTR prefix = "")))
		: _next(0), _first(0) DBG_OPT((, _prefix(prefix)))
	{
	}

	~RsrcNode();

	bool IsStringValue() const
	{
		return _wType;
	}

	const void* getValue(ULONG& cb)
	{
		cb = _cbValue;
		return _pvValue;
	}

	void setValue(const void* pv, ULONG cb)
	{
		_pvValue = pv, _cbValue = cb;
		_wValueLength = (WORD)(_wType ? cb / sizeof(WCHAR) : cb);
	}

	RsrcNode* find(const PCWSTR strings[], ULONG n);

	ULONG GetSize() const;

	PVOID Store(PVOID buf, ULONG* pcb) const;
};
