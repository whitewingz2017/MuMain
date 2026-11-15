///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzInterface.h"
#include "Render/Textures/ZzzTexture.h"
#include "SceneCore.h"

#ifdef _EDITOR
#include "../MuEditor/Core/MuEditorCore.h"
#include "imgui.h"
#endif

#include "Engine/Physics/PhysicsManager.h"

#include "GameLogic/Quests/CSQuest.h"
#include "UI/HUD/UIMapName.h"	// rozy
#include "Core/Time/Timer.h"
#include "LoadingScene.h"
#include "GameLogic/Events/Cinematic/CDirection.h"

#include "UI/Core/WindowSystem.h"
#include <chrono>
#include <thread>

#include "Camera/CameraUtility.h"
#include "Scenes/SceneManager.h"
#include "Core/Input/ImeInput.h"

extern int g_iChatInputType;
extern BOOL g_bUseChatListBox;
extern DWORD g_dwMouseUseUIID;
extern DWORD g_dwActiveUIID;
extern DWORD g_dwKeyFocusUIID;
extern CUIMapName* g_pUIMapName;
extern bool HighLight;
extern CTimer* g_pTimer;
extern BOOL g_bGameServerConnected;

bool	g_bTimeCheck = false;
int 	g_iBackupTime = 0;

float	g_fMULogoAlpha = 0;

extern float g_fSpecialHeight;

short   g_shCameraLevel = 0;

int g_iLengthAuthorityCode = 20;

const wchar_t* szServerIpAddress = L"127.127.127.127";
WORD g_ServerPort = 44406;

EGameScene  SceneFlag = WEBZEN_SCENE;

extern int g_iKeyPadEnable;

CPhysicsManager g_PhysicsManager;

#define MAX_LENGTH_CMB	( 38)

DWORD   g_dwWaitingStartTick;
int     g_iRequestCount;

int     g_iMessageTextStart = 0;
char    g_cMessageTextCurrNum = 0;
char    g_cMessageTextNum = 0;
int     g_iNumLineMessageBoxCustom;
wchar_t    g_lpszMessageBoxCustom[NUM_LINE_CMB][MAX_LENGTH_CMB];
int     g_iCustomMessageBoxButton[NUM_BUTTON_CMB][NUM_PAR_BUTTON_CMB];

int     g_iCustomMessageBoxButton_Cancel[NUM_PAR_BUTTON_CMB];

int		g_iCancelSkillTarget = 0;

#define NUM_LINE_DA		( 1)
int g_iCurrentDialogScript = -1;
int g_iNumAnswer = 0;
wchar_t g_lpszDialogAnswer[MAX_ANSWER_FOR_DIALOG][NUM_LINE_DA][MAX_LENGTH_CMB];


extern int HeroKey;

int DeleteGuildIndex = -1;

int  ErrorMessage = 0;
extern bool g_bEnterPressed;

extern wchar_t LogInID[MAX_USERNAME_SIZE + 1];
extern wchar_t m_ExeVersion[11];

BOOL Util_CheckOption(std::wstring lpszCommandLine, wchar_t cOption, std::wstring &lpszString);


bool MoveMainCamera();

// Note: Camera variables now in g_Camera (see backward compatibility layer in ZzzOpenglUtil.h)

extern int WaterTextureNumber;

int TestTime = 0;
extern int  GrabScreen;

void MoveCharacter(CHARACTER* c, OBJECT* o);

float g_Luminosity;

extern int g_iNoMouseTime;

namespace
{
// How a wait for the next capped frame sleeps. A sleep can wake up to a timer tick late: about 1 ms
// with the 1 ms timer resolution (precise), the default 15.6 ms tick otherwise.
struct FrameSleepTuning
{
    double wakeMarginMs; // the sleep ends this long before the frame's deadline
    double minSleepMs;   // a shorter wait is yielded instead of slept
};

constexpr FrameSleepTuning PreciseFrameSleep{1.5, 1.0};
constexpr FrameSleepTuning CoarseFrameSleep{4.0, 16.0};

// Longest single sleep, so events and packets keep being polled while a frame waits.
constexpr double MaxFrameSleepMs = 10.0;

// The last stretch before a frame's deadline is spun instead of slept or spent on another pass of the
// main loop, so the frame starts on its deadline. Longer than the precise wake margin, so a precise
// sleep always ends in this spin.
constexpr double FrameSpinWindowMs = 2.0;

void SpinUntilFrameDeadline()
{
    do
    {
        g_frameTiming.UpdateCurrentTime(g_pTimer->GetTimeElapsed());
    } while (!g_frameTiming.ShouldRenderNextFrame());
}
} // namespace

void WaitForNextActivity(bool usePreciseSleep)
{
    const FrameSleepTuning& tuning = usePreciseSleep ? PreciseFrameSleep : CoarseFrameSleep;
    const double sleepMs = g_frameTiming.GetTimeUntilNextFrame() - tuning.wakeMarginMs;
    if (sleepMs < tuning.minSleepMs)
    {
        std::this_thread::yield();
        return;
    }

    const double cappedSleepMs = std::min(sleepMs, MaxFrameSleepMs);
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(cappedSleepMs)));
}

bool CheckRenderNextFrame()
{
    g_frameTiming.UpdateCurrentTime(g_pTimer->GetTimeElapsed());
    if (g_frameTiming.ShouldRenderNextFrame())
    {
        return true;
    }

    if (g_frameTiming.GetTimeUntilNextFrame() > FrameSpinWindowMs)
    {
        return false;
    }

    SpinUntilFrameDeadline();
    return true;
}
