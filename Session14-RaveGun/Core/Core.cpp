#include "Core.h"
#include "oglconsole.h"

// debug output
bool DEBUGPRINT_OUTPUTCONSOLE = false;
bool DEBUGPRINT_OUTPUTDEBUG = false;
bool DEBUGPRINT_OUTPUTSTDERR = false;

// console
extern OGLCONSOLE_Console console;

// debug output
int DebugPrint(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
#ifdef WIN32
	char buf[4096];
	int n = vsnprintf(buf, sizeof(buf), format, ap);
	if (DEBUGPRINT_OUTPUTDEBUG)
		OutputDebugStringA(buf);
	if (DEBUGPRINT_OUTPUTCONSOLE && console)
		OGLCONSOLE_Output(console, "%s", buf);
	if (DEBUGPRINT_OUTPUTSTDERR)
		fputs(buf, stderr);
#else
	int n = vfprintf(stderr, format, ap);
#endif
	va_end(ap);
	return n;
}

