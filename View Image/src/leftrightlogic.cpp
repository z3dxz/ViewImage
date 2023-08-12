#include "headers/leftrightlogic.h"

#include <shlwapi.h>

std::string GetNextFilePath(const char* file_Path) {
	 std::string imagePath = std::string(file_Path);
	 std::string folderPath = imagePath.substr(0, imagePath.find_last_of("\\/"));
	 std::string currentFileName = imagePath.substr(imagePath.find_last_of("\\/") + 1);
	 bool foundCurrentFile = false;

	 WIN32_FIND_DATAA fileData;
	 HANDLE hFind;

	 std::string searchPath = folderPath + "\\*.*";
	 hFind = FindFirstFileA(searchPath.c_str(), &fileData);

	 if (hFind != INVALID_HANDLE_VALUE)
	 {
		 do
		 {
			 if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			 {
				 std::string fileName = fileData.cFileName;
				 std::string extension = fileName.substr(fileName.find_last_of(".") + 1);
				 if (extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "bmp" || extension == "gif" || extension == "m45")
				 {
					 if (foundCurrentFile)
					 {
						 std::wstring currentFileNameW = std::wstring(currentFileName.begin(), currentFileName.end());
						 std::wstring fileNameW = std::wstring(fileName.begin(), fileName.end());
						 if (StrCmpLogicalW(fileNameW.c_str(), currentFileNameW.c_str()) > 0)
						 {
							 std::string nextImagePath = folderPath + "\\" + fileName;
							 FindClose(hFind);
							 return nextImagePath;
						 }
					 }
					 if (fileName == currentFileName)
					 {
						 foundCurrentFile = true;
					 }
				 }
			 }
			 
		 } while (FindNextFileA(hFind, &fileData) != 0);

		 FindClose(hFind);
	 }
	 return "No";
}
