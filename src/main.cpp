// Copyright (c) 2013-2014 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <SSVUtils/SSVUtils.hpp>
#include <SSVUtilsJson/SSVUtilsJson.hpp>
#include <DiscountCpp/DiscountCpp.hpp>

using namespace std;
using namespace ssvu;
using namespace ssvu::FileSystem;
using namespace ssvu::TemplateSystem;

std::size_t getDepth(const ssvufs::Path& mPath) { return getCharCount(mPath, '/'); }

ssvufs::Path getResourcesFolderPath(std::size_t mDepth)
{
	std::string result;
	for(auto i(0u); i < mDepth; ++i) result += "../";
	return result + "Resources";
}

Dictionary getDictionaryFromJson(const ssvuj::Obj& mValue)
{
	Dictionary result;
	for(auto itr(begin(mValue)); itr != end(mValue); ++itr) result[ssvuj::getExtr<string>(itr.key())] = ssvuj::getExtr<string>(*itr);
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

	void expandItem(const Path& mTplPath, ssvuj::Obj mRoot, vector<string>& mTarget, const Path& mPagePath = "")
	{
		if(ssvuj::hasObj(mRoot, "Markdown"))
		{
			Path mdPath{mPagePath.getParent() + "Entries/" + ssvuj::getExtr<string>(mRoot, "Markdown")};
			ssvuj::arch(mRoot, "Text", discountcpp::getHTMLFromMarkdownFile(mdPath));
		}

		mTarget.emplace_back(getDictionaryFromJson(mRoot).getExpanded(getFileContents(mTplPath)));
	}

	void addEntry(const ssvuj::Obj& mRoot, const Path& mPagePath) { expandItem(ssvuj::getExtr<string>(mRoot, "Template"), mRoot["ToExpand"], expandedEntries, mPagePath); }
	void addAside(const ssvuj::Obj& mRoot) { expandItem(ssvuj::getExtr<string>(mRoot, "Template"), mRoot["ToExpand"], expandedAsides); }
	void addMenu(const ssvuj::Obj& mRoot)
	{
		Dictionary dict;
		for(const auto& i : mRoot["MenuItems"]) dict += {"MenuItems", getDictionaryFromJson(i)};
		expandedEntries.emplace_back(dict.getExpanded(getFileContents("Templates/Entries/menu.tpl")));
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
	Path myPath;
	ssvuj::Obj root;

	MainMenu mainMenu{ssvuj::getFromFile("Json/mainMenu.json")};
	Main main;

	Page(const Path& mPath, const ssvuj::Obj& mRoot) : myPath{mPath}, root{mRoot}
	{
		Path pageFolder{myPath.getParent()};
		Path entriesFolder{pageFolder + "Entries/"}, asidesFolder{pageFolder + "Asides/"};

		vector<Path> entryPaths{getScan<Mode::Recurse, Type::File>(entriesFolder)}, asidePaths{getScan<Mode::Recurse, Type::File>(asidesFolder)};

		for(const auto& s : entryPaths)
		{
			if(!endsWith(s, ".json")) continue;
			ssvuj::Obj eRoot{ssvuj::getFromFile(s)};

			if(!ssvuj::hasObj(eRoot, "Entries")) appendEntry(eRoot);
			else for(const auto& v : eRoot["Entries"]) appendEntry(v);
		}
		for(const auto& s : asidePaths) main.addAside(ssvuj::getFromFile(s));
	}

	void appendEntry(const ssvuj::Obj& mRoot) { if(ssvuj::hasObj(mRoot, "MenuItems")) main.addMenu(mRoot); else main.addEntry(mRoot, myPath); }

	Path getResultPath() const { return "Result/" + ssvuj::getExtr<string>(root, "fileName"); }

	string getOutput() const
	{
		Path resourcesPath{getResourcesFolderPath(getDepth(getResultPath()) - 1)};

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
	lo("loadPages") << "Getting all page.json files" << endl;

	Path pagesPath("Json/Pages/");
	vector<Path> pageJsonPaths{getScan<Mode::Recurse, Type::File, Pick::ByName>(pagesPath, "page.json")};

	for(const auto& s : pageJsonPaths)
	{
		lo("loadPages") << "> " << s << endl;
		pages.emplace_back(s, ssvuj::getFromFile(s));
	}
}

void expandPages()
{
	lo("expandPages") << "Writing pages to result" << endl;

	for(auto& p : pages)
	{
		// Check path
		Path parentPath{p.getResultPath().getParent()};
		lo("expandPages") << "Checking if path exists: " << parentPath << endl;
		if(!exists(parentPath)) createFolder(parentPath);

		// Write page to file
		Path resultPath{p.getResultPath()};

		lo("expandPages") << "> " << resultPath << endl;
		ofstream o{resultPath + "temp"}; o << p.getOutput(); o.flush(); o.close();
		lo() << endl;

		ifstream inFile(resultPath + "temp");
		ofstream outFile(resultPath);

		string line;
		while(getline(inFile, line)) if(!line.empty()) outFile << line << "\n";

		inFile.close(); outFile.flush(); outFile.close();
		removeFile(p.getResultPath() + "temp");
	}
}

int main() { SSVUT_RUN(); loadPages(); expandPages(); return 0; }
