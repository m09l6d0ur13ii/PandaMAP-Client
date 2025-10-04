#include <engine/shared/config.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include "pmclient.h"

static const float FONT_SIZE = 14.0f;
static const float HEADLINE_FONT_SIZE = 20.0f;
static const float LINE_HEIGHT = 20.0f;
static const float MARGIN_SMALL = 5.0f;
static const float MARGIN_SECTION = 25.0f;

void CMenus::RenderSettingsPmClient(CUIRect MainView)
{
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0.f, 0.f);
	CScrollRegionParams Params;
	Params.m_ScrollUnit = 120.f;
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &Params);
	MainView.y += ScrollOffset.y;
	MainView.VSplitRight(5.f, &MainView, nullptr);
	MainView.VSplitLeft(5.f, nullptr, &MainView);

	CUIRect Label, Line, ColLeft, ColRight;
	MainView.VSplitMid(&ColLeft, &ColRight, 20.0f);

	auto Section = [&](CUIRect &Col, const char *pTitle) {
		Col.HSplitTop(HEADLINE_FONT_SIZE + 5.f, &Label, &Col);
		Ui()->DoLabel(&Label, pTitle, HEADLINE_FONT_SIZE, TEXTALIGN_ML);
		Col.HSplitTop(MARGIN_SMALL, nullptr, &Col);
	};

	auto DoCheck = [&](CUIRect &Col, int *pVar, const char *pText, const char *pTooltip = nullptr) {
		Col.HSplitTop(LINE_HEIGHT, &Line, &Col);
		if(DoButton_CheckBox(pVar, pText, *pVar, &Line))
			*pVar ^= 1;
		if(pTooltip)
			GameClient()->m_Tooltips.DoToolTip(pVar, &Line, pTooltip);
		Col.HSplitTop(MARGIN_SMALL, nullptr, &Col);
	};

	// LEFT COLUMN
	{
		Section(ColLeft, Localize("PMClient - Dummy Hammer"));

		ColLeft.HSplitTop(LINE_HEIGHT, &Line, &ColLeft);
		if(DoButton_CheckBox(&g_Config.m_PmDummyHammer, Localize("Enable pm_dummy_hammer"), g_Config.m_PmDummyHammer, &Line))
			g_Config.m_PmDummyHammer ^= 1;
		GameClient()->m_Tooltips.DoToolTip(&g_Config.m_PmDummyHammer, &Line,
			Localize("Advanced dummy hammer automation"));

		if(g_Config.m_PmDummyHammer && g_Config.m_ClDummyHammer)
		{
			ColLeft.HSplitTop(LINE_HEIGHT, &Line, &ColLeft);
			ColorRGBA WarnColor(1.f, 0.4f, 0.4f, 0.85f);
			Line.Draw(WarnColor, IGraphics::CORNER_ALL, 5.f);
			Line.Margin(3.f, &Label);
			Ui()->DoLabel(&Label, Localize("cl_dummy_hammer will be disabled"), FONT_SIZE, TEXTALIGN_MC);
			ColLeft.HSplitTop(MARGIN_SMALL, nullptr, &ColLeft);
		}

		ColLeft.HSplitTop(LINE_HEIGHT * 2.f, &Line, &ColLeft);
		Ui()->DoScrollbarOption(&g_Config.m_PmDummyHammerDelay, &g_Config.m_PmDummyHammerDelay, &Line,
			Localize("Delay (ticks)"), 1, 1000, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_MULTILINE);
		ColLeft.HSplitTop(MARGIN_SMALL, nullptr, &ColLeft);

		Section(ColLeft, Localize("Keep Inputs"));
		DoCheck(ColLeft, &g_Config.m_PmDummyKeepMove, Localize("Keep movement"),
			Localize("Retain dummy movement input"));
		DoCheck(ColLeft, &g_Config.m_PmDummyKeepJump, Localize("Keep jump"),
			Localize("Keep jump pressed during cycle"));
		DoCheck(ColLeft, &g_Config.m_PmDummyKeepHook, Localize("Keep hook"),
			Localize("Keep hook pressed"));

		ColLeft.HSplitTop(LINE_HEIGHT, &Line, &ColLeft);
		CUIRect BtnAll, BtnNone;
		Line.VSplitMid(&BtnAll, &BtnNone);
		static CButtonContainer s_AllId, s_NoneId;
		if(DoButton_Menu(&s_AllId, Localize("All"), 0, &BtnAll))
			g_Config.m_PmDummyKeepMove = g_Config.m_PmDummyKeepJump = g_Config.m_PmDummyKeepHook = 1;
		if(DoButton_Menu(&s_NoneId, Localize("None"), 0, &BtnNone))
			g_Config.m_PmDummyKeepMove = g_Config.m_PmDummyKeepJump = g_Config.m_PmDummyKeepHook = 0;
		ColLeft.HSplitTop(MARGIN_SECTION, nullptr, &ColLeft);

		Section(ColLeft, Localize("Quick Console Commands"));
		static const char *s_aCmds[] = {
			"pm_hammer_on",
			"pm_hammer_off",
			"pm_hammer_toggle",
			"pm_hammer_delay 25",
			"pm_hammer_keep all",
			"pm_hammer_keep none",
			"pm_hammer_keep hook+jump",
		};
		static CButtonContainer s_aCmdBtns[sizeof(s_aCmds) / sizeof(s_aCmds[0])];
		for(size_t i = 0; i < sizeof(s_aCmds) / sizeof(s_aCmds[0]); ++i)
		{
			const char *pCmd = s_aCmds[i];
			ColLeft.HSplitTop(LINE_HEIGHT, &Line, &ColLeft);
			if(DoButton_Menu(&s_aCmdBtns[i], pCmd, 0, &Line, BUTTONFLAG_LEFT, nullptr,
				   IGraphics::CORNER_ALL, 5.f, 0.f, ColorRGBA(0, 0, 0, 0.25f)))
			{
				GameClient()->Console()->ExecuteLine(pCmd);
			}
			ColLeft.HSplitTop(MARGIN_SMALL, nullptr, &ColLeft);
		}
	}

	// RIGHT COLUMN
	{
		Section(ColRight, Localize("Bind Suggestions"));
		CUIRect LineBind;
		ColRight.HSplitTop(LINE_HEIGHT, &LineBind, &ColRight);
		Ui()->DoLabel(&LineBind, Localize("Example binds:"), FONT_SIZE, TEXTALIGN_ML);

		auto AddBindRow = [&](const char *pDesc, const char *pBind) {
			CUIRect ThisLine, L, R;
			ColRight.HSplitTop(LINE_HEIGHT, &ThisLine, &ColRight);
			ThisLine.VSplitMid(&L, &R);
			Ui()->DoLabel(&L, pDesc, FONT_SIZE, TEXTALIGN_ML);
			static CButtonContainer s_Btns[32];
			static int s_Idx = 0;
			int Idx = (s_Idx++) % 32;
			if(DoButton_Menu(&s_Btns[Idx], pBind, 0, &R, BUTTONFLAG_LEFT, nullptr,
				   IGraphics::CORNER_ALL, 5.f, 0.f, ColorRGBA(0, 0, 0, 0.35f)))
			{
				char aExec[128];
				str_format(aExec, sizeof(aExec), "echo \"%s\"; echo \"Bind manually: bind key '%s'\"", pBind, pBind);
				GameClient()->Console()->ExecuteLine(aExec);
			}
			ColRight.HSplitTop(MARGIN_SMALL, nullptr, &ColRight);
		};

		AddBindRow(Localize("Toggle pm hammer"), "pm_hammer_toggle");
		AddBindRow(Localize("All keep"), "pm_hammer_keep all");
		AddBindRow(Localize("Keep hook+jump"), "pm_hammer_keep hook+jump");
		AddBindRow(Localize("Disable keep"), "pm_hammer_keep none");

		ColRight.HSplitTop(MARGIN_SECTION, nullptr, &ColRight);
		Section(ColRight, Localize("Runtime Info"));

		CUIRect LineInfo;
		char aBuf[128];

		ColRight.HSplitTop(LINE_HEIGHT, &LineInfo, &ColRight);
		str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("State"),
			g_Config.m_PmDummyHammer ? Localize("ON") : Localize("OFF"));
		Ui()->DoLabel(&LineInfo, aBuf, FONT_SIZE, TEXTALIGN_ML);

		ColRight.HSplitTop(LINE_HEIGHT, &LineInfo, &ColRight);
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Delay (ticks)"),
			g_Config.m_PmDummyHammerDelay);
		Ui()->DoLabel(&LineInfo, aBuf, FONT_SIZE, TEXTALIGN_ML);

		ColRight.HSplitTop(LINE_HEIGHT, &LineInfo, &ColRight);
		str_format(aBuf, sizeof(aBuf), "Keep: M=%d J=%d H=%d",
			g_Config.m_PmDummyKeepMove, g_Config.m_PmDummyKeepJump, g_Config.m_PmDummyKeepHook);
		Ui()->DoLabel(&LineInfo, aBuf, FONT_SIZE, TEXTALIGN_ML);

		ColRight.HSplitTop(MARGIN_SECTION, nullptr, &ColRight);
		Section(ColRight, Localize("Notes"));

		static const char *s_aNotes[] = {
			"1) Requests hammer every tick",
			"2) Disables cl_dummy_hammer automatically",
			"3) Fire uses delay counter",
			"4) Keep flags can be mixed",
			"5) Custom binds via console/settings"};
		for(size_t i = 0; i < sizeof(s_aNotes) / sizeof(s_aNotes[0]); ++i)
		{
			CUIRect LN;
			ColRight.HSplitTop(LINE_HEIGHT, &LN, &ColRight);
			Ui()->DoLabel(&LN, s_aNotes[i], FONT_SIZE - 1.f, TEXTALIGN_ML);
		}
	}

	CUIRect EndDummy;
	EndDummy.x = MainView.x;
	EndDummy.y = std::max(ColLeft.y, ColRight.y);
	EndDummy.w = MainView.w;
	EndDummy.h = 0.f;
	s_ScrollRegion.AddRect(EndDummy);
	s_ScrollRegion.End();
}