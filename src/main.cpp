// Copyright (c) 2013 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <SSVUtils/SSVUtils.h>
#include <SSVUtilsJson/SSVUtilsJson.h>
extern "C" {
	#include <mkdio.h>
}

using namespace std;
using namespace ssvu;
using namespace ssvu::FileSystem;
using namespace ssvu::TemplateSystem;
using namespace ssvuj;

int getDepth(const string& mPath) { return getCharCount(getNormalizedPath(mPath), '/'); }

string getResourcesFolderPath(int mDepth)
{
	string s{"../"}, result;
	for(int i{0}; i < mDepth; ++i) result += s;
	return result + "Resources";
}

Dictionary getDictionaryFromJson(const ssvuj::Obj& mValue)
{
	Dictionary result;
	for(auto itr(begin(mValue)); itr != end(mValue); ++itr) result[as<string>(itr.key())] = as<string>(*itr);
	return result;
}

struct MainMenu
{
	ssvuj::Obj root;

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

	void expandItem(const string& mTplPath, ssvuj::Obj mRoot, vector<string>& mTarget, const std::string& mPagePath = "")
	{
		if(ssvuj::has(mRoot, "Markdown"))
		{
			auto mdPath = getNormalizedPath(getParentPath(mPagePath) + "Entries/" + ssvuj::as<string>(mRoot, "Markdown"));
			auto mdResPath = getNormalizedPath(mdPath + ".tmp");

			// I HATE THIS
			FILE* f1 = fopen(mdPath.c_str(), "r");
			FILE* f2 = fopen(mdResPath.c_str(), "w");
			MMIOT* in = mkd_in(f1, 0);
			markdown(in, f2, 0);
			fclose(f1);
			fclose(f2);

			ssvuj::set(mRoot, "Text", getFileContents(mdResPath));

			removeFile(mdResPath);
		}

		mTarget.push_back(getDictionaryFromJson(mRoot).getExpanded(getFileContents(mTplPath)));
	}

	void addEntry(const ssvuj::Obj& mRoot, const std::string& mPagePath) { expandItem(as<string>(mRoot, "Template"), mRoot["ToExpand"], expandedEntries, mPagePath); }
	void addAside(const ssvuj::Obj& mRoot) { expandItem(as<string>(mRoot, "Template"), mRoot["ToExpand"], expandedAsides); }
	void addMenu(const ssvuj::Obj& mRoot)
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
	ssvuj::Obj root;

	MainMenu mainMenu{readFromFile("Json/mainMenu.json")};
	Main main;

	Page(const string& mPath, const ssvuj::Obj& mRoot) : myPath{mPath}, root{mRoot}
	{
		string pageFolder{getParentPath(myPath)};
		string entriesFolder{pageFolder + "Entries/"}, asidesFolder{pageFolder + "Asides/"};

		vector<string> entryPaths{getScan<Mode::Recurse, Type::File>(entriesFolder)}, asidePaths{getScan<Mode::Recurse, Type::File>(asidesFolder)};

		for(const auto& s : entryPaths)
		{
			if(!endsWith(s, ".json")) continue;
			ssvuj::Obj root{readFromFile(s)};

			if(!has(root, "Entries")) appendEntry(root);
			else for(const auto& v : root["Entries"]) appendEntry(v);
		}
		for(const auto& s : asidePaths) main.addAside(readFromFile(s));
	}

	void appendEntry(ssvuj::Obj mRoot) { if(has(mRoot, "MenuItems")) main.addMenu(mRoot); else main.addEntry(mRoot, myPath); }

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
	lo << lt("loadPages") << "Getting all page.json files" << endl;

	string pagesPath("Json/Pages/");
	vector<string> pageJsonPaths{getScan<Mode::Recurse, Type::File, Pick::ByName>(pagesPath, "page.json")};

	for(const auto& s : pageJsonPaths)
	{
		lo << lt("loadPages") << "> " << s << endl;
		pages.push_back(Page{s, readFromFile(s)});
	}
}

void expandPages()
{
	lo << lt("expandPages") << "Writing pages to result" << endl;

	for(auto& p : pages)
	{
		// Check path
		string parentPath{getParentPath(p.getResultPath())};
		lo << lt("expandPages") << "Checking if path exists: " << parentPath << endl;
		if(!exists(parentPath)) createFolder(parentPath);

		// Write page to file
		string resultPath{p.getResultPath()};

		lo << lt("expandPages") << "> " << resultPath << endl;
		ofstream o{resultPath + "temp"}; o << p.getOutput(); o.flush(); o.close();
		lo << endl;

		ifstream inFile(resultPath + "temp");
		ofstream outFile(resultPath);

		string line;
		while(getline(inFile, line)) if(!line.empty()) outFile << line << "\n";

		inFile.close(); outFile.flush(); outFile.close();
		removeFile(p.getResultPath() + "temp");
	}
}

int main() { loadPages(); expandPages(); return 0; }
