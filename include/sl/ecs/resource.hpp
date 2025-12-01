//
// Created by usatiynyan.
//

#pragma once

#include "sl/ecs/vendor.hpp"

#include <sl/exec/algo/emit/force.hpp>
#include <sl/exec/algo/make/contract.hpp>
#include <sl/exec/algo/sched/continue_on.hpp>
#include <sl/exec/algo/sched/start_on.hpp>
#include <sl/exec/algo/sync/mutex.hpp>
#include <sl/exec/coro/async.hpp>
#include <sl/exec/model/concept.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/unique_string_convenience.hpp>
#include <sl/meta/traits/is_specialization.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::ecs {

template <typename T>
struct resource : meta::unique {
    using storage_type = meta::persistent_storage<meta::unique_string, T>;

public: // creation
    struct make_result {
        resource& value;
        bool is_emplaced;
    };

    // executor is expected to be manual or serial, guaranteeing safety to access registry
    static make_result make_root(entt::registry& registry, exec::executor& executor, entt::entity on) {
        return make_impl(registry, executor, on, meta::null);
    }

    make_result make_child(entt::registry& registry, entt::entity on) & {
        return make_impl(registry, executor_, on, entity_);
    }

private:
    static make_result make_impl(
        entt::registry& registry,
        exec::executor& executor,
        entt::entity entity,
        meta::maybe<entt::entity> parent
    ) {
        if (resource* present = registry.try_get<resource<T>>(entity); present != nullptr) {
            return make_result{
                .value = *present,
                .is_emplaced = false,
            };
        }
        typename storage_type::init_type storage_init;
        if (parent.has_value()) {
            storage_init.get_parent = [&registry, p_entity = parent.value()] {
                auto* p_ptr = registry.try_get<resource<T>>(p_entity);
                DEBUG_ASSERT(p_ptr, "parent resource not found");
                return p_ptr;
            };
        }
        return make_result{
            .value = registry.emplace<resource<T>>(
                entity,
                // args:
                registry,
                executor,
                entity,
                std::move(storage_init)
            ),
            .is_emplaced = true,
        };
    }

    resource(
        entt::registry& registry,
        exec::executor& executor,
        entt::entity entity,
        typename storage_type::init_type storage_init
    )
        : storage_{ std::move(storage_init) }, executor_{ executor }, registry_{ registry }, entity_{ entity } {}

public: // behaviour
    using reference_type = meta::persistent<T>;
    using const_reference_type = meta::const_persistent<T>;
    using promise_type = exec::promise<reference_type, meta::unit>;

    // PRECONDITION: expects to be started on provided executor
    //
    // there can be 2 different contexts:
    //  - loader - unique, executes descriptor::load, and fulfills promises
    //  - awaiter - can be many, awaits for loader to
    //
    // override_after_load - will override value by given id in storage, even if it has somehow appeared in the storage
    // during load
    exec::async<meta::maybe<reference_type>>
        require(meta::unique_string id, auto loader, bool override_after_load = true) & {
        if (auto maybe_value = storage_.lookup(id)) {
            co_return maybe_value.value();
        }

        {
            const auto [promises_it, promises_is_emplaced] = promises_by_id_.emplace(id);
            if (!promises_is_emplaced) {
                // awaiter context
                auto [future, promise] = exec::make_contract<reference_type, meta::unit>();
                promises_it.value().push_back(std::move(promise));
                // future continuation is guaranteed to be on the executor_ below, where promises are fulfilled
                auto result = co_await std::move(future) /* | exec::continue_on(executor_) */;
                if (result.has_value()) {
                    co_return std::move(result).value();
                }
                co_return meta::null;
            }
        }

        // loader context

        // caching before awaiting loader
        entt::registry& registry = registry_;
        entt::entity entity = entity_;

        meta::maybe<T> maybe_loaded_value = co_await std::move(loader) | exec::continue_on(executor_);
        DEBUG_ASSERT(registry.try_get<resource<T>>(entity) == this, "loader outlived resource");

        auto maybe_reference =
            std::move(maybe_loaded_value).map([this, override_after_load](T value) -> reference_type {
                if (override_after_load) {
                    auto [reference, _] = storage_.insert(std::move(value));
                    return reference;
                } else {
                    auto [reference, _] = storage_.emplace(std::move(value));
                    return reference;
                }
            });

        const auto promises_it = promises_by_id_.find(id);
        ASSERT(promises_it != promises_by_id_.end());

        std::vector<promise_type> promises = std::move(promises_it.value());
        promises_by_id_.erase_fast(promises_it);

        for (promise_type& promise : promises) {
            if (maybe_reference.has_value()) {
                promise.set_value(maybe_reference.value());
            } else {
                promise.set_error(meta::null);
            }
        }

        co_return maybe_reference;
    }

    // PRECONDITION: expects to be started on provided executor
    //
    // There can be only awaiter context.
    // If storage does not contain id and there is not a loader present - meta::null is returned.
    //
    exec::async<meta::maybe<T>> request(meta::unique_string id) & {
        if (auto maybe_value = storage_.lookup(id)) {
            co_return maybe_value.value();
        }

        const auto promises_it = promises_by_id_.find(id);
        if (promises_it == promises_by_id_.end()) {
            co_return meta::null;
        }

        auto [future, promise] = exec::make_contract<reference_type, meta::unit>();
        promises_it.value().push_back(std::move(promise));
        co_return co_await std::move(future);
    }

    [[nodiscard]] meta::maybe<reference_type> lookup(meta::unique_string id) & { return storage_.lookup(id); }
    [[nodiscard]] meta::maybe<const_reference_type> lookup(meta::unique_string id) const& {
        return storage_.lookup(id);
    }

private:
    storage_type storage_;
    tsl::robin_map<meta::unique_string, std::vector<promise_type>> promises_by_id_;
    exec::executor& executor_;
    entt::registry& registry_;
    entt::entity entity_;
};

} // namespace sl::ecs
