#pragma once

class OBJECT;

namespace GameLogic::Cosmetics::FenrirMounts
{
void SetItem(const OBJECT* owner, int itemType);
int Item(const OBJECT* owner);
void Clear(const OBJECT* owner);
void Reset();
}
