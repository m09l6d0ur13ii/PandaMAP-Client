#include <game/client/animstate.h>
#include <game/client/components/chat.h>
#include <game/client/gameclient.h>

#include <game/localization.h>
#include <game/version.h>
#include <generated/protocol.h>

#include <engine/shared/config.h>
#include <engine/shared/json.h>

#include "rclient.h"

CRClient::CRClient()
{
	OnReset();

	// Initialize server info collection variables
	m_aCurrentServerAddress[0] = '\0';
}

void CRClient::OnInit()
{
	FetchRclientVersionCheck();
	RiSplitRegex();
	FetchAuthToken();
}

void CRClient::OnConsoleInit()
{
	Console()->Register("ri_find_player_from_ddstats", "s[type]", CFGFLAG_CLIENT, ConFindPlayerFromDdstats, this, "Fetch player from DDstats");
	Console()->Register("ri_copy_player_from_ddstats", "s[type]", CFGFLAG_CLIENT, ConCopyPlayerFromDdstats, this, "Fetch and copy player from DDstats");
	Console()->Register("ri_find_skin_from_ddstats", "s[type]", CFGFLAG_CLIENT, ConFindSkinFromDdstats, this, "Fetch player's skin from DDstats");
	Console()->Register("ri_copy_skin_from_ddstats", "s[type]", CFGFLAG_CLIENT, ConCopySkinFromDdstats, this, "Fetch and copy player's skin from DDstats");
	Console()->Register("ri_backup_player_profile", "", CFGFLAG_CLIENT, ConBackupPlayerProfile, this, "Backup player profile");
	Console()->Register("ri_tracker_spectator", "", CFGFLAG_CLIENT, ConSpectatorAddTracker, this, "Backup player profile");
	Console()->Register("ri_find_time_on_map", "s[nickname] s[map] ?s[map1] ?s[map2] ?s[map3] ?s[map4] ?s[map5]", CFGFLAG_CLIENT, ConFindTimeMap, this, "Search time on map. Example: ri_find_time_on_map \"[D] Voix\" Grandma");
	Console()->Register("ri_search_map_info", "s[map] ?s[map1] ?s[map2] ?s[map3] ?s[map4] ?s[map5]", CFGFLAG_CLIENT, ConFindMapInfo, this, "Search time on map. Example: ri_search_map_info Grandma");
	Console()->Register("find_hours", "s[nickname] ?s[Wchatflag]", CFGFLAG_CLIENT, ConFindHours, this, "Find hours");
	Console()->Register("+ri_45_degrees", "", CFGFLAG_CLIENT, ConToggle45Degrees, this, "45° bind");
	Console()->Register("+ri_small_sens", "", CFGFLAG_CLIENT, ConToggleSmallSens, this, "Small sens bind");
	Console()->Register("ri_deepfly_toggle", "", CFGFLAG_CLIENT, ConToggleDeepfly, this, "Deep fly toggle");
	Console()->Register("ri_nameplates_editor_update", "", CFGFLAG_CLIENT, ConUpdateNameplatesEditor, this, "Update nameplates. Use after change ri_nameplate_scheme");
	Console()->Register("add_white_list", "s[nickname]", CFGFLAG_CLIENT, ConAddWhiteList, this, "Add player to white list of censor list");
	Console()->Register("ri_update_regex_list_ignore", "s[nickname]", CFGFLAG_CLIENT, ConUpdateRegexIgnore, this, "Add player to white list of censor list");
	Console()->Register("find_skin", "r[player]", CFGFLAG_CLIENT, ConFindSkin, this, "Find skin");
	Console()->Register("copy_skin", "r[player]", CFGFLAG_CLIENT, ConCopySkin, this, "Copy skin");
	Console()->Register("find_player", "r[player]", CFGFLAG_CLIENT, ConFindPlayer, this, "Find Player");
	Console()->Register("copy_player", "r[player]", CFGFLAG_CLIENT, ConCopyPlayer, this, "Copy Player");
	Console()->Register("copy_color", "r[player]", CFGFLAG_CLIENT, ConCopyColor, this, "Copy Color skin");
	Console()->Register("tracker", "r[player]", CFGFLAG_CLIENT, ConTargetPlayerPos, this, "Track player pos");
	Console()->Register("tracker_reset", "", CFGFLAG_CLIENT, ConTargetPlayerPosReset, this, "Reset tracker pos");
	Console()->Register("tracker_remove", "r[player]", CFGFLAG_CLIENT, ConTargetPlayerPosRemove, this, "Remove tracker pos of player");
	Console()->Register("add_censor_list", "r[word]", CFGFLAG_CLIENT, ConAddCensorList, this, "Reset tracker pos");
}

void CRClient::OnRender()
{
	if(m_pRClientDDstatsTask)
	{
		if(m_pRClientDDstatsTask && m_pRClientDDstatsTask->State() == EHttpState::DONE)
		{
			FinishRclientDDstatsProfile();
			ResetRclientDDstatsProfile();
		}
	}
	if(m_pRClientVersionCheck)
	{
		if(m_pRClientVersionCheck && m_pRClientVersionCheck->State() == EHttpState::DONE)
		{
			FinishRclientVersionCheck();
			ResetRclientVersionCheck();
		}
	}
	if(m_pAuthTokenTask)
	{
		if(m_pAuthTokenTask->State() == EHttpState::DONE)
		{
			FinishAuthToken();
			ResetAuthToken();
		}
	}
	if(m_pSearchRankOnMapTask)
	{
		if(m_pSearchRankOnMapTask && m_pSearchRankOnMapTask->State() == EHttpState::DONE)
		{
			FinishSearchRankOnMap();
			ResetSearchRankOnMap();
		}
	}
	if(m_pSearchMapInfoTask)
	{
		if(m_pSearchMapInfoTask && m_pSearchMapInfoTask->State() == EHttpState::DONE)
		{
			FinishSearchMapInfo();
			ResetSearchMapInfo();
		}
	}

	if(m_pFindHoursTask)
	{
		if(m_pFindHoursTask && m_pFindHoursTask->State() == EHttpState::DONE)
		{
			FinishFindHours();
			ResetFindHours();
		}
	}
	if(GameClient()->m_Menus.m_RPC_Ratelimit < time_get() && (GameClient()->m_Menus.m_RPC_Ratelimit - time_get()) / time_freq() > -1)
	{
		Client()->DiscordRPCchange();
		GameClient()->m_Menus.m_RPC_Ratelimit = -2;
	}

	// Handle server info collection
	if(m_pRClientUsersTask)
	{
		if(m_pRClientUsersTask->State() == EHttpState::DONE)
		{
			FinishRClientUsers();
			ResetRClientUsers();
		}
	}

	if(m_pRClientUsersTaskSend)
	{
		if(m_pRClientUsersTaskSend->State() == EHttpState::DONE)
		{
			// FinishRClientUsersSend();
			ResetRClientUsersSend();
		}
	}

	// Do initial fetch when first connected
	if(Client()->State() == IClient::STATE_ONLINE && !s_InitialFetchDone && g_Config.m_RiShowRclientIndicator)
	{
		s_InitialFetchDone = true;
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "RClient", "Send/Get RClient Players for indicator on init");
		SendServerPlayerInfo();
		FetchRClientUsers();
		s_LastFetch = time_get();
	}
	else if(Client()->State() != IClient::STATE_ONLINE && g_Config.m_RiShowRclientIndicator)
	{
		s_InitialFetchDone = false;
		s_InitialFetchDoneDummy = false; // Reset when disconnected
	}

	if(Client()->State() == IClient::STATE_ONLINE && (!m_pRClientUsersTask || m_pRClientUsersTask->Done()) && g_Config.m_RiShowRclientIndicator)
	{
		if(time_get() - s_LastFetch > time_freq() * 30)
		{
			if(s_RclientIndicatorCount == 10)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "RClient", "Send/Get RClient Players for indicator x10");
				s_RclientIndicatorCount = 0;
			}
			else
				s_RclientIndicatorCount++;
			SendServerPlayerInfo();
			FetchRClientUsers();
			s_LastFetch = time_get();
		}
	}
}

