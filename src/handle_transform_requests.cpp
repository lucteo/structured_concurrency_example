#include "handle_transform_requests.hpp"

#include "http_server/create_response.hpp"

#if HAS_OPENCV

#include "http_server/http_request.hpp"
#include "profiling.hpp"

#include <execution.hpp>

#include <opencv2/imgcodecs.hpp>

namespace ex = std::execution;

namespace {

auto sv_to_int(std::string_view value) -> int {
    int res = 0;
    for (char c : value)
        res = res * 10 + (c - '0');
    return res;
}

auto get_param_int(const parsed_uri& uri, std::string_view name, int default_val) -> int {
    for (auto p : uri.params_)
        if (p.name_ == name)
            return sv_to_int(p.value_);
    return default_val;
}

auto to_cv(const std::string& bytes) -> cv::Mat {
    PROFILING_SCOPE();
    cv::Mat raw_data(1, bytes.size(), CV_8UC1, (void*)bytes.data());
    return cv::imdecode(raw_data, cv::IMREAD_COLOR);
}

auto img_to_response(const cv::Mat& img) -> http_server::http_response {
    PROFILING_SCOPE();
    std::vector<uchar> buf;
    if (!cv::imencode(".jpeg", img, buf))
        throw std::logic_error("Cannot encode OpenCV image");
    std::string body;
    body.resize(buf.size());
    std::memcpy(body.data(), buf.data(), buf.size());
    PROFILING_SET_TEXT_FMT(32, "body_size=%d", int(buf.size()));
    return http_server::create_response(
            http_server::status_code::s_200_ok, "application/jpeg", std::move(body));
}

} // namespace

auto handle_blur(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    PROFILING_SCOPE();
    int size = get_param_int(puri, "size", 3);
    auto src = to_cv(req.body_);
    auto res = tr_blur(src, size);
    return img_to_response(res);
}

auto handle_adaptthresh(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    PROFILING_SCOPE();
    int blur_size = get_param_int(puri, "blur_size", 3);
    int block_size = get_param_int(puri, "block_size", 5);
    int diff = get_param_int(puri, "diff", 5);

    auto src = to_cv(req.body_);
    auto blurred = tr_blur(src, blur_size);
    auto gray = tr_to_grayscale(blurred);
    auto res = tr_adaptthresh(gray, block_size, diff);

    return img_to_response(res);
}

auto handle_reducecolors(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    PROFILING_SCOPE();
    int num_colors = get_param_int(puri, "num_colors", 5);
    auto src = to_cv(req.body_);
    auto res = tr_reducecolors(src, num_colors);
    return img_to_response(res);
}

auto handle_cartoonify(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> task<http_server::http_response> {
    int blur_size = get_param_int(puri, "blur_size", 3);
    int num_colors = get_param_int(puri, "num_colors", 5);
    int block_size = get_param_int(puri, "block_size", 5);
    int diff = get_param_int(puri, "diff", 5);

    auto src = to_cv(req.body_);

    ex::sender auto snd =                                               //
            ex::when_all(                                               //
                    ex::transfer_just(cdata.pool_.get_scheduler(), src) //
                            | ex::then([=](const cv::Mat& src) {
                                  PROFILING_SCOPE_N("compute edges");
                                  auto blurred = tr_blur(src, blur_size);
                                  auto gray = tr_to_grayscale(blurred);
                                  auto edges = tr_adaptthresh(gray, block_size, diff);
                                  return edges;
                              }),
                    ex::transfer_just(cdata.pool_.get_scheduler(), src) //
                            | ex::then([=](const cv::Mat& src) {        //
                                  PROFILING_SCOPE_N("reduce colors");
                                  return tr_reducecolors(src, num_colors);
                              })                                                 //
                    )                                                            //
            | ex::then([](const cv::Mat& edges, const cv::Mat& reduced_colors) { //
                  PROFILING_SCOPE_N("apply mask");
                  return tr_apply_mask(reduced_colors, edges);
              }) //
            | ex::then(img_to_response);
    co_return co_await std::move(snd);
}

auto handle_oilpainting(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    PROFILING_SCOPE();
    int size = get_param_int(puri, "size", 10);
    int dyn_ratio = get_param_int(puri, "dyn_ratio", 1);
    auto src = to_cv(req.body_);
    auto res = tr_oilpainting(src, size, dyn_ratio);
    return img_to_response(res);
}

auto handle_contourpaint(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> task<http_server::http_response> {
    int blur_size = get_param_int(puri, "blur_size", 3);
    int block_size = get_param_int(puri, "block_size", 5);
    int diff = get_param_int(puri, "diff", 5);
    int oil_size = get_param_int(puri, "oil_size", 3);
    int dyn_ratio = get_param_int(puri, "dyn_ratio", 5);

    auto src = to_cv(req.body_);

    ex::sender auto snd =                                               //
            ex::when_all(                                               //
                    ex::transfer_just(cdata.pool_.get_scheduler(), src) //
                            | ex::then([=](const cv::Mat& src) {
                                  PROFILING_SCOPE_N("compute edges");
                                  auto blurred = tr_blur(src, blur_size);
                                  auto gray = tr_to_grayscale(blurred);
                                  auto edges = tr_adaptthresh(gray, block_size, diff);
                                  return edges;
                              }),
                    ex::transfer_just(cdata.pool_.get_scheduler(), src) //
                            | ex::then([=](const cv::Mat& src) {        //
                                  PROFILING_SCOPE_N("oil painting");
                                  return tr_oilpainting(src, oil_size, dyn_ratio);
                              })                                                 //
                    )                                                            //
            | ex::then([](const cv::Mat& edges, const cv::Mat& reduced_colors) { //
                  PROFILING_SCOPE_N("apply mask");
                  return tr_apply_mask(reduced_colors, edges);
              }) //
            | ex::then(img_to_response);
    co_return co_await std::move(snd);
}

#else

auto handle_blur(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    return http_server::create_response(http_server::status_code::s_500_internal_server_error);
}

auto handle_adaptthresh(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    return http_server::create_response(http_server::status_code::s_500_internal_server_error);
}

auto handle_reducecolors(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    return http_server::create_response(http_server::status_code::s_500_internal_server_error);
}

auto handle_cartoonify(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> task<http_server::http_response> {
    co_return http_server::create_response(http_server::status_code::s_500_internal_server_error);
}

auto handle_oilpainting(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> http_server::http_response {
    return http_server::create_response(http_server::status_code::s_500_internal_server_error);
}

auto handle_contourpaint(const conn_data& cdata, http_server::http_request&& req, parsed_uri puri)
        -> task<http_server::http_response> {
    co_return http_server::create_response(http_server::status_code::s_500_internal_server_error);
}

#endif