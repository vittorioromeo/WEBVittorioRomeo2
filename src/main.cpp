// Copyright (c) 2013-2014 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include <SSVUtils/Core/Core.hpp>
#include <SSVUtils/Json/Json.hpp>
#include <SSVUtils/TemplateSystem/TemplateSystem.hpp>
#include <SSVUtils/Test/Test.hpp>
#include <SSVUtils/Tests/TestsJson.hpp>
#include <DiscountCpp/DiscountCpp.hpp>

using namespace ssvu::FileSystem;
using namespace ssvu::TemplateSystem;
using namespace ssvu::Json;

auto getDepth(const Path& mPath) { return ssvu::getCharCount(mPath, '/'); }

auto getResourcesFolderPath(std::size_t mDepth)
{
	Path result;
	for(auto i(0u); i < mDepth; ++i) result += "../";
	return result + "Resources";
}

auto getDictionaryFromJson(const Val& mVal)
{
	Dictionary result;
	mVal.forObjAs<Str>([&result](const auto& mKey, const auto& mVal){ result[mKey] = mVal; });
	// TODO ?: for(const auto& p : mVal.forObjAs<std::string>()) result[p.key] = p.value;
	return result;
}

struct MainMenu
{
	Val root;

	auto getOutput() const
	{
		Dictionary dict;
		root["MenuItems"].forArr([&dict](const auto& mVal){ dict += {"MenuItems", getDictionaryFromJson(mVal)}; });
		// TODO ?: for(const auto& i : root["MenuItems"].forArr()) dict += {"MenuItems", getDictionaryFromJson(i)};
		return dict.getExpanded(Path{"Templates/Base/mainMenu.tpl"}.getContentsAsString());
	}
};

struct Main
{
	std::vector<std::string> expandedEntries{""}, expandedAsides{""};

	void expandItem(const Path& mTplPath, Val mRoot, std::vector<std::string>& mTarget, const Path& mPagePath = "")
	{
		if(mRoot.has("Markdown"))
		{
			Path mdPath{mPagePath.getParent() + "Entries/" + mRoot["Markdown"].as<Str>()};
			mRoot["Text"] = discountcpp::getHTMLFromMarkdownFile(mdPath);
		}

		mTarget.emplace_back(getDictionaryFromJson(mRoot).getExpanded(mTplPath.getContentsAsString()));
	}

	void addEntry(const Val& mRoot, const Path& mPagePath)
	{
		expandItem(mRoot["Template"].as<Str>(), mRoot["ToExpand"], expandedEntries, mPagePath);
	}
	void addAside(const Val& mRoot)
	{
		expandItem(mRoot["Template"].as<Str>(), mRoot["ToExpand"], expandedAsides);
	}
	void addMenu(const Val& mRoot)
	{
		Dictionary dict;
		mRoot["MenuItems"].forArr([&dict](const auto& mVal){ dict += {"MenuItems", getDictionaryFromJson(mVal)}; });
		expandedEntries.emplace_back(dict.getExpanded(Path{"Templates/Entries/menu.tpl"}.getContentsAsString()));
	}

	auto getOutput() const
	{
		Dictionary dict;
		for(const auto& e : expandedEntries) dict += {"Entries", {{"Entry", e}}};
		for(const auto& a : expandedAsides) dict += {"Asides", {{"Aside", a}}};
		return dict.getExpanded(Path{"Templates/Base/main.tpl"}.getContentsAsString());
	}
};

struct Page
{
	Path myPath;
	Val root;

	MainMenu mainMenu{Val::fromFile("Json/mainMenu.json")};
	Main main;

	Page(const Path& mPath, const Val& mRoot) : myPath{mPath}, root{mRoot}
	{
		Path pageFolder{myPath.getParent()};
		Path entriesFolder{pageFolder + "Entries/"}, asidesFolder{pageFolder + "Asides/"};

		std::vector<Path> entryPaths{getScan<Mode::Recurse, Type::File>(entriesFolder)};

		std::vector<Path> asidePaths;
		if(asidesFolder.exists<Type::Folder>()) asidePaths = getScan<Mode::Recurse, Type::File>(asidesFolder);

		for(const auto& s : entryPaths)
		{
			if(!ssvu::endsWith(s, ".json")) continue;
			auto eRoot(Val::fromFile(s));

			if(!eRoot.has("Entries")) appendEntry(eRoot);
			else eRoot["Entries"].forArr([this](const auto& mEntry){ this->appendEntry(mEntry); });
		}
		for(const auto& s : asidePaths) main.addAside(Val::fromFile(s));
	}

	void appendEntry(const Val& mRoot) { if(mRoot.has("MenuItems")) main.addMenu(mRoot); else main.addEntry(mRoot, myPath); }

	auto getResultPath() const { return Path{"Result/" + root["fileName"].as<Str>()}; }

	auto getOutput() const
	{
		Path resourcesPath{getResourcesFolderPath(getDepth(getResultPath()) - 1)};

		Dictionary dict;
		dict["MainMenu"] = mainMenu.getOutput();
		dict["Main"] = main.getOutput();
		dict["ResourcesPath"] = resourcesPath;
		return dict.getExpanded(Path{"Templates/page.tpl"}.getContentsAsString());
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
		pages.emplace_back(s, Val::fromFile(s));
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
		if(!parentPath.exists<Type::Folder>()) createFolder(parentPath);

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
