// Copyright (c) 2013-2014 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include <SSVUtils/SSVUtils.hpp>
#include <SSVUtilsJson/SSVUtilsJson.hpp>
#include <DiscountCpp/DiscountCpp.hpp>

using namespace ssvu::FileSystem;
using namespace ssvu::TemplateSystem;

auto getDepth(const ssvufs::Path& mPath) { return ssvu::getCharCount(mPath, '/'); }

auto getResourcesFolderPath(std::size_t mDepth)
{
	ssvufs::Path result;
	for(auto i(0u); i < mDepth; ++i) result += "../";
	return result + "Resources";
}

auto getDictionaryFromJson(const ssvuj::Obj& mValue)
{
	Dictionary result;
	for(auto itr(std::begin(mValue)); itr != std::end(mValue); ++itr) result[ssvuj::getExtr<std::string>(itr.key())] = ssvuj::getExtr<std::string>(*itr);
	return result;
}

struct MainMenu
{
	ssvuj::Obj root;

	auto getOutput() const
	{
		Dictionary dict;
		for(const auto& i : root["MenuItems"]) dict += {"MenuItems", getDictionaryFromJson(i)};
		return dict.getExpanded(ssvufs::Path{"Templates/Base/mainMenu.tpl"}.getContentsAsString());
	}
};

struct Main
{
	std::vector<std::string> expandedEntries{""}, expandedAsides{""};

	void expandItem(const Path& mTplPath, ssvuj::Obj mRoot, std::vector<std::string>& mTarget, const Path& mPagePath = "")
	{
		if(ssvuj::hasObj(mRoot, "Markdown"))
		{
			Path mdPath{mPagePath.getParent() + "Entries/" + ssvuj::getExtr<std::string>(mRoot, "Markdown")};
			ssvuj::arch(mRoot, "Text", discountcpp::getHTMLFromMarkdownFile(mdPath));
		}

		mTarget.emplace_back(getDictionaryFromJson(mRoot).getExpanded(mTplPath.getContentsAsString()));
	}

	void addEntry(const ssvuj::Obj& mRoot, const Path& mPagePath) { expandItem(ssvuj::getExtr<std::string>(mRoot, "Template"), mRoot["ToExpand"], expandedEntries, mPagePath); }
	void addAside(const ssvuj::Obj& mRoot) { expandItem(ssvuj::getExtr<std::string>(mRoot, "Template"), mRoot["ToExpand"], expandedAsides); }
	void addMenu(const ssvuj::Obj& mRoot)
	{
		Dictionary dict;
		for(const auto& i : mRoot["MenuItems"]) dict += {"MenuItems", getDictionaryFromJson(i)};
		expandedEntries.emplace_back(dict.getExpanded(ssvufs::Path{"Templates/Entries/menu.tpl"}.getContentsAsString()));
	}

	auto getOutput() const
	{
		Dictionary dict;
		for(const auto& e : expandedEntries) dict += {"Entries", {{"Entry", e}}};
		for(const auto& a : expandedAsides) dict += {"Asides", {{"Aside", a}}};
		return dict.getExpanded(ssvufs::Path{"Templates/Base/main.tpl"}.getContentsAsString());
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

		std::vector<Path> entryPaths{getScan<Mode::Recurse, Type::File>(entriesFolder)};

		std::vector<Path> asidePaths;
		if(asidesFolder.exists<ssvufs::Type::Folder>()) asidePaths = getScan<Mode::Recurse, Type::File>(asidesFolder);

		for(const auto& s : entryPaths)
		{
			if(!ssvu::endsWith(s, ".json")) continue;
			ssvuj::Obj eRoot{ssvuj::getFromFile(s)};

			if(!ssvuj::hasObj(eRoot, "Entries")) appendEntry(eRoot);
			else for(const auto& v : eRoot["Entries"]) appendEntry(v);
		}
		for(const auto& s : asidePaths) main.addAside(ssvuj::getFromFile(s));
	}

	void appendEntry(const ssvuj::Obj& mRoot) { if(ssvuj::hasObj(mRoot, "MenuItems")) main.addMenu(mRoot); else main.addEntry(mRoot, myPath); }

	auto getResultPath() const { return Path{"Result/" + ssvuj::getExtr<std::string>(root, "fileName")}; }

	auto getOutput() const
	{
		Path resourcesPath{getResourcesFolderPath(getDepth(getResultPath()) - 1)};

		Dictionary dict;
		dict["MainMenu"] = mainMenu.getOutput();
		dict["Main"] = main.getOutput();
		dict["ResourcesPath"] = resourcesPath;
		return dict.getExpanded(ssvufs::Path{"Templates/page.tpl"}.getContentsAsString());
	}
};

// ---
std::vector<Page> pages;

void loadPages()
{
	ssvu::lo("loadPages") << "Getting all page.json files\n";

	Path pagesPath("Json/Pages/");
	std::vector<Path> pageJsonPaths{getScan<Mode::Recurse, Type::File, Pick::ByName>(pagesPath, "page.json")};

	for(const auto& s : pageJsonPaths)
	{
		ssvu::lo("loadPages") << "> " << s << "\n";
		pages.emplace_back(s, ssvuj::getFromFile(s));
	}
}

void expandPages()
{
	ssvu::lo("expandPages") << "Writing pages to result\n";

	for(auto& p : pages)
	{
		// Check path
		Path parentPath{p.getResultPath().getParent()};
		ssvu::lo("expandPages") << "Checking if path exists: " << parentPath << "\n";
		if(!exists(parentPath)) createFolder(parentPath);

		// Write page to file
		Path resultPath{p.getResultPath()};

		ssvu::lo("expandPages") << "> " << resultPath << "\n";
		std::ofstream o{resultPath + "temp"}; o << p.getOutput(); o.flush(); o.close();
		ssvu::lo() << "\n";

		std::ifstream inFile(resultPath + "temp");
		std::ofstream outFile(resultPath);

		std::string line;
		while(std::getline(inFile, line)) if(!line.empty()) outFile << line << "\n";

		inFile.close(); outFile.flush(); outFile.close();
		removeFile(p.getResultPath() + "temp");
	}
}

int main()
{
	SSVUT_RUN();
	loadPages(); ssvu::lo().flush();
	expandPages(); ssvu::lo().flush();
	return 0;
}
