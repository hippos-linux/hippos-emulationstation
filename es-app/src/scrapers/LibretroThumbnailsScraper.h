#pragma once
#ifndef ES_APP_SCRAPERS_LIBRETRO_THUMBNAILS_SCRAPER_H
#define ES_APP_SCRAPERS_LIBRETRO_THUMBNAILS_SCRAPER_H

#include "scrapers/Scraper.h"

class LibretroThumbnailsScraper : public Scraper
{
public:
	void generateRequests(const ScraperSearchParams& params,
		std::queue<std::unique_ptr<ScraperRequest>>& requests,
		std::vector<ScraperSearchResult>& results) override;

	bool isSupportedPlatform(SystemData* system) override;

	const std::set<ScraperMediaSource>& getSupportedMedias() override;
};

class LibretroThumbnailsRequest : public ScraperHttpRequest
{
public:
	LibretroThumbnailsRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& systemUrl,
		const std::string& fileName, const MetaDataList& metadata)
		: ScraperHttpRequest(resultsWrite, systemUrl + "/Named_Boxarts/" + fileName),
		mSystemUrl(systemUrl), mFileName(fileName), mMetadata(metadata)
	{
	}

protected:
	bool process(const std::string& response, std::vector<ScraperSearchResult>& results) override;

private:
	std::string mSystemUrl;
	std::string mFileName;
	MetaDataList mMetadata;
};

#endif // ES_APP_SCRAPERS_LIBRETRO_THUMBNAILS_SCRAPER_H
