#include <Windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include "FreerunVST.h"

#pragma warning(push)
#pragma warning(disable : 4996)
#include "public.sdk/source/main/pluginfactory.h"
#pragma warning(pop)

using namespace Steinberg;

typedef bool (PLUGIN_API* InitFunc)(void);
typedef IPluginFactory* (PLUGIN_API* GetFactoryFunc)(void);

HMODULE hmodule = NULL;
WCHAR sublibrary_name[MAX_PATH] = {};

SMTG_EXPORT_SYMBOL bool PLUGIN_API InitDll()
{
	LOG("InitDll called.\n");
	return true;
}

SMTG_EXPORT_SYMBOL bool PLUGIN_API ExitDll() {
	LOG("ExitDll called.\n");
	return true;
}

// Try looking for a link to the sublibrary if the sublibrary isn't found.
HMODULE load_linkto_sublibrary()
{
	LOG("load_linkto_sublibrary: Checking for a shortcut link named \"%ls.lnk\"...", sublibrary_name);
	WCHAR linkname[MAX_PATH] = {};
	memcpy(linkname, sublibrary_name, sizeof(linkname));
	WCHAR* ext = linkname + wcslen(linkname);
	if (ext + 4 > linkname + MAX_PATH) return NULL;
	*ext++ = L'.';
	*ext++ = L'l';
	*ext++ = L'n';
	*ext++ = L'k';
	*ext = L'\0';

	if (GetFileAttributesW(linkname) == INVALID_FILE_ATTRIBUTES)
	{
		LOG("load_linkto_sublibrary: No link file \"%ls\" found.", linkname);
		return NULL;
	}

	HMODULE hResult = NULL;
	bool needUninit = (CoInitialize(NULL) == S_OK);
	IShellLinkW* pShellLink = nullptr;

	if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink)) && pShellLink)
	{
		IPersistFile* pPersistFile = nullptr;
		if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile)) && pPersistFile)
		{
			if (SUCCEEDED(pPersistFile->Load(linkname, STGM_READ)))
			{
				WCHAR targetPath[MAX_PATH];
				WIN32_FIND_DATAW fd;
				if (SUCCEEDED(pShellLink->GetPath(targetPath, MAX_PATH, &fd, SLGP_UNCPRIORITY)) && targetPath[0] != L'\0')
					hResult = LoadLibraryW(targetPath);
			}
			else
			{
				LOG("load_linkto_sublibrary: IPersistFile::Load failed for \"%l\".\n", linkname);
			}
			pPersistFile->Release();
		}
		else
		{
			LOG("load_linkto_sublibrary: IPersistFile interface failure.\n");
		}
		pShellLink->Release();
	}
	else
	{
		LOG("load_linkto_sublibrary: CoCreateInstance failed.\n");
	}

	if (needUninit) CoUninitialize();
	return hResult;
}

HMODULE load_sublibrary()
{
	// The following check for *sublibrary_name does not prevent reentrancy; it is merely an
	// optimization.  We must therefore carefully ensure that threads simultaneously executing
	// this code never assign differing values to the same byte (even temporarily).
	if (!*sublibrary_name)
	{
		LOG("sublibrary_name called.\n");
		HMODULE hm = NULL;
		if (!GetModuleHandleEx(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)&load_sublibrary, &hm))
		{
			LOG("load_sublibrary: GetModuleHandleEx failed.\n");
			return NULL;
		}

		// Use a local buffer (thread safety)
		WCHAR path[MAX_PATH];
		if (!GetModuleFileNameW(hm, path, MAX_PATH))
		{
			LOG("load_sublibrary: GetModuleFileName failed.\n");
			return NULL;
		}
		path[MAX_PATH - 1] = 0; // This dodges a rare Windows XP bug.

		WCHAR* p = wcsrchr(path, '\\');
		if (p && *p) ++p; else p = path;
		constexpr size_t tw_prefix_len = sizeof(FILENAME_PREFIX) / sizeof(*FILENAME_PREFIX) - 1;
		if (_wcsnicmp(p, FILENAME_PREFIX, tw_prefix_len))
		{
			LOG("load_sublibrary: Freerun prefix not found in: %ls\n", path);
			return NULL;
		}
		const size_t len = p - path;
		p += tw_prefix_len;
		while (*p && (*p != L'-')) ++p;
		if (*p == L'-') ++p;
		while (*p == L' ') ++p;
		if (!*p)
		{
			LOG("load_sublibrary: Freerun prefix not followed by filename in: %ls\n", path);
			return NULL;
		}
		memcpy(sublibrary_name, path, len * sizeof(*path)); // don't copy the null-terminator (thread safety)
		wcscpy_s(&sublibrary_name[len], sizeof(sublibrary_name) / sizeof(*sublibrary_name) - len, p);
		LOG("load_sublibrary: Loading \"%ls\"...\n", sublibrary_name);
	}

	HMODULE hLib = LoadLibraryW(sublibrary_name);
	if (!hLib) hLib = load_linkto_sublibrary();

	return hLib;
}

SMTG_EXPORT_SYMBOL IPluginFactory* PLUGIN_API GetPluginFactory()
{
	LOG("GetPluginFactory called.\n");
	if (!hmodule)
		hmodule = load_sublibrary();
	if (!hmodule)
	{
		LOG("GetPluginFactory: LoadLibrary failed.\n");
		return NULL;
	}
	InitFunc initProc = (InitFunc)GetProcAddress(hmodule, "InitDll");
	if (initProc)
	{
		if (!initProc())
		{
			LOG("GetPluginFactory: Call to InitDll returned false.\n");
			return NULL;
		}
	}

	GetFactoryFunc getFactory = (GetFactoryFunc)GetProcAddress(hmodule, "GetPluginFactory");
	if (!getFactory)
	{
		LOG("GetPluginFactory: Could not find sub-library's GetPluginFactory.\n");
		return NULL;
	}
	IPluginFactory* factory = getFactory();
	if (!factory)
	{
		LOG("GetPluginFactory: Call to sub-library's GetPluginFactory returned null.\n");
		return NULL;
	}
	IPluginFactory* proxy_factory = new FreerunVST(factory);
	if (!proxy_factory)
	{
		LOG("GetPluginFactory: FreerunVST constructor returned null.\n");
		return NULL;
	}
#ifdef LOGGING
	int32 n = proxy_factory->countClasses();
	LOG("GetPluginFactory returned factory with %d classes.\n", n);
#endif

	// Decrease reference count for sub-library.  (This doesn't unload it because FreerunVST's constructor just increased the count.)
	FreeLibrary(hmodule);

	return proxy_factory;
}
