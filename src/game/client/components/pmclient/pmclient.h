#ifndef GAME_CLIENT_COMPONENTS_PMCLIENT_PMCLIENT_H
#define GAME_CLIENT_COMPONENTS_PMCLIENT_PMCLIENT_H

#include <engine/console.h>
#include <game/client/component.h>

class CGameClient;

class CPmClient : public CComponent
{
public:
	CPmClient() = default;
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;

	int BuildPmDummyHammerInput(CGameClient &GC, int *pData);

	static int ClampDelay(int Delay);
	static void Echo(CPmClient *pSelf, const char *pMsg);

	static void ConPmHammerOn(IConsole::IResult *pResult, void *pUserData);
	static void ConPmHammerOff(IConsole::IResult *pResult, void *pUserData);
	static void ConPmHammerToggle(IConsole::IResult *pResult, void *pUserData);
	static void ConPmHammerDelay(IConsole::IResult *pResult, void *pUserData);
	static void ConPmHammerKeep(IConsole::IResult *pResult, void *pUserData);
	static void ConPmHammerPresetAll(IConsole::IResult *pResult, void *pUserData);
	static void ConPmHammerPresetNone(IConsole::IResult *pResult, void *pUserData);
};

#endif