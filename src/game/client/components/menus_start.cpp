/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <algorithm>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>

#include <engine/client/updater.h>
#include <engine/shared/config.h>

#include <generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/localization.h>
#include <game/version.h>

#include "menus_start.h"

#if defined(CONF_PLATFORM_ANDROID)
#include <android/android_main.h>
#endif

using namespace FontIcons;

void CMenusStart::RenderStartMenu(CUIRect MainView)
{
	GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_START);

	// render logo
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BANNER].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, 1);
	IGraphics::CQuadItem QuadItem(MainView.w / 2 - 170, 60, 360, 103);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	const float Rounding = 10.0f;
	const float VMargin = MainView.w / 2 - 190.0f;

	CUIRect Menu;
	MainView.VMargin(VMargin, &Menu);

	// отступ от логотипа сверху
	Menu.HSplitTop(200.0f, nullptr, &Menu);

	CUIRect Button;
	int NewPage = -1;

	const float ButtonHeight = 40.0f;
	const float ButtonSpacing = 5.0f;

	// --- Play ---
	Menu.HSplitTop(ButtonHeight, &Button, &Menu);
	static CButtonContainer s_PlayButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_PlayButton, Localize("Play", "Start menu"), 0, &Button, BUTTONFLAG_LEFT,
		   g_Config.m_ClShowStartMenuImages ? "play_game" : nullptr,
		   IGraphics::CORNER_ALL, Rounding, 0.5f,
		   ColorRGBA(0.0f, 0.0f, 0.0, 0.25f)) ||
		Ui()->ConsumeHotkey(CUi::HOTKEY_ENTER) || CheckHotKey(KEY_P))
	{
		NewPage = g_Config.m_UiPage >= CMenus::PAGE_INTERNET && g_Config.m_UiPage <= CMenus::PAGE_FAVORITE_COMMUNITY_5 ? g_Config.m_UiPage : CMenus::PAGE_INTERNET;
	}
	Menu.HSplitTop(ButtonSpacing, nullptr, &Menu);

	// --- Demos ---
	Menu.HSplitTop(ButtonHeight, &Button, &Menu);
	static CButtonContainer s_DemoButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_DemoButton, Localize("Demos"), 0, &Button, BUTTONFLAG_LEFT,
		   g_Config.m_ClShowStartMenuImages ? "demos" : nullptr,
		   IGraphics::CORNER_ALL, Rounding, 0.5f,
		   ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) ||
		CheckHotKey(KEY_D))
		NewPage = CMenus::PAGE_DEMOS;
	Menu.HSplitTop(ButtonSpacing, nullptr, &Menu);

	// --- Editor ---
	Menu.HSplitTop(ButtonHeight, &Button, &Menu);
	static CButtonContainer s_MapEditorButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_MapEditorButton, Localize("Editor"), 0, &Button, BUTTONFLAG_LEFT,
		   g_Config.m_ClShowStartMenuImages ? "editor" : nullptr,
		   IGraphics::CORNER_ALL, Rounding, 0.5f,
		   GameClient()->Editor()->HasUnsavedData() ? ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f) : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) ||
		CheckHotKey(KEY_E))
	{
		g_Config.m_ClEditor = 1;
		Input()->MouseModeRelative();
	}
	Menu.HSplitTop(ButtonSpacing, nullptr, &Menu);

	// --- Local server ---
	Menu.HSplitTop(ButtonHeight, &Button, &Menu);
	static CButtonContainer s_LocalServerButton;
	const bool LocalServerRunning = GameClient()->m_LocalServer.IsServerRunning();
	if(GameClient()->m_Menus.DoButton_Menu(&s_LocalServerButton, LocalServerRunning ? Localize("Stop server") : Localize("Run server"), 0, &Button, BUTTONFLAG_LEFT,
		   g_Config.m_ClShowStartMenuImages ? "local_server" : nullptr,
		   IGraphics::CORNER_ALL, Rounding, 0.5f,
		   LocalServerRunning ? ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f) : ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) ||
		(CheckHotKey(KEY_R) && Input()->KeyPress(KEY_R)))
	{
		if(LocalServerRunning)
			GameClient()->m_LocalServer.KillServer();
		else
			GameClient()->m_LocalServer.RunServer({});
	}
	Menu.HSplitTop(ButtonSpacing, nullptr, &Menu);

	// --- Settings ---
	Menu.HSplitTop(ButtonHeight, &Button, &Menu);
	static CButtonContainer s_SettingsButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_SettingsButton, Localize("Settings"), 0, &Button, BUTTONFLAG_LEFT,
		   g_Config.m_ClShowStartMenuImages ? "settings" : nullptr,
		   IGraphics::CORNER_ALL, Rounding, 0.5f,
		   ColorRGBA(0.0f, 0.0f, 0.0, 0.25f)) ||
		CheckHotKey(KEY_S))
		NewPage = CMenus::PAGE_SETTINGS;
	Menu.HSplitTop(ButtonSpacing, nullptr, &Menu);

	// --- Quit ---
	Menu.HSplitTop(ButtonHeight, &Button, &Menu);
	static CButtonContainer s_QuitButton;
	if(GameClient()->m_Menus.DoButton_Menu(&s_QuitButton, Localize("Quit"), 0, &Button, BUTTONFLAG_LEFT,
		   g_Config.m_ClShowStartMenuImages ? "quit_btn" : nullptr,
		   IGraphics::CORNER_ALL, Rounding, 0.5f,
		   ColorRGBA(0.0f, 0.0f, 0.0, 0.25f)) ||
		CheckHotKey(KEY_Q))
	{
		if(GameClient()->Editor()->HasUnsavedData() ||
			(GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmQuitTime && g_Config.m_ClConfirmQuitTime >= 0))
			GameClient()->m_Menus.ShowQuitPopup();
		else
			Client()->Quit();
	}

	// --- Персонаж, смотрящий за курсором, справа от кнопок ---
	{
		const float CharPreviewW = std::clamp(MainView.w * 0.13f, 80.0f, 140.0f); // ширина блока предпросмотра (регулируется в процентах от ширины экрана, но зажата в диапазоне)
		const float CharPreviewH = std::clamp(MainView.h * 0.28f, 90.0f, 180.0f); // высота блока предпросмотра
		CUIRect CharPreviewRect;

		// Центрируем по вертикали относительно MainView (0.5f = центр, 0.65f = чуть ниже центра)
		CharPreviewRect.y = MainView.y + (MainView.h - CharPreviewH) * 0.65f;

		// Ставим справа от центра (0.5f = центр, 1.0f = край справа, сейчас 0.70f = немного ближе к центру)
		CharPreviewRect.x = MainView.x + MainView.w * 0.70f;

		CharPreviewRect.w = CharPreviewW; // фиксируем ширину
		CharPreviewRect.h = CharPreviewH; // фиксируем высоту

		// --- Фон всего блока ---
		ColorRGBA bgColor = ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f); // цвет фона (RGBA)
		CUIRect CharPreviewWithLabels = CharPreviewRect;
		CharPreviewWithLabels.y -= 40.0f; // ↑ увеличить фон сверху, чтобы влез ник и клан (регулируй)
		CharPreviewWithLabels.h += 40.0f; // ↑ расширяем фон вниз, чтобы компенсировать смещение (регулируй)
		CharPreviewWithLabels.Draw(bgColor, IGraphics::CORNER_ALL, 10.0f);

		// --- Скины и параметры игрока ---
		const char *pSkinName = g_Config.m_ClPlayerSkin;
		int UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
		unsigned ColorBody = g_Config.m_ClPlayerColorBody;
		unsigned ColorFeet = g_Config.m_ClPlayerColorFeet;
		int Emote = g_Config.m_ClPlayerDefaultEyes;

		const CSkin *pDefaultSkin = GameClient()->m_Skins.Find("default");
		const CSkins::CSkinContainer *pOwnSkinContainer = GameClient()->m_Skins.FindContainerOrNullptr(pSkinName[0] == '\0' ? "default" : pSkinName);
		if(pOwnSkinContainer != nullptr && pOwnSkinContainer->IsSpecial())
		{
			pOwnSkinContainer = nullptr;
		}

		CTeeRenderInfo OwnSkinInfo;
		OwnSkinInfo.Apply(pOwnSkinContainer == nullptr || pOwnSkinContainer->Skin() == nullptr ? pDefaultSkin : pOwnSkinContainer->Skin().get());
		OwnSkinInfo.ApplyColors(UseCustomColor, ColorBody, ColorFeet);
		OwnSkinInfo.m_Size = 50.0f; // размер тишки

		// --- Ник и клан над тишкой ---
		CUIRect NameRect = CharPreviewRect;
		NameRect.h = 16.0f; // высота области для текста (регулируй)
		NameRect.y -= 3.0f; // на сколько поднять над тишкой (регулируй)
		char aNameBuf[64];
		str_format(aNameBuf, sizeof(aNameBuf), "%s", g_Config.m_PlayerName);
		Ui()->DoLabel(&NameRect, aNameBuf, 13.0f, TEXTALIGN_MC); // размер 13.0f (регулируй)

		NameRect.y += 13.0f; // отступ вниз для клана (регулируй)
		char aClanBuf[64];
		str_format(aClanBuf, sizeof(aClanBuf), "%s", g_Config.m_PlayerClan);
		Ui()->DoLabel(&NameRect, aClanBuf, 10.0f, TEXTALIGN_MC); // размер клана меньше (10.0f)

		// --- Позиция самой тишки ---
		vec2 OffsetToMid;
		CRenderTools::GetRenderTeeOffsetToRenderedTee(CAnimState::GetIdle(), &OwnSkinInfo, OffsetToMid);
		const vec2 TeeRenderPos = vec2(
			CharPreviewRect.x + CharPreviewRect.w / 2.0f, // центр блока по X (регулируй сдвиг)
			CharPreviewRect.y + CharPreviewRect.h * 0.32f + OffsetToMid.y // позиция по Y (0.32f = регулируй высоту)
		);

		// --- Реакция тишки на курсор ---
		const vec2 DeltaPosition = Ui()->MousePos() - TeeRenderPos;
		const float Distance = length(DeltaPosition);
		const float InteractionDistance = 20.0f; // дистанция, на которой меняется эмоция (регулируй)
		const vec2 TeeDirection = Distance < InteractionDistance ? normalize(vec2(DeltaPosition.x, maximum(DeltaPosition.y, 0.5f))) : normalize(DeltaPosition);
		const int TeeEmote = Distance < InteractionDistance ? EMOTE_HAPPY : Emote;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, TeeEmote, TeeDirection, TeeRenderPos);

		// --- Кнопка профиля под тишкой ---
		CUIRect ProfileBtn;
		float btnHeight = std::clamp(CharPreviewRect.h * 0.13f, 22.0f, 30.0f); // высота кнопки (регулируй)
		float btnWidth = std::clamp(CharPreviewRect.w * 0.85f, 40.0f, 90.0f); // ширина кнопки (регулируй)
		CharPreviewRect.HSplitBottom(btnHeight + 6.0f, &CharPreviewRect, &ProfileBtn); // отступ кнопки от низа блока (6.0f регулируй)
		ProfileBtn.HMargin((ProfileBtn.h - btnHeight) / 2.0f, &ProfileBtn); // вертикальный отступ внутри (регулируй)
		ProfileBtn.x = CharPreviewRect.x + (CharPreviewRect.w - btnWidth) / 2.0f; // центрируем кнопку по X (регулируй)
		ProfileBtn.w = btnWidth; // ширина кнопки
		ProfileBtn.h = btnHeight; // высота кнопки
		static CButtonContainer s_ProfileBtn;
		if(GameClient()->m_Menus.DoButton_Menu(&s_ProfileBtn, Localize("TClient Profiles"), 0, &ProfileBtn, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 8.0f, 0.5f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			NewPage = CMenus::PAGE_SETTINGS;
			g_Config.m_UiSettingsPage = CMenus::SETTINGS_PROFILES; // Открывает вкладку профилей TClient через глобальный конфиг
		}
	}

	// --- Версии снизу справа ---
	CUIRect VersionBlock;
	MainView.HSplitBottom(80.0f, &MainView, &VersionBlock);
	VersionBlock.VSplitLeft(MainView.w - 200.0f, &MainView, &VersionBlock);

	float FontSize = 14.0f;
	float LineSpacing = 18.0f;

	CUIRect LineBlock = VersionBlock;
	char aBuf[64];

	str_format(aBuf, sizeof(aBuf), "DDNet %s", GAME_RELEASE_VERSION);
	Ui()->DoLabel(&LineBlock, aBuf, FontSize, TEXTALIGN_MR);
	LineBlock.y += LineSpacing;

	str_format(aBuf, sizeof(aBuf), "TClient %s", TCLIENT_VERSION);
	Ui()->DoLabel(&LineBlock, aBuf, FontSize, TEXTALIGN_MR);
	LineBlock.y += LineSpacing;

	str_format(aBuf, sizeof(aBuf), "RClient %s", RCLIENT_VERSION);
	Ui()->DoLabel(&LineBlock, aBuf, FontSize, TEXTALIGN_MR);