void CRClient::FetchRclientDDstatsProfile()
{
	if(m_pRClientDDstatsTask && !m_pRClientDDstatsTask->Done())
	{
		return;
	}
	char aUrl[256];
	char aEncodedNickname[256];
	// URL encode the nickname
	EscapeUrl(aEncodedNickname, sizeof(aEncodedNickname), RclientSearchingNickname);
	str_format(aUrl, sizeof(aUrl), "https://ddstats.tw/profile/json?player=%s", aEncodedNickname);
	m_pRClientDDstatsTask = HttpGet(aUrl);
	m_pRClientDDstatsTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pRClientDDstatsTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pRClientDDstatsTask);
}
void CRClient::FinishRclientDDstatsProfile()
{
	json_value *pJson = m_pRClientDDstatsTask->ResultJson();
	if(!pJson)
	{
		GameClient()->Echo("No that player");
		// Reset all DDstats search flags if JSON parsing fails
		RclientFindPlayerDDstatsSearch = 0;
		RclientCopyPlayerDDstatsSearch = 0;
		RclientFindSkinDDstatsSearch = 0;
		RclientCopySkinDDstatsSearch = 0;
		return;
	}
	const json_value &Json = *pJson;
	const json_value &Nickname = Json["name"];
	const json_value &Clan = Json["clan"];
	const json_value &Country = Json["country"];
	const json_value &Skin = Json["skin_name"];
	const json_value &Skin_color_body = Json["skin_color_body"];
	const json_value &Skin_color_feet = Json["skin_color_feet"];

	if(Nickname.type == json_string)
	{
		char aCountry[32];
		char aBodycolor[32];
		char aFeetcolor[32];
		char aBuf[32];
		int Countryint = Country.u.integer;
		int Skin_color_bodyint = Skin_color_body.u.integer;
		int Skin_color_feetint = Skin_color_feet.u.integer;
		str_format(aCountry, sizeof(aCountry), "%d", Countryint);
		str_format(aBodycolor, sizeof(aBodycolor), "%d", Skin_color_bodyint);
		str_format(aFeetcolor, sizeof(aFeetcolor), "%d", Skin_color_feetint);
		int CustomColor = (str_length(aFeetcolor) > 1 ? 1 : 0);
		if(RclientFindPlayerDDstatsSearch == 1)
		{
			RclientFindPlayerDDstatsSearch = 0;
			str_format(aBuf, sizeof(aBuf), "- Nickname: %s", Nickname.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", Skin.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Clan: %s", Clan.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Country: %s", aCountry);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			if(CustomColor)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 1");
				str_format(aBuf, sizeof(aBuf), "- Body Color: %s", aBodycolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
				str_format(aBuf, sizeof(aBuf), "- Feet Color: %s", aFeetcolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			}
			else
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 0");
		}
		if(RclientCopyPlayerDDstatsSearch == 1)
		{
			RclientCopyPlayerDDstatsSearch = 0;
			str_format(aBuf, sizeof(aBuf), "- Nickname: %s", Nickname.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", Skin.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Clan: %s", Clan.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			str_format(aBuf, sizeof(aBuf), "- Country: %s", aCountry);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			if(CustomColor)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 1");
				str_format(aBuf, sizeof(aBuf), "- Body Color: %s", aBodycolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
				str_format(aBuf, sizeof(aBuf), "- Feet Color: %s", aFeetcolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			}
			else
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 0");
			if(g_Config.m_ClDummy == 1)
			{
				str_copy(DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(DummySkinBeforeCopyPlayer));
				str_copy(DummyNameBeforeCopyPlayer, g_Config.m_ClDummyName, sizeof(DummyNameBeforeCopyPlayer));
				str_copy(DummyClanBeforeCopyPlayer, g_Config.m_ClDummyClan, sizeof(DummyClanBeforeCopyPlayer));
				DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				DummyCountryBeforeCopyPlayer = g_Config.m_ClDummyCountry;
				str_copy(g_Config.m_ClDummySkin, Skin.u.string.ptr, sizeof(g_Config.m_ClDummySkin));
				str_copy(g_Config.m_ClDummyName, Nickname.u.string.ptr, sizeof(g_Config.m_ClDummyName));
				str_copy(g_Config.m_ClDummyClan, Clan.u.string.ptr, sizeof(g_Config.m_ClDummyClan));
				g_Config.m_ClDummyUseCustomColor = CustomColor;
				g_Config.m_ClDummyColorBody = Skin_color_bodyint;
				g_Config.m_ClDummyColorFeet = Skin_color_feetint;
				g_Config.m_ClDummyCountry = Countryint;
				GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(PlayerSkinBeforeCopyPlayer));
				str_copy(PlayerNameBeforeCopyPlayer, g_Config.m_PlayerName, sizeof(PlayerNameBeforeCopyPlayer));
				str_copy(PlayerClanBeforeCopyPlayer, g_Config.m_PlayerClan, sizeof(PlayerClanBeforeCopyPlayer));
				PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				PlayerCountryBeforeCopyPlayer = g_Config.m_PlayerCountry;
				str_copy(g_Config.m_PlayerName, Nickname.u.string.ptr, sizeof(g_Config.m_PlayerName));
				str_copy(g_Config.m_PlayerClan, Clan.u.string.ptr, sizeof(g_Config.m_PlayerClan));
				str_copy(g_Config.m_ClPlayerSkin, Skin.u.string.ptr, sizeof(g_Config.m_ClPlayerSkin));
				g_Config.m_PlayerCountry = Countryint;
				g_Config.m_ClPlayerUseCustomColor = CustomColor;
				g_Config.m_ClPlayerColorBody = Skin_color_bodyint;
				g_Config.m_ClPlayerColorFeet = Skin_color_feetint;
				GameClient()->SendInfo(false);
			}
		}
		if(RclientFindSkinDDstatsSearch == 1)
		{
			RclientFindSkinDDstatsSearch = 0;
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", Skin.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			if(CustomColor)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 1");
				str_format(aBuf, sizeof(aBuf), "- Body Color: %s", aBodycolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
				str_format(aBuf, sizeof(aBuf), "- Feet Color: %s", aFeetcolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			}
			else
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 0");
		}
		if(RclientCopySkinDDstatsSearch == 1)
		{
			RclientCopySkinDDstatsSearch = 0;
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", Skin.u.string.ptr);
			GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			if(CustomColor)
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 1");
				str_format(aBuf, sizeof(aBuf), "- Body Color: %s", aBodycolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
				str_format(aBuf, sizeof(aBuf), "- Feet Color: %s", aFeetcolor);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", aBuf);
			}
			else
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "DDstats", "- Custom Color: 0");
			if(g_Config.m_ClDummy == 1)
			{
				str_copy(DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(DummySkinBeforeCopyPlayer));
				str_copy(DummyNameBeforeCopyPlayer, g_Config.m_ClDummyName, sizeof(DummyNameBeforeCopyPlayer));
				str_copy(DummyClanBeforeCopyPlayer, g_Config.m_ClDummyClan, sizeof(DummyClanBeforeCopyPlayer));
				DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				DummyCountryBeforeCopyPlayer = g_Config.m_ClDummyCountry;
				str_copy(g_Config.m_ClDummySkin, Skin.u.string.ptr, sizeof(g_Config.m_ClDummySkin));
				g_Config.m_ClDummyUseCustomColor = CustomColor;
				g_Config.m_ClDummyColorBody = Skin_color_bodyint;
				g_Config.m_ClDummyColorFeet = Skin_color_feetint;
				GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(PlayerSkinBeforeCopyPlayer));
				str_copy(PlayerNameBeforeCopyPlayer, g_Config.m_PlayerName, sizeof(PlayerNameBeforeCopyPlayer));
				str_copy(PlayerClanBeforeCopyPlayer, g_Config.m_PlayerClan, sizeof(PlayerClanBeforeCopyPlayer));
				PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				PlayerCountryBeforeCopyPlayer = g_Config.m_PlayerCountry;
				str_copy(g_Config.m_ClPlayerSkin, Skin.u.string.ptr, sizeof(g_Config.m_ClPlayerSkin));
				g_Config.m_ClPlayerUseCustomColor = CustomColor;
				g_Config.m_ClPlayerColorBody = Skin_color_bodyint;
				g_Config.m_ClPlayerColorFeet = Skin_color_feetint;
				GameClient()->SendInfo(false);
			}
		}
	}
	json_value_free(pJson);
}
void CRClient::ResetRclientDDstatsProfile()
{
	if(m_pRClientDDstatsTask)
	{
		m_pRClientDDstatsTask->Abort();
		m_pRClientDDstatsTask = NULL;
	}
}
void CRClient::ConFindPlayerFromDdstats(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pThis->RclientFindPlayerDDstatsSearch = 1;
	str_copy(pThis->RclientSearchingNickname, aInput, sizeof(aInput));
	pThis->FetchRclientDDstatsProfile();
}
void CRClient::ConCopyPlayerFromDdstats(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pThis->RclientCopyPlayerDDstatsSearch = 1;
	str_copy(pThis->RclientSearchingNickname, aInput, sizeof(aInput));
	pThis->FetchRclientDDstatsProfile();
}
void CRClient::ConFindSkinFromDdstats(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pThis->RclientFindSkinDDstatsSearch = 1;
	str_copy(pThis->RclientSearchingNickname, aInput, sizeof(aInput));
	pThis->FetchRclientDDstatsProfile();
}
void CRClient::ConCopySkinFromDdstats(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	pThis->RclientCopySkinDDstatsSearch = 1;
	str_copy(pThis->RclientSearchingNickname, aInput, sizeof(aInput));
	pThis->FetchRclientDDstatsProfile();
}

