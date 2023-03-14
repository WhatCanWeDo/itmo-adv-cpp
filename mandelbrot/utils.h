#pragma once

#include <array>

#include <QColor>
#include <QThread>

namespace mandelbrot {
inline constexpr size_t ITERATIONS_NUM = 2048;
inline constexpr size_t MIN_STEP_VALUE = 5;
inline constexpr size_t SKIP_X_DOWNSCALED = 12;
inline constexpr size_t SKIP_Y_DOWNSCALED = 8;

inline constexpr double MAX_SCALE = 7.5;

static size_t THREADS_COUNT = QThread::idealThreadCount();

inline constexpr double MOVE_STEP = 7.;
inline constexpr double ZOOM_VALUE = 0.9;

inline constexpr QRgb DEFAULT_COLOR_VALUE = qRgb(0, 0, 0);

inline constexpr double START_CENTER_X = 0.;
inline constexpr double START_CENTER_Y = 0.;

inline constexpr double START_SCALE = 0.005;

inline constexpr int bound(int n) {
    if (n < 0) return 0;
    if (n > 255) return 255;
    return n;
}

static constexpr std::array<QRgb, ITERATIONS_NUM + 1> initialize_colors() {
    std::array<QRgb, ITERATIONS_NUM + 1> res{};
    for (size_t i = 0; i < ITERATIONS_NUM; ++i) {
        double v = static_cast<double>(i) / ITERATIONS_NUM;
        double v2 = v * v;
        double v3 = v2 * v;
        // magic
        // std::round is not constexpr :(
        int r = bound(static_cast<int>(1048.565 * v3 - 1469.821 * v2 + 427.482 * v + 180.704));
        int g = bound(static_cast<int>(2180.652 * v3 - 2748.375 * v2 + 767.097 * v + 87.703));
        int b = bound(static_cast<int>(-389.602 * v3 + 552.344 * v2 - 42.702 * v + 104.353));
        res[i] = qRgb(r, g, b);
    }
    res[ITERATIONS_NUM] = DEFAULT_COLOR_VALUE;
    return res;
}

static constexpr std::array<QRgb, ITERATIONS_NUM + 1> colors = initialize_colors();
};

struct parameters {
    double center_x;
    double center_y;
    double scale;
    size_t w;
    size_t h;
};
