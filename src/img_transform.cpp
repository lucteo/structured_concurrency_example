#include "img_transform.hpp"

#if HAS_OPENCV

#include <opencv2/imgproc.hpp>
#include <opencv2/xphoto.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cstring>

auto tr_apply_mask(const cv::Mat& img_main, const cv::Mat& img_mask) -> cv::Mat {
    cv::Mat res;
    cv::bitwise_and(img_main, img_main, res, img_mask);
    return res;
}

auto tr_blur(const cv::Mat& src, int size) -> cv::Mat {
    cv::Mat res;
    cv::GaussianBlur(src, res, cv::Size(size, size), 0, 0, cv::BORDER_DEFAULT);
    return res;
}
auto tr_to_grayscale(const cv::Mat& src) -> cv::Mat {
    cv::Mat res;
    cv::cvtColor(src, res, cv::COLOR_BGR2GRAY);
    return res;
}
auto tr_adaptthresh(const cv::Mat& img, int block_size, int diff) -> cv::Mat {
    cv::Mat res;
    cv::adaptiveThreshold(
            img, res, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, block_size, diff);
    return res;
}
auto tr_reducecolors(const cv::Mat& img, int num_colors) -> cv::Mat {
    auto size = img.rows * img.cols;
    cv::Mat data = img.reshape(1, size);
    data.convertTo(data, CV_32F);
    auto criteria = cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0);
    cv::Mat labels;
    cv::Mat1f colors;
    cv::kmeans(data, num_colors, labels, criteria, 1, cv::KMEANS_RANDOM_CENTERS, colors);
    for (unsigned int i = 0; i < size; i++) {
        data.at<float>(i, 0) = colors(labels.at<int>(i), 0);
        data.at<float>(i, 1) = colors(labels.at<int>(i), 1);
        data.at<float>(i, 2) = colors(labels.at<int>(i), 2);
    }
    cv::Mat res = data.reshape(3, img.rows);
    res.convertTo(res, CV_8U);
    return res;
}
auto tr_cartoonify(const cv::Mat& img) -> cv::Mat {
    cv::Mat res;
    return res;
}
auto tr_oilpainting(const cv::Mat& img, int size, int dyn_ratio) -> cv::Mat {
    cv::Mat res;
    cv::xphoto::oilPainting(img, res, size, dyn_ratio, cv::COLOR_BGR2Lab);
    return res;
}
auto tr_contourpaint(const cv::Mat& img) -> cv::Mat {
    cv::Mat res;
    return res;
}

#endif