#include <IO/IOServer.h>
#include <IO/FS/NpkTOC.h>
#include <IO/FSBrowser.h>
#include <IO/Streams/FileStream.h>
#include <Data/Buffer.h>
#include <ConsoleApp.h>

void PrintNpkTOCEntry(IO::CNpkTOCEntry& Entry, int Level)
{
	CString Str;
	for (int i = 0; i < Level; ++i) Str.Add("  "); //Str.Add("| ");
	Str.Add((Entry.GetType() == IO::FSE_DIR) ? "+ " : "  ");
	Str += Entry.GetName();

	n_printf(Str.CStr());

	if (Entry.GetType() == IO::FSE_DIR)
	{
		n_printf("\n");

		IO::CNpkTOCEntry::CIterator ItSubEntry = Entry.GetEntryIterator();
		while (ItSubEntry)
		{
			if ((*ItSubEntry)->GetType() == IO::FSE_DIR)
				PrintNpkTOCEntry(**ItSubEntry, Level + 1);
			++ItSubEntry;
		}

		ItSubEntry = Entry.GetEntryIterator();
		while (ItSubEntry)
		{
			if ((*ItSubEntry)->GetType() == IO::FSE_FILE)
				PrintNpkTOCEntry(**ItSubEntry, Level + 1);
			++ItSubEntry;
		}
	}
	else n_printf(" (%d B)\n", Entry.GetFileLength());
}
//---------------------------------------------------------------------

bool AddDirectoryToTOC(CString DirName, IO::CNpkTOC& TOC) //, int& Offset)
{
	bool Result = true;

	DirName.StripTrailingSlash();
	DirName.ToLower();

	if (DirName == ".svn") OK;

	IO::CNpkTOCEntry* pNPKDir = TOC.BeginDirEntry(DirName.CStr());

	IO::CFSBrowser Browser;
	CString FullDirName = pNPKDir->GetFullName() + "/";
	if (Browser.SetAbsolutePath(FullDirName))
	{
		if (!Browser.IsCurrDirEmpty()) do
		{
			if (Browser.IsCurrEntryFile())
			{
				CString FilePart = Browser.GetCurrEntryName();
				CString FullFilePath = FullDirName + Browser.GetCurrEntryName();
				FilePart.ToLower();

				IO::CFileStream File;
				if (File.Open(FullFilePath, IO::SAM_READ))
				{
					int FileLength = File.GetSize();
					File.Close();
					TOC.AddFileEntry(FilePart.CStr(), 0, FileLength); //Offset, FileLength);
					//Offset += FileLength;
				}
				else n_msg(VL_ERROR, "Error reading file %s\n", FullFilePath.CStr());
			}
			else if (Browser.IsCurrEntryDir())
			{
				if (!AddDirectoryToTOC(Browser.GetCurrEntryName(), TOC)) //, Offset))
				{
					Result = false;
					break;
				}
			}
		}
		while (Browser.NextCurrDirEntry());
	}
	else
	{
		n_msg(VL_ERROR, "Could not open directory '%s' for reading!\n", FullDirName.CStr());
		Result = false;
	}

	TOC.EndDirEntry();

	return Result;
}
//---------------------------------------------------------------------

