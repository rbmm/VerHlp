#include "stdafx.h"

#include "VerHlp.h"

bool RsrcNode::ParseResourse(PVOID buf, ULONG size, ULONG* pLength, PCSTR prefix)
{
	union {
		PVOID pv;
		RsrcHeader* ph;
		ULONG_PTR up;
		PCWSTR sz;
	};

	pv = buf;

	if (size < sizeof(RsrcHeader) || (up & 3))
	{
		return false;
	}

	WORD wType = ph->wType;
	ULONG wValueLength = ph->wValueLength, wLength = ph->wLength;
	ULONG cbValue = 0;

	switch (wType)
	{
	case 1:
		cbValue = wValueLength * sizeof(WCHAR);
		break;
	case 0:
		cbValue = wValueLength;
		break;
	default:
		return false;
	}

	*pLength = wLength;

	if (wLength > size || wLength < sizeof(RsrcHeader) || cbValue >= (wLength -= sizeof(RsrcHeader)))
	{
		return false;
	}

	wLength -= cbValue;

	sz = ph->szKey, _name = sz;

	do 
	{
		if (wLength < sizeof(WCHAR))
		{
			return false;
		}

		wLength -= sizeof(WCHAR);

	} while (*sz++);

	DbgPrint("%s%S {\n", prefix, _name);

	if (up & 3)
	{
		if (wLength < 2)
		{
			return false;
		}
		up += 2, wLength -= 2;
	}

	_wType = wType, _wValueLength = (WORD)wValueLength, _cbValue = cbValue, _pvValue = pv;

	if (wValueLength && wType)
	{
		if (sz[wValueLength - 1])
		{
			return false;
		}
		DbgPrint("%s\t%S\n", prefix, sz);
	}

	if (wLength)
	{
		if (!*--prefix) return false;

		up += wValueLength;

		do 
		{
			if (up & 3)
			{
				if (wLength < 2)
				{
					return false;
				}

				up += 2;

				if (!(wLength -= 2))
				{
					break;
				}
			}

			if (RsrcNode* node = new RsrcNode(
				DBG_OPT((prefix))
				))
			{
				node->_next = _first, _first = node;

				if (node->ParseResourse(ph, wLength, &size, prefix))
				{
					continue;
				}
			}

			return false;

		} while (up += size, wLength -= size);

		prefix++;
	}

	DbgPrint("%s}\n", prefix);

	return true;
}

RsrcNode::~RsrcNode()
{
	if (RsrcNode* next = _first)
	{
		do 
		{
			RsrcNode* cur = next;
			next = next->_next;
			delete cur;
		} while (next);
	}

	DBG_OPT((DbgPrint("%s%S\n", _prefix, _name)));
}

RsrcNode* RsrcNode::find(const PCWSTR strings[], ULONG n)
{
	PCWSTR str = *strings++;

	if (!str || !wcscmp(str, _name))
	{
		if (!--n)
		{
			return this;
		}

		if (RsrcNode* next = _first)
		{
			do 
			{
				if (RsrcNode* p = next->find(strings, n))
				{
					return p;
				}
			} while (next = next->_next);
		}
	}

	return 0;
}

ULONG RsrcNode::GetSize() const
{
	ULONG size = sizeof(RsrcHeader) + (1 + (ULONG)wcslen(_name)) * sizeof(WCHAR);

	if (_cbValue)
	{
		size = ((size + 3) & ~3) + _cbValue;
	}

	if (RsrcNode* next = _first)
	{
		do 
		{
			size = ((size + 3) & ~3) + next->GetSize();
		} while (next = next->_next);
	}

	return size;
}

PVOID RsrcNode::Store(PVOID buf, ULONG* pcb) const
{
	union {
		RsrcHeader* ph;
		ULONG_PTR up;
		PVOID pv;
	};

	pv = buf;

	ph->wType = _wType;
	ph->wValueLength = _wValueLength;

	ULONG size = (1 + (ULONG)wcslen(_name)) * sizeof(WCHAR), cb;

	memcpy(ph->szKey, _name, size);

	up += (size += sizeof(RsrcHeader));

	if (_cbValue)
	{
		up = (up + 3) & ~3;
		memcpy(pv, _pvValue, _cbValue);
		up += _cbValue;
		size = ((size + 3) & ~3) + _cbValue;
	}

	if (RsrcNode* next = _first)
	{
		do 
		{
			up = (up + 3) & ~3;
			pv = next->Store(pv, &cb);
			size = ((size + 3) & ~3) + cb;
		} while (next = next->_next);
	}

	reinterpret_cast<RsrcHeader*>(buf)->wLength = (WORD)size;

	*pcb = size;

	return pv;
}
