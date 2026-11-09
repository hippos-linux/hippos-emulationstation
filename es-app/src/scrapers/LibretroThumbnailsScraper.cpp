#include "scrapers/LibretroThumbnailsScraper.h"

#include "FileData.h"
#include "HttpReq.h"
#include "PlatformId.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <map>

using namespace PlatformIds;

namespace
{
	const std::string THUMBNAILS_URL = "https://thumbnails.libretro.com/";

	const std::map<PlatformId, std::string> libretroSystems =
	{
		{ THREEDO, "The 3DO Company - 3DO" },
		{ AMIGA, "Commodore - Amiga" },
		{ AMSTRAD_CPC, "Amstrad - CPC" },
		{ ATARI_2600, "Atari - 2600" },
		{ ATARI_5200, "Atari - 5200" },
		{ ATARI_7800, "Atari - 7800" },
		{ ATARI_JAGUAR, "Atari - Jaguar" },
		{ ATARI_LYNX, "Atari - Lynx" },
		{ ATARI_ST, "Atari - ST" },
		{ COLECOVISION, "Coleco - ColecoVision" },
		{ COMMODORE_64, "Commodore - 64" },
		{ INTELLIVISION, "Mattel - Intellivision" },
		{ MSX, "Microsoft - MSX" },
		{ MSX2, "Microsoft - MSX2" },
		{ XBOX, "Microsoft - Xbox" },
		{ NEOGEO_CD, "SNK - Neo Geo CD" },
		{ NEOGEO_POCKET, "SNK - Neo Geo Pocket" },
		{ NEOGEO_POCKET_COLOR, "SNK - Neo Geo Pocket Color" },
		{ NINTENDO_3DS, "Nintendo - Nintendo 3DS" },
		{ NINTENDO_64, "Nintendo - Nintendo 64" },
		{ NINTENDO_DS, "Nintendo - Nintendo DS" },
		{ FAMICOM_DISK_SYSTEM, "Nintendo - Family Computer Disk System" },
		{ NINTENDO_ENTERTAINMENT_SYSTEM, "Nintendo - Nintendo Entertainment System" },
		{ GAME_BOY, "Nintendo - Game Boy" },
		{ GAME_BOY_ADVANCE, "Nintendo - Game Boy Advance" },
		{ GAME_BOY_COLOR, "Nintendo - Game Boy Color" },
		{ NINTENDO_GAMECUBE, "Nintendo - GameCube" },
		{ NINTENDO_WII, "Nintendo - Wii" },
		{ NINTENDO_VIRTUAL_BOY, "Nintendo - Virtual Boy" },
		{ NINTENDO_GAME_AND_WATCH, "Handheld Electronic Game" },
		{ POKEMINI, "Nintendo - Pokemon Mini" },
		{ SATELLAVIEW, "Nintendo - Satellaview" },
		{ SUFAMITURBO, "Nintendo - Sufami Turbo" },
		{ SUPER_NINTENDO, "Nintendo - Super Nintendo Entertainment System" },
		{ PC_98, "NEC - PC-98" },
		{ PCFX, "NEC - PC-FX" },
		{ TURBOGRAFX_16, "NEC - PC Engine - TurboGrafx 16" },
		{ TURBOGRAFX_CD, "NEC - PC Engine CD - TurboGrafx-CD" },
		{ SUPERGRAFX, "NEC - PC Engine SuperGrafx" },
		{ SEGA_32X, "Sega - 32X" },
		{ SEGA_CD, "Sega - Mega-CD - Sega CD" },
		{ SEGA_DREAMCAST, "Sega - Dreamcast" },
		{ SEGA_GAME_GEAR, "Sega - Game Gear" },
		{ SEGA_GENESIS, "Sega - Mega Drive - Genesis" },
		{ SEGA_MEGA_DRIVE, "Sega - Mega Drive - Genesis" },
		{ SEGA_MASTER_SYSTEM, "Sega - Master System - Mark III" },
		{ SEGA_PICO, "Sega - PICO" },
		{ SEGA_SATURN, "Sega - Saturn" },
		{ SEGA_SG1000, "Sega - SG-1000" },
		{ PLAYSTATION, "Sony - PlayStation" },
		{ PLAYSTATION_2, "Sony - PlayStation 2" },
		{ PLAYSTATION_3, "Sony - PlayStation 3" },
		{ PLAYSTATION_PORTABLE, "Sony - PlayStation Portable" },
		{ PLAYSTATION_VITA, "Sony - PlayStation Vita" },
		{ WONDERSWAN, "Bandai - WonderSwan" },
		{ WONDERSWAN_COLOR, "Bandai - WonderSwan Color" },
		{ ZX_SPECTRUM, "Sinclair - ZX Spectrum" },
		{ VIDEOPAC_ODYSSEY2, "Magnavox - Odyssey2" },
		{ VECTREX, "GCE - Vectrex" },
		{ PHILIPS_CDI, "Philips - CD-i" },
		{ WATARA_SUPERVISION, "Watara - Supervision" },
		{ CHANNELF, "Fairchild - Channel F" },
		{ SHARP_X6800, "Sharp - X68000" },
	};

