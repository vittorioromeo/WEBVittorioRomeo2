#include <stdio.h>
#include <json/json.h>
#include <string>
#include <fstream>
#include <string.h>
#include <sstream>
#include <ctemplate/template.h>
#include <boost/filesystem.hpp>
#include <vector>

using namespace std;
using namespace boost::filesystem;
using namespace ctemplate;

Json::Value getJsonFileRoot(string mFilePath)
{
	Json::Value root;
	Json::Reader reader;
	ifstream stream{mFilePath, ifstream::binary};
	bool parsingSuccessful{reader.parse(stream, root, false)};
	if(!parsingSuccessful) cout << reader.getFormatedErrorMessages() << endl;
	return root;
}

int countChar(char mChar, string mString)
{
	int result{0};
	for(auto c = mString.begin(); c != mString.end(); ++c) if(mChar == *c) result++;
	return result;
}

int getDepth(path mPath) { return countChar('/', mPath.string()) + countChar('\\', mPath.string()); }

string getResourcesFolderPath(int mDepth)
{
	string s{"../"}, result{""};

	for(int i{0}; i < mDepth; ++i) result += s;
	return result + "Resources";
}

void recursiveFill(vector<path>& mPathVector, const path& mDirectoryPath)
{
	if(!exists(mDirectoryPath)) return;
	for (directory_iterator itr{mDirectoryPath}; itr != directory_iterator{}; ++itr)
	{
		if(is_directory(itr->status())) recursiveFill(mPathVector, itr->path());
		else mPathVector.push_back(itr->path());
	}
}

void recursiveFill(vector<path>& mPathVector, const path& mDirectoryPath, const string mFileName)
{
	if(!exists(mDirectoryPath)) return;
	for (directory_iterator itr{mDirectoryPath}; itr != directory_iterator{}; ++itr)
	{
		if(is_directory(itr->status())) recursiveFill(mPathVector, itr->path(), mFileName);
		else if(itr->path().filename() == mFileName) mPathVector.push_back(itr->path());
	}
}

void fillDict(TemplateDictionary* mDict, Json::Value mValue)
{
	for(Json::ValueIterator itr{mValue.begin()}; itr != mValue.end(); ++itr) mDict->SetValue(itr.key().asString(), (*itr).asString());
}

// ---

struct MainMenu
{
	Json::Value root;

	string getOutput()
	{
		Json::Value menuItems{root["MenuItems"]};
		string result;
		TemplateDictionary dict("mainMenu");

		for(Json::ValueIterator itr{menuItems.begin()}; itr != menuItems.end(); ++itr)
		{
			Json::Value menuItem{(*itr)};
			TemplateDictionary* subDict = dict.AddSectionDictionary("MenuItems");
			fillDict(subDict, menuItem);
		}

		ExpandTemplate("Templates/Base/mainMenu.tpl", ctemplate::DO_NOT_STRIP, &dict, &result);
		return result;
	}
};

struct Main
{
	vector<string> expandedEntries, expandedAsides;

	void expandItem(string mTemplatePath, Json::Value mToExpand, string mDictName, vector<string>& mTargetVector)
	{
		string result;
		TemplateDictionary dict(mDictName);

		fillDict(&dict, mToExpand);

		ExpandTemplate(mTemplatePath, ctemplate::DO_NOT_STRIP, &dict, &result);
		mTargetVector.push_back(result);
	}

	void addEntry(Json::Value mRoot) { expandItem(mRoot["Template"].asString(), mRoot["ToExpand"], "entry", expandedEntries); }
	void addAside(Json::Value mRoot) { expandItem(mRoot["Template"].asString(), mRoot["ToExpand"], "aside", expandedAsides); }
	void addMenu(Json::Value mRoot)
	{
		Json::Value menuItems{mRoot["MenuItems"]};
		string result;
		TemplateDictionary dict("menu");

		for(Json::ValueIterator itr{menuItems.begin()}; itr != menuItems.end(); ++itr)
		{
			Json::Value menuItem{(*itr)};
			TemplateDictionary* subDict = dict.AddSectionDictionary("MenuItems");
			fillDict(subDict, menuItem);
		}

		ExpandTemplate("Templates/Entries/menu.tpl", ctemplate::DO_NOT_STRIP, &dict, &result);
		expandedEntries.push_back(result);
	}

