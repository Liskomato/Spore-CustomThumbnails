#include "stdafx.h"
#include "CustomThumbnailManager.h"
#include <iostream>
#include <windows.h>
#include <commdlg.h>
#include <wininet.h>
#include <filesystem>
#include <d3d9helper.h>

CustomThumbnailManager::CustomThumbnailManager()
{
	//Resource::Paths::CreateSaveAreaDirectoryDatabase(Resource::PathID::Creations,u"Thumbnails",directoryCache,static_cast<Resource::SaveAreaID>(id(u"CustomAdventureThumbnails")));
	//directoryCache->AddExtensionMapping(u"png", TypeIDs::png);

	directoryPath = u"CustomThumbnailsCache.package";
	directoryPath = Resource::Paths::GetDirFromID(Resource::PathID::AppData) + directoryPath;

	dbCache = new Resource::DatabasePackedFile(directoryPath.c_str());

	ResourceManager.RegisterDatabase(true, dbCache.get());
}


CustomThumbnailManager::~CustomThumbnailManager()
{
}


void CustomThumbnailManager::ParseLine(const ArgScript::Line& line)
{
	// This method is called when your cheat is invoked.
	// Put your cheat code here.
	if (!Simulator::IsScenarioMode() || ScenarioMode.GetMode() != App::cScenarioMode::Mode::EditMode) {
		App::ConsolePrintF("You must be in the adventure editor in order to use this cheat.");
		return;
	}

	size_t numArgs;
	auto args = line.GetArgumentsRange(&numArgs, 0, 1);
	int thumbnailIndex = -1;
	if (numArgs == 1) {
		thumbnailIndex = mpFormatParser->ParseInt(args[0]);
	}

	if (thumbnailIndex > 4 || thumbnailIndex < -1) {
		App::ConsolePrintF("The given argument for the thumbnail index must be between 0 and 4.");
		return;
	}


	string16 filePath;

	WCHAR file[1025] = { 0 };
	file[0] = '\0';

	OPENFILENAMEW openedFile;
	ZeroMemory(&openedFile, sizeof(openedFile));
	openedFile.lStructSize = sizeof(openedFile);
	openedFile.lpstrFilter = L"PNG image files (*.png)\0*.png\0\0";
	openedFile.nFileOffset = 1;
	openedFile.lpstrFile = file;
	openedFile.lpstrFile[0] = '\0';
	openedFile.nMaxFile = 1025;
	openedFile.lpstrTitle = L"Select a PNG image to work as your adventure thumbnail.";
	openedFile.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

	bool check = GetOpenFileNameW(&openedFile);

	if (check) {
		TexturePtr texture;

		string16 str = (char16_t*)openedFile.lpstrFile;

		FileStreamPtr stream = new IO::FileStream((char16_t*)openedFile.lpstrFile);

		uint32_t hash = id(str.substr(str.find_last_of(u"\\/") + 1).c_str());
		ResourceKey fileKey = ResourceKey(hash, TypeIDs::png, 0);
		

		if (stream->Open(IO::AccessFlags::Read, IO::CD::OpenExisting)) {
			char* data = new char[stream->GetSize()];
			stream->Read(data,stream->GetSize());
			
			Resource::IRecord* pfData;
			
			if (dbCache->Open(IO::AccessFlags::ReadWrite,IO::CD::OpenAlways) && dbCache->OpenRecord(fileKey,&pfData, IO::AccessFlags::ReadWrite, IO::CD::CreateAlways)) {

				if (pfData->GetStream()->Write(data,stream->GetSize())) {
					
					ModAPI::Log("CustomAdventureThumbnails: PNG file %ls written successfully to %ls",str.c_str(),directoryPath.c_str());
				}
				else {
					SporeDebugPrint("Failed to write record.");
				}
				dbCache->CloseRecord(pfData);
				
				
				
			}
			else {
				SporeDebugPrint("Failed to open record.");
			}

			stream->Close();
			delete[] data;

		}

		//string16 dest = u"Thumbnails\\";
		//
		//string16 hashString;
		//hashString.append_sprintf(u"%#10x!%#10x.%#10x",0, hash, TypeIDs::png);
		//dest = Resource::Paths::GetDirFromID(Resource::PathID::Creations) + dest + hashString;
		//
		//

		//if (std::filesystem::exists(dest.c_str())) {
		//	SporeDebugPrint("File already exists, using existing version instead.");
		//}
		//else if (!std::filesystem::copy_file(str.c_str(), dest.c_str())) {
		//	App::ConsolePrintF("Failed to copy file to folder.");
		//	return;
		//}



		//if (dbCache->Open(IO::AccessFlags::Read,IO::CD::OpenAlways) && ResourceManager.GetResource(fileKey, &res, nullptr, dbCache.get(), factory.get())) {
		//	
		//	SporeDebugPrint("Resource found.");
		//	auto raster = object_cast<Graphics::cRwRasterDirectResource>(res);

		//	texture = TextureManager.AddRaster(hash,0,raster->GetData(), Graphics::TextureFlags::kTextureFlagForceLoad);
		//	

		//}

		texture = TextureManager.GetTexture(fileKey, Graphics::TextureFlags::kTextureFlagForceLoad);
		if (texture != nullptr) {
			
			ScenarioMode.GetData()->StartHistoryEntry();

			switch (thumbnailIndex) {

			case -1:
				ScenarioMode.GetData()->mThumbnail = texture;
				ScenarioMode.GetData()->mLargeThumbnail = texture;
				break;
			case 0:
				ScenarioMode.GetData()->mThumbnail = texture;
				break;
			case 1: 
				ScenarioMode.GetData()->mLargeThumbnail = texture;
				break;
			case 2:
				ScenarioMode.GetData()->mSecondThumbnail = texture;
				break;
			case 3:
				ScenarioMode.GetData()->mThirdThumbnail = texture;
				break;
			case 4:
				ScenarioMode.GetData()->mFourthThumbnail = texture;
				break;
			default:
				return;
			}

			ScenarioMode.GetData()->CommitHistoryEntry();

		}
		else {
			App::ConsolePrintF("Error: TextureManager could not find a texture file. Check if the file was named correctly in My Spore Creations/Thumbnails folder.");
		}
	}


}

const char* CustomThumbnailManager::GetDescription(ArgScript::DescriptionMode mode) const
{
	if (mode == ArgScript::DescriptionMode::Basic) {
		return "Replace the thumbnails for your adventure.";
	}
	else {
		return "setCustomThumbnail: Manually set your thumbnails for adventures into any image.\n"
			   "Usage:\n"
			   "setCustomThumbnail (no arguments): Select a PNG file from your system to serve as the adventure thumbnail.\n"
			   "setCustomThumbnail <0-4>: Select a PNG file to serve as a specific thumbnail image index for the adventure preview. Image 0 is the PNG shown in the Sporepedia, 1-4 are the large preview images. By default (without arguments) the mod replaces both the icon and the first large preview thumbnail with the same image.";
	}
}
