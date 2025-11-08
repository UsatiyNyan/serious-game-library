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

template <typename ResourceDescriptorT>
concept ResourceDescriptor =
    requires() {
        typename ResourceDescriptorT::value_type;
        typename ResourceDescriptorT::error_type;
        typename ResourceDescriptorT::args_type;
    } && meta::IsSpecialization<typename ResourceDescriptorT::args_type, std::tuple>
    && requires(ResourceDescriptorT rd, typename ResourceDescriptorT::args_type args) {
           {
               std::apply(rd.load, std::move(args))
           } -> exec::Signal<typename ResourceDescriptorT::value_type, typename ResourceDescriptorT::error_type>;
       };

template <ResourceDescriptor RDT>
struct resource : meta::unique {
    using value_type = typename RDT::value_type;
    using error_type = typename RDT::error_type;
    using args_type = typename RDT::args_type;
    using storage_type = meta::persistent_storage<meta::unique_string, value_type>;

public: // creation
    struct make_result {
        resource& value;
        bool is_emplaced;
    };

    // executor is expected to be manual or serial, guaranteeing safety to access registry
    static make_result make_root(entt::registry& registry, exec::executor& executor, entt::entity on, auto descriptor) {
        return make_impl(registry, executor, on, meta::null, std::move(descriptor));
    }

    make_result make_child(entt::registry& registry, entt::entity on, auto descriptor) & {
        return make_impl(registry, executor_, on, entity_, std::move(descriptor));
    }

private:
    static make_result make_impl(
        entt::registry& registry,
        exec::executor& executor,
        entt::entity entity,
        meta::maybe<entt::entity> parent,
        auto descriptor
    ) {
        if (resource* present = registry.try_get<resource<RDT>>(entity); present != nullptr) {
            return make_result{
                .value = *present,
                .is_emplaced = false,
            };
        }
        typename storage_type::init_type storage_init;
        if (parent.has_value()) {
            storage_init.get_parent = [&registry, p_entity = parent.value()] {
                auto* p_ptr = registry.try_get<resource<RDT>>(p_entity);
                DEBUG_ASSERT(p_ptr, "parent resource not found");
                return p_ptr;
            };
        }
        return make_result{
            .value = registry.emplace<resource<RDT>>(
                entity,
                // args:
                registry,
                executor,
                entity,
                std::move(storage_init),
                descriptor
            ),
            .is_emplaced = true,
        };
    }

    resource(
        entt::registry& registry,
        exec::executor& executor,
        entt::entity entity,
        typename storage_type::init_type storage_init,
        auto descriptor
    )
        : storage_{ std::move(storage_init) }, descriptor_{ std::move(descriptor) }, executor_{ executor },
          registry_{ registry }, entity_{ entity } {}

public: // behaviour
    using reference_type = meta::persistent<value_type>;
    using result_type = meta::result<reference_type, error_type>;
    using promise_type = exec::promise<value_type, error_type>;

    // PRECONDITION: expects to be started on provided executor
    //
    // there can be 2 different contexts:
    //  - loader - unique, executes descriptor::load, and fulfills promises
    //  - awaiter - can be many, awaits for loader to
    //
    // override_after_load - will override value by given id in storage, even if it has somehow appeared in the storage
    // during load
    exec::async<meta::maybe<result_type>>
        require(meta::unique_string id, auto args, bool override_after_load = true) & {
        if (auto maybe_value = storage_.lookup(id)) {
            co_return maybe_value.value();
        }

        {
            const auto [promises_it, promises_is_emplaced] = promises_by_id_.emplace(id);
            if (!promises_is_emplaced) {
                // awaiter context
                auto [future, promise] = exec::make_contract<reference_type, error_type>();
                promises_it.value().push_back(std::move(promise));
                // future continuation is guaranteed to be on the executor_ below, where promises are fulfilled
                co_return co_await std::move(future) /* | exec::continue_on(executor_) */;
            }
        }

        // loader context

        // caching before awaiting loader
        entt::registry& registry = registry_;
        entt::entity entity = entity_;

        meta::result<value_type, error_type> load_result =
            co_await std::apply(descriptor_.load, std::move(args)) | exec::continue_on(executor_);
        ASSERT(registry.try_get<resource<RDT>>(entity) == this, "loader outlived resource");

        auto result = std::move(load_result).map([this, override_after_load](value_type value) -> reference_type {
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
            if (result.has_value()) {
                promise.set_value(result.value());
            } else {
                promise.set_error(result.error());
            }
        }

        co_return std::move(result);
    }

    // PRECONDITION: expects to be started on provided executor
    //
    // There can be only awaiter context.
    // If storage does not contain id and there is not a loader present - meta::null is returned.
    //
    exec::async<meta::maybe<result_type>> request(meta::unique_string id) & {
        if (auto maybe_value = storage_.lookup(id)) {
            co_return maybe_value.value();
        }

        const auto promises_it = promises_by_id_.find(id);
        if (promises_it == promises_by_id_.end()) {
            co_return meta::null;
        }

        auto [future, promise] = exec::make_contract<reference_type, error_type>();
        promises_it.value().push_back(std::move(promise));
        co_return co_await std::move(future);
    }

private:
    storage_type storage_;
    RDT descriptor_;
    tsl::robin_map<meta::unique_string, std::vector<promise_type>> promises_by_id_;
    exec::executor& executor_;
    entt::registry& registry_;
    entt::entity entity_;
};

} // namespace sl::ecs
