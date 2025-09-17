#ifndef GAME_CLIENT_COMPONENTS_RCLIENT_RUSHIE_H
#define GAME_CLIENT_COMPONENTS_RCLIENT_RUSHIE_H

#include <game/client/component.h>

#include <engine/shared/console.h>
#include <engine/shared/http.h>

class CRClient : public CComponent
{
	static void ConFindPlayerFromDdstats(IConsole::IResult *pResult, void *pUserData);
	static void ConCopyPlayerFromDdstats(IConsole::IResult *pResult, void *pUserData);
	static void ConFindSkinFromDdstats(IConsole::IResult *pResult, void *pUserData);
	static void ConCopySkinFromDdstats(IConsole::IResult *pResult, void *pUserData);
	static void ConBackupPlayerProfile(IConsole::IResult *pResult, void *pUserData);

	static void ConSpectatorAddTracker(IConsole::IResult *pResult, void *pUserData);

	static void ConFindTimeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConFindMapInfo(IConsole::IResult *pResult, void *pUserData);

	static void ConFindHours(IConsole::IResult *pResult, void *pUserData);

	static void ConToggle45Degrees(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleSmallSens(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleDeepfly(IConsole::IResult *pResult, void *pUserData);

	static void ConUpdateNameplatesEditor(IConsole::IResult *pResult, void *pUserData);

	static void ConAddWhiteList(IConsole::IResult *pResult, void *pUserData);

public:
	CRClient();
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;
	void OnConsoleInit() override;
	void OnRender() override;

	static constexpr const char *RCLIENT_URL = "https://rushie-client.ru";
	static constexpr const char *RCLIENT_URL_USERS = "https://server.rushie-client.ru/users.json";
	static constexpr const char *RCLIENT_VERSION_URL = "https://server.rushie-client.ru/version";
	static constexpr const char *RCLIENT_TOKEN_URL = "https://server.rushie-client.ru/token";
	char m_aVersionStr[10] = "0";
	char m_aAuthToken[128] = {0};

	// GetInfofromDDstats
	std::shared_ptr<CHttpRequest> m_pRClientDDstatsTask = nullptr;
	void FetchRclientDDstatsProfile();
	void FinishRclientDDstatsProfile();
	void ResetRclientDDstatsProfile();
	char RclientSearchingNickname[16];
	int RclientFindSkinDDstatsSearch = 0;
	int RclientCopySkinDDstatsSearch = 0;
	int RclientFindPlayerDDstatsSearch = 0;
	int RclientCopyPlayerDDstatsSearch = 0;

	std::shared_ptr<CHttpRequest> m_pRClientVersionCheck = nullptr;
	void FetchRclientVersionCheck();
	void FinishRclientVersionCheck();
	void ResetRclientVersionCheck();
	int RclientVersionCheckDone = 0;
	bool NeedUpdate();

	std::shared_ptr<CHttpRequest> m_pAuthTokenTask = nullptr;
	void FetchAuthToken();
	void FinishAuthToken();
	void ResetAuthToken();

	// Find map rank
	std::shared_ptr<CHttpRequest> m_pSearchRankOnMapTask = nullptr;
	void FetchSearchRankOnMap();
	void FinishSearchRankOnMap();
	void ResetSearchRankOnMap();
	char NicknameForSearch[32];
	char MapForSearch[128];

	// Search map info
	std::shared_ptr<CHttpRequest> m_pSearchMapInfoTask = nullptr;
	void FetchSearchMapInfo();
	void FinishSearchMapInfo();
	void ResetSearchMapInfo();
	char MapForSearchMapInfo[128];

	//Back player profile after copy player
	char PlayerNameBeforeCopyPlayer[32];
	char PlayerSkinBeforeCopyPlayer[42];
	char PlayerClanBeforeCopyPlayer[24];
	int PlayerUseCustomColorBeforeCopyPlayer = 0;
	int PlayerBodyColorBeforeCopyPlayer = 0;
	int PlayerFeetColorBeforeCopyPlayer = 0;
	int PlayerCountryBeforeCopyPlayer = 0;
	char DummyNameBeforeCopyPlayer[32];
	char DummySkinBeforeCopyPlayer[42];
	char DummyClanBeforeCopyPlayer[24];
	int DummyUseCustomColorBeforeCopyPlayer = 0;
	int DummyBodyColorBeforeCopyPlayer = 0;
	int DummyFeetColorBeforeCopyPlayer = 0;
	int DummyCountryBeforeCopyPlayer = 0;

	//Tracker
	int TargetPositionId[MAX_CLIENTS];
	char TargetPositionNickname[MAX_CLIENTS][32];
	int TargetCount = 0;
	bool IsTracked(int ClientId);

	//WarList
	bool IsInWarlist(int ClientId, int Index);

	//FindHours
	//Async find_hours task and data
	std::shared_ptr<CHttpRequest> m_pFindHoursTask;
	char m_aFindHoursPlayer[32];
	// Flag to indicate if FindHours output should be written in chat
	bool m_WriteFindHoursInChat;
	void FetchFindHours(const char *pNickname, const char *pWriteinchat);
	void FinishFindHours();
	void ResetFindHours();

	//45 degrees
	int m_45degreestoggle = 0;
	int m_45degreestogglelastinput = 0;
	int m_45degreesEnabled = 0;
	// Small sens
	int m_Smallsenstoggle = 0;
	int m_Smallsenstogglelastinput = 0;
	int m_SmallsensEnabled = 0;
	//Deepfly
	int m_DeepflyEnabled = 0;
	char m_Oldmouse1Bind[128];

	// Copy nickname
	void RiCopyNicknamePlayer(const char *pNickname);

	// Regex
	static std::vector<std::string> SplitRegex(const char *aboba);
	static std::vector<std::string> SplitWords(const char *MSG);

	// Server and Player Info Collection
	std::shared_ptr<CHttpRequest> m_pRClientUsersTaskSend = nullptr;
	void SendServerPlayerInfo();
	void SendPlayerData(const char *pServerAddress, int ClientId, int DummyClientId = -1);
	void FetchRClientUsers();
	void FinishRClientUsers();
	void ResetRClientUsers();
	// void FinishRClientUsersSend();
	void ResetRClientUsersSend();
	bool IsPlayerRClient(int ClientId);
	char m_aCurrentServerAddress[256];
	std::shared_ptr<CHttpRequest> m_pRClientUsersTask = nullptr;
	std::vector<std::pair<std::string, int>> m_vRClientUsers; // server address, player id
	void SendDummyRclientUsers();
	int64_t s_LastFetch = 0;
	bool s_InitialFetchDone = false;
	bool s_InitialFetchDoneDummy = false;
	int s_RclientIndicatorCount = 0;
};

#endif
