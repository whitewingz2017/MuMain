#include "stdafx.h"

#include "Scenes/ScenePostProcessing.h"
#include "Data/GameConfig/GameConfig.h"

#include "GameLogic/Pets/w_PetProcess.h"
#include "Render/PostProcessing/MapColorGrading.h"
#include "Render/PostProcessing/PostProcessingSetting.h"
#include "Render/Renderer/MuRenderer.h"
#include "World/MapInfra/MapManager.h"

namespace Scenes::PostProcessing
{

void EndSceneGeometry()
{
    Render::PostProcessing::SelectMapColorGrading(gMapManager.WorldActive);
    Render::PostProcessing::SubmitPostProcessingLook();
    const float scale = 1.f;
    mu::GetRenderer().EndSceneGeometry(scale);
}

void RenderPetsWithoutBloom()
{
    const mu::ScopedBloomExclusion classicPets;
    ThePetProcess().RenderPets();
}

} // namespace Scenes::PostProcessing
