#include "UI/RmlBridge/RmlElementStack.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Property.h>
#include <cmath>
#include <string>

bool UI::RmlBridge::StackAbove(Rml::Element& upper, Rml::Element& lower)
{
    constexpr float PixelTolerance = 0.1f;
    constexpr float ScaleTolerance = 0.001f;
    Rml::Vector2f origin, size, currentOrigin, currentSize;
    const float height = upper.GetBox().GetSize(Rml::BoxArea::Border).y;
    if (height <= 0.f || !DrawnBox(lower, Rml::BoxArea::Border, origin, size))
        return false;
    const float scale = DrawnScale(lower);
    const float top = origin.y - height * scale;
    if (DrawnBox(upper, Rml::BoxArea::Border, currentOrigin, currentSize) &&
        std::abs(currentOrigin.x - origin.x) < PixelTolerance && std::abs(currentOrigin.y - top) < PixelTolerance &&
        std::abs(DrawnScale(upper) - scale) < ScaleTolerance)
        return true;
    upper.SetProperty("bottom", "auto");
    upper.SetProperty("right", "auto");
    upper.SetProperty("transform-origin", "left top");
    upper.SetProperty("transform", "scale(" + std::to_string(scale) + ")");
    upper.SetProperty(Rml::PropertyId::Left, Rml::Property(origin.x, Rml::Unit::PX));
    upper.SetProperty(Rml::PropertyId::Top, Rml::Property(top, Rml::Unit::PX));
    upper.GetOwnerDocument()->UpdateDocument();
    return true;
}
