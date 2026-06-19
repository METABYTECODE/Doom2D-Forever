#pragma once

#include <filesystem>
#include <optional>

#include <rivet/game/model/model_types.hpp>
#include <rivet/game/resources/resource_pack.hpp>
#include <rivet/resources/resource_manager.hpp>

namespace rivet::game::model {

[[nodiscard]] std::optional<ModelDef> load_model(
    const std::filesystem::path& json_path,
    rivet::resources::ResourceManager& resources,
    const resources::ResourcePack& pack);

} // namespace rivet::game::model