void WriteTOCEntry(IO::CFileStream& File, IO::CNpkTOCEntry* pTOCEntry, int& Offset)
{
	n_assert(pTOCEntry);

	if (pTOCEntry->GetType() == IO::FSE_DIR)
	{
		File.Put<int>('DIR_');
		File.Put<int>(sizeof(short) + pTOCEntry->GetName().Length());
		File.Put<ushort>((ushort)pTOCEntry->GetName().Length());
		File.Write(pTOCEntry->GetName(), pTOCEntry->GetName().Length());

		IO::CNpkTOCEntry::CIterator ItSubEntry = pTOCEntry->GetEntryIterator();
		while (ItSubEntry)
		{
			WriteTOCEntry(File, *ItSubEntry, Offset);
			++ItSubEntry;
		}

		File.Put<int>('DEND');
		File.Put<int>(0);
	}
	else if (pTOCEntry->GetType() == IO::FSE_FILE)
	{
		File.Put<int>('FILE');
		File.Put<int>(2 * sizeof(int) + sizeof(short) + pTOCEntry->GetName().Length());

		// CHashTable iterator doesn't keep insertion order due to hash table nature, so offset can't
		// be calculated until all the TOC is formed. Now we can calculate real offset, which is
		// order-dependent, but TOC entries take only R/O access to the offset stored. So we ignore this
		// field at all and calculate real offset here.
		//File.Put<int>(pTOCEntry->GetFileOffset());
		File.Put<int>(Offset);
		Offset += pTOCEntry->GetFileLength();

		File.Put<int>(pTOCEntry->GetFileLength());
		File.Put<ushort>((ushort)pTOCEntry->GetName().Length());
		File.Write(pTOCEntry->GetName(), pTOCEntry->GetName().Length());
	}
}
//---------------------------------------------------------------------

