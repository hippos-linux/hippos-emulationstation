#pragma once
#ifndef ES_APP_SCRAPERS_STEAMGRIDDB_SCRAPER_H
#define ES_APP_SCRAPERS_STEAMGRIDDB_SCRAPER_H

#include "scrapers/Scraper.h"

class SteamGridDBScraper : public Scraper
{
public:
	void generateRequests(const ScraperSearchParams& params,
		std::queue<std::unique_ptr<ScraperRequest>>& requests,
		std::vector<ScraperSearchResult>& results) override;

	bool isSupportedPlatform(SystemData* system) override;

	const std::set<ScraperMediaSource>& getSupportedMedias() override;
};

class SteamGridDBRequest : public ScraperHttpRequest
{
public:
	SteamGridDBRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& url,
		HttpReqOptions* options, const MetaDataList& metadata)
		: ScraperHttpRequest(resultsWrite, url, options), mMetadata(metadata)
	{
	}

protected:
	void preProcess(const std::string& response) override;
	bool process(const std::string& response, std::vector<ScraperSearchResult>& results) override;

private:
	MetaDataList mMetadata;
};

#endif // ES_APP_SCRAPERS_STEAMGRIDDB_SCRAPER_H
