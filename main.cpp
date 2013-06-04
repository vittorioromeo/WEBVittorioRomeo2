
#include <string>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <ctemplate/template.h>
#include <boost/filesystem.hpp>
#include <SSVJsonCpp/SSVJsonCpp.h>
#include <SSVUtils/SSVUtils.h>
#include <SSVUtilsJson/SSVUtilsJson.h>

using namespace std;
using namespace boost::filesystem;
using namespace ctemplate;
using namespace ssvu;
using namespace ssvu::FileSystem;
using namespace ssvuj;

int getDepth(path mPath) { return getCharCount(mPath.string(), '/') + getCharCount(mPath.string(), '\\'); }

string getResourcesFolderPath(int mDepth)
{
	string s{"../"}, result{""};

	for(int i{0}; i < mDepth; ++i) result += s;
	return result + "Resources";
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
		TemplateDictionary dict{"mainMenu"};

		for(auto& i : as<vector<Json::Value>>(menuItems))
		{
			TemplateDictionary* subDict{dict.AddSectionDictionary("MenuItems")};
			fillDict(subDict, i);
		}

		ExpandTemplate("Templates/Base/mainMenu.tpl", ctemplate::DO_NOT_STRIP, &dict, &result);
		return result;
	}
};

struct Main
{
	vector<string> expandedEntries, expandedAsides;

	void expandItem(const string& mTemplatePath, Json::Value mToExpand, const string& mDictName, vector<string>& mTargetVector)
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
		TemplateDictionary dict{"menu"};

		for(auto& i : as<vector<Json::Value>>(menuItems))
		{
			TemplateDictionary* subDict{dict.AddSectionDictionary("MenuItems")};
			fillDict(subDict, i);
		}

		ExpandTemplate("Templates/Entries/menu.tpl", ctemplate::DO_NOT_STRIP, &dict, &result);
		expandedEntries.push_back(result);
	}

	string getOutput()
	{
		string result;
		TemplateDictionary dict("main");

		for(const auto& e : expandedEntries)
		{
			TemplateDictionary* subDict{dict.AddSectionDictionary("Entries")};
			subDict->SetValue("Entry", e);
		}

		for(const auto& a : expandedAsides)
		{
			TemplateDictionary* subDict{dict.AddSectionDictionary("Asides")};
			subDict->SetValue("Aside", a);
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
		mainMenu = MainMenu{getRootFromFile("Json/mainMenu.json")};

		string pageFolder{path{myPath}.remove_leaf().string()};
		string entriesFolder{pageFolder + "/Entries/"}, asidesFolder{pageFolder + "/Asides/"};

		vector<string> entryPaths{getRecursiveFiles(entriesFolder)}, asidePaths{getRecursiveFiles(asidesFolder)};

		for(const auto& s : entryPaths)
		{
			Json::Value root{getRootFromFile(s)};

			if(!root.isMember("Entries")) appendEntry(root);
			else for(Json::Value value : root["Entries"]) appendEntry(value);
		}
		for(const auto& s : asidePaths) main.addAside(getRootFromFile(s));
	}

	void appendEntry(Json::Value mRoot) { if(mRoot.isMember("MenuItems")) main.addMenu(mRoot); else main.addEntry(mRoot); }

	path getResultPath() const { return path("Result/" + root["fileName"].asString()); }

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
	log("Getting all page.json files", "loadPages");

	string pagesPath("Json/Pages/");
	vector<string> pageJsonPaths{getRecursiveFilesByName(pagesPath, "page.json")};

	for(const auto& s : pageJsonPaths)
	{
		log("> " + s, "loadPages");
		pages.push_back(Page{s, getRootFromFile(s)});
	}
}

void expandPages()
{
	log("Writing pages to result", "expandPages");

	for(auto& p : pages)
	{
		// Check path
		path parentPath{p.getResultPath().remove_leaf()};
		log("Checking if path exists: " + parentPath.string(), "expandPages");
		if(!exists(parentPath)) createFolder(parentPath.string());

		// Write page to file
		string resultPath{p.getResultPath().string()};

		log("> " + resultPath, "expandPages");
		ofstream o{resultPath + "temp"}; o << p.getOutput(); o.flush(); o.close();
		log("");

		ifstream inFile(resultPath + "temp");
		ofstream outFile(resultPath);

		string line;
		while(getline(inFile, line)) if(!line.empty()) outFile << line;

		inFile.close(); outFile.flush(); outFile.close();
		remove(p.getResultPath().string() + "temp");
	}
}

int main(){ loadPages(); expandPages(); return 0; }