bool CRClient::NeedUpdate()
{
	return str_comp(m_aVersionStr, "0") != 0;
}

void CRClient::FetchRclientVersionCheck()
{
	if(m_pRClientVersionCheck && !m_pRClientVersionCheck->Done())
		return;
	char aUrl[256];
	str_copy(aUrl, RCLIENT_VERSION_URL);
	m_pRClientVersionCheck = HttpGet(aUrl);
	m_pRClientVersionCheck->Timeout(CTimeout{20000, 0, 500, 10});
	m_pRClientVersionCheck->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pRClientVersionCheck);
}
void CRClient::ResetRclientVersionCheck()
{
	if(m_pRClientVersionCheck)
	{
		m_pRClientVersionCheck->Abort();
		m_pRClientVersionCheck = NULL;
	}
}
typedef std::tuple<int, int, int> TVersion;
static const TVersion gs_InvalidTCVersion = std::make_tuple(-1, -1, -1);
static TVersion ToTCVersion(char *pStr)
{
	int aVersion[3] = {0, 0, 0};
	const char *p = strtok(pStr, ".");

	for(int i = 0; i < 3 && p; ++i)
	{
		if(!str_isallnum(p))
			return gs_InvalidTCVersion;

		aVersion[i] = str_toint(p);
		p = strtok(NULL, ".");
	}

	if(p)
		return gs_InvalidTCVersion;

	return std::make_tuple(aVersion[0], aVersion[1], aVersion[2]);
}
void CRClient::FinishRclientVersionCheck()
{
	json_value *pJson = m_pRClientVersionCheck->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &CurrentVersion = Json["version"];

	if(CurrentVersion.type == json_string)
	{
		char aNewVersionStr[64];
		str_copy(aNewVersionStr, CurrentVersion);
		char aCurVersionStr[64];
		str_copy(aCurVersionStr, RCLIENT_VERSION);
		if(ToTCVersion(aNewVersionStr) > ToTCVersion(aCurVersionStr))
		{
			str_copy(m_aVersionStr, CurrentVersion);
		}
		else
		{
			m_aVersionStr[0] = '0';
			m_aVersionStr[1] = '\0';
		}
	}
	json_value_free(pJson);
}

//Backup player profile
void CRClient::ConBackupPlayerProfile(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	if(g_Config.m_ClDummy == 1)
	{
		if(str_length(pSelf->DummySkinBeforeCopyPlayer) > 0 || str_length(pSelf->DummyNameBeforeCopyPlayer) > 0)
		{
			str_copy(g_Config.m_ClDummySkin, pSelf->DummySkinBeforeCopyPlayer, sizeof(g_Config.m_ClDummySkin));
			str_copy(g_Config.m_ClDummyName, pSelf->DummyNameBeforeCopyPlayer, sizeof(g_Config.m_ClDummyName));
			str_copy(g_Config.m_ClDummyClan, pSelf->DummyClanBeforeCopyPlayer, sizeof(g_Config.m_ClDummyClan));
			g_Config.m_ClDummyUseCustomColor = pSelf->DummyUseCustomColorBeforeCopyPlayer;
			g_Config.m_ClDummyColorBody = pSelf->DummyBodyColorBeforeCopyPlayer;
			g_Config.m_ClDummyColorFeet = pSelf->DummyFeetColorBeforeCopyPlayer;
			g_Config.m_ClDummyCountry = pSelf->DummyCountryBeforeCopyPlayer;
			pSelf->GameClient()->SendDummyInfo(false);
		}
		else
		{
			pSelf->GameClient()->Echo("There no info of player/skin copy");
		}
	}
	if(g_Config.m_ClDummy == 0)
	{
		if(str_length(pSelf->PlayerSkinBeforeCopyPlayer) > 0 || str_length(pSelf->PlayerNameBeforeCopyPlayer) > 0)
		{
			str_copy(g_Config.m_ClPlayerSkin, pSelf->PlayerSkinBeforeCopyPlayer, sizeof(g_Config.m_ClPlayerSkin));
			str_copy(g_Config.m_PlayerName, pSelf->PlayerNameBeforeCopyPlayer, sizeof(g_Config.m_PlayerName));
			str_copy(g_Config.m_PlayerClan, pSelf->PlayerClanBeforeCopyPlayer, sizeof(g_Config.m_PlayerClan));
			g_Config.m_ClPlayerUseCustomColor = pSelf->PlayerUseCustomColorBeforeCopyPlayer;
			g_Config.m_ClPlayerColorBody = pSelf->PlayerBodyColorBeforeCopyPlayer;
			g_Config.m_ClPlayerColorFeet = pSelf->PlayerFeetColorBeforeCopyPlayer;
			g_Config.m_PlayerCountry = pSelf->PlayerCountryBeforeCopyPlayer;
			pSelf->GameClient()->SendInfo(false);
		}
		else
		{
			pSelf->GameClient()->Echo("There no info of player/skin copy");
		}
	}
}

// Find map rank
void CRClient::ConFindTimeMap(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	const char *Nickname = pResult->GetString(0);

	char aMapName[256] = {0};
	for(int i = 1; i < pResult->NumArguments(); i++)
	{
		if(i > 1)
			str_append(aMapName, " ", sizeof(aMapName));
		str_append(aMapName, pResult->GetString(i), sizeof(aMapName));
	}

	str_copy(pThis->NicknameForSearch, Nickname, sizeof(pThis->NicknameForSearch));
	str_copy(pThis->MapForSearch, aMapName, sizeof(pThis->MapForSearch));
	pThis->FetchSearchRankOnMap();
}

void CRClient::FetchSearchRankOnMap()
{
	if(m_pSearchRankOnMapTask && !m_pSearchRankOnMapTask->Done())
		return;
	char aUrl[256];
	char aEncodedNickname[256];

	// URL encode the nickname
	EscapeUrl(aEncodedNickname, sizeof(aEncodedNickname), NicknameForSearch);
	str_format(aUrl, sizeof(aUrl), "https://ddstats.tw/player/json?player=%s", aEncodedNickname);
	m_pSearchRankOnMapTask = HttpGet(aUrl);
	m_pSearchRankOnMapTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pSearchRankOnMapTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pSearchRankOnMapTask);
}
void CRClient::FinishSearchRankOnMap()
{
	json_value *pJson = m_pSearchRankOnMapTask->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &Finishes = Json["finishes"];
	bool bFound = false;
	for(unsigned int i = 0; i < Finishes.u.object.length; ++i)
	{
		const json_value &Finish = Finishes[i];
		const json_value &MapInfo = Finish["map"];
		const json_value &MapName = MapInfo["map"];
		const char *pMapName = MapName;
		if(str_find_nocase(pMapName, MapForSearch))
		{
			const json_value &Time = Finish["time"];
			char aBuf[128];
			char TimeStr[32];
			if(Time.type == json_double || Time.type == json_integer)
			{
				double FinishTimeSec = (Time.type == json_double) ? Time.u.dbl : (double)Time.u.integer;
				int hours = static_cast<int>(FinishTimeSec) / 3600;
				int minutes = (static_cast<int>(FinishTimeSec) % 3600) / 60;
				int seconds = static_cast<int>(FinishTimeSec) % 60;
				str_format(TimeStr, sizeof(TimeStr), "%02d:%02d:%02d", hours, minutes, seconds);
				str_format(aBuf, sizeof(aBuf), "%s finished %s for %s", NicknameForSearch, pMapName, TimeStr);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
				GameClient()->Echo(aBuf);
			}
			else
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", "Finish time not found");
				GameClient()->Echo("Finish time not found");
			}
			bFound = true;
			break;
		}
		if(bFound)
			break;
	}
	if(!bFound)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Map '%s' not found for %s", MapForSearch, NicknameForSearch);
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
		GameClient()->Echo(aBuf);
	}
	json_value_free(pJson);
}
void CRClient::ResetSearchRankOnMap()
{
	if(m_pSearchRankOnMapTask)
	{
		m_pSearchRankOnMapTask->Abort();
		m_pSearchRankOnMapTask = NULL;
	}
}

