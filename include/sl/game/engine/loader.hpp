//
// Created by usatiynyan.
// TODO:
// - current_executor!
// - asynchronous rw_mutex

#pragma once

#include "sl/game/layer.hpp"

#include <sl/exec/algo/make/contract.hpp>
#include <sl/exec/algo/make/schedule.hpp>
#include <sl/exec/algo/sync/mutex.hpp>
#include <sl/exec/algo/sync/serial.hpp>
#include <sl/exec/coro/async.hpp>
#include <sl/exec/coro/await.hpp>

#include <sl/meta/monad/result.hpp>
#include <tsl/robin_map.h>

namespace sl::game::engine {

template <typename T>
struct loader {
    using promise_type = exec::promise<meta::persistent<T>, meta::undefined>;

    loader(exec::executor& executor, game::storage<T>& storage) : mutex_{ executor }, storage_{ storage } {}

    [[nodiscard]] auto emplace(const meta::unique_string& id, exec::async<T> construct) {
        return emplace(id, exec::as_signal(std::move(construct)));
    }

    template <exec::Signal SignalT>
    [[nodiscard]] exec::async<tl::expected<meta::persistent<T>, meta::persistent<T>>>
        emplace(const meta::unique_string& id, SignalT construct) {
        using exec::operator co_await;

        {
            auto lock = *(co_await mutex_.lock());
            tl::optional<meta::persistent<T>> maybe_ref = storage_.lookup(id);
            co_await lock.unlock();

            if (maybe_ref.has_value()) {
                co_return meta::err(std::move(maybe_ref).value());
            }
        }

        auto construct_result = co_await std::move(construct);
        ASSERT(construct_result.has_value());
        auto value = std::move(construct_result).value();

        auto lock = *(co_await mutex_.lock());
        tl::expected<meta::persistent<T>, meta::persistent<T>> result = storage_.insert(id, std::move(value));

        if (auto waiters_it = waiters_.find(id); waiters_it != waiters_.end()) {
            std::vector<promise_type>& waiters = waiters_it.value();

            meta::persistent<T>& reference = ASSUME_VAL(result.has_value()) //
                                                 ? result.value()
                                                 : result.error();

            for (promise_type& waiter : waiters) {
                waiter.set_value(meta::persistent<T>{ reference });
            }

            waiters_.erase_fast(waiters_it);
        }

        co_await lock.unlock();
        co_return result;
    }

    [[nodiscard]] exec::async<meta::persistent<T>> get(const meta::unique_string& id) {
        using exec::operator co_await;

        auto lock = *(co_await mutex_.lock());

        if (tl::optional<meta::persistent<T>> maybe_reference = storage_.lookup(id)) {
            co_await lock.unlock();
            co_return std::move(maybe_reference).value();
        }

        exec::contract<meta::persistent<T>, meta::undefined> contract =
            exec::make_contract<meta::persistent<T>, meta::undefined>();
        std::vector<promise_type>& waiters = waiters_[id];
        waiters.push_back(std::move(contract.promise));

        co_await lock.unlock();
        auto result = co_await std::move(contract.future);
        co_return std::move(result).value();
    }

private:
    tsl::robin_map<meta::unique_string, std::vector<promise_type>> waiters_{};
    exec::mutex mutex_;
    game::storage<T>& storage_;
};

} // namespace sl::game::engine
