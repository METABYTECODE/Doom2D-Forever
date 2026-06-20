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

[[nodiscard]] std::unordered_map<std::string, ModelAnimation> parse_animations(const nlohmann::json& json) {
    std::unordered_map<std::string, ModelAnimation> animations;
    if (!json.contains("animations") || !json.at("animations").is_object()) {
        return animations;
    }

    for (const auto& [key, value] : json.at("animations").items()) {
        if (!value.is_object()) {
            continue;
        }
        animations.emplace(key, parse_animation(value));
    }
    return animations;
}

[[nodiscard]] ModelPivot feet_pivot(const float frame_width, const float frame_height) {
    return {
        .x = frame_width * 0.5f,
        .y = frame_height,
    };
}

[[nodiscard]] ModelCollider humanoid_hull(const float frame_width, const float frame_height) {
    const float width = std::min(34.0f, frame_width);
    const float height = std::min(52.0f, frame_height);
    const auto pivot = feet_pivot(frame_width, frame_height);
    return {
        .x = pivot.x - width * 0.5f,
        .y = pivot.y - height,
        .width = width,
        .height = height,
    };
}

[[nodiscard]] ModelCollider blank_hull() {
    return {
        .x = 6.0f,
        .y = 8.0f,
        .width = 20.0f,
        .height = 24.0f,
    };
}

[[nodiscard]] ModelPivot parse_pivot(
    const nlohmann::json& json,
    const float frame_width,
    const float frame_height) {
    if (json.contains("pivot") && json.at("pivot").is_object()) {
        const auto& pivot_json = json.at("pivot");
        return {
            .x = pivot_json.value("x", frame_width * 0.5f),
            .y = pivot_json.value("y", frame_height),
        };
    }
    return feet_pivot(frame_width, frame_height);
}

[[nodiscard]] ModelCollider parse_collider_rect(
    const nlohmann::json& collider_json,
    const ModelPivot& pivot,
    const float frame_width,
    const float frame_height) {
    if (collider_json.contains("offset_x") || collider_json.contains("offset_y")) {
        const float width = collider_json.value("width", frame_width);
        const float height = collider_json.value("height", frame_height);
        const float offset_x = collider_json.value("offset_x", -width * 0.5f);
        const float offset_y = collider_json.value("offset_y", -height);
        return {
            .x = pivot.x + offset_x,
            .y = pivot.y + offset_y,
            .width = width,
            .height = height,
        };
    }

    return {
        .x = collider_json.value("x", 0.0f),
        .y = collider_json.value("y", 0.0f),
        .width = collider_json.value("width", frame_width),
        .height = collider_json.value("height", frame_height),
    };
}

[[nodiscard]] std::optional<ModelBody> migrate_deprecated_hitbox(
    const nlohmann::json& json,
    const std::unordered_map<std::string, ModelAnimation>& animations) {
    if (!json.contains("hitbox") || !json.at("hitbox").is_object()) {
        return std::nullopt;
    }

    const auto& hitbox_json = json.at("hitbox");
    const float width = hitbox_json.value("width", 34.0f);
    const float height = hitbox_json.value("height", 52.0f);
    const float origin_x = hitbox_json.value("origin_x", 0.0f);
    const float origin_y = hitbox_json.value("origin_y", 0.0f);

    const ModelAnimation* idle = nullptr;
    if (const auto it = animations.find("IDLE"); it != animations.end()) {
        idle = &it->second;
    } else if (!animations.empty()) {
        idle = &animations.begin()->second;
    }

    const float frame_w = idle != nullptr ? static_cast<float>(idle->frame_width) : 64.0f;
    const float frame_h = idle != nullptr ? static_cast<float>(idle->frame_height) : 64.0f;
    const auto pivot = feet_pivot(frame_w, frame_h);

    return ModelBody{
        .pivot = pivot,
        .collider = {
            .x = origin_x,
            .y = origin_y,
            .width = width,
            .height = height,
        },
    };
}

} // namespace

ModelBody parse_model_body(
    const nlohmann::json& json,
    const std::unordered_map<std::string, ModelAnimation>& animations) {
    const auto migrated = migrate_deprecated_hitbox(json, animations);
    if (migrated.has_value()) {
        return *migrated;
    }

    const ModelAnimation* idle = nullptr;
    if (const auto it = animations.find("IDLE"); it != animations.end()) {
        idle = &it->second;
    } else if (!animations.empty()) {
        idle = &animations.begin()->second;
    }

    const float frame_w = idle != nullptr ? static_cast<float>(idle->frame_width) : 32.0f;
    const float frame_h = idle != nullptr ? static_cast<float>(idle->frame_height) : 32.0f;
    const auto pivot = parse_pivot(json, frame_w, frame_h);

    if (json.contains("collider") && json.at("collider").is_object()) {
        return {
            .pivot = pivot,
            .collider = parse_collider_rect(json.at("collider"), pivot, frame_w, frame_h),
        };
    }

    if (idle != nullptr) {
        return {
            .pivot = feet_pivot(frame_w, frame_h),
            .collider = humanoid_hull(frame_w, frame_h),
        };
    }

    return {
        .pivot = feet_pivot(32.0f, 32.0f),
        .collider = blank_hull(),
    };
}

std::optional<ModelBody> load_model_body(const std::filesystem::path& json_path) {
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
    if (version != 1 && version != 2 && version != 3) {
        spdlog::warn("Unsupported model version in {}: {}", json_path.string(), version);
        return std::nullopt;
    }

    const auto animations = parse_animations(json);
    return parse_model_body(json, animations);
}

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
    if (version != 1 && version != 2 && version != 3) {
        spdlog::warn("Unsupported model version in {}: {}", json_path.string(), version);
        return std::nullopt;
    }

    ModelDef model;
    model.id = json.value("id", json_path.stem().string());
    model.name = json.value("name", model.id);
    model.resource_pack = json.value("resource_pack", pack.manifest().id);
    model.animations = parse_animations(json);

    const auto body = parse_model_body(json, model.animations);
    model.pivot = body.pivot;
    model.collider = body.collider;

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