// Search map info
void CRClient::ConFindMapInfo(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	char aMapName[256] = {0};
	for(int i = 0; i < pResult->NumArguments(); i++)
	{
		if(i > 0)
			str_append(aMapName, " ", sizeof(aMapName));
		str_append(aMapName, pResult->GetString(i), sizeof(aMapName));
	}

	str_copy(pThis->MapForSearchMapInfo, aMapName, sizeof(pThis->MapForSearchMapInfo));
	pThis->FetchSearchMapInfo();
}

void CRClient::FetchSearchMapInfo()
{
	if(m_pSearchMapInfoTask && !m_pSearchMapInfoTask->Done())
		return;
	char aUrl[256];

	// URL encode the nickname
	str_copy(aUrl, "https://ddstats.tw/maps/json");
	m_pSearchMapInfoTask = HttpGet(aUrl);
	m_pSearchMapInfoTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pSearchMapInfoTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pSearchMapInfoTask);
}
void CRClient::FinishSearchMapInfo()
{
	json_value *pJson = m_pSearchMapInfoTask->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &Maps = Json;
	bool bFound = false;
	for(unsigned int i = 0; i < Maps.u.object.length; ++i)
	{
		const json_value &Map = Maps[i];
		const char *pMapName = Map["map"];
		if(str_find_nocase(pMapName, MapForSearchMapInfo))
		{
			const json_value &Points = Map["points"];
			const json_value &Stars = Map["stars"];
			const json_value &Difficulty = Map["server"];
			const char *pDifficulty = (Difficulty.type == json_string) ? Difficulty.u.string.ptr : "Unknown";
			char aBuf[256];
			if(Points.type == json_integer && Stars.type == json_integer)
			{
				int PointsFormated = Points.u.integer;
				int StarsFormated = Stars.u.integer;
				int StarsFormatedMinus = 5 - Stars.u.integer;
				char aStars[16] = {0};
				for(int s = 0; s < StarsFormated; s++)
					str_append(aStars, "★", sizeof(aStars));
				for(int s = 0; s < StarsFormatedMinus; s++)
					str_append(aStars, "✰", sizeof(aStars));
				str_format(aBuf, sizeof(aBuf), "Map: %s, Points: %i, Difficulty: %s %s", pMapName, PointsFormated, pDifficulty, aStars);
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
				GameClient()->Echo(aBuf);
			}
			else
			{
				GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", "Map info not found");
				GameClient()->Echo("Map info not found");
			}
			bFound = true;
			break;
		}
		if(bFound)
			break;
	}
	if(!bFound)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Map '%s' not found", MapForSearch);
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Rushie", aBuf);
		GameClient()->Echo(aBuf);
	}
	json_value_free(pJson);
}
void CRClient::ResetSearchMapInfo()
{
	if(m_pSearchMapInfoTask)
	{
		m_pSearchMapInfoTask->Abort();
		m_pSearchMapInfoTask = NULL;
	}
}

// Tracker
bool CRClient::IsTracked(int ClientId)
{
	// Check if the client is being tracked
	for(int i = 0; i < TargetCount; i++)
	{
		if(ClientId == TargetPositionId[i])
		{
			return true;
		}
	}
	return false;
}

void CRClient::ConSpectatorAddTracker(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pThis = static_cast<CRClient *>(pUserData);
	char PlayerName[32];
	int PlayerId = -1;
	if(pThis->GameClient()->m_Snap.m_SpecInfo.m_SpectatorId != SPEC_FREEVIEW && pThis->GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		const auto &Player = pThis->GameClient()->m_aClients[pThis->GameClient()->m_Snap.m_SpecInfo.m_SpectatorId];
		str_copy(PlayerName, Player.m_aName, sizeof(PlayerName));
		PlayerId = Player.ClientId();
	}
	else
		pThis->GameClient()->Echo("You're not spectating the player");
	if(PlayerId != -1)
	{
		if(!pThis->IsTracked(PlayerId))
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "tracker %s", PlayerName);
			pThis->GameClient()->Console()->ExecuteLine(aBuf);
		}
		else
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "tracker_remove %s", PlayerName);
			pThis->GameClient()->Console()->ExecuteLine(aBuf);
		}
	}
}

//Warlist
bool CRClient::IsInWarlist(int ClientId, int Index)
{
	CWarDataCache &WarData = GameClient()->m_WarList.m_WarPlayers[ClientId];
	for(int i = 0; i < (int)WarData.m_WarGroupMatches.size(); i++)
	{
		if(WarData.m_WarGroupMatches[i])
		{
			if(Index == i)
				return true;
		}
	}
	return false;
}

//Find hours
void CRClient::ConFindHours(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	// Return if a find_hours request is already in progress
	if(pSelf->m_pFindHoursTask && !pSelf->m_pFindHoursTask->Done())
	{
		pSelf->GameClient()->Echo("Request already in progress");
		return;
	}
	const char *pNickname = pResult->GetString(0);
	const char *pWriteinchat = pResult->GetString(1);

	pSelf->FetchFindHours(pNickname, pWriteinchat);
}

void CRClient::FetchFindHours(const char *pNickname, const char *pWriteinchat)
{
	if(m_pFindHoursTask && !m_pFindHoursTask->Done())
		return;

	if(!pNickname)
		return;
	else
		str_copy(m_aFindHoursPlayer, pNickname, sizeof(m_aFindHoursPlayer));

	if((str_comp("W", pWriteinchat) == 0) || ((str_comp("w", pWriteinchat) == 0)))
		m_WriteFindHoursInChat = true;
	else
		m_WriteFindHoursInChat = false;
	char aUrl[256];
	char aEncodedNickname[256];
	// URL encode the nickname
	EscapeUrl(aEncodedNickname, sizeof(aEncodedNickname), pNickname);
	str_format(aUrl, sizeof(aUrl), "https://ddstats.tw/player/json?player=%s", aEncodedNickname);
	m_pFindHoursTask = HttpGet(aUrl);
	m_pFindHoursTask->Timeout(CTimeout{20000, 0, 500, 10});
	m_pFindHoursTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pFindHoursTask);
}

// Added helper to finish async find_hours
void CRClient::FinishFindHours()
{
	json_value *pJson = m_pFindHoursTask->ResultJson();
	if(!pJson)
		return;
	const json_value &Json = *pJson;
	const json_value &General = Json["general_activity"];
	const json_value &Profile = Json["profile"];
	if(General.type == json_object && Profile.type == json_object)
	{
		const json_value &Seconds = General["total_seconds_played"];
		const json_value &Points = Profile["points"];
		if(Seconds.type == json_integer && Points.type == json_integer)
		{
			int Hours = Seconds.u.integer / 3600;
			int Pointsfinal = Points.u.integer;
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Player %s has %d hours and %d points", m_aFindHoursPlayer, Hours, Pointsfinal);
			GameClient()->Echo(aBuf);
			if(m_WriteFindHoursInChat)
				GameClient()->m_Chat.SendChat(0, aBuf);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FindHours", aBuf);
		}
		else
		{
			GameClient()->Echo("Invalid 'total_seconds_played' in JSON");
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FindHours", "Invalid 'total_seconds_played' in JSON");
		}
	}
	else
	{
		GameClient()->Echo("Invalid 'general_activity' in JSON");
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "FindHours", "Invalid 'general_activity' in JSON");
	}
	json_value_free(pJson);
}

void CRClient::ResetFindHours()
{
	if(m_pFindHoursTask)
	{
		m_pFindHoursTask->Abort();
		m_pFindHoursTask = NULL;
	}
}

