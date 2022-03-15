#pragma once

#include <string_view>
#include <vector>

struct uri_param {
    const std::string_view name_;
    const std::string_view value_;
};

using uri_params = std::vector<uri_param>;

struct parsed_uri {
    const std::string_view path_;
    const std::string_view params_string_;
    const uri_params params_;
};

auto parse_uri(std::string_view uri) -> parsed_uri;