	string getOutput()
	{
		string result;
		TemplateDictionary dict("main");

		for(auto entry : expandedEntries)
		{
			TemplateDictionary* subDict{dict.AddSectionDictionary("Entries")};
			subDict->SetValue("Entry", entry);
		}

		for(auto aside : expandedAsides)
		{
			TemplateDictionary* subDict{dict.AddSectionDictionary("Asides")};
			subDict->SetValue("Aside", aside);
		}

		ExpandTemplate("Templates/Base/main.tpl", ctemplate::DO_NOT_STRIP, &dict, &result);
		return result;
	}
};

struct Page
{
	path myPath;
	Json::Value root;

	MainMenu mainMenu;
	Main main;

	Page(path mPath, Json::Value mRoot) : myPath{mPath}, root{mRoot}
	{
		mainMenu = MainMenu{getJsonFileRoot("Json/mainMenu.json")};

		string pageFolder{path{myPath}.remove_leaf().string()};
		string entriesFolder{pageFolder + "/Entries"}, asidesFolder{pageFolder + "/Asides"};
		
		vector<path> entryPaths; recursiveFill(entryPaths, path(entriesFolder));
		vector<path> asidePaths; recursiveFill(asidePaths, path(asidesFolder));

		for(auto s : entryPaths)
		{
			Json::Value root{getJsonFileRoot(s.string())};

			if(!root.isMember("Entries")) appendEntry(root);
			else for (Json::Value value : root["Entries"]) appendEntry(value);
		}
		for(auto s : asidePaths) main.addAside(getJsonFileRoot(s.string()));
	}

	void appendEntry(Json::Value mRoot) { if(mRoot.isMember("MenuItems")) main.addMenu(mRoot); else main.addEntry(mRoot); }

	path getResultPath()
	{
		string s{myPath.string()};
		return path("Result/" + root["fileName"].asString());
	}

	string getOutput()
	{
		string resourcesPath{getResourcesFolderPath(getDepth(getResultPath()) - 1)};
		
		string result;
		TemplateDictionary dict("page");		
		dict["MainMenu"] = mainMenu.getOutput();
		dict["Main"] = main.getOutput();
		dict["ResourcesPath"] = resourcesPath;
		ExpandTemplate("Templates/page.tpl", ctemplate::DO_NOT_STRIP, &dict, &result);
		StringToTemplateCache(root["fileName"].asString(), result, ctemplate::DO_NOT_STRIP);

		string finalResult;
		TemplateDictionary dict2("page2");
		dict2["ResourcesPath"] = resourcesPath;
		ExpandTemplate(root["fileName"].asString(), ctemplate::DO_NOT_STRIP, &dict2, &finalResult);
		return finalResult;
	}
};

// ---
vector<Page> pages;

void loadPages()
{
	cout << "Getting all page.json files" << endl;
	path pagesPath{"Json/Pages"};
	vector<path> pageJsonPaths;
	recursiveFill(pageJsonPaths, pagesPath, "page.json");
	for(auto s : pageJsonPaths)
	{
		cout << "> " << s.string() << endl;
		pages.push_back(Page{s, getJsonFileRoot(s.string())});
	}
}

void expandPages()
{
	cout << "Writing pages to result" << endl;
	for (Page page : pages)
	{
		// --- Check path
		path parentPath{page.getResultPath().remove_leaf()};
		cout << "Checking if path exists: " << parentPath.string() << endl;
		if(!exists(parentPath)) create_directory(parentPath);

		// --- Write page to file
		cout << "> " << page.getResultPath() << endl;
		ofstream o{page.getResultPath().string() + "temp"};
		o << page.getOutput(); o.flush(); o.close();
		cout << endl;

		ifstream in_file(page.getResultPath().string() + "temp");
		ofstream out_file(page.getResultPath().string());

		string line;
		while (getline(in_file, line)) if(!line.empty()) out_file << line;

		in_file.close();
		out_file.flush();
		out_file.close();

		remove(page.getResultPath().string() + "temp");
	}
}

int main()
{
	loadPages();
	expandPages();

	return 0;
}