// 45 degrees toggle
void CRClient::ConToggle45Degrees(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_45degreestoggle = pResult->GetInteger(0) != 0;

	// сохраняем предыдущее состояние лазера (своего)
	static int s_PrevShowHookCollOwn = g_Config.m_ClShowHookCollOwn;

	if(!g_Config.m_RiToggle45degrees)
	{
		if(pSelf->m_45degreestoggle && !pSelf->m_45degreestogglelastinput)
		{
			pSelf->m_45degreesEnabled = 1;
			pSelf->GameClient()->Echo("[[green]] 45° on");

			// сохраняем прошлые значения
			g_Config.m_RiPrevInpMousesens45degrees = (pSelf->m_SmallsensEnabled == 1 ? g_Config.m_RiPrevInpMousesensSmallsens : g_Config.m_InpMousesens);
			g_Config.m_RiPrevMouseMaxDistance45degrees = g_Config.m_ClMouseMaxDistance;

			// устанавливаем новые значения
			g_Config.m_ClMouseMaxDistance = 2;
			g_Config.m_InpMousesens = 4;

			// включаем постоянный лазер
			s_PrevShowHookCollOwn = g_Config.m_ClShowHookCollOwn;
			g_Config.m_ClShowHookCollOwn = 2; // всегда показывать
		}
		else if(!pSelf->m_45degreestoggle)
		{
			pSelf->m_45degreesEnabled = 0;
			pSelf->GameClient()->Echo("[[red]] 45° off");

			// возвращаем прежние значения
			g_Config.m_ClMouseMaxDistance = g_Config.m_RiPrevMouseMaxDistance45degrees;
			g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesens45degrees;

			// возвращаем состояние лазера
			g_Config.m_ClShowHookCollOwn = s_PrevShowHookCollOwn;
		}
		pSelf->m_45degreestogglelastinput = pSelf->m_45degreestoggle;
	}

	if(g_Config.m_RiToggle45degrees)
	{
		if(pSelf->m_45degreestoggle && !pSelf->m_45degreestogglelastinput)
		{
			if(g_Config.m_ClMouseMaxDistance == 2)
			{
				pSelf->m_45degreesEnabled = 0;
				pSelf->GameClient()->Echo("[[red]] 45° off");
				g_Config.m_ClMouseMaxDistance = g_Config.m_RiPrevMouseMaxDistance45degrees;
				g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesens45degrees;

				// возвращаем состояние лазера
				g_Config.m_ClShowHookCollOwn = s_PrevShowHookCollOwn;
			}
			else
			{
				pSelf->m_45degreesEnabled = 1;
				pSelf->GameClient()->Echo("[[green]] 45° on");
				g_Config.m_RiPrevInpMousesens45degrees = (pSelf->m_SmallsensEnabled == 1 ? g_Config.m_RiPrevInpMousesensSmallsens : g_Config.m_InpMousesens);
				g_Config.m_RiPrevMouseMaxDistance45degrees = g_Config.m_ClMouseMaxDistance;
				g_Config.m_ClMouseMaxDistance = 2;
				g_Config.m_InpMousesens = 4;

				// включаем постоянный лазер
				s_PrevShowHookCollOwn = g_Config.m_ClShowHookCollOwn;
				g_Config.m_ClShowHookCollOwn = 2; // всегда показывать
			}
		}
		pSelf->m_45degreestogglelastinput = pSelf->m_45degreestoggle;
	}
}


//Small sens toggle
void CRClient::ConToggleSmallSens(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->m_Smallsenstoggle = pResult->GetInteger(0) != 0;
	if(!g_Config.m_RiToggleSmallSens)
	{
		if(pSelf->m_Smallsenstoggle && !pSelf->m_Smallsenstogglelastinput)
		{
			pSelf->m_SmallsensEnabled = 1;
			pSelf->GameClient()->Echo("[[green]] small sens on");
			g_Config.m_RiPrevInpMousesensSmallsens = (pSelf->m_45degreesEnabled == 1 ? g_Config.m_RiPrevInpMousesens45degrees : g_Config.m_InpMousesens);
			g_Config.m_InpMousesens = 1;
		}
		else if(!pSelf->m_Smallsenstoggle)
		{
			pSelf->m_SmallsensEnabled = 0;
			pSelf->GameClient()->Echo("[[red]] small sens off");
			g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesensSmallsens;
		}
		pSelf->m_Smallsenstogglelastinput = pSelf->m_Smallsenstoggle;
	}

	if(g_Config.m_RiToggleSmallSens)
	{
		if(pSelf->m_Smallsenstoggle && !pSelf->m_Smallsenstogglelastinput)
		{
			if(g_Config.m_InpMousesens == 1)
			{
				pSelf->m_SmallsensEnabled = 0;
				pSelf->GameClient()->Echo("[[red]] small sens off");
				g_Config.m_InpMousesens = g_Config.m_RiPrevInpMousesensSmallsens;
			}
			else
			{
				pSelf->m_SmallsensEnabled = 1;
				pSelf->GameClient()->Echo("[[green]] small sens on");
				g_Config.m_RiPrevInpMousesensSmallsens = (pSelf->m_45degreesEnabled == 1 ? g_Config.m_RiPrevInpMousesens45degrees : g_Config.m_InpMousesens);
				g_Config.m_InpMousesens = 1;
			}
		}
		pSelf->m_Smallsenstogglelastinput = pSelf->m_Smallsenstoggle;
	}
}

//Deepfly
void CRClient::ConToggleDeepfly(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	char CurBind[128];
	str_copy(CurBind, pSelf->GameClient()->m_Binds.Get(291, 0), sizeof(CurBind));
	if(str_find_nocase(CurBind, "+toggle cl_dummy_hammer"))
	{
		pSelf->GameClient()->Echo("[[red]] Deepfly off");
		if(str_length(pSelf->m_Oldmouse1Bind) > 1)
			pSelf->GameClient()->m_Binds.Bind(291, pSelf->m_Oldmouse1Bind, false, 0);
		else
		{
			pSelf->GameClient()->Echo("[[red]] No old bind in memory. Binding +fire");
			pSelf->GameClient()->m_Binds.Bind(291, "+fire", false, 0);
		};
	}
	else
	{
		pSelf->GameClient()->Echo("[[green]] Deepfly on");
		str_copy(pSelf->m_Oldmouse1Bind, CurBind, sizeof(CurBind));
		pSelf->GameClient()->m_Binds.Bind(291, "+fire; +toggle cl_dummy_hammer 1 0", false, 0);
	}
}

//Copy nickname
void CRClient::RiCopyNicknamePlayer(const char *pNickname)
{
	if(g_Config.m_ClDummy == 0)
	{
		str_copy(PlayerNameBeforeCopyPlayer, g_Config.m_PlayerName, sizeof(PlayerNameBeforeCopyPlayer));
		str_copy(g_Config.m_PlayerName, pNickname, sizeof(g_Config.m_PlayerName));
		GameClient()->SendInfo(false);
	}
	if(g_Config.m_ClDummy == 1)
	{
		str_copy(DummyNameBeforeCopyPlayer, g_Config.m_PlayerName, sizeof(DummyNameBeforeCopyPlayer));
		str_copy(g_Config.m_ClDummyName, pNickname, sizeof(g_Config.m_ClDummyName));
		GameClient()->SendDummyInfo(false);
	}
}

//Nameplates
void CRClient::ConUpdateNameplatesEditor(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->GameClient()->m_NamePlates.RiResetNameplatesPos(*pSelf->GameClient(), g_Config.m_RiNamePlateScheme);
}

void CRClient::ConUpdateRegexIgnore(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	pSelf->RiSplitRegex();
}

std::vector<std::string> CRClient::SplitRegex(const char *aboba)
{
	std::vector<std::string> parts;
	std::string str(aboba);

	size_t start = 0;
	size_t end = 0;

	while((end = str.find('|', start)) != std::string::npos)
	{
		parts.push_back(str.substr(start, end - start));
		start = end + 1;
	}
	parts.push_back(str.substr(start));

	// for(size_t i = 0; i < parts.size(); i++)
	// {
	// 	dbg_msg("Parts", "%s", parts[i].c_str());
	// }

	return parts;
}

std::vector<std::string> CRClient::SplitWords(const char *aboba)
{
	std::vector<std::string> parts;
	std::string str(aboba);

	size_t start = 0;
	size_t end = 0;

	while((end = str.find(' ', start)) != std::string::npos)
	{
		parts.push_back(str.substr(start, end - start));
		start = end + 1;
	}
	parts.push_back(str.substr(start));

	// for(size_t i = 0; i < parts.size(); i++)
	// {
	// 	dbg_msg("Parts", "%s", parts[i].c_str());
	// }

	return parts;
}

void CRClient::RiSplitRegex() const
{
	GameClient()->m_RClient.m_RegexSplited = SplitRegex(g_Config.m_TcRegexChatIgnore);
	GameClient()->m_RClient.m_RegexSplitedPlayer = SplitRegex(g_Config.m_RiRegexPlayerWhitelist);
}

