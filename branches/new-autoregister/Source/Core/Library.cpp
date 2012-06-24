#include "StdAfx.h"
#include "Library.h"

namespace Database
{
	Typed<HMODULE> library;

	namespace Loader
	{
		static void LibraryConfigure(unsigned int aId, const tinyxml2::XMLElement *element)
		{
			// library configuration
			const char *name = element->Attribute("name");
			HMODULE handle = LoadLibraryA(name);
			if (!handle)
			{
				DWORD error = GetLastError(); 
				LPTSTR buf[256];

				FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					error,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &buf,
					sizeof(buf), NULL );

				DebugPrint("error loading dynamic library \"%s\": %s\n", name, buf);
				return;
			}

			// add to the active library list
			DebugPrint("loaded dynamic library \"%s\"\n", name);
			library.Put(Hash(name), handle);
		}
		Configure libraryconfigure (0x90f6fbd0 /* "library" */, LibraryConfigure);
	}
}

void FreeLibraries()
{
	for (Database::Typed<HMODULE>::Iterator itor(&Database::library); itor.IsValid(); ++itor)
		FreeLibrary(itor.GetValue());
	Database::library.Clear();
}