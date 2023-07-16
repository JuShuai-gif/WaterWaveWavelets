#pragma once

#include <cmath>
#include <type_traits>
#include <tuple>
#include <utility>
#include <eigen/Eigen/Dense>

    template <class T> struct ValueTraits {

        static constexpr T zero() {
            if constexpr (std::is_base_of_v<Eigen::DenseBase<T>, T>) {
                return T::Zero();
            }
            else {
                return 0;
            }
        }
    };

    template <class U, class... T>
    static constexpr bool is_all_same() {
        return (std::is_same_v<U, T> && ... && true);
    }

    template <int I, typename... Ts>
    using getType = typename std::tuple_element<I, std::tuple<Ts...>>::type;

    template <int I, class... Ts>
    static constexpr decltype(auto) get(Ts &&... ts) {
        return std::get<I>(std::forward_as_tuple(ts...));
    }

    template <int First, int Last, class Lambda>
    static void static_for(Lambda const& f) {

        if constexpr (First < Last) {
            f(std::integral_constant<int, First>{});
            static_for<First + 1, Last>(f);
        }
    }

    template <typename> struct ApplyNTimes_impl;

    template <std::size_t... I>
    struct ApplyNTimes_impl<std::index_sequence<I...>> {
        template <typename V, int N> using type = V;

        template <template <typename...> typename T, typename V>
        using apply = T<type<V, I>...>;
    };

    template <int N>
    using ApplyNTimes = ApplyNTimes_impl<std::make_index_sequence<N>>;



    template <int I>
    static auto InterpolateNthArgument = [](auto fun, auto interpolation) {
        return [=](auto... x) mutable {
            // static_assert(std::is_invocable<decltype(fun), decltype(x)...>::value,
                //        "Invalid aguments");

            auto foo = [fun, x...](int i) mutable {
                get<I>(x...) = i;
                return fun(x...);
            };

            auto xI = get<I>(x...);
            return interpolation(foo)(xI);
        };
    };

    template <int I, class Fun, class... Interpolation>
    static auto InterpolationDimWise_impl(Fun fun, Interpolation... interpolation) {

        if constexpr (I == sizeof...(interpolation)) {
            return fun;
        }
        else {
            auto interpol = get<I>(interpolation...);
            return InterpolationDimWise_impl<I + 1>(
                InterpolateNthArgument<I>(fun, interpol), interpolation...);
        }
    }

    template <int I, int... J, class Fun, class... Interpolation>
    static auto InterpolationDimWise_impl2(Fun fun, Interpolation... interpolation) {

        if constexpr (I == sizeof...(J)) {
            return fun;
        }
        else {
            constexpr int K = get<I>(J...);
            auto interpol = get<K>(interpolation...);
            return InterpolationDimWise_impl2<I + 1, J...>(
                InterpolateNthArgument<K>(fun, interpol), interpolation...);
        }
    }

    template <class... Interpolation>
    static auto InterpolationDimWise(Interpolation... interpolation) {
        return [=](auto fun) {
            return InterpolationDimWise_impl<0>(fun, interpolation...);
        };
    }

    template <int... J, class... Interpolation>
    static auto InterpolationDimWise2(Interpolation... interpolation) {
        static_assert(
            sizeof...(J) == sizeof...(Interpolation),
            "The number of interpolations must match the number order indices");

        return [=](auto fun) {
            return InterpolationDimWise_impl2<0, J...>(fun, interpolation...);
        };
    }

    static auto ConstantInterpolation = [](auto fun) {
        using ValueType = std::remove_reference_t<decltype(fun(0))>;
        return [=](auto x) mutable -> ValueType {
            int ix = (int)round(x);
            return fun(ix);
        };
    };

    static auto LinearInterpolation = [](auto fun) {
        // The first two lines are here because Eigen types do
        using ValueType = std::remove_reference_t<decltype(fun(0))>;
        ValueType zero = ValueTraits<ValueType>::zero();
        return [=](auto x) mutable -> ValueType {
            int ix = (int)floor(x);
            auto wx = x - ix;

            return (wx != 0 ? wx * fun(ix + 1) : zero) +
                (wx != 1 ? (1 - wx) * fun(ix) : zero);

        };
    };

    static auto CubicInterpolation = [](auto fun) {
        using ReturnType = std::remove_reference_t<decltype(fun(0))>;
        return [=](auto x) mutable -> ReturnType {
            // for explanation see https://en.wikipedia.org/wiki/Cubic_Hermite_spline
            int ix = (int)floor(x);
            auto wx = x - ix;

            if (wx == 0)
                return 1.0 * fun(ix);

            auto pm1 = fun(ix - 1);
            auto p0 = fun(ix);
            auto p1 = fun(ix + 1);
            auto p2 = fun(ix + 2);

            auto m0 = 0.5 * (p1 - pm1);
            auto m1 = 0.5 * (p2 - p0);

            auto t1 = wx;
            auto t2 = t1 * wx;
            auto t3 = t2 * wx;

            return (2.0 * t3 - 3.0 * t2 + 1.0) * p0 + (t3 - 2.0 * t2 + t1) * m0 +
                (-2.0 * t3 + 3.0 * t2) * p1 + (t3 - t2) * m1;
        };
    };

    template <class Interpolation, class Domain>
    auto DomainInterpolation(Interpolation interpolation, Domain domain) {
        return [=](auto fun) mutable {

            // The function `dom_fun` collects function values and weights inside of
            // the domain
            auto dom_fun = [=](auto... x) mutable -> Eigen::Vector2d {
                Eigen::Vector2d val;
                if (domain(x...) == true) {
                    val << fun(x...), 1.0;
                }
                else {
                    val << 0.0, 0.0;
                }
                return val;
            };

            // Interpolates `dom_fun`
            auto int_fun = interpolation(dom_fun);

            return [=](auto... x) mutable {
                auto val = int_fun(x...);
                double f = val[0];
                double w = val[1];
                return w != 0.0 ? f / w : 0.0;
            };
        };
    }

    template <typename Fun>
    auto integrate(int integration_nodes, double x_min, double x_max, Fun const& fun) {

        double dx = (x_max - x_min) / integration_nodes;
        double x = x_min + 0.5 * dx;

        auto result = dx * fun(x); // the first integration node
        for (int i = 1; i < integration_nodes; i++) { // proceed with other nodes, notice `int i= 1`
            x += dx;
            result += dx * fun(x);
        }

        return result;
    }

    constexpr int pos_modulo(int n, int d) { return (n % d + d) % d; }


