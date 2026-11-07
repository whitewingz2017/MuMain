"""Exercise the real skin glow and rank overlay code without starting the game."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/source"


def implementation(path):
    return "\n".join(line for line in (SOURCE / path).read_text(encoding="utf-8").splitlines()
                     if not line.startswith("#include"))


def glow_test():
    source = implementation("Render/Items/ItemGlow.cpp")
    look = "look, " if "DrawLook&" in source else ""
    return r'''
#include <algorithm>
#include <cassert>
#include "Render/Items/ItemGlow.h"
constexpr int MODEL_EVENT=100000, MODEL_ARMORINVEN_74=100100;
constexpr int RENDER_TEXTURE=2, RENDER_BRIGHT=64, RENDER_METAL=16;
class OBJECT { public: int Type=42,BlendMesh=-1; float Alpha=1,BlendMeshLight=1,BlendMeshTexCoordU=0,BlendMeshTexCoordV=0; };
class BMD { public: bool HideSkin=false; int draws=0;
 template<class... T> void RenderMesh(T...){++draws;}
 template<class... T> void RenderBody(T...){++draws;}
};
Data::Items::ItemModelDefinition model;
struct Database {
 const Data::Items::ItemModelDefinition* Find(int id){return id==42?&model:nullptr;}
 const Data::Items::ItemGlowColors* FindGlowColors(int){return nullptr;}
} g_ItemModelDatabase;
namespace Render::Items {
int GetItemTypeOfModel(int id){return id;}
namespace Display { std::optional<int> GetItemOfInventoryModel(int){return {};} }
namespace Styles { template<class... T> bool RenderGlow(T...){return false;} }
}
''' + source + r'''
int main(){
 using namespace Render::Items::Glow;
 BMD mesh; OBJECT object;
''' + (" DrawLook look;\n" if look else "") + r'''
 for(int material=0;material<4;++material){
  model.material={}; model.staticEffects.clear();
  if(material==1) model.material.layers.emplace_back();
  if(material==2) model.material.additiveMeshes={1};
  if(material==3) model.staticEffects.emplace_back();
  for(int level=0;level<=15;++level){
   const bool ownGlow=material==1 || material==2;
   assert(GetLevel(42,level)==(ownGlow?0:level));
   mesh.draws=0;
''' + f'''
   RenderGlow(&mesh,&object,{look}42,1.f,RENDER_METAL,-1);
   RenderShine(&mesh,&object,{look}42,1.f,RENDER_METAL,-1);
   RenderExcellentGlow(&mesh,&object,{look}42,1.f);
''' + r'''
   assert(mesh.draws==(ownGlow?0:3));
  }
 }
 model.material={}; model.glow.levels.emplace(); model.glow.levels->fill(8);
 assert(GetLevel(42,15)==8);
 assert(GetLevel(77,15)==15);
}
'''


def rank_test():
    chat = implementation("UI/Chat/Chat.cpp")
    name_top = chat[chat.index("int NameLabelTop("):chat.index("void RenderBooleans(")]
    return r'''
#include <algorithm>
#include <cassert>
#include <climits>
#include <cwchar>
#include <unordered_map>
#include <vector>
#include "json.hpp"
#include "GameLogic/RankTitles/RankTitleStore.h"
#include "GameLogic/RankTitles/RankShop.h"
#include "Data/GameData/RankTitles/RankTitles.h"
#include "UI/Chat/RankBadge.h"
using BYTE=std::uint8_t; using Uint64=std::uint64_t; using GLuint=unsigned int; using vec3_t=float[3];
constexpr int MAX_CHARACTERS_CLIENT=4, MAIN_SCENE=1, KIND_PLAYER=1, MODEL_PLAYER=2;
constexpr int eBuff_Cloaking=1,eBuff_CrywolfNPCHide=2,BITMAP_UNKNOWN=0;
constexpr int PARTS_WEBZEN=3,MODEL_WEBZEN_MARK=55;
int SceneFlag=MAIN_SCENE;
struct TestBounds { float StartPos[3]{},XAxis[3]{},YAxis[3]{},ZAxis[3]{}; };
class OBJECT { public: bool Live=true,Visible=true,cloaked=false,hidden=false; int Kind=KIND_PLAYER,Type=MODEL_PLAYER;
 float Alpha=1,Position[3]{},BoundingBoxMax[3]{0,0,100}; TestBounds OBB; };
class CHARACTER { public: OBJECT Object; int Key=0,EtcPart=0; void* m_pParts=nullptr; wchar_t ID[40]{}; } CharactersClient[MAX_CHARACTERS_CLIENT];
class CSIPartsMDL { public: OBJECT object; OBJECT* GetObject(){return &object;} };
CHARACTER* Hero=&CharactersClient[0];
int FindCharacterIndex(int id){for(int i=0;i<MAX_CHARACTERS_CLIENT;++i)if(CharactersClient[i].Key==id)return i;return -1;}
bool g_isCharacterBuff(OBJECT* object,int buff){return buff==eBuff_Cloaking?object->cloaked:object->hidden;}
void Vector(float x,float y,float z,float* out){out[0]=x;out[1]=y;out[2]=z;}
int g_Camera=0;
class CameraProjection { public: static void WorldToScreen(int,const float* world,int* x,int* y){*x=100+int(world[0]);*y=260-int(world[2]);} };
namespace UI::Chat {
struct CHAT { CHARACTER* Owner=nullptr; float IDLifeTime=0,LifeTime[2]{}; int y=100; wchar_t ID[40]{}; } Chat[4];
bool HasCurrentOwnerName(const wchar_t* cached,const wchar_t* current){return std::wcscmp(cached,current)==0;}
''' + name_top + r'''
}
int badges=0; float badgeX=0,badgeY=0;
int badgeTexture=0;
float badgeU=0,badgeV=0,badgeWidth=0,badgeHeight=0;
struct BadgeDraw { int texture; float x,y,width,height,opacity; };
std::vector<BadgeDraw> badgeDraws;
void EnableAlphaTest(){}
void RenderBitmap(int texture,float x,float y,float drawWidth,float drawHeight,float u=0,float v=0,
 float width=1,float height=1,bool=true,bool=true,float opacity=1){
 ++badges;badgeTexture=texture;badgeX=x;badgeY=y;badgeU=u;badgeV=v;badgeWidth=width;badgeHeight=height;
 badgeDraws.push_back({texture,x,y,drawWidth,drawHeight,opacity});
}
namespace Render::Textures::ImageByPath {
 GLuint Get(const std::string& path){return path=="frames.tga"?10:path=="rank.tga"?9:BITMAP_UNKNOWN;}
}
namespace StringUtils { std::wstring NarrowToWide(const std::string& text){return {text.begin(),text.end()};} }
namespace Data::RankTitles {
RankTitle title;
const RankTitle* Find(int rank){return rank>0 && rank<=18 && rank!=10?&title:nullptr;}
}
Uint64 ticks=0;
Uint64 SDL_GetTicks(){return ticks;}
struct Connection {
 std::uint32_t request=0;
 bool IsConnected(){return true;}
 void Send(const BYTE* data,size_t){request=0;for(int i=0;i<4;++i)request|=std::uint32_t(data[11+i])<<(8*i);}
} connection;
auto* SocketClient=&connection;
''' + implementation("GameLogic/RankTitles/RankTitleStore.cpp") + implementation("GameLogic/RankTitles/RankShop.cpp") + implementation("UI/Chat/RankBadge.cpp") + r'''
std::vector<BYTE> Packet(int sub,const nlohmann::json& json){
 auto text=json.dump();size_t size=text.size()+5;
 std::vector<BYTE> result{0xC2,BYTE(size>>8),BYTE(size),0xFD,BYTE(sub)};
 result.insert(result.end(),text.begin(),text.end());return result;
}
void Confirm(int rank){
 GameLogic::RankTitles::Shop::Receive(Packet(8,{{"RequestId",connection.request},{"Status","Ready"},
  {"Balance",100},{"Active",rank},{"Ranks",nlohmann::json::array({{{"Rank",4},{"Price",100},{"Owned",true}}})}}));
}
int main(){
 using namespace GameLogic::RankTitles;
 Hero->Key=100;std::wcscpy(Hero->ID,L"Alice");
 Data::RankTitles::title.badge.image="rank.tga";
 Data::RankTitles::title.badge.width=78;Data::RankTitles::title.badge.height=40;
 assert(Shop::Load());Confirm(4);
 assert(Shop::Active()==4 && RankOf(Hero)==4);
 // No name/chat bubble exists: the badge still draws from the world overlay.
 UI::RankTitles::Badge::RenderVisible();assert(badges==1 && badgeX==61 && badgeY==58);
 auto& name=UI::Chat::Chat[0];name.Owner=Hero;std::wcscpy(name.ID,L"Alice");name.IDLifeTime=1;name.y=60;
 UI::RankTitles::Badge::RenderVisible();assert(badgeY==18);
 name.y=45;UI::RankTitles::Badge::RenderVisible();assert(badgeY==3);
 name.IDLifeTime=0;UI::RankTitles::Badge::RenderVisible();assert(badgeY==58);
 CSIPartsMDL gmMark;gmMark.object.Type=MODEL_WEBZEN_MARK;
 gmMark.object.OBB.StartPos[2]=200;gmMark.object.OBB.XAxis[0]=30;
 gmMark.object.OBB.YAxis[1]=10;gmMark.object.OBB.ZAxis[2]=20;
 Hero->EtcPart=PARTS_WEBZEN;Hero->m_pParts=&gmMark;
 UI::RankTitles::Badge::RenderVisible();assert(badgeY==-2);
 // A lower GM mark replaces the idle head clearance instead of leaving a large gap.
 gmMark.object.OBB.StartPos[2]=100;
 UI::RankTitles::Badge::RenderVisible();assert(badgeY==98);
 name.IDLifeTime=1;name.y=10;UI::RankTitles::Badge::RenderVisible();assert(badgeY==-32);
 name.y=125;UI::RankTitles::Badge::RenderVisible();assert(badgeY==83);
 gmMark.object.Alpha=0;name.IDLifeTime=0;
 UI::RankTitles::Badge::RenderVisible();assert(badgeY==58);
 name.IDLifeTime=0;Hero->m_pParts=nullptr;Hero->EtcPart=0;
 UI::RankTitles::Badge::RenderVisible();assert(badgeY==58);
 assert(badgeTexture==9 && badgeWidth==1 && badgeHeight==1);
 auto& animation=Data::RankTitles::title.badge.animation;
 animation=Data::RankTitles::BadgeAnimation{"frames.tga",4,8,24,90};
 ticks=23*90;UI::RankTitles::Badge::RenderVisible();
 assert(badgeTexture==10 && badgeU==.75f && badgeV==.625f && badgeWidth==.25f && badgeHeight==.125f);
 ticks=24*90;UI::RankTitles::Badge::RenderVisible();
 assert(badgeTexture==10 && badgeU==0 && badgeV==0);
 animation->image="missing.tga";UI::RankTitles::Badge::RenderVisible();
 assert(badgeTexture==9 && badgeWidth==1 && badgeHeight==1);
 animation.reset();
 auto& effect=Data::RankTitles::title.badge.effect;
 effect=Data::RankTitles::BadgeEffect{{"frames.tga",4,4,16,65},1.25f,.7f};
 badgeDraws.clear();UI::RankTitles::Badge::RenderVisible();
 assert(badgeDraws.size()==2 && badgeDraws[0].texture==10 && badgeDraws[1].texture==9);
 assert(badgeDraws[0].width==97.5f && badgeDraws[0].height==50 && badgeDraws[0].opacity==.7f);
 effect->opacity=0;badgeDraws.clear();UI::RankTitles::Badge::RenderVisible();
 assert(badgeDraws.size()==1 && badgeDraws[0].texture==9);
 effect->opacity=.7f;effect->animation.image="missing.tga";
 badgeDraws.clear();UI::RankTitles::Badge::RenderVisible();assert(badgeDraws.size()==1);
 effect.reset();
 auto checkHidden=[](){badges=0;UI::RankTitles::Badge::RenderVisible();assert(badges==0);};
 Hero->Object.Visible=false;checkHidden();Hero->Object.Visible=true;
 Hero->Object.cloaked=true;checkHidden();Hero->Object.cloaked=false;
 Hero->Object.hidden=true;checkHidden();Hero->Object.hidden=false;
 Hero->Object.Alpha=0;checkHidden();Hero->Object.Alpha=1;
 Hero->Object.Live=false;checkHidden();Hero->Object.Live=true;
 assert(Shop::Equip(0));Confirm(0);assert(RankOf(Hero)==0);checkHidden();
 auto& remote=CharactersClient[1];remote.Key=101;std::wcscpy(remote.ID,L"Bob");
 Receive(Packet(7,{{"Ranks",nlohmann::json::array({{{"Id",101},{"Name","Bob"},{"Rank",5}}})}}));
 assert(RankOf(&remote)==5);badges=0;UI::RankTitles::Badge::RenderVisible();assert(badges==1);
 std::wcscpy(remote.ID,L"Other");assert(RankOf(&remote)==0);checkHidden();
 assert(Shop::Load());Confirm(4);assert(RankOf(Hero)==4);
 std::wcscpy(Hero->ID,L"NewCharacter");Shop::Update();assert(Shop::Load());Confirm(0);
 assert(RankOf(Hero)==0 && RankOf(&remote)==0);
}
'''

def badge_data_test():
    source = implementation("Data/GameData/RankTitles/RankTitles.cpp")
    readers = source[source.index("float ReadFloat("):source.index("Color ReadColor(")]
    badge = source[source.index("std::optional<BadgeAnimation> ReadBadgeAnimation("):source.index("std::vector<AuraLayer> ReadAura(")]
    return r'''
#include <cassert>
#include <cmath>
#include "json.hpp"
#include "Data/GameData/RankTitles/RankTitles.h"
using namespace Data::RankTitles;
using Json=nlohmann::json;
constexpr float MaximumSize=1000;
int warnings=0;
void Warn(const std::string&,const char*){++warnings;}
''' + readers + badge + r'''
int main(){
 Json plain={{"badge",{{"image","rank.tga"},{"width",75},{"height",40}}}};
 auto badge=ReadBadge(plain,"rank");
 assert(!badge.effect && !badge.animation && badge.width==75);
 const Json animation={{"image","frames.tga"},{"columns",4},{"rows",4},{"frames",16},{"frameMs",65},
  {"scale",1.4},{"opacity",.5}};
 plain["badge"]["effect"]=animation;
 badge=ReadBadge(plain,"rank");
 assert(badge.effect && badge.effect->scale==1.4f && badge.effect->opacity==.5f);
 assert(badge.effect->animation.frames==16 && badge.effect->animation.frameMs==65);
 for(const char* key:{"columns","rows","frames","frameMs"}){
  auto invalid=plain;invalid["badge"]["effect"][key]=0;
  assert(!ReadBadge(invalid,"rank").effect);
 }
 auto invalid=plain;invalid["badge"]["effect"]["frames"]=17;assert(!ReadBadge(invalid,"rank").effect);
 invalid=plain;invalid["badge"]["effect"]["opacity"]=2;
 assert(ReadBadge(invalid,"rank").effect->opacity==.7f);
 invalid=plain;invalid["badge"]["effect"]["scale"]=1000;
 assert(ReadBadge(invalid,"rank").effect->scale==1.25f);
 invalid=plain;invalid["badge"]["effect"]="fire";assert(!ReadBadge(invalid,"rank").effect);
 invalid=plain;invalid["badge"]["effect"]["opacity"]=0;
 assert(ReadBadge(invalid,"rank").effect->opacity==0);
 assert(warnings>0);
}
'''

def gm_bounds_test():
    source = implementation("Character/CSParts.cpp")
    bounds = source[source.index("void UpdateGameMasterMarkBounds("):source.index("bool ShouldHideCastleMarks(")]
    return r'''
#include <algorithm>
#include <cassert>
#include <limits>
using vec3_t=float[3];
constexpr int MODEL_WEBZEN_MARK=1;
struct Bounds { vec3_t StartPos{},XAxis{},YAxis{},ZAxis{}; };
struct OBJECT { int Type=MODEL_WEBZEN_MARK; Bounds OBB; };
struct Mesh { int NumVertices=3; };
struct BMD {
 int NumMeshs=1,calls=0; Mesh Meshs[1];
 void EnsureCpuVertices(int){++calls;}
} Models[2];
float VertexTransform[1][3][3]={{{-12,8,200},{14,-7,220},{4,3,210}}};
void VectorCopy(const float* from,float* to){std::copy_n(from,3,to);}
void Vector(float x,float y,float z,float* to){to[0]=x;to[1]=y;to[2]=z;}
''' + bounds + r'''
int main(){
 OBJECT mark;UpdateGameMasterMarkBounds(mark);
 assert(Models[1].calls==1);
 assert(mark.OBB.StartPos[0]==-12 && mark.OBB.StartPos[1]==-7 && mark.OBB.StartPos[2]==200);
 assert(mark.OBB.XAxis[0]==26 && mark.OBB.YAxis[1]==15 && mark.OBB.ZAxis[2]==20);
 Models[1].NumMeshs=0;UpdateGameMasterMarkBounds(mark);assert(mark.OBB.ZAxis[2]==20);
 mark.Type=0;UpdateGameMasterMarkBounds(mark);assert(Models[0].calls==0);
}
'''


def main():
    name_window = (SOURCE / "UI/Character/NameWindow.cpp").read_text(encoding="utf-8")
    assert "UI::RankTitles::Badge::RenderVisible();" in name_window
    assert "RankTitles::Badge::Render(" not in (SOURCE / "UI/Chat/Chat.cpp").read_text(encoding="utf-8")
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                  "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="rank-skin-rendering-") as directory:
        work = Path(directory)
        for name, code in (("glow", glow_test()), ("rank", rank_test()),
                           ("badge_data", badge_data_test()), ("gm_bounds", gm_bounds_test())):
            (work / (name + ".cpp")).write_text(code, encoding="utf-8")
            (work / "run.cmd").write_text(f'@echo off\ncall "{vs}/VC/Auxiliary/Build/vcvars64.bat" >nul\n'
                                         f'cl /nologo /std:c++20 /EHsc /I"{SOURCE}" /I"{ROOT}/src/ThirdParty" {name}.cpp /Fe:{name}.exe\n'
                                         f'if errorlevel 1 exit /b 1\n{name}.exe\n')
            subprocess.run(["cmd.exe", "/d", "/c", str(work / "run.cmd")], cwd=work, check=True)
            print("PASS:", name, flush=True)


if __name__ == "__main__":
    main()
