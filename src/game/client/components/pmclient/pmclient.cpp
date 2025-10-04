#include "pmclient.h"
#include <engine/console.h>
#include <engine/shared/config.h>
#include <game/client/gameclient.h>

int CPmClient::BuildPmDummyHammerInput(CGameClient &GC, int *pData)
{
	if(!g_Config.m_PmDummyHammer)
		return 0;
	if(GC.m_aLocalIds[!g_Config.m_ClDummy] < 0)
		return 0;

	const bool FireThisTick = (GC.m_DummyFire % g_Config.m_PmDummyHammerDelay) == 0;
	GC.m_DummyFire++;

	GC.m_HammerInput.m_Direction = g_Config.m_PmDummyKeepMove ? GC.m_DummyInput.m_Direction : 0;
	GC.m_HammerInput.m_Jump = g_Config.m_PmDummyKeepJump ? GC.m_DummyInput.m_Jump : 0;
	GC.m_HammerInput.m_Hook = g_Config.m_PmDummyKeepHook ? GC.m_DummyInput.m_Hook : 0;

	// Disable simple dummy hammer to avoid conflict
	g_Config.m_ClDummyHammer = 0;

	const vec2 Dir = GC.m_LocalCharacterPos - GC.m_aClients[GC.m_aLocalIds[!g_Config.m_ClDummy]].m_Predicted.m_Pos;
	GC.m_HammerInput.m_TargetX = (int)Dir.x;
	GC.m_HammerInput.m_TargetY = (int)Dir.y;

	// Force hammer
	GC.m_HammerInput.m_WantedWeapon = WEAPON_HAMMER + 1;
	GC.m_HammerInput.m_Fire = FireThisTick ? ((GC.m_HammerInput.m_Fire + 1) | 1) : 0;

	if(!g_Config.m_ClDummyRestoreWeapon)
		GC.m_DummyInput.m_WantedWeapon = WEAPON_HAMMER + 1;

	mem_copy(pData, &GC.m_HammerInput, sizeof(GC.m_HammerInput));
	return sizeof(GC.m_HammerInput);
}

void CPmClient::OnConsoleInit()
{
	Console()->Register("pm_hammer_on", "", CFGFLAG_CLIENT, ConPmHammerOn, this, "Enable pm_dummy_hammer");
	Console()->Register("pm_hammer_off", "", CFGFLAG_CLIENT, ConPmHammerOff, this, "Disable pm_dummy_hammer");
	Console()->Register("pm_hammer_toggle", "", CFGFLAG_CLIENT, ConPmHammerToggle, this, "Toggle pm_dummy_hammer");
	Console()->Register("pm_hammer_delay", "i[delay]", CFGFLAG_CLIENT, ConPmHammerDelay, this, "Set hammer delay in ticks");
	Console()->Register("pm_hammer_keep", "s[mask]", CFGFLAG_CLIENT, ConPmHammerKeep, this, "Configure keep mask (all|none|hook|move|jump or combination with '+')");
	Console()->Register("pm_hammer_keep_all", "", CFGFLAG_CLIENT, ConPmHammerPresetAll, this, "Keep hook+move+jump");
	Console()->Register("pm_hammer_keep_none", "", CFGFLAG_CLIENT, ConPmHammerPresetNone, this, "Disable keep hook/move/jump");
}

int CPmClient::ClampDelay(int Delay)
{
	if(Delay < 1)
		Delay = 1;
	if(Delay > 1000)
		Delay = 1000;
	return Delay;
}

void CPmClient::Echo(CPmClient *pSelf, const char *pMsg)
{
	if(pSelf && pSelf->GameClient())
		pSelf->GameClient()->Echo(pMsg);
}

void CPmClient::ConPmHammerOn(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CPmClient *>(pUserData);
	g_Config.m_PmDummyHammer = 1;
	g_Config.m_ClDummyHammer = 0;
	Echo(pSelf, "[[green]]pm_dummy_hammer enabled");
}

void CPmClient::ConPmHammerOff(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CPmClient *>(pUserData);
	g_Config.m_PmDummyHammer = 0;
	Echo(pSelf, "[[red]]pm_dummy_hammer disabled");
}

void CPmClient::ConPmHammerToggle(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CPmClient *>(pUserData);
	g_Config.m_PmDummyHammer = !g_Config.m_PmDummyHammer;
	if(g_Config.m_PmDummyHammer)
	{
		g_Config.m_ClDummyHammer = 0;
		Echo(pSelf, "[[green]]pm_dummy_hammer ON");
	}
	else
		Echo(pSelf, "[[red]]pm_dummy_hammer OFF");
}

void CPmClient::ConPmHammerDelay(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CPmClient *>(pUserData);
	int Delay = pResult->GetInteger(0);
	Delay = ClampDelay(Delay);
	g_Config.m_PmDummyHammerDelay = Delay;
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "[[white]]Delay: %d", Delay);
	Echo(pSelf, aBuf);
}

void CPmClient::ConPmHammerKeep(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CPmClient *>(pUserData);
	const char *pMask = pResult->GetString(0);

	g_Config.m_PmDummyKeepHook = 0;
	g_Config.m_PmDummyKeepMove = 0;
	g_Config.m_PmDummyKeepJump = 0;

	if(!str_comp_nocase(pMask, "all"))
	{
		g_Config.m_PmDummyKeepHook = 1;
		g_Config.m_PmDummyKeepMove = 1;
		g_Config.m_PmDummyKeepJump = 1;
	}
	else if(!str_comp_nocase(pMask, "none"))
	{
		// already cleared
	}
	else
	{
		char aCopy[64];
		str_copy(aCopy, pMask, sizeof(aCopy));
		char *pTok = strtok(aCopy, "+");
		while(pTok)
		{
			if(!str_comp_nocase(pTok, "hook"))
				g_Config.m_PmDummyKeepHook = 1;
			else if(!str_comp_nocase(pTok, "move"))
				g_Config.m_PmDummyKeepMove = 1;
			else if(!str_comp_nocase(pTok, "jump"))
				g_Config.m_PmDummyKeepJump = 1;
			pTok = strtok(nullptr, "+");
		}
	}

	char aBuf[96];
	str_format(aBuf, sizeof(aBuf), "[[white]]keep: hook=%d move=%d jump=%d",
		g_Config.m_PmDummyKeepHook,
		g_Config.m_PmDummyKeepMove,
		g_Config.m_PmDummyKeepJump);
	Echo(pSelf, aBuf);
}

void CPmClient::ConPmHammerPresetAll(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CPmClient *>(pUserData);
	g_Config.m_PmDummyKeepHook = 1;
	g_Config.m_PmDummyKeepMove = 1;
	g_Config.m_PmDummyKeepJump = 1;
	Echo(pSelf, "[[white]]keep preset ALL (hook/move/jump)");
}

void CPmClient::ConPmHammerPresetNone(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CPmClient *>(pUserData);
	g_Config.m_PmDummyKeepHook = 0;
	g_Config.m_PmDummyKeepMove = 0;
	g_Config.m_PmDummyKeepJump = 0;
	Echo(pSelf, "[[white]]keep preset NONE");
}