// Server and Player Info Collection
void CRClient::SendServerPlayerInfo()
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	// Get server address
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	// Send data for main player
	int LocalClientId = GameClient()->m_aLocalIds[0];
	int DummyClientId = -1;
	if(Client()->DummyConnected())
	{
		DummyClientId = GameClient()->m_aLocalIds[1];
	}
	if(LocalClientId >= 0)
	{
		SendPlayerData(CurrentServerInfo.m_aAddress, LocalClientId, DummyClientId);
	}
	// Store current server info for comparison
	str_copy(m_aCurrentServerAddress, CurrentServerInfo.m_aAddress, sizeof(m_aCurrentServerAddress));
}

void CRClient::SendPlayerData(const char *pServerAddress, int ClientId, int DummyClientId)
{
	if(m_aAuthToken[0] == '\0')
	{
		// Token not yet fetched, try again later.
		if(!m_pAuthTokenTask)
			FetchAuthToken();
		return;
	}
	// Create JSON data for this specific player
	char aJsonData[512];

	if(DummyClientId >= 0)
		str_format(aJsonData, sizeof(aJsonData),
			"{"
			"\"server_address\":\"%s\","
			"\"player_id\":%d,"
			"\"dummy_id\":%d,"
			"\"auth_token\":\"%s\","
			"\"timestamp\":%lld"
			"}",
			pServerAddress,
			ClientId,
			DummyClientId,
			m_aAuthToken,
			(long long)time_get());
	else
		str_format(aJsonData, sizeof(aJsonData),
			"{"
			"\"server_address\":\"%s\","
			"\"player_id\":%d,"
			"\"auth_token\":\"%s\","
			"\"timestamp\":%lld"
			"}",
			pServerAddress,
			ClientId,
			m_aAuthToken,
			(long long)time_get());

	// Create and send HTTP request
	m_pRClientUsersTaskSend = std::make_shared<CHttpRequest>(CRClient::RCLIENT_URL_USERS);
	m_pRClientUsersTaskSend->PostJson(aJsonData);
	m_pRClientUsersTaskSend->Timeout(CTimeout{10000, 0, 500, 5});
	m_pRClientUsersTaskSend->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pRClientUsersTaskSend);
}

void CRClient::FetchRClientUsers()
{
	if(m_pRClientUsersTask && !m_pRClientUsersTask->Done())
		return;

	m_pRClientUsersTask = HttpGet(CRClient::RCLIENT_URL_USERS);
	m_pRClientUsersTask->Timeout(CTimeout{10000, 0, 500, 5});
	m_pRClientUsersTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pRClientUsersTask);
}

void CRClient::FetchAuthToken()
{
	if(m_pAuthTokenTask && !m_pAuthTokenTask->Done())
		return;

	m_pAuthTokenTask = HttpGet(CRClient::RCLIENT_TOKEN_URL);
	m_pAuthTokenTask->Timeout(CTimeout{10000, 0, 500, 5});
	m_pAuthTokenTask->IpResolve(IPRESOLVE::V4);
	Http()->Run(m_pAuthTokenTask);
}

void CRClient::FinishAuthToken()
{
	if(m_pAuthTokenTask->State() != EHttpState::DONE)
		return;

	json_value *pJson = m_pAuthTokenTask->ResultJson();
	if(!pJson)
	{
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "RClient", "Failed to fetch auth token: no JSON");
		return;
	}

	const json_value &Json = *pJson;
	const json_value &Token = Json["token"];

	if(Token.type == json_string)
	{
		str_copy(m_aAuthToken, Token.u.string.ptr, sizeof(m_aAuthToken));
		// The token might have a newline at the end
		str_utf8_trim_right(m_aAuthToken);
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "RClient", "Fetched auth token");
	}
	else
	{
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "RClient", "Failed to fetch auth token: token not found in JSON");
	}
	json_value_free(pJson);
}

void CRClient::ResetAuthToken()
{
	if(m_pAuthTokenTask)
	{
		m_pAuthTokenTask->Abort();
		m_pAuthTokenTask = nullptr;
	}
}

// void CRClient::FinishRClientUsersSend()
// {
// 	json_value *pJson = m_pRClientUsersTask->ResultJson();
// 	if(!pJson)
// 		return;
//
// 	json_value_free(pJson);
// }

void CRClient::ResetRClientUsersSend()
{
	if(m_pRClientUsersTaskSend)
	{
		m_pRClientUsersTaskSend->Abort();
		m_pRClientUsersTaskSend = nullptr;
	}
}

void CRClient::FinishRClientUsers()
{
	json_value *pJson = m_pRClientUsersTask->ResultJson();
	if(!pJson)
		return;

	const json_value &Json = *pJson;
	m_vRClientUsers.clear();

	// Parse the JSON response to get list of RClient users
	if(Json.type == json_object)
	{
		// The response format is: {"server_address": {"player_id": {...}}}
		for(unsigned int i = 0; i < Json.u.object.length; i++)
		{
			const char *pServerAddr = Json.u.object.values[i].name;
			const json_value &PlayersObj = *Json.u.object.values[i].value;

			if(PlayersObj.type == json_object)
			{
				for(unsigned int j = 0; j < PlayersObj.u.object.length; j++)
				{
					const char *pPlayerIdStr = PlayersObj.u.object.values[j].name;
					int PlayerId = atoi(pPlayerIdStr);
					const json_value &PlayerData = *PlayersObj.u.object.values[j].value;

					// Add main player ID
					m_vRClientUsers.emplace_back(std::string(pServerAddr), PlayerId);

					// Check if this player has a dummy and add dummy ID too
					if(PlayerData.type == json_object)
					{
						for(unsigned int k = 0; k < PlayerData.u.object.length; k++)
						{
							if(str_comp(PlayerData.u.object.values[k].name, "dummy_id") == 0 &&
								PlayerData.u.object.values[k].value->type == json_integer)
							{
								int DummyId = PlayerData.u.object.values[k].value->u.integer;
								m_vRClientUsers.emplace_back(std::string(pServerAddr), DummyId);
								break;
							}
						}
					}
				}
			}
		}
	}

	json_value_free(pJson);
}

void CRClient::ResetRClientUsers()
{
	if(m_pRClientUsersTask)
	{
		m_pRClientUsersTask->Abort();
		m_pRClientUsersTask = nullptr;
	}
}

bool CRClient::IsPlayerRClient(int ClientId)
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return false;

	// // Always show RClient indicator for local players (main and dummy)
	// if(GameClient()->m_Snap.m_apPlayerInfos[ClientId] && GameClient()->m_Snap.m_apPlayerInfos[ClientId]->m_Local)
	// 	return true;
	//
	// // Also check if this is our dummy using m_aLocalIds
	// if(ClientId == GameClient()->m_aLocalIds[0] || ClientId == GameClient()->m_aLocalIds[1])
	// 	return true;

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	// Check if this player is in our RClient users list
	for(const auto &User : m_vRClientUsers)
	{
		if(str_comp(User.first.c_str(), CurrentServerInfo.m_aAddress) == 0 && User.second == ClientId)
			return true;
	}

	return false;
}

