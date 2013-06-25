#include <IO/IOServer.h>
#include <Data/Params.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <ConsoleApp.h>

//!!!tiyxml2 mustn't be linked!

#define TOOL_NAME	"CFLua"
#define VERSION		"1.0"

#define SUCCESS						0
#define ERR_COMPILATION_FAILED		1
#define ERR_IN_OUT_TYPES_DONT_MATCH 2
#define ERR_IN_NOT_FOUND			3
#define ERR_NOT_IMPLEMENTED_YET		4
#define ERR_INVALID_CMD_LINE		5

int		Verbose = VR_ERROR;

int		ExitApp(int Code, bool WaitKey);

bool	LuaCompile(char* pData, uint Size, LPCSTR Name, LPCSTR pFileOut);
bool	LuaCompileClass(Data::CParams& LoadedHRD, LPCSTR Name, LPCSTR pFileOut);
void	LuaRelease();
bool	ProcessSingleFile(const nString& InFileName, const nString& OutFileName, bool CheckFileType = true, bool IsClass = false);

API int Run(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	Verbose = Args.GetIntArg("-v");
	nString In = Args.GetStringArg("-in");
	nString Out = Args.GetStringArg("-out");

	if (In.IsEmpty() || Out.IsEmpty()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	Ptr<IO::CIOServer> IOServer = n_new(IO::CIOServer);

	nArray<nString> InList, OutList;
	In.Tokenize(";", InList);
	Out.Tokenize(";", OutList);

	if (InList.GetCount() != OutList.GetCount()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	for (int i = 0; i < InList.GetCount(); ++i)
	{
		In = InList[i];
		Out = OutList[i];

		n_msg(VR_INFO, "Compiling pair %d: '%s' -> '%s'\n", i, In.CStr(), Out.CStr());

		bool Dir = IOSrv->DirectoryExists(In);
		if (!Dir && IOSrv->DirectoryExists(Out)) return ExitApp(ERR_IN_OUT_TYPES_DONT_MATCH, WaitKey);

		if (Dir)
		{
			//PathExport + Name + ".cls"
			return ExitApp(ERR_NOT_IMPLEMENTED_YET, WaitKey);
		}
		else
		{
			if (!IOSrv->FileExists(In)) return ExitApp(ERR_IN_NOT_FOUND, WaitKey);
			if (!ProcessSingleFile(In, Out)) return ExitApp(ERR_COMPILATION_FAILED, WaitKey);
		}
	}

	return ExitApp(SUCCESS, WaitKey);
}
//---------------------------------------------------------------------

bool ProcessSingleFile(const nString& InFileName, const nString& OutFileName, bool CheckFileType, bool IsClass)
{
	nString Name = InFileName.ExtractFileName();
	Name.StripExtension();

	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(InFileName, Buffer))
	{
		FAIL;
	}

	Data::PParams ClassDesc;
	if (CheckFileType || IsClass)
	{
		Data::CHRDParser Parser;
		IsClass = Parser.ParseBuffer((LPCSTR)Buffer.GetPtr(), Buffer.GetSize(), ClassDesc);
		if (!CheckFileType && !IsClass)
		{
			n_msg(VR_ERROR, "Class HDR parsing failed\n", InFileName.CStr());
			FAIL;
		}
	}

	IOSrv->CreateDirectory(OutFileName.ExtractDirName());

	if (IsClass) return LuaCompileClass(*ClassDesc, Name.CStr(), OutFileName.CStr());
	else return LuaCompile((char*)Buffer.GetPtr(), Buffer.GetSize(), Name.CStr(), OutFileName.CStr());
}
//---------------------------------------------------------------------

int main(int argc, const char** argv)
{
	return Run(argc, argv);
}
//---------------------------------------------------------------------

int ExitApp(int Code, bool WaitKey)
{
	if (Code != SUCCESS) n_msg(VR_ERROR, TOOL_NAME" v"VERSION": Error occured with code %d\n", Code);

	LuaRelease();

	if (WaitKey)
	{
		n_printf("\nPress any key to exit...\n");
		getch();
	}

	return Code;
}
//---------------------------------------------------------------------
