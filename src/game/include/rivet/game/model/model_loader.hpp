#pragma once

#include <filesystem>
#include <optional>

#include <nlohmann/json.hpp>

#include <rivet/game/model/model_types.hpp>
#include <rivet/game/resources/resource_pack.hpp>
#include <rivet/resources/resource_manager.hpp>

namespace rivet::game::model {

struct ModelBody {
    ModelPivot pivot;
    ModelCollider collider;
};

[[nodiscard]] ModelBody parse_model_body(
    const nlohmann::json& json,
    const std::unordered_map<std::string, ModelAnimation>& animations);

[[nodiscard]] std::optional<ModelBody> load_model_body(const std::filesystem::path& json_path);

[[nodiscard]] std::optional<ModelDef> load_model(
    const std::filesystem::path& json_path,
    rivet::resources::ResourceManager& resources,
    const resources::ResourcePack& pack);

} // namespace rivet::game::model
