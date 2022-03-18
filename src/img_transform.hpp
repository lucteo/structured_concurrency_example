#pragma once

#if HAS_OPENCV

#include <opencv2/imgproc.hpp>

#include <string>

using img_bytes = std::string;

auto tr_apply_mask(const cv::Mat& img_main, const cv::Mat& img_mask) -> cv::Mat;
auto tr_blur(const cv::Mat& src, int size) -> cv::Mat;
auto tr_to_grayscale(const cv::Mat& src) -> cv::Mat;
auto tr_adaptthresh(const cv::Mat& src, int block_size, int diff) -> cv::Mat;
auto tr_reducecolors(const cv::Mat& src, int num_colors) -> cv::Mat;
auto tr_oilpainting(const cv::Mat& src, int size, int dyn_ratio) -> cv::Mat;

#endif