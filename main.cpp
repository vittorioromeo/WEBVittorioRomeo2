#include <string>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <SSVJsonCpp/SSVJsonCpp.h>
#include <SSVUtils/SSVUtils.h>
#include <SSVUtilsJson/SSVUtilsJson.h>

using namespace std;
using namespace ssvu;
using namespace ssvu::FileSystem;
using namespace ssvu::TemplateSystem;
using namespace ssvuj;

int getDepth(const string& mPath) { return getCharCount(getNormalizedPath(mPath), '/'); }

string getResourcesFolderPath(int mDepth)
{
	string s{"../"}, result{""};
	for(int i{0}; i < mDepth; ++i) result += s;
	return result + "Resources";
}

Dictionary getDictionaryFromJson(const Json::Value& mValue)
{
	Dictionary result;
	for(auto itr(begin(mValue)); itr != end(mValue); ++itr) result[itr.key().asString()] = (*itr).asString();
	return result;
}

struct MainMenu
{
	Json::Value root;

	string getOutput() const
	{
		Dictionary dict;
		for(const auto& i : root["MenuItems"]) dict += {"MenuItems", getDictionaryFromJson(i)};
		return dict.getExpanded(getFileContents("Templates/Base/mainMenu.tpl"));
	}
};

struct Main
{
	vector<string> expandedEntries{""}, expandedAsides{""};

	void expandItem(const string& mTplPath, const Json::Value& mRoot, vector<string>& mTarget) { mTarget.push_back(getDictionaryFromJson(mRoot).getExpanded(getFileContents(mTplPath))); }

	void addEntry(const Json::Value& mRoot) { expandItem(as<string>(mRoot, "Template"), mRoot["ToExpand"], expandedEntries); }
	void addAside(const Json::Value& mRoot) { expandItem(as<string>(mRoot, "Template"), mRoot["ToExpand"], expandedAsides); }
	void addMenu(const Json::Value& mRoot)
	{
		Dictionary dict;
		for(const auto& i : mRoot["MenuItems"]) dict += {"MenuItems", getDictionaryFromJson(i)};
		expandedEntries.push_back(dict.getExpanded(getFileContents("Templates/Entries/menu.tpl")));
	}

	string getOutput() const
	{
		Dictionary dict;
		for(const auto& e : expandedEntries) dict += {"Entries", {{"Entry", e}}};
		for(const auto& a : expandedAsides) dict += {"Asides", {{"Aside", a}}};
		return dict.getExpanded(getFileContents("Templates/Base/main.tpl"));
	}
};

struct Page
{
	string myPath;
	Json::Value root;

	MainMenu mainMenu{getRootFromFile("Json/mainMenu.json")};
	Main main;

	Page(const string& mPath, const Json::Value& mRoot) : myPath{mPath}, root{mRoot}
	{
		string pageFolder{getParentPath(myPath)};
		string entriesFolder{pageFolder + "Entries/"}, asidesFolder{pageFolder + "Asides/"};

		vector<string> entryPaths{getScan<Mode::Recurse, Type::File>(entriesFolder)}, asidePaths{getScan<Mode::Recurse, Type::File>(asidesFolder)};

		for(const auto& s : entryPaths)
		{
			Json::Value root{getRootFromFile(s)};

			if(!root.isMember("Entries")) appendEntry(root);
			else for(const auto& v : root["Entries"]) appendEntry(v);
		}
		for(const auto& s : asidePaths) main.addAside(getRootFromFile(s));
	}

	void appendEntry(Json::Value mRoot) { if(mRoot.isMember("MenuItems")) main.addMenu(mRoot); else main.addEntry(mRoot); }

	string getResultPath() const { return "Result/" + as<string>(root, "fileName"); }

	string getOutput() const
	{
		string resourcesPath{getResourcesFolderPath(getDepth(getResultPath()) - 1)};

		Dictionary dict;
		dict["MainMenu"] = mainMenu.getOutput();
		dict["Main"] = main.getOutput();
		dict["ResourcesPath"] = resourcesPath;
		return dict.getExpanded(getFileContents("Templates/page.tpl"));
	}
};

// ---
vector<Page> pages;

void loadPages()
{
	log("Getting all page.json files", "loadPages");

	string pagesPath("Json/Pages/");
	vector<string> pageJsonPaths{getScan<Mode::Recurse, Type::File, Pick::ByName>(pagesPath, "page.json")};

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
		string parentPath{getParentPath(p.getResultPath())};
		log("Checking if path exists: " + parentPath, "expandPages");
		if(!exists(parentPath)) createFolder(parentPath);

		// Write page to file
		string resultPath{p.getResultPath()};

		log("> " + resultPath, "expandPages");
		ofstream o{resultPath + "temp"}; o << p.getOutput(); o.flush(); o.close();
		log("");

		ifstream inFile(resultPath + "temp");
		ofstream outFile(resultPath);

		string line;
		while(getline(inFile, line)) if(!line.empty()) outFile << line;

		inFile.close(); outFile.flush(); outFile.close();
		removeFile(p.getResultPath() + "temp");
	}
}

int main() { loadPages(); expandPages(); return 0; }