void CRClient::ConAddWhiteList(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = static_cast<CRClient *>(pUserData);
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	char aBuf[256];
	if(aInput[0])
	{
		if(g_Config.m_RiRegexPlayerWhitelist[0])
		{
			char aNewRegex[512];
			str_format(aBuf, sizeof(aBuf), "Added to existing regex: %s", aInput);
			str_format(aNewRegex, sizeof(aNewRegex), "%s|%s", g_Config.m_RiRegexPlayerWhitelist, aInput);
			str_copy(g_Config.m_RiRegexPlayerWhitelist, aNewRegex, sizeof(g_Config.m_RiRegexPlayerWhitelist));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
		else
		{
			str_copy(g_Config.m_RiRegexPlayerWhitelist, aInput, sizeof(g_Config.m_RiRegexPlayerWhitelist));
			str_format(aBuf, sizeof(aBuf), "New regex added: %s", aInput);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", "No word in this");
	}
}

std::vector<std::string> CRClient::GetWordsListRegex(int IsPlayer = 0) const
{
	if(IsPlayer == 1)
		return m_RegexSplitedPlayer;
	else
		return m_RegexSplited;
}

void CRClient::ConFindSkin(IConsole::IResult *pResult, void *pUserData)
{
	CGameClient *pSelf = (CGameClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}
	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Название скина
			str_format(aBuf, sizeof(aBuf), "- Skin Name: %s", ClientData.m_aSkinName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->Echo("No that player on server");
	}
}

void CRClient::ConCopySkin(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}

	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Название скина
			str_format(aBuf, sizeof(aBuf), "- Skin Name: %s", ClientData.m_aSkinName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			if(g_Config.m_ClDummy == 1)
			{
				str_copy(pSelf->GameClient()->m_RClient.DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(pSelf->GameClient()->m_RClient.DummySkinBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.DummyNameBeforeCopyPlayer, g_Config.m_ClDummyName, sizeof(pSelf->GameClient()->m_RClient.DummyNameBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.DummyClanBeforeCopyPlayer, g_Config.m_ClDummyClan, sizeof(pSelf->GameClient()->m_RClient.DummyClanBeforeCopyPlayer));
				pSelf->GameClient()->m_RClient.DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				pSelf->GameClient()->m_RClient.DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				pSelf->GameClient()->m_RClient.DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				pSelf->GameClient()->m_RClient.DummyCountryBeforeCopyPlayer = g_Config.m_ClDummyCountry;
				str_copy(g_Config.m_ClDummySkin, ClientData.m_aSkinName, sizeof(g_Config.m_ClDummySkin));
				g_Config.m_ClDummyUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClDummyColorBody = ClientData.m_ColorBody;
				g_Config.m_ClDummyColorFeet = ClientData.m_ColorFeet;
				pSelf->GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(pSelf->GameClient()->m_RClient.PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(pSelf->GameClient()->m_RClient.PlayerSkinBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.PlayerNameBeforeCopyPlayer, g_Config.m_PlayerName, sizeof(pSelf->GameClient()->m_RClient.PlayerNameBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.PlayerClanBeforeCopyPlayer, g_Config.m_PlayerClan, sizeof(pSelf->GameClient()->m_RClient.PlayerClanBeforeCopyPlayer));
				pSelf->GameClient()->m_RClient.PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				pSelf->GameClient()->m_RClient.PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				pSelf->GameClient()->m_RClient.PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				pSelf->GameClient()->m_RClient.PlayerCountryBeforeCopyPlayer = g_Config.m_PlayerCountry;
				str_copy(g_Config.m_ClPlayerSkin, ClientData.m_aSkinName, sizeof(g_Config.m_ClPlayerSkin));
				g_Config.m_ClPlayerUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClPlayerColorBody = ClientData.m_ColorBody;
				g_Config.m_ClPlayerColorFeet = ClientData.m_ColorFeet;
				pSelf->GameClient()->SendInfo(false);
			}
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->GameClient()->Echo("No that player on server");
	}
}

void CRClient::ConCopyPlayer(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}

	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Название скина
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", ClientData.m_aSkinName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Clan: %s", ClientData.m_aClan);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Country: %d",
				ClientData.m_Country);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			if(g_Config.m_ClDummy == 1)
			{
				str_copy(pSelf->GameClient()->m_RClient.DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(pSelf->GameClient()->m_RClient.DummySkinBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.DummyNameBeforeCopyPlayer, g_Config.m_ClDummyName, sizeof(pSelf->GameClient()->m_RClient.DummyNameBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.DummyClanBeforeCopyPlayer, g_Config.m_ClDummyClan, sizeof(pSelf->GameClient()->m_RClient.DummyClanBeforeCopyPlayer));
				pSelf->GameClient()->m_RClient.DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				pSelf->GameClient()->m_RClient.DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				pSelf->GameClient()->m_RClient.DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				pSelf->GameClient()->m_RClient.DummyCountryBeforeCopyPlayer = g_Config.m_ClDummyCountry;
				str_copy(g_Config.m_ClDummySkin, ClientData.m_aSkinName, sizeof(g_Config.m_ClDummySkin));
				if(g_Config.m_ClCopyNickWithDot)
				{
					str_format(g_Config.m_ClDummyName, sizeof(g_Config.m_ClDummyName), "%s.", ClientData.m_aName);
				}
				else
				{
					str_copy(g_Config.m_ClDummyName, ClientData.m_aName, sizeof(g_Config.m_ClDummyName));
				}
				str_copy(g_Config.m_ClDummyClan, ClientData.m_aClan, sizeof(g_Config.m_ClDummyClan));
				g_Config.m_ClDummyUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClDummyColorBody = ClientData.m_ColorBody;
				g_Config.m_ClDummyColorFeet = ClientData.m_ColorFeet;
				g_Config.m_ClDummyCountry = ClientData.m_Country;

				pSelf->GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(pSelf->GameClient()->m_RClient.PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(pSelf->GameClient()->m_RClient.PlayerSkinBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.PlayerNameBeforeCopyPlayer, g_Config.m_PlayerName, sizeof(pSelf->GameClient()->m_RClient.PlayerNameBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.PlayerClanBeforeCopyPlayer, g_Config.m_PlayerClan, sizeof(pSelf->GameClient()->m_RClient.PlayerClanBeforeCopyPlayer));
				pSelf->GameClient()->m_RClient.PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				pSelf->GameClient()->m_RClient.PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				pSelf->GameClient()->m_RClient.PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				pSelf->GameClient()->m_RClient.PlayerCountryBeforeCopyPlayer = g_Config.m_PlayerCountry;
				str_copy(g_Config.m_ClPlayerSkin, ClientData.m_aSkinName, sizeof(g_Config.m_ClPlayerSkin));
				if(g_Config.m_ClCopyNickWithDot)
				{
					str_format(g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), "%s.", ClientData.m_aName);
				}
				else
				{
					str_copy(g_Config.m_PlayerName, ClientData.m_aName, sizeof(g_Config.m_PlayerName));
				}
				str_copy(g_Config.m_PlayerClan, ClientData.m_aClan, sizeof(g_Config.m_PlayerClan));
				g_Config.m_ClPlayerUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClPlayerColorBody = ClientData.m_ColorBody;
				g_Config.m_ClPlayerColorFeet = ClientData.m_ColorFeet;
				g_Config.m_PlayerCountry = ClientData.m_Country;
				pSelf->GameClient()->SendInfo(false);
			}
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No player found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->GameClient()->Echo("No that player on server");
	}
}

void CRClient::ConFindPlayer(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}

	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Название скина
			str_format(aBuf, sizeof(aBuf), "- Skin name: %s", ClientData.m_aSkinName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Clan: %s", ClientData.m_aClan);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Country: %d",
				ClientData.m_Country);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->GameClient()->Echo("No that player on server");
	}
}

void CRClient::ConCopyColor(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
	}

	// Validate client ID
	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CGameClient::CClientData &ClientData = pSelf->GameClient()->m_aClients[ClientID];
		if(ClientData.m_aSkinName[0])
		{
			char aBuf[512];

			// Базовая информация о скине
			str_format(aBuf, sizeof(aBuf), "Skin info for client %d:\n", ClientID);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			str_format(aBuf, sizeof(aBuf), "- Name: %s", ClientData.m_aName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет тела
			str_format(aBuf, sizeof(aBuf), "- Body Color: %d",
				ClientData.m_ColorBody);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Цвет ног
			str_format(aBuf, sizeof(aBuf), "- Feet Color: %d",
				ClientData.m_ColorFeet);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			// Включены ли кастом цвет
			str_format(aBuf, sizeof(aBuf), "- Custom Color: %d",
				ClientData.m_UseCustomColor);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

			if(g_Config.m_ClDummy == 1)
			{
				str_copy(pSelf->GameClient()->m_RClient.DummySkinBeforeCopyPlayer, g_Config.m_ClDummySkin, sizeof(pSelf->GameClient()->m_RClient.DummySkinBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.DummyNameBeforeCopyPlayer, g_Config.m_ClDummyName, sizeof(pSelf->GameClient()->m_RClient.DummyNameBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.DummyClanBeforeCopyPlayer, g_Config.m_ClDummyClan, sizeof(pSelf->GameClient()->m_RClient.DummyClanBeforeCopyPlayer));
				pSelf->GameClient()->m_RClient.DummyUseCustomColorBeforeCopyPlayer = g_Config.m_ClDummyUseCustomColor;
				pSelf->GameClient()->m_RClient.DummyBodyColorBeforeCopyPlayer = g_Config.m_ClDummyColorBody;
				pSelf->GameClient()->m_RClient.DummyFeetColorBeforeCopyPlayer = g_Config.m_ClDummyColorFeet;
				pSelf->GameClient()->m_RClient.DummyCountryBeforeCopyPlayer = g_Config.m_ClDummyCountry;
				g_Config.m_ClDummyUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClDummyColorBody = ClientData.m_ColorBody;
				g_Config.m_ClDummyColorFeet = ClientData.m_ColorFeet;
				pSelf->GameClient()->SendDummyInfo(false);
			}
			if(g_Config.m_ClDummy == 0)
			{
				str_copy(pSelf->GameClient()->m_RClient.PlayerSkinBeforeCopyPlayer, g_Config.m_ClPlayerSkin, sizeof(pSelf->GameClient()->m_RClient.PlayerSkinBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.PlayerNameBeforeCopyPlayer, g_Config.m_PlayerName, sizeof(pSelf->GameClient()->m_RClient.PlayerNameBeforeCopyPlayer));
				str_copy(pSelf->GameClient()->m_RClient.PlayerClanBeforeCopyPlayer, g_Config.m_PlayerClan, sizeof(pSelf->GameClient()->m_RClient.PlayerClanBeforeCopyPlayer));
				pSelf->GameClient()->m_RClient.PlayerUseCustomColorBeforeCopyPlayer = g_Config.m_ClPlayerUseCustomColor;
				pSelf->GameClient()->m_RClient.PlayerBodyColorBeforeCopyPlayer = g_Config.m_ClPlayerColorBody;
				pSelf->GameClient()->m_RClient.PlayerFeetColorBeforeCopyPlayer = g_Config.m_ClPlayerColorFeet;
				pSelf->GameClient()->m_RClient.PlayerCountryBeforeCopyPlayer = g_Config.m_PlayerCountry;
				g_Config.m_ClPlayerUseCustomColor = ClientData.m_UseCustomColor;
				g_Config.m_ClPlayerColorBody = ClientData.m_ColorBody;
				g_Config.m_ClPlayerColorFeet = ClientData.m_ColorFeet;
				pSelf->GameClient()->SendInfo(false);
			}
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "No skin found for this client");
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Invalid client ID");
		pSelf->GameClient()->Echo("No that player on server");
	}
}

void CRClient::ConTargetPlayerPos(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	if(pResult->NumArguments() == 0)
	{
		return;
	}
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			// Find first free slot
			for(int j = 0; j < MAX_CLIENTS; j++)
			{
				if(pSelf->GameClient()->m_RClient.TargetPositionId[j] == -1)
				{
					str_copy(pSelf->GameClient()->m_RClient.TargetPositionNickname[j], pSelf->GameClient()->m_aClients[ClientID].m_aName);
					pSelf->GameClient()->m_RClient.TargetPositionId[j] = ClientID;
					pSelf->GameClient()->m_RClient.TargetCount++;
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "%s added to target list", pSelf->GameClient()->m_aClients[ClientID].m_aName);
					pSelf->GameClient()->Echo(aBuf);
					break;
				}
			}
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
		if(ClientID >= 0 && ClientID < MAX_CLIENTS && pSelf->GameClient()->m_aClients[ClientID].m_aName[0] != '\0')
		{
			// Find first free slot
			for(int j = 0; j < MAX_CLIENTS; j++)
			{
				if(pSelf->GameClient()->m_RClient.TargetPositionId[j] == -1)
				{
					str_copy(pSelf->GameClient()->m_RClient.TargetPositionNickname[j], pSelf->GameClient()->m_aClients[ClientID].m_aName);
					pSelf->GameClient()->m_RClient.TargetPositionId[j] = ClientID;
					pSelf->GameClient()->m_RClient.TargetCount++;
					break;
				}
			}
		}
		else
		{
			pSelf->GameClient()->Echo("Invalid player ID or player not found");
			dbg_msg("Search player", "Invalid player ID or player not found");
		}
	}
}
void CRClient::ConTargetPlayerPosReset(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		pSelf->GameClient()->m_RClient.TargetPositionId[i] = -1;
		pSelf->GameClient()->m_RClient.TargetPositionNickname[i][0] = '\0';
	}
	pSelf->GameClient()->m_RClient.TargetCount = 0;
	pSelf->GameClient()->Echo("Target list resetted");
}
void CRClient::ConTargetPlayerPosRemove(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	int ClientID = -1;
	// First try to find by name
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(str_comp_nocase(pSelf->GameClient()->m_aClients[i].m_aName, aInput) == 0)
		{
			ClientID = i;
			// Find first free slot
			for(int j = 0; j < pSelf->GameClient()->m_RClient.TargetCount; j++)
			{
				if(str_comp_nocase(pSelf->GameClient()->m_RClient.TargetPositionNickname[j], pSelf->GameClient()->m_aClients[ClientID].m_aName) == 0)
				{
					pSelf->GameClient()->m_RClient.TargetCount--;

					// Shift remaining targets to fill the gap
					for(int k = j; k < pSelf->GameClient()->m_RClient.TargetCount; k++)
					{
						pSelf->GameClient()->m_RClient.TargetPositionId[k] = pSelf->GameClient()->m_RClient.TargetPositionId[k + 1];
						str_copy(pSelf->GameClient()->m_RClient.TargetPositionNickname[k], pSelf->GameClient()->m_RClient.TargetPositionNickname[k + 1], sizeof(pSelf->GameClient()->m_RClient.TargetPositionNickname[k]));
					}

					// Clear the last slot
					pSelf->GameClient()->m_RClient.TargetPositionId[pSelf->GameClient()->m_RClient.TargetCount] = -1;
					pSelf->GameClient()->m_RClient.TargetPositionNickname[pSelf->GameClient()->m_RClient.TargetCount][0] = '\0';
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "%s removed from target list", pSelf->GameClient()->m_aClients[ClientID].m_aName);
					pSelf->GameClient()->Echo(aBuf);
					return;
				}
			}
			break;
		}
	}

	// If not found by name, try to use input as ID
	if(ClientID == -1)
	{
		ClientID = str_toint(aInput);
		if(ClientID >= 0 && ClientID < MAX_CLIENTS && pSelf->GameClient()->m_aClients[ClientID].m_aName[0] != '\0')
		{
			// Find matching slot
			for(int j = 0; j < pSelf->GameClient()->m_RClient.TargetCount; j++)
			{
				if(pSelf->GameClient()->m_RClient.TargetPositionId[j] == ClientID)
				{
					pSelf->GameClient()->m_RClient.TargetCount--;

					// Shift remaining targets to fill the gap
					for(int k = j; k < pSelf->GameClient()->m_RClient.TargetCount; k++)
					{
						pSelf->GameClient()->m_RClient.TargetPositionId[k] = pSelf->GameClient()->m_RClient.TargetPositionId[k + 1];
						str_copy(pSelf->GameClient()->m_RClient.TargetPositionNickname[k], pSelf->GameClient()->m_RClient.TargetPositionNickname[k + 1], sizeof(pSelf->GameClient()->m_RClient.TargetPositionNickname[k]));
					}

					// Clear the last slot
					pSelf->GameClient()->m_RClient.TargetPositionId[pSelf->GameClient()->m_RClient.TargetCount] = -1;
					pSelf->GameClient()->m_RClient.TargetPositionNickname[pSelf->GameClient()->m_RClient.TargetCount][0] = '\0';

					pSelf->GameClient()->Echo("Player removed from target list");
					return;
				}
			}
			pSelf->GameClient()->Echo("Player not in target list");
		}
		else
		{
			pSelf->GameClient()->Echo("Invalid player ID or player not found");
			dbg_msg("Search player", "Invalid player ID or player not found");
		}
	}
}

void CRClient::ConAddCensorList(IConsole::IResult *pResult, void *pUserData)
{
	CRClient *pSelf = (CRClient *)pUserData;
	const char *pInput = pResult->GetString(0);
	char aInput[256];
	str_copy(aInput, pInput, sizeof(aInput));
	str_utf8_trim_right(aInput);
	char aBuf[256];
	if(aInput[0])
	{
		if(g_Config.m_TcRegexChatIgnore[0])
		{
			char aNewRegex[512];
			str_format(aBuf, sizeof(aBuf), "Added to existing regex: %s", aInput);
			str_format(aNewRegex, sizeof(aNewRegex), "%s|%s", g_Config.m_TcRegexChatIgnore, aInput);
			str_copy(g_Config.m_TcRegexChatIgnore, aNewRegex, sizeof(g_Config.m_TcRegexChatIgnore));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
		else
		{
			str_copy(g_Config.m_TcRegexChatIgnore, aInput, sizeof(g_Config.m_TcRegexChatIgnore));
			str_format(aBuf, sizeof(aBuf), "New regex added: %s", aInput);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", aBuf);
		}
	}
	else
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Regex", "No word in this");
	}
	pSelf->GameClient()->m_RClient.RiSplitRegex();
}
