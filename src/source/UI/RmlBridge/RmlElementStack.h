#pragma once

namespace Rml { class Element; }

namespace UI::RmlBridge
{
// Place an absolute root directly above another document's root, matching its drawn left edge and scale.
bool StackAbove(Rml::Element& upper, Rml::Element& lower);
}
