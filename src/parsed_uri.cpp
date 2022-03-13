#include "parsed_uri.hpp"

namespace {
constexpr auto end = std::string_view::npos;

auto parse_one_param(std::string_view param_str) -> uri_param {
    auto idx = param_str.find("=");
    if (idx != end)
        return {param_str.substr(0, idx), param_str.substr(idx + 1)};
    else
        return {param_str, {}};
}
} // namespace

auto parse_uri(std::string_view uri) -> parsed_uri {
    // Separate method from params
    auto idx = uri.find("?");
    if (idx == end) {
        return {uri, {}, {}};
    }

    std::string_view path = uri.substr(0, idx);
    std::string_view params_string = uri.substr(idx + 1);
    auto start = idx;
    uri_params params;
    while (start != end) {
        idx = uri.find("&", start + 1);
        auto param_str =
                idx == end ? uri.substr(start + 1) : uri.substr(start + 1, idx - start - 1);
        uri_param param = parse_one_param(param_str);
        if (!param.name_.empty())
            params.push_back(param);
        start = idx;
    }
    return {path, params_string, std::move(params)};
}