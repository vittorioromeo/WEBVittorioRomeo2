#include "Utils.h"

using namespace boost::filesystem;
using namespace std;

string getResourcesFolderPath() { return "Resources"; }

Json::Value getJsonFileRoot(string mFilePath)
{
	Json::Value root;
	Json::Reader reader;
	ifstream stream{mFilePath, ifstream::binary};
	bool parsingSuccessful{reader.parse(stream, root, false)};
	if (!parsingSuccessful) cout << reader.getFormatedErrorMessages() << endl;

	return root;
}

void recursiveDirectoryFill(vector<path>& mPathVector, const path& mDirectoryPath)
{
	if (!exists(mDirectoryPath)) return;

	for (directory_iterator itr{mDirectoryPath}; itr != directory_iterator{}; ++itr)
		if (is_directory(itr->status()))
		{
			mPathVector.push_back(itr->path());
			recursiveDirectoryFill(mPathVector, itr->path());			
		}
}

vector<string> getAllSubFolderNames(string mPath)
{
	vector<string> result;

	for (directory_iterator itr{mPath}; itr != directory_iterator{}; ++itr)
		if (is_directory(itr->status()))
		{			
			string directoryName{itr->path().filename().string()};
			result.push_back(directoryName);
		}

	return result;
}
vector<path> getAllFilePaths(string mFolderPath, string mExtension)
{
	vector<path> result;

	for (directory_iterator itr{mFolderPath}; itr != directory_iterator{}; ++itr)
		if (!is_directory(itr->status()))
		{
			if(itr->path().extension().string() != mExtension) continue;
			result.push_back(itr->path());
		}
		
	return result;
}
string getFileNameFromFilePath(string mFilePath, string mPrefix, string mSuffix)
{
	return mFilePath.substr(mPrefix.length(), mFilePath.length() - mPrefix.length() - mSuffix.length());
}

void fillDictFromJsonRoot(ctemplate::TemplateDictionary& mDict, Json::Value mRoot)
{
	for(Json::ValueIterator itr{mRoot.begin()}; itr != mRoot.end(); itr++)
		mDict[itr.key().asString()] = (*itr).asString();
}
