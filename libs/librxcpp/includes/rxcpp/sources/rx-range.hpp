// Copyright (c) Microsoft Open Technologies, Inc. All rights reserved. See License.txt in the project root for license information.

#pragma once

#if !defined(RXCPP_SOURCES_RX_RANGE_HPP)
#define RXCPP_SOURCES_RX_RANGE_HPP

#include "../rx-includes.hpp"

namespace rxcpp {

namespace sources {

namespace detail {

template<class T, class Coordination>
struct range : public source_base<T>
{
    typedef typename std::decay<Coordination>::type coordination_type;
    typedef typename coordination_type::coordinator_type coordinator_type;

    struct range_state_type
    {
        range_state_type(T f, T l, ptrdiff_t s, coordination_type cn)
            : next(f)
            , last(l)
            , step(s)
            , coordination(std::move(cn))
        {
        }
        mutable T next;
        T last;
        ptrdiff_t step;
        coordination_type coordination;
    };
    range_state_type initial;
    range(T f, T l, ptrdiff_t s, coordination_type cn)
        : initial(f, l, s, std::move(cn))
    {
    }
    template<class Subscriber>
    void on_subscribe(Subscriber o) const {
        static_assert(is_subscriber<Subscriber>::value, "subscribe must be passed a subscriber");

        typedef Subscriber output_type;

        // creates a worker whose lifetime is the same as this subscription
        auto coordinator = initial.coordination.create_coordinator(o.get_subscription());

        auto controller = coordinator.get_worker();

        auto state = initial;

        auto producer = [=](const rxsc::schedulable& self){
                auto& dest = o;
                if (!dest.is_subscribed()) {
                    // terminate loop
                    return;
                }

                // send next value
                dest.on_next(state.next);
                if (!dest.is_subscribed()) {
                    // terminate loop
                    return;
                }

                if (std::abs(state.last - state.next) < std::abs(state.step)) {
                    if (state.last != state.next) {
                        dest.on_next(state.last);
                    }
                    dest.on_completed();
                    // o is unsubscribed
                    return;
                }
                state.next = static_cast<T>(state.step + state.next);

                // tail recurse this same action to continue loop
                self();
            };

        auto selectedProducer = on_exception(
            [&](){return coordinator.act(producer);},
            o);
        if (selectedProducer.empty()) {
            return;
        }

        controller.schedule(selectedProducer.get());
    }
};

}

template<class T>
auto range(T first = 0, T last = std::numeric_limits<T>::max(), ptrdiff_t step = 1)
    ->      observable<T,   detail::range<T, identity_one_worker>> {
    return  observable<T,   detail::range<T, identity_one_worker>>(
                            detail::range<T, identity_one_worker>(first, last, step, identity_current_thread()));
}
template<class T, class Coordination>
auto range(T first, T last, ptrdiff_t step, Coordination cn)
    ->      observable<T,   detail::range<T, Coordination>> {
    return  observable<T,   detail::range<T, Coordination>>(
                            detail::range<T, Coordination>(first, last, step, std::move(cn)));
}
template<class T, class Coordination>
auto range(T first, T last, Coordination cn)
    -> typename std::enable_if<is_coordination<Coordination>::value,
            observable<T,   detail::range<T, Coordination>>>::type {
    return  observable<T,   detail::range<T, Coordination>>(
                            detail::range<T, Coordination>(first, last, 1, std::move(cn)));
}
template<class T, class Coordination>
auto range(T first, Coordination cn)
    -> typename std::enable_if<is_coordination<Coordination>::value,
            observable<T,   detail::range<T, Coordination>>>::type {
    return  observable<T,   detail::range<T, Coordination>>(
                            detail::range<T, Coordination>(first, std::numeric_limits<T>::max(), 1, std::move(cn)));
}

}

}

#endif