bool WriteEntryData(IO::CFileStream& File, IO::CNpkTOCEntry* pTOCEntry, int DataOffset, int& DataSize)
{
	n_assert(pTOCEntry);

	if (pTOCEntry->GetType() == IO::FSE_DIR)
	{
		IO::CNpkTOCEntry::CIterator ItSubEntry = pTOCEntry->GetEntryIterator();
		while (ItSubEntry)
		{
			if (!WriteEntryData(File, *ItSubEntry, DataOffset, DataSize)) FAIL;
			++ItSubEntry;
		}
	}
	else if (pTOCEntry->GetType() == IO::FSE_FILE)
	{
		// It was a very good assert, but now it can't be used since GetFileOffset() always returns 0
		//n_assert(File.GetPosition() == (DataOffset + pTOCEntry->GetFileOffset()));

		CString FullFileName = pTOCEntry->GetFullName();

		//???write streamed file copying?
		Data::CBuffer Buffer;
		if (IOSrv->LoadFileToBuffer(FullFileName, Buffer))
		{
			if (File.Write(Buffer.GetPtr(), Buffer.GetSize()) != pTOCEntry->GetFileLength())
			{
				n_printf("Error writing %s to NPK!\n", FullFileName.CStr());
				FAIL;
			}
			DataSize += pTOCEntry->GetFileLength();
		}
		else
		{
			n_msg(VL_ERROR, "Error reading file %s\n", FullFileName.CStr());
			FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool PackFiles(const CArray<CString>& FilesToPack, const CString& PkgFileName, const CString& PkgRoot, CString PkgRootDir)
{
	// Create TOC

	n_msg(VL_INFO, "Creating NPK TOC...\n");

	CString RootPath = IOSrv->ManglePath(PkgRoot);
	RootPath.ToLower();

	PkgRootDir.ToLower();

	CString RootDirPath = RootPath + "/" + PkgRootDir;

	int i = 0;
	for (; i < FilesToPack.GetCount(); ++i)
	{
		const CString& FileName = FilesToPack[i];
		if (FileName.Length() <= RootDirPath.Length() + 1) continue;
		if (strncmp(RootDirPath.CStr(), FileName.CStr(), RootDirPath.Length()) >= 0) break;
		n_msg(VL_WARNING, "File is out of the export package scope:\n - %s\n", FileName.CStr());
	}

	CArray<CString> DirStack;

	//int Offset = 0;

	IO::CNpkTOC TOC;
	TOC.SetRootPath(RootPath.CStr());
	TOC.BeginDirEntry(PkgRootDir.CStr());

	for (; i < FilesToPack.GetCount(); ++i)
	{
		const CString& FileName = FilesToPack[i];
		if (FileName.Length() <= RootDirPath.Length() + 1) continue;
		if (strncmp(RootDirPath.CStr(), FileName.CStr(), RootDirPath.Length()) > 0) break;

		CString RelFile = FileName.CStr() + RootDirPath.Length() + 1;

		int CurrStartChar = 0;
		int DirCount = 0;
		int RFLen = RelFile.Length();
		while (CurrStartChar < RFLen)
		{
			int DirSepChar = RelFile.FindCharIndex('/', CurrStartChar);
			if (DirSepChar == INVALID_INDEX) break;

			CString Dir = RelFile.SubString(CurrStartChar, DirSepChar - CurrStartChar);
			if (DirCount < DirStack.GetCount())
			{
				if (Dir != DirStack[DirCount])
				{
					for (int StackIdx = DirCount; StackIdx < DirStack.GetCount(); ++StackIdx)
						TOC.EndDirEntry();
					DirStack.Truncate(DirStack.GetCount() - DirCount);
					TOC.BeginDirEntry(Dir.CStr());
					DirStack.Add(Dir);
				}
			}
			else
			{
				TOC.BeginDirEntry(Dir.CStr());
				DirStack.Add(Dir);
			}

			++DirCount;

			CurrStartChar = DirSepChar + 1;
		}

		for (int StackIdx = DirCount; StackIdx < DirStack.GetCount(); ++StackIdx)
			TOC.EndDirEntry();
		DirStack.Truncate(DirStack.GetCount() - DirCount);

		CString FilePart = RelFile.ExtractFileName();
		CString FullFilePath = RootDirPath + "/" + RelFile;

		if (IOSrv->DirectoryExists(FullFilePath))
		{
			if (!AddDirectoryToTOC(FilePart, TOC)) //, Offset))
				n_msg(VL_ERROR, "Error reading directory %s\n", FullFilePath.CStr());
		}
		else
		{
			IO::CFileStream File;
			if (File.Open(FullFilePath, IO::SAM_READ))
			{
				int FileLength = File.GetSize();
				File.Close();
				TOC.AddFileEntry(FilePart.CStr(), 0, FileLength); //Offset, FileLength);
				//Offset += FileLength;
			}
			else n_msg(VL_ERROR, "Error reading file %s\n", FullFilePath.CStr());
		}
	}

	TOC.EndDirEntry();

	for (; i < FilesToPack.GetCount(); ++i)
		n_msg(VL_WARNING, "File is out of the export package scope:\n - %s\n", FilesToPack[i].CStr());

	// Write NPK file to disk

	n_msg(VL_INFO, "Writing NPK...\n");

	IO::CFileStream File;
	IOSrv->CreateDirectory(PkgFileName.ExtractDirName());
	if (!File.Open(PkgFileName, IO::SAM_WRITE))
	{
		n_msg(VL_ERROR, "Could not open file '%s' for writing!\n", PkgFileName.CStr());
		FAIL;
	}

	File.Put<int>('NPK0');	// Magic
	File.Put<int>(4);		// Block length
	File.Put<int>(0);		// DataBlockStart (at 8, fixed later)

	n_msg(VL_DETAILS, " - Writing TOC...\n");

	int Offset = 0;
	WriteTOCEntry(File, TOC.GetRootEntry(), Offset);

	n_msg(VL_DETAILS, " - Writing data...\n");
	
	int DataBlockStart = File.GetPosition();
	int DataOffset = DataBlockStart + 4;

	File.Put<int>('DATA');
	File.Put<int>(0);		// DataSize (at DataOffset, fixed later)

	int DataSize = 0;
	if (WriteEntryData(File, TOC.GetRootEntry(), DataOffset + 4, DataSize))
	{
		File.Seek(8, IO::Seek_Begin);
		File.Put<int>(DataBlockStart);
		File.Seek(DataOffset, IO::Seek_Begin);
		File.Put<int>(DataSize);
		File.Seek(0, IO::Seek_End);
	}
	else
	{
		n_msg(VL_ERROR, "ERROR WRITING DATA BLOCK\n");
		FAIL;
	}

	n_msg(VL_DETAILS, " - NPK writing done\n");

	// Print TOC

	if (TOC.GetRootEntry() && Verbose >= VL_DETAILS)
	{
		n_printf("\n"SEP_LINE"Package TOC:\n'+' = directory\n"SEP_LINE);
		PrintNpkTOCEntry(*TOC.GetRootEntry(), 0);
	}

	OK;
}
//---------------------------------------------------------------------
