#pragma once
class CHARACTER;
namespace GameLogic::Combat
{
// Keep nested class-specific target checks on the explicit ground aim for this cast only.
class ScopedGroundAim
{
public:
    explicit ScopedGroundAim(CHARACTER* character);
    ~ScopedGroundAim();
    ScopedGroundAim(const ScopedGroundAim&) = delete;
    ScopedGroundAim& operator=(const ScopedGroundAim&) = delete;
private:
    CHARACTER* m_Previous;
};
bool HasGroundAim(const CHARACTER* character);
}
