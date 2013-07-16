#include <Core/Logger.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void n_barf(const char* exp, const char* file, int line)
{
	n_error("*** NEBULA ASSERTION ***\nexpression: %s\nfile: %s\nline: %d\n", exp, file, line);
}
//---------------------------------------------------------------------

void n_barf2(const char* exp, const char* pMsg, const char* file, int line)
{
	n_error("*** NEBULA ASSERTION ***\nprogrammer says: %s\nexpression: %s\nfile: %s\nline: %d\n", pMsg, exp, file, line);
}
//---------------------------------------------------------------------

// Critical error, program will be closed
void __cdecl n_error(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList, pMsg);
	if (CoreLoggerExists) CoreLogger->Error(pMsg, ArgList);
	else
	{
		vprintf(pMsg, ArgList);
		fflush(stdout);
	}
	va_end(ArgList);
	abort();
}
//---------------------------------------------------------------------

// Message that will be shown to the user
void __cdecl n_message(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList, pMsg);
	if (CoreLoggerExists) CoreLogger->Message(pMsg, ArgList);
	else vprintf(pMsg, ArgList);
	va_end(ArgList);
}
//---------------------------------------------------------------------

// Logable version of printf
void __cdecl n_printf(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList,pMsg);
	if (CoreLoggerExists) CoreLogger->Print(pMsg, ArgList);
	else vprintf(pMsg, ArgList);
	va_end(ArgList);
}
//---------------------------------------------------------------------

// Message printed to the debug output window
void __cdecl n_dbgout(const char* pMsg, ...)
{
	va_list ArgList;
	va_start(ArgList,pMsg);
	if (CoreLoggerExists) CoreLogger->OutputDebug(pMsg, ArgList);
	else
	{
		vprintf(pMsg, ArgList);
		fflush(stdout);
	}
	va_end(ArgList);
}
//---------------------------------------------------------------------

void n_sleep(double sec)
{
	Sleep((int)(sec * 1000.0));
}
//---------------------------------------------------------------------

// A strdup() implementation using Nebula's malloc() override.
char* n_strdup(const char* from)
{
	n_assert(from);
	char* to = (char*)n_malloc(strlen(from) + 1);
	if (to) strcpy(to, from);
	return to;
}
//---------------------------------------------------------------------

// A safe strncpy() implementation.
char* n_strncpy2(char* dest, const char* src, size_t size)
{
	strncpy(dest, src, size);
	dest[size - 1] = 0;
	return dest;
}
//---------------------------------------------------------------------

// A safe strcat implementation.
void n_strcat(char* dest, const char* src, size_t dest_size)
{
	unsigned int l = strlen(dest) + strlen(src) + 1;
	n_assert(l < dest_size);
	strcat(dest, src);
}
//---------------------------------------------------------------------

// A string matching function using Tcl's matching rules.
bool n_strmatch(const char* str, const char* pat)
{
    char c2;

    while (true)
    {
        if (!*pat) return !*str;
        if (!*str && *pat != '*') return false;
        if (*pat == '*')
        {
            ++pat;
            if (!*pat) return true;
            while (true)
            {
                if (n_strmatch(str, pat)) return true;
                if (!*str) return false;
                ++str;
            }
        }
        if (*pat == '?') goto match;
        if (*pat == '[')
        {
            ++pat;
            while (true)
            {
                if (*pat == ']' || !*pat) return false;
                if (*pat == *str) break;
                if (pat[1] == '-')
                {
                    c2 = pat[2];
                    if (!c2) return false;
                    if (*pat <= *str && c2 >= *str) break;
                    if (*pat >= *str && c2 <= *str) break;
                    pat += 2;
                }
                ++pat;
            }
            while (*pat != ']')
            {
                if (!*pat)
                {
                    --pat;
                    break;
                }
                ++pat;
            }
            goto match;
        }

        if (*pat == '\\')
        {
            ++pat;
            if (!*pat) return false;
        }
        if (*pat != *str) return false;

match:
        ++pat;
        ++str;
    }
}
//---------------------------------------------------------------------