#if defined(CONF_AUTOUPDATE)
	// --- Для автообновления используем тот же блок ---
	CUIRect VersionUpdate = VersionBlock;

	CUIRect UpdateButton;
	VersionUpdate.VSplitRight(100.0f, &VersionUpdate, &UpdateButton);
	VersionUpdate.VSplitRight(10.0f, &VersionUpdate, nullptr);

	char aBuf2[128];
	const IUpdater::EUpdaterState State = Updater()->GetCurrentState();
	const bool NeedUpdate = GameClient()->m_RClient.NeedUpdate();

	if(State == IUpdater::CLEAN && NeedUpdate)
	{
		static CButtonContainer s_VersionUpdate;
		if(GameClient()->m_Menus.DoButton_Menu(&s_VersionUpdate, Localize("Update now"), 0, &UpdateButton, BUTTONFLAG_LEFT, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(1.0f, 0.2f, 0.2f, 0.25f)))
		{
			Updater()->InitiateUpdate();
		}
	}
	else if(State == IUpdater::NEED_RESTART)
	{
		static CButtonContainer s_VersionUpdate;
		if(GameClient()->m_Menus.DoButton_Menu(&s_VersionUpdate, Localize("Restart"), 0, &UpdateButton, BUTTONFLAG_LEFT, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(1.0f, 0.2f, 0.2f, 0.25f)))
		{
			Client()->Restart();
		}
	}
	else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
	{
		Ui()->RenderProgressBar(UpdateButton, Updater()->GetCurrentPercent() / 100.0f);
	}

	if(State == IUpdater::CLEAN && NeedUpdate)
	{
		str_format(aBuf2, sizeof(aBuf2), Localize("Rushie client %s is out!"), GameClient()->m_RClient.m_aVersionStr);
		TextRender()->TextColor(1.0f, 0.2f, 0.2f, 1.0f);
	}
	else if(State == IUpdater::CLEAN)
	{
		aBuf2[0] = '\0';
	}
	else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
	{
		char aCurrentFile[64];
		Updater()->GetCurrentFile(aCurrentFile, sizeof(aCurrentFile));
		str_format(aBuf2, sizeof(aBuf2), Localize("Downloading %s:"), aCurrentFile);
	}
	else if(State == IUpdater::FAIL)
	{
		str_copy(aBuf2, Localize("Update failed! Check log…"));
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	else if(State == IUpdater::NEED_RESTART)
	{
		str_copy(aBuf2, Localize("DDNet Client updated!"));
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	Ui()->DoLabel(&VersionUpdate, aBuf2, 14.0f, TEXTALIGN_ML);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
#elif defined(CONF_INFORM_UPDATE)
	if(str_comp(Client()->LatestVersion(), "0") != 0 && false)
	{
		char aBuf2[64];
		str_format(aBuf2, sizeof(aBuf2), Localize("DDNet %s is out!"), Client()->LatestVersion());
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		Ui()->DoLabel(&VersionBlock, aBuf2, 14.0f, TEXTALIGN_MC);
	}
#endif

	if(NewPage != -1)
	{
		GameClient()->m_Menus.SetShowStart(false);
		GameClient()->m_Menus.SetMenuPage(NewPage);
	}
}

bool CMenusStart::CheckHotKey(int Key) const
{
	return !Input()->ShiftIsPressed() && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() &&
	       Input()->KeyPress(Key) &&
	       !GameClient()->m_GameConsole.IsActive();
}
