//
// Created by usatiynyan.
//

#include "06_engine/common.hpp"
#include "06_engine/script/scene.hpp"

#include <imgui.h>
#include <sl/exec.hpp>
#include <sl/exec/coro/async.hpp>
#include <sl/game.hpp>
#include <sl/gfx.hpp>
#include <sl/meta.hpp>
#include <sl/rt.hpp>

#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

int main(int argc, char** argv) {
    game::logger().set_level(spdlog::level::debug);
    game::logger().sinks().push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    engine::context e_ctx =
        game::window_context::initialize("06_engine", { 1280, 720 }, { 0.1f, 0.1f, 0.1f, 0.1f })
            .map([&](auto&& w_ctx) { return engine::context::initialize(std::move(w_ctx), argc, argv); })
            .or_else([] { PANIC("engine context"); })
            .value();

    engine::layer layer{ *e_ctx.script_exec, engine::layer::config{} };
    exec::coro_schedule(*e_ctx.script_exec, script::create_scene(e_ctx, layer));

    while (!e_ctx.w_ctx.current_window.should_close()) {
        // input
        e_ctx.w_ctx.context->poll_events();
        e_ctx.w_ctx.state->frame_buffer_size.release().map( //
            [&cw = e_ctx.w_ctx.current_window](const glm::ivec2& frame_buffer_size) {
                cw.viewport(glm::ivec2{}, frame_buffer_size);
            }
        );
        e_ctx.w_ctx.state->window_content_scale.release().map([](const glm::fvec2& window_content_scale) {
            ImGui::GetStyle().ScaleAllSizes(window_content_scale.x);
        });
        e_ctx.in_sys->process(layer);

        const rt::time_point time_point = e_ctx.time.calculate();

        // update and execute scripts
        std::ignore = e_ctx.script_exec->execute_batch();
        game::update_system(layer, time_point);

        // transform update
        game::local_transform_system(layer, time_point);

        // render
        const auto window_frame = e_ctx.w_ctx.new_frame();
        game::graphics_system(layer, window_frame);

        // overlay
        auto imgui_frame = e_ctx.w_ctx.imgui.new_frame(); // TODO: require gfx_frame here too
        if (const auto imgui_window = imgui_frame.begin("debug")) {
            // TODO: maybe use actual angles against the world basis
            // auto& directional_light_rot = layer.registry.get<game::transform>(global_entity).rot;
            // ImGui::SliderFloat4("directional_light rot", glm::value_ptr(directional_light_rot), -1.0f, 1.0f);
            // directional_light_rot = glm::normalize(directional_light_rot);
            //
            // auto& directional_light = layer.registry.get<render::directional_light>(global_entity);
            // ImGui::ColorEdit3("directional_light ambient", glm::value_ptr(directional_light.ambient));
            // ImGui::ColorEdit3("directional_light diffuse", glm::value_ptr(directional_light.diffuse));
            // ImGui::ColorEdit3("directional_light specular", glm::value_ptr(directional_light.specular));
            //
            // ImGui::Spacing();
            // layer.storage.material.lookup(cube_material_id).map([](sl::meta::persistent<game::material> material) {
            //     ImGui::SliderFloat("shininess", &material->shininess, 2.0f, 256, "%.0f",
            //     ImGuiSliderFlags_Logarithmic);
            // });
        }
    }

    return 0;
}
