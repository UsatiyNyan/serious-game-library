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
#include <sl/exec/coro/await.hpp>
#include <sl/exec/model/concept.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/storage/persistent.hpp>
#include <sl/meta/storage/unique_string_convenience.hpp>
#include <sl/meta/traits/is_specialization.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::ecs {

template <typename T>
struct resource : meta::unique {
    using ptr_type = std::unique_ptr<resource<T>>;
    using storage_type = meta::persistent_storage<meta::unique_string, T>;

public: // creation
    // executor is expected to be manual or serial, guaranteeing safety to access registry
    static ptr_type make(exec::executor& executor, resource* parent = nullptr) {
        return ptr_type{ new resource{
            typename storage_type::init_type{
                .get_parent = [parent]() -> storage_type* { return parent == nullptr ? nullptr : &parent->storage_; },
            },
            executor,
        } };
    }

    resource(typename storage_type::init_type storage_init, exec::executor& executor)
        : storage_{ std::move(storage_init) }, mutex_{ executor } {}

public: // behaviour
    using reference_type = meta::persistent<T>;
    using const_reference_type = meta::const_persistent<T>;
    using promise_type = exec::promise<reference_type, meta::unit>;

    // there can be 2 different contexts:
    //  - loader - unique, executes descriptor::load, and fulfills promises
    //  - awaiter - can be many, awaits for loader to
    //
    // override_after_load - will override value by given id in storage, even if it has somehow appeared in the storage
    // during load
    exec::async<meta::maybe<reference_type>>
        require(meta::unique_string id, auto loader, bool override_after_load = true) & {
        using exec::operator co_await;

        {
            exec::mutex_lock lock = (co_await mutex_.lock()).value();

            if (auto maybe_value = storage_.lookup(id)) {
                co_await std::move(lock).unlock();
                co_return maybe_value.value();
            }

            const auto [promises_it, promises_is_emplaced] = promises_by_id_.try_emplace(id);
            if (!promises_is_emplaced) {
                // awaiter context
                auto [future, promise] = exec::make_contract<reference_type, meta::unit>();
                promises_it.value().push_back(std::move(promise));
                co_await std::move(lock).unlock();

                auto result = co_await std::move(future);
                if (result.has_value()) {
                    co_return std::move(result).value();
                }
                co_return meta::null;
            }

            co_await std::move(lock).unlock();
        }

        // loader context
        meta::maybe<T> maybe_loaded_value = co_await std::move(loader);

        exec::mutex_lock lock = (co_await mutex_.lock()).value();
        meta::maybe<reference_type> maybe_reference =
            std::move(maybe_loaded_value).map([this, id, override_after_load](T value) -> reference_type {
                if (override_after_load) {
                    auto [reference, _] = storage_.insert(id, std::move(value));
                    return reference;
                } else {
                    auto [reference, _] = storage_.emplace(id, std::move(value));
                    return reference;
                }
            });

        const auto promises_it = promises_by_id_.find(id);
        ASSERT(promises_it != promises_by_id_.end());

        std::vector<promise_type> promises = std::move(promises_it.value());
        promises_by_id_.erase_fast(promises_it);

        if (maybe_reference.has_value()) {
            for (promise_type& promise : promises) {
                promise.set_value(reference_type{ maybe_reference.value() });
            }
        } else {
            for (promise_type& promise : promises) {
                promise.set_error(meta::unit{});
            }
        }

        co_await std::move(lock).unlock();
        co_return maybe_reference;
    }

    // There can be only awaiter context.
    // If storage does not contain id and there is not a loader present - meta::null is returned.
    //
    exec::async<meta::maybe<reference_type>> request(meta::unique_string id) & {
        using exec::operator co_await;

        exec::mutex_lock lock = (co_await mutex_.lock()).value();
        if (auto maybe_value = storage_.lookup(id)) {
            co_return maybe_value.value();
        }

        const auto promises_it = promises_by_id_.find(id);
        if (promises_it == promises_by_id_.end()) {
            co_return meta::null;
        }

        auto [future, promise] = exec::make_contract<reference_type, meta::unit>();
        promises_it.value().push_back(std::move(promise));
        co_await std::move(lock).unlock();

        auto result = co_await std::move(future);
        if (result.has_value()) {
            co_return std::move(result).value();
        }
        co_return meta::null;
    }

    [[nodiscard]] meta::maybe<reference_type> lookup_unsafe(meta::unique_string id) & { return storage_.lookup(id); }
    [[nodiscard]] meta::maybe<const_reference_type> lookup_unsafe(meta::unique_string id) const& {
        return storage_.lookup(id);
    }

private:
    storage_type storage_;
    tsl::robin_map<meta::unique_string, std::vector<promise_type>> promises_by_id_{};
    exec::mutex<> mutex_;
};

} // namespace sl::ecs
