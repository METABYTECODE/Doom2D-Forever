#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <rivet/game/model/model_loader.hpp>

namespace rivet::game::model {

namespace {

[[nodiscard]] ModelAnimation parse_animation(const nlohmann::json& json) {
    ModelAnimation anim;
    anim.atlas = json.value("atlas", json.value("sprite", ""));
    anim.frame_width = json.value("frame_width", 64);
    anim.frame_height = json.value("frame_height", 64);
    anim.columns = std::max(1, json.value("columns", 1));
    anim.frame_ms = std::max(1, json.value("frame_ms", 120));
    anim.loop = json.value("loop", true);

    if (json.contains("frames") && json.at("frames").is_array()) {
        for (const auto& frame_json : json.at("frames")) {
            anim.frame_ids.push_back(frame_json.value("id", 0));
        }
    }
    if (anim.frame_ids.empty()) {
        anim.frame_ids.push_back(0);
    }

    return anim;
}

} // namespace

std::optional<ModelDef> load_model(
    const std::filesystem::path& json_path,
    rivet::resources::ResourceManager& resources,
    const resources::ResourcePack& pack) {
    std::ifstream input(json_path);
    if (!input) {
        spdlog::warn("Failed to open model: {}", json_path.string());
        return std::nullopt;
    }

    const auto json = nlohmann::json::parse(input);
    const std::string format = json.value("format", "");
    if (format != "rivet-model") {
        spdlog::warn("Unsupported model format in {}: {}", json_path.string(), format);
        return std::nullopt;
    }

    const int version = json.value("version", 0);
    if (version != 1 && version != 2) {
        spdlog::warn("Unsupported model version in {}: {}", json_path.string(), version);
        return std::nullopt;
    }

    ModelDef model;
    model.id = json.value("id", json_path.stem().string());
    model.name = json.value("name", model.id);
    model.resource_pack = json.value("resource_pack", pack.manifest().id);

    if (json.contains("animations") && json.at("animations").is_object()) {
        for (const auto& [key, value] : json.at("animations").items()) {
            if (!value.is_object()) {
                continue;
            }
            model.animations.emplace(key, parse_animation(value));
        }
    }

    for (auto& [name, anim] : model.animations) {
        (void)name;
        if (anim.atlas.empty()) {
            spdlog::warn("Model {} animation missing atlas", model.id);
            continue;
        }

        const auto atlas_path = pack.resolve_sprite_atlas(anim.atlas);
        if (!atlas_path) {
            spdlog::warn("Model {} atlas not found: {}", model.id, anim.atlas);
            continue;
        }

        anim.texture = resources.load_texture(*atlas_path);
        if (anim.texture == rivet::resources::kInvalidTexture) {
            spdlog::warn("Model {} failed to load atlas texture: {}", model.id, atlas_path->string());
        }
    }

  return model;
}

} // namespace rivet::game::model
