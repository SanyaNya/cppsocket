// slightly modified https://github.com/gdelugre/literal_ipaddr

#pragma once

#include "platform_headers.hpp"

#include <array>
#include <cstdint>
#include <bit>
#include <concepts>

namespace cpps::details
{

using in_addr_t_type = decltype(std::declval<in_addr>().s_addr);

template <std::integral T>
consteval T ct_hton(T hostval)
{
    if constexpr (std::endian::native == std::endian::big)
        return hostval;

    return std::byteswap(hostval);
}

template <typename T>
consteval T ct_ntoh(T netval)
{
    return ct_hton(netval);
}

namespace details_ct_inet_pton
{

consteval bool isdigit(char c)
{
    return c >= '0' && c <= '9';
}

consteval bool isxdigit(char c)
{
    return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

consteval char islower(char c)
{
    return (c >= 'a' && c <= 'z');
}

consteval char toupper(char c)
{
    if ( !islower(c) )
        return c;

    return c ^ 0x20;
}

template<std::size_t N>
consteval long long rfind_chr(const char (&str)[N], size_t from, char c)
{
    for ( long long i = from; i >= 0; i-- )
        if ( str[i] == c )
            return i;

    return -1;
}

template<std::size_t N>
consteval long long find_chr(const char (&str)[N], size_t from, char c)
{
    for ( size_t i = from; str[i] != '\0'; i++ )
        if ( str[i] == c )
            return i;

    return -1;
}

template <int base>
consteval bool is_valid_digit(char c)
{
    static_assert(base == 8 || base == 10 || base == 16, "Invalid base parameter");

    if constexpr ( base == 8 )
        return (c >= '0' && c <= '7');
    else if constexpr ( base == 10 )
        return isdigit(c);
    else if constexpr ( base == 16 )
        return isxdigit(c);
}

template <int base>
consteval int convert_digit(char c)
{
    static_assert(base == 8 || base == 10 || base == 16, "Invalid base parameter");

    if ( !is_valid_digit<base>(c) )
        return -1;

    if constexpr ( base == 8 || base == 10 ) {
        return c - '0';
    }
    else if constexpr ( base == 16 )
    {
        if ( isdigit(c) )
            return convert_digit<10>(c);
        else if ( c >= 'A' && c <= 'F' )
            return c - 'A' + 10;
        else
            return c - 'a' + 10;
    }
}

template <int base, char sep, unsigned max_value, size_t max_length = 0, std::size_t N = 0>
consteval long long parse_address_component(const char (&str)[N], size_t idx)
{
    long long res = 0;

    if ( N - 1 - idx <= 0 || str[idx] == sep )
        return -1;

    for ( size_t i = idx; i < N-1 && str[i] != sep; i++ )
    {
        if ( max_length > 0 && (i - idx + 1) > max_length )
           return -1;

        if ( !is_valid_digit<base>(str[i]) )
            return -1;

        res *= base;
        res += convert_digit<base>(str[i]);

        if ( res > max_value )
            return -1;
    }

    return res;
}

template <int base, unsigned max_value, std::size_t N>
consteval int parse_inet_component_base(const char (&str)[N], size_t idx)
{
    return parse_address_component<base, '.', max_value>(str, idx);
}

template <unsigned max_value, std::size_t N>
consteval int parse_inet_component_oct(const char (&str)[N], size_t idx)
{
    return parse_inet_component_base<8, max_value>(str, idx);
}

template <unsigned max_value, std::size_t N>
consteval int parse_inet_component_dec(const char (&str)[N], size_t idx)
{
    return parse_inet_component_base<10, max_value>(str, idx);
}

template <unsigned max_value, std::size_t N>
consteval int parse_inet_component_hex(const char (&str)[N], size_t idx)
{
    return parse_inet_component_base<16, max_value>(str, idx);
}

//
// Parse a component of an IPv4 address.
//
template <unsigned max_value = 255, std::size_t N>
consteval int parse_inet_component(const char (&str)[N], size_t idx)
{
    if ( (N - idx) > 2 && str[idx] == '0' && (toupper(str[idx+1]) == 'X') )
        return parse_inet_component_hex<max_value>(str, idx + 2);
    else if ( (N - idx) > 2 && str[idx] == '0' && isdigit(str[idx+1]) && str[idx+1] != '0' )
        return parse_inet_component_oct<max_value>(str, idx + 1);
    else
        return parse_inet_component_dec<max_value>(str, idx);
}

//
// Parse a component of an IPv4 address in its canonical form.
// Leading zeros are not allowed, and component must be expressed in decimal form.
//
template<std::size_t N>
consteval int parse_inet_component_canonical(const char (&str)[N], size_t idx)
{
    if ( (N - idx) > 2 && str[idx] == '0' && isdigit(str[idx + 1]) )
        return -1;

    return parse_address_component<10, '.', 255, 3>(str, idx);
}

//
// Parse a component of an IPv6 address.
//
template<std::size_t N>
consteval int parse_inet6_hexlet(const char (&str)[N], size_t idx)
{
    return parse_address_component<16, ':', 0xFFFF, 4>(str, idx);
}

template<std::size_t N>
consteval int inet_addr_canonical_at(const char (&str)[N], long long idx, in_addr_t_type& addr)
{
    // Split and parse each component according to POSIX rules.
    long long sep3 = rfind_chr(str, N-1, '.'),
            sep2 = rfind_chr(str, sep3-1, '.'),
            sep1 = rfind_chr(str, sep2-1, '.');

    if ( sep3 < idx+1 || sep2 < idx+1 || sep1 < idx+1 || rfind_chr(str, sep1-1, '.') >= idx )
        return -1;

    long long c1 = parse_inet_component_canonical(str, idx),
              c2 = parse_inet_component_canonical(str, sep1 + 1),
              c3 = parse_inet_component_canonical(str, sep2 + 1),
              c4 = parse_inet_component_canonical(str, sep3 + 1);

    if ( c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0 )
        return -1;

    addr = ct_hton(static_cast<std::uint32_t>((c1 << 24) | (c2 << 16) | (c3 << 8) | c4));
    return 0;
}

template<std::size_t N>
consteval int inet_addr_canonical(const char (&str)[N], in_addr_t_type& addr)
{
    return inet_addr_canonical_at(str, 0, addr);
}

template<std::size_t N>
consteval int inet_aton_canonical(const char (&str)[N], struct in_addr& in)
{
    return inet_addr_canonical(str, in.s_addr);
}

consteval void inet6_array_to_saddr(std::array<std::uint16_t, 8> const& ip6_comps, struct in6_addr& in6)
{
    for ( size_t i = 0; i < ip6_comps.size(); i++ ) {
        std::uint16_t hexlet = ip6_comps[i];

        in6.s6_addr[i * 2] = hexlet >> 8;
        in6.s6_addr[i * 2 + 1] = hexlet & 0xff;
    }
}

template <typename T, size_t N>
consteval void rshift_array(std::array<T, N>& a, size_t from, size_t shift)
{
    if ( from > N - 1 )
        return;

    for ( long long pos = N - 1; pos >= static_cast<long long>(from + shift); pos-- ) {
        if ( pos - static_cast<long long>(shift) >= 0 ) {
            a[pos] = a[pos - shift];
            a[pos - shift] = 0;
        }
        else
            a[pos] = 0;
    }
}

//
// Parse an IPv6 address.
// Format can be:
//  1. x:x:x:x:x:x:x:x with each component being a 16-bit hexadecimal number
//  2. Contiguous zero components can be compacted as "::", allowed to appear only once in the address.
//  3. First 96 bits in above representation and last 32 bits represented as an IPv4 address.
//
template<std::size_t N>
consteval int inet6_aton(const char (&str)[N], struct in6_addr& in6)
{
    std::array<std::uint16_t, 8> comps = { 0 };
    int shortener_pos = -1;
    size_t idx = 0;
    in_addr_t_type v4_addr = -1;
    auto remaining_chars = [](size_t pos) constexpr { return N - 1 - pos; };

    // The address must contain at least two chars, cannot start/end with a separator alone.
    if ( N < 3 || (str[0] == ':' && str[1] != ':') || (str[N-1] == ':' && str[N-2] != ':') )
        return -1;

    for ( unsigned i = 0; i < comps.size(); i++ ) {

        // We have reached the end of the string before parsing all the components.
        // That is possible only if we have previously encountered a shortener token.
        if ( idx == N-1 ) {
            if ( shortener_pos == -1 )
                return -1;
            else {
                rshift_array(comps, shortener_pos, comps.size() - i);
                break;
            }
        }

        // Check if we have an embedded IPv4 address.
        if ( (i == 6 || (i < 6 && shortener_pos != -1)) && inet_addr_canonical_at(str, idx, v4_addr) != -1 )
        {
            v4_addr = ct_ntoh(v4_addr);

            comps[i++] = (v4_addr >> 16) & 0xffff;
            comps[i++] = v4_addr & 0xffff;

            if ( shortener_pos != -1 )
                rshift_array(comps, shortener_pos, comps.size() - i);

            idx = N - 1;
            break;
        }

        // A shortener token (::) is encountered.
        if ( remaining_chars(idx) >= 2 && str[idx] == ':' && str[idx+1] == ':' )
        {
            // The address shortener syntax can only appear once.
            if ( shortener_pos != -1 )
                return -1;

            // It cannot be followed by another separator token.
            if ( remaining_chars(idx) >= 3 && str[idx+2] == ':' )
                return -1;

            // Save the component position where the token was found.
            shortener_pos = i;

            idx += 2;
        }
        else
        {
            int hexlet = parse_inet6_hexlet(str, idx);
            if ( hexlet == -1 )
                return -1;

            comps[i] = hexlet;

            long long next_sep = find_chr(str, idx, ':');
            if ( next_sep == -1 ) {
                idx = N-1;
            }
            else if ( remaining_chars(next_sep) >= 2 && str[next_sep+1] == ':' ) {
                idx = next_sep;
            }
            else {
                idx = next_sep + 1;
            }
        }
    }

    // Once all components have been parsed, we must be pointing at the end of the string.
    if ( idx != N-1 )
        return -1;

    inet6_array_to_saddr(comps, in6);
    return 0;
}

} //namespace details_ct_inet_pton

template<std::size_t N>
inline void constexpr_fail(const char (&)[N]) {}

template <int AddressF, std::size_t N>
consteval auto ct_inet_pton(const char (&str)[N])
{
    static_assert(AddressF == AF_INET || AddressF == AF_INET6, "Unsupported address family.");

    if constexpr (AddressF == AF_INET ) {
        struct in_addr in = {};
        int r = details_ct_inet_pton::inet_aton_canonical(str, in);
        if(r < 0) constexpr_fail("Invalid address");

        return in;
    }
    else {
        struct in6_addr in6 = {};
        int r = details_ct_inet_pton::inet6_aton(str, in6);
        if(r < 0) constexpr_fail("Invalid address");

        return in6;
    }
}

} //namespace cpps::details