	std::string systemFolder(SystemData* system)
	{
		for (auto platform : system->getPlatformIds())
		{
			auto it = libretroSystems.find(platform);
			if (it != libretroSystems.cend())
				return it->second;
		}
		return "";
	}

	std::string thumbnailFileName(const std::string& gameName)
	{
		std::string name = gameName;
		for (char& c : name)
			if (std::string("&*/:`<>?\\|\"").find(c) != std::string::npos)
				c = '_';
		return HttpReq::urlEncode(name) + ".png";
	}

	std::string folderFor(const std::string& mediaSource)
	{
		if (mediaSource == "box-2D" || mediaSource == "box-3D")
			return "Named_Boxarts";
		if (mediaSource == "sstitle")
			return "Named_Titles";
		if (mediaSource.empty())
			return "";
		return "Named_Snaps";
	}
}

void LibretroThumbnailsScraper::generateRequests(const ScraperSearchParams& params,
	std::queue<std::unique_ptr<ScraperRequest>>& requests, std::vector<ScraperSearchResult>& results)
{
	std::string folder = systemFolder(params.system);
	if (folder.empty())
		return;

	std::string name = params.nameOverride.empty() ? Utils::FileSystem::getStem(params.game->getPath()) : params.nameOverride;
	std::string systemUrl = THUMBNAILS_URL + HttpReq::urlEncode(folder);

	requests.push(std::unique_ptr<ScraperRequest>(
		new LibretroThumbnailsRequest(results, systemUrl, thumbnailFileName(name), params.game->getMetadata())));
}

bool LibretroThumbnailsScraper::isSupportedPlatform(SystemData* system)
{
	return system && !systemFolder(system).empty();
}

const std::set<Scraper::ScraperMediaSource>& LibretroThumbnailsScraper::getSupportedMedias()
{
	static std::set<ScraperMediaSource> medias = {
		ScraperMediaSource::Box2d,
		ScraperMediaSource::Screenshot,
		ScraperMediaSource::TitleShot,
	};
	return medias;
}

bool LibretroThumbnailsRequest::process(const std::string&, std::vector<ScraperSearchResult>& results)
{
	ScraperSearchResult result("Libretro Thumbnails");
	result.mdl = mMetadata;

	auto urlFor = [this](const std::string& mediaSource)
	{
		std::string folder = folderFor(mediaSource);
		return folder.empty() ? std::string() : mSystemUrl + "/" + folder + "/" + mFileName;
	};

	std::string image = urlFor(Settings::getInstance()->getString("ScrapperImageSrc"));
	if (!image.empty())
		result.urls[MetaDataId::Image] = ScraperSearchItem(image, ".png");

	std::string thumbnailSource = Settings::getInstance()->getString("ScrapperThumbSrc");
	if (thumbnailSource != Settings::getInstance()->getString("ScrapperImageSrc"))
	{
		std::string thumbnail = urlFor(thumbnailSource);
		if (!thumbnail.empty())
			result.urls[MetaDataId::Thumbnail] = ScraperSearchItem(thumbnail, ".png");
	}

	if (Settings::getInstance()->getBool("ScrapeTitleShot"))
		result.urls[MetaDataId::TitleShot] = ScraperSearchItem(urlFor("sstitle"), ".png");

	results.push_back(result);
	return true;
}
