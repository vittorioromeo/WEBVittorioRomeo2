#ifndef UTILS_H
#define UTILS_H

#include <json/json.h>
#include <string>
#include <fstream>
#include <string.h>
#include <sstream>
#include <ctemplate/template.h>
#include <boost/filesystem.hpp>

std::string getResourcesFolderPath();
void recursiveDirectoryFill(std::vector<boost::filesystem::path>& mPathVector, const boost::filesystem::path& mDirectoryPath);
Json::Value getJsonFileRoot(std::string mFilePath);
std::vector<std::string> getAllSubFolderNames(std::string mPath);
std::vector<boost::filesystem::path> getAllFilePaths(std::string mFolderPath, std::string mExtension);
std::string getFileNameFromFilePath(std::string mFilePath, std::string mPrefix, std::string mSuffix);
void fillDictFromJsonRoot(ctemplate::TemplateDictionary& mDict, Json::Value mRoot);

#endif // UTILS_H
