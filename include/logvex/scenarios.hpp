#pragma once

#include <string>
#include <vector>

#include "logvex/world.hpp"

namespace lvx {

struct ScenarioInfo {
    std::string name;
    std::string description;
};

const std::vector<ScenarioInfo>& scenarios();

// Builds a fresh world for the named scenario. Returns std::nullopt when the
// scenario is unknown. Geometry is synthetic but dimensionally faithful to CS2
// units (1 unit ~= 1.9 cm, eye height 64 units).
std::optional<World> make_scenario(const std::string& name);

}  // namespace lvx
