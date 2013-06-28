#include "Main.h"

#include <IO/IOServer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void BatchToolInOut(CStrID Name, const nString& InStr, const nString& OutStr)
{
	//???insert sorted?
	nArray<nString>& InList = InFileLists.GetOrAdd(Name);
	InList.Append(InStr);
	nArray<nString>& OutList = OutFileLists.GetOrAdd(Name);
	OutList.Append(OutStr);
}
//---------------------------------------------------------------------

int RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine, LPCSTR pWorkingDir)
{
	n_msg(VL_DETAILS, "> %s %s\n", Name.CStr(), pCmdLine);

	nString Path = IOSrv->ManglePath("home:");
	Path += "\\..\\ContentForge\\";
	Path += Name.CStr();
	Path +=	".exe";
	Path.Replace('/', '\\');

	PROCESS_INFORMATION Info;
	RtlZeroMemory(&Info, sizeof(Info));

	STARTUPINFO SUI;
	RtlZeroMemory(&SUI, sizeof(SUI));
	SUI.cb = sizeof(SUI);
	SUI.dwFlags = STARTF_FORCEOFFFEEDBACK;
	if (CreateProcess(Path.CStr(), pCmdLine, NULL, NULL, FALSE, 0, NULL, pWorkingDir, &SUI, &Info) == FALSE) return -1;

	int Result = -1;
	WaitForSingleObject(Info.hProcess, INFINITE);

	GetExitCodeProcess(Info.hProcess, (DWORD*)&Result);

	CloseHandle(Info.hThread);
	CloseHandle(Info.hProcess);

	return Result;
}
//---------------------------------------------------------------------

int RunExternalToolBatch(CStrID Tool, int Verb, LPCSTR pWorkingDir)
{
	int Idx = InFileLists.FindIndex(Tool);
	if (Idx == INVALID_INDEX) return 0;
	nArray<nString>& InList = InFileLists.ValueAt(Idx);

	Idx = OutFileLists.FindIndex(Tool);
	if (Idx == INVALID_INDEX) return 0;
	nArray<nString>& OutList = OutFileLists.ValueAt(Idx);

	if (InList.GetCount() != OutList.GetCount()) return -1;
	if (InList.GetCount() == 0) return 0;

	for (int i = 0; i < InList.GetCount(); ++i)
	{
		InList[i] = IOSrv->ManglePath(InList[i]);
		OutList[i] = IOSrv->ManglePath(OutList[i]);
		//!!!GetRelativePath(Base) to reduce cmd line size!
		//!!!don't forget to pass BasePath or override working directory in that case!
		//!!!???use Curr/Overridden Working Directory as Base?!
	}

	nString InStr = InList[0], OutStr = OutList[0];

	for (int i = 1; i < InList.GetCount(); ++i)
	{
		DWORD NextLength = 32 + InStr.Length() + OutStr.Length() + InList[i].Length() + OutList[i].Length();
		if (NextLength >= MAX_CMDLINE_CHARS)
		{
			if (InStr.FindCharIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
			if (OutStr.FindCharIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

			char CmdLine[MAX_CMDLINE_CHARS];
			sprintf_s(CmdLine, "-v %d -in %s -out %s", Verb, InStr.CStr(), OutStr.CStr());
			int ExitCode = RunExternalToolAsProcess(Tool, CmdLine, pWorkingDir);
			if (ExitCode != 0) return ExitCode;

			InStr = InList[i];
			OutStr = OutList[i];
		}
		else
		{
			InStr.Append(';');
			InStr += InList[i];
			OutStr.Append(';');
			OutStr += OutList[i];
		}
	}

	if (InStr.FindCharIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
	if (OutStr.FindCharIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

	char CmdLine[MAX_CMDLINE_CHARS];
	sprintf_s(CmdLine, "-v %d -in %s -out %s", Verb, InStr.CStr(), OutStr.CStr());
	return RunExternalToolAsProcess(Tool, CmdLine, pWorkingDir);
}
//---------------------------------------------------------------------
