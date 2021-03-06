#include <StdCfg.h>
#include "CEGUINebula2ResourceProvider.h"
#include <Data/DataServer.h>
#include <Data/Streams/FileStream.h>

namespace CEGUI
{

void CNebula2ResourceProvider::loadRawDataContainer(const String& filename, RawDataContainer& output,
													const String& resourceGroup)
{
	n_assert2(!filename.empty(), "Filename supplied for data loading must be valid");

	String FinalFilename;
	if (resourceGroup.empty()) FinalFilename = d_defaultResourceGroup + filename;
	else
	{
		int Idx = ResourceGroups.FindIndex(resourceGroup);
		if (Idx != -1) FinalFilename = ResourceGroups.ValueAtIndex(Idx) + filename;
		else FinalFilename = filename;
	}

	Data::CFileStream File;
	if (File.Open(FinalFilename.c_str(), Data::SAM_READ))
	{
		const long Size = File.GetSize();
		unsigned char* const pBuffer = n_new_array(unsigned char, Size);
		const size_t BytesRead = File.Read(pBuffer, Size);
		if (BytesRead != Size)
		{
			n_delete_array(pBuffer);
			n_error("A problem occurred while reading file: %s", FinalFilename.c_str());
		}
		File.Close();

		output.setData(pBuffer);
		output.setSize(Size);
	}
}
//---------------------------------------------------------------------

void CNebula2ResourceProvider::unloadRawDataContainer(RawDataContainer& data)
{
	uint8* const ptr = data.getDataPtr();
	delete[] ptr;
	data.setData(0);
	data.setSize(0);
}
//---------------------------------------------------------------------

void CNebula2ResourceProvider::setResourceGroupDirectory(const String& resourceGroup, const String& directory)
{
	if (directory.length() == 0) return;
	const String separators("\\/");
	if (String::npos == separators.find(directory[directory.length() - 1]))
		ResourceGroups.Add(resourceGroup, directory + '/');
	else ResourceGroups.Add(resourceGroup, directory);
}
//---------------------------------------------------------------------

const String& CNebula2ResourceProvider::getResourceGroupDirectory(const String& resourceGroup)
{
	return ResourceGroups[resourceGroup];
}
//---------------------------------------------------------------------

void CNebula2ResourceProvider::clearResourceGroupDirectory(const String& resourceGroup)
{
	ResourceGroups.Clear();
}
//---------------------------------------------------------------------

size_t CNebula2ResourceProvider::getResourceGroupFileNames(std::vector<String>& out_vec,
														   const String& file_pattern,
														   const String& resource_group)
{
	String DirName;
	if (resource_group.empty()) DirName = d_defaultResourceGroup;
	else
	{
		int Idx = ResourceGroups.FindIndex(resource_group);
		if (Idx != -1) DirName = ResourceGroups.ValueAtIndex(Idx);
		else DirName = "./";
	}

    size_t Entries = 0;

	nString Pattern = DirName.c_str();
	Pattern += file_pattern.c_str();

	Data::PFileSystem FS;
	nString EntryName;
	Data::EFSEntryType EntryType;
	nString RootDir(DirName.c_str());
	void* hDir = DataSrv->OpenDirectory(RootDir, NULL, FS, EntryName, EntryType);
	if (hDir)
	{
		if (EntryType != Data::FSE_NONE) do
		{
			if (EntryType == Data::FSE_FILE)
			{
				nString FullEntryName = RootDir + EntryName;
				if (FullEntryName.MatchPattern(Pattern))
				{
					out_vec.push_back(String(FullEntryName.Get()));
					++Entries;
				}
			}
		}
		while (FS->NextDirectoryEntry(hDir, EntryName, EntryType));

		FS->CloseDirectory(hDir);
	}

	return Entries;
}
//---------------------------------------------------------------------

}