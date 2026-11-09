#ifdef STEAMGRIDDB_APIKEY

#include "scrapers/SteamGridDBScraper.h"

#include "FileData.h"
#include "HttpReq.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

#include <rapidjson/document.h>

#include <algorithm>
#include <cctype>

using namespace rapidjson;

namespace
{
	const std::string API_URL = "https://www.steamgriddb.com/api/v2";
	const std::string IMAGE_FILTER = "mimes=image/png,image/jpeg";

	const std::set<std::string> pcSystems = { "steam", "lutris", "heroic", "windows", "dos", "ports" };

	std::string steamAppId(const ScraperSearchParams& params)
	{
		if (params.system->getName() != "steam")
			return "";

		std::string id = Utils::String::trim(Utils::FileSystem::readAllText(params.game->getPath()));
		bool numeric = !id.empty() && std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isdigit(c); });
		return numeric ? id : "";
	}

	const Value* gameObject(const Document& doc)
	{
		if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("data"))
			return nullptr;

		const Value& data = doc["data"];
		if (data.IsObject())
			return &data;
		if (data.IsArray() && data.Size() > 0 && data[0].IsObject())
			return &data[0];
		return nullptr;
	}

	std::string firstImageUrl(const std::string& response)
	{
		Document doc;
		doc.Parse(response.c_str());
		if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("data") || !doc["data"].IsArray())
			return "";

		for (const auto& image : doc["data"].GetArray())
			if (image.IsObject() && image.HasMember("url") && image["url"].IsString())
				return image["url"].GetString();
		return "";
	}

	bool isBoxSource(const std::string& mediaSource)
	{
		return mediaSource == "box-2D" || mediaSource == "box-3D";
	}
}

void SteamGridDBScraper::generateRequests(const ScraperSearchParams& params,
	std::queue<std::unique_ptr<ScraperRequest>>& requests, std::vector<ScraperSearchResult>& results)
{
	HttpReqOptions options;
	options.customHeaders.push_back("Authorization: Bearer " STEAMGRIDDB_APIKEY);

	std::string url;
	std::string appId = steamAppId(params);
	if (!appId.empty())
		url = API_URL + "/games/steam/" + appId;
	else
	{
		std::string name = params.nameOverride.empty() ? params.game->getName() : params.nameOverride;
		url = API_URL + "/search/autocomplete/" + HttpReq::urlEncode(Utils::String::removeParenthesis(name));
	}

	requests.push(std::unique_ptr<ScraperRequest>(new SteamGridDBRequest(results, url, &options, params.game->getMetadata())));
}

bool SteamGridDBScraper::isSupportedPlatform(SystemData* system)
{
	return system && pcSystems.count(system->getName()) > 0;
}

const std::set<Scraper::ScraperMediaSource>& SteamGridDBScraper::getSupportedMedias()
{
	static std::set<ScraperMediaSource> medias = {
		ScraperMediaSource::Box2d,
		ScraperMediaSource::Screenshot,
		ScraperMediaSource::Marquee,
		ScraperMediaSource::Wheel,
		ScraperMediaSource::FanArt,
	};
	return medias;
}

void SteamGridDBRequest::preProcess(const std::string& response)
{
	Document doc;
	doc.Parse(response.c_str());
	const Value* game = gameObject(doc);
	if (game == nullptr || !game->HasMember("id") || !(*game)["id"].IsInt())
		return;

	std::string id = std::to_string((*game)["id"].GetInt());
	mDependencyQueue.push({ "grid", API_URL + "/grids/game/" + id + "?dimensions=600x900,342x482,660x930&" + IMAGE_FILTER });
	mDependencyQueue.push({ "hero", API_URL + "/heroes/game/" + id + "?" + IMAGE_FILTER });
	mDependencyQueue.push({ "logo", API_URL + "/logos/game/" + id + "?mimes=image/png" });
}

bool SteamGridDBRequest::process(const std::string& response, std::vector<ScraperSearchResult>& results)
{
	Document doc;
	doc.Parse(response.c_str());
	const Value* game = gameObject(doc);
	if (game == nullptr)
	{
		LOG(LogDebug) << "SteamGridDB: no game found";
		return true;
	}

	ScraperSearchResult result("SteamGridDB");
	result.mdl = mMetadata;

	if (game->HasMember("name") && (*game)["name"].IsString())
		result.mdl.set(MetaDataId::Name, (*game)["name"].GetString());
	if (game->HasMember("release_date") && (*game)["release_date"].IsInt64())
		result.mdl.set(MetaDataId::ReleaseDate, Utils::Time::DateTime((time_t)(*game)["release_date"].GetInt64()));

	std::string grid = firstImageUrl(getDependencyResponse("grid"));
	std::string hero = firstImageUrl(getDependencyResponse("hero"));
	std::string logo = firstImageUrl(getDependencyResponse("logo"));

	auto artFor = [&](const std::string& mediaSource)
	{
		if (mediaSource.empty())
			return std::string();
		return isBoxSource(mediaSource) ? grid : hero;
	};

	auto addMedia = [&](MetaDataId id, const std::string& url)
	{
		if (!url.empty())
			result.urls[id] = ScraperSearchItem(url, Utils::FileSystem::getExtension(url));
	};

	std::string imageSource = Settings::getInstance()->getString("ScrapperImageSrc");
	std::string thumbnailSource = Settings::getInstance()->getString("ScrapperThumbSrc");
	addMedia(MetaDataId::Image, artFor(imageSource));
	if (thumbnailSource != imageSource)
		addMedia(MetaDataId::Thumbnail, artFor(thumbnailSource));
	if (!Settings::getInstance()->getString("ScrapperLogoSrc").empty())
		addMedia(MetaDataId::Marquee, logo);
	if (Settings::getInstance()->getBool("ScrapeFanart"))
		addMedia(MetaDataId::FanArt, hero);

	results.push_back(result);
	return true;
}

#endif // STEAMGRIDDB_APIKEY
