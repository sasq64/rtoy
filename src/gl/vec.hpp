#pragma once
#include <array>
#include <cstddef>

template <typename F, size_t N>
struct vecbase
{
    std::array<F, N> data;

    F x() const { return data[0]; }
    F y() const { return data[1]; }
    F z() const { return data[2]; }

    F& x() { return data[0]; }
    F& y() { return data[1]; }
    F& z() { return data[2]; }

    F& operator[](size_t i) { return data[i]; }
    F const& operator[](size_t i) const { return data[i]; }

    template <typename Vec>
    bool operator==(const Vec& v) const
    {
        for (size_t i = 0; i < N; i++) {
            if (v[i] != data[i]) return false;
        }
        return true;
    }

    template <typename Vec>
    vecbase operator+(const Vec& v) const
    {
        vecbase r;
        for (int i = 0; i < N; i++)
            r.data[i] = data[i] + v[i];
        return r;
    }

    vecbase operator+(const vecbase& v) const
    {
        vecbase r; // NOLINT
        for (int i = 0; i < N; i++)
            r.data[i] = data[i] + v.data[i];
        return r;
    }

    vecbase operator*(float f) const
    {
        vecbase r; // NOLINT
        for (int i = 0; i < N; i++)
            r.data[i] = data[i] * f;
        return r;
    }

    template <typename Vec>
    vecbase operator+=(const Vec& v)
    {
        for (int i = 0; i < N; i++)
            data[i] += v[i];
        return *this;
    }

    vecbase operator+=(const vecbase& v)
    {
        for (int i = 0; i < N; i++)
            data[i] += v.data[i];
        return *this;
    }
/*
    vecbase operator+=(const std::initializer_list<float>& v) const
    {
        int i = 0;
        for (auto f : v) {
            data[i] = data[i] + f;
            i++;
        }
        return *this;
    }
    */
};

using vec2 = vecbase<float, 2>;

template <size_t I, typename T, size_t N>
constexpr T& get(vecbase<T, N>&& vec)
{
    return vec.data[I];
}

template <size_t I, typename T, size_t N>
constexpr T const& get(vecbase<T, N> const& vec)
{
    return vec.data[I];
}

namespace std {

template <typename F, size_t N>
class tuple_size<vecbase<F, N>> : public std::integral_constant<size_t, N>
{};

template <size_t I, typename T, size_t N>
class tuple_element<I, vecbase<T, N>>
{
public:
    using type = T;
};
} // namespace std

