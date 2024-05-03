//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/lifetime/defer.hpp>

#include <boost/container/static_vector.hpp>
#include <function2/function2.hpp>
#include <libassert/assert.hpp>
#include <tl/optional.hpp>

#include <stack>

namespace sl::game {

template <typename Signature, std::size_t max_callbacks = 16>
class generic_control;

template <std::size_t max_callbacks, typename Ret, typename... TArgs>
class generic_control<Ret(TArgs...), max_callbacks> {
public:
    using callback = fu2::function_base<
        /*IsOwning=*/true,
        /*IsCopyable=*/false,
        /*Capacity=*/fu2::capacity_default,
        /*IsThrowing=*/false,
        /*HasStrongExceptGuarantee=*/true,
        void(TArgs...)>;

public:
    template <typename... UArgs>
        requires(!std::is_void_v<Ret>)
    tl::optional<Ret> operator()(UArgs&&... args) {
        if (cbs_.empty()) {
            return tl::nullopt;
        }
        return cbs_.top()(std::forward<UArgs>(args)...);
    }

    template <typename... UArgs>
        requires std::is_void_v<Ret>
    void operator()(UArgs&&... args) {
        if (!cbs_.empty()) {
            cbs_.top()(std::forward<UArgs>(args)...);
        }
    }

    auto attach(callback cb) {
        ASSERT(cbs_.size() < max_callbacks);
        cbs_.push(std::move(cb));
        return meta::defer{ [this] {
            ASSERT_VAL(!cbs_.empty());
            cbs_.pop();
        } };
    }

private:
    using callbacks_storage = boost::container::static_vector<callback, max_callbacks>;
    std::stack<callback, callbacks_storage> cbs_;
};

} // namespace sl::game
