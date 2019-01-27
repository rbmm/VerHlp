#include "stdafx.h"

#include "VerHlp.h"

BOOL UpdateVersion(PVOID pvVersion, ULONG cbVersion, PVOID& pvNewVersion, ULONG& cbNewVersion)
{
	BOOL fOk = FALSE;

	char prefix[16];
	memset(prefix, '\t', sizeof(prefix));
	prefix[RTL_NUMBER_OF(prefix) - 1] = 0;
	*prefix = 0;

	if (RsrcNode* node = new RsrcNode)
	{
		if (node->ParseResourse(pvVersion, cbVersion, &cbVersion, prefix + RTL_NUMBER_OF(prefix) - 1))
		{
			static const PCWSTR str[] = {
				L"VS_VERSION_INFO", L"StringFileInfo", 0, L"CompanyName"
			};

			if (RsrcNode *p = node->find(str, RTL_NUMBER_OF(str)))
			{
				if (p->IsStringValue())
				{
					ULONG cb;
					const void* pvCompanyName = p->getValue(cb);
					DbgPrint("CompanyName: %S\n", pvCompanyName);

					static WCHAR CompanyName[] = L"[ New Company Name ]";

					if (cb != sizeof(CompanyName) || 
						memcmp(pvCompanyName, CompanyName, sizeof(CompanyName)))
					{
						p->setValue(CompanyName, sizeof(CompanyName));

						cbVersion = node->GetSize();

						if (pvVersion = LocalAlloc(0, cbVersion))
						{
							node->Store(pvVersion, &cbNewVersion);
							pvNewVersion = pvVersion;
							fOk = TRUE;
						}
					}
				}
			}
		}
		delete node;
	}

	return fOk;
}

struct EnumVerData 
{
	HANDLE hUpdate;
	BOOL fDiscard;
};

BOOL CALLBACK EnumResLangProc(HMODULE hModule,
							  PCWSTR lpszType,
							  PCWSTR lpszName,
							  WORD wIDLanguage,
							  EnumVerData* Ctx
							  )
{
	if (HRSRC hResInfo = FindResourceExW(hModule, lpszType, lpszName, wIDLanguage))
	{
		if (HGLOBAL hg = LoadResource(hModule, hResInfo))
		{
			if (ULONG size = SizeofResource(hModule, hResInfo))
			{
				if (PVOID pv = LockResource(hg))
				{
					if (UpdateVersion(pv, size, pv, size))
					{
						if (UpdateResource(Ctx->hUpdate, lpszType, lpszName, wIDLanguage, pv, size))
						{
							Ctx->fDiscard = FALSE;
						}

						LocalFree(pv);
					}
				}
			}
		}
	}

	return TRUE;
}

ULONG UpdateVersion(PCWSTR FileName)
{
	ULONG dwError = NOERROR;

	EnumVerData ctx;

	if (ctx.hUpdate = BeginUpdateResource(FileName, FALSE))
	{
		ctx.fDiscard = TRUE;

		if (HMODULE hmod = LoadLibraryExW(FileName, 0, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE))
		{
			if (!EnumResourceLanguages(hmod, RT_VERSION, 
				MAKEINTRESOURCE(VS_VERSION_INFO), 
				(ENUMRESLANGPROCW)EnumResLangProc, (LONG_PTR)&ctx))
			{
				dwError = GetLastError();
			}

			FreeLibrary(hmod);
		}
		else
		{
			dwError = GetLastError();
		}

		if (!dwError && !EndUpdateResourceW(ctx.hUpdate, ctx.fDiscard))
		{
			dwError = GetLastError();
		}
	}
	else
	{
		dwError = GetLastError();
	}

	return dwError;
}


