#include "mandelbrot_renderer.h"

#include "utils.h"
#include <algorithm>

#include <QSize>
#include <QImage>

mandelbrot_renderer::mandelbrot_renderer()
    : cancel_operation(false), abort(false) {};

mandelbrot_renderer::~mandelbrot_renderer() {
    abort = true;
    cancel_operation = true;
    cv.notify_one();
    stop_worker_threads();

}

void mandelbrot_renderer::stop_worker_threads() {
    for (size_t i = 0; i < mandelbrot::THREADS_COUNT; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
}


void mandelbrot_renderer::recieve_updates(parameters const& requested_params) {
    if (abort.load()) return;
    params = requested_params;
    if (isRunning()) {
        cancel_operation = true;
        cv.notify_one();
    }
    start(QThread::LowPriority);
}

void mandelbrot_renderer::run() {
    while (!abort.load()) {
        run_render_threads(std::bind(&mandelbrot_renderer::render<true>, this, std::placeholders::_1, std::placeholders::_2), true);
        if (!cancel_operation.load() && !abort.load()) {
            run_render_threads(std::bind(&mandelbrot_renderer::render<false>, this, std::placeholders::_1, std::placeholders::_2), false);
        }

        {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [this]{
                return cancel_operation.load();
            });
            cancel_operation = false;
        }
    }
    stop_worker_threads();
}

void mandelbrot_renderer::run_render_threads(std::function<void(size_t, size_t)> f, bool low_pixmap_requested) {
    image = QImage(params.w, params.h, QImage::Format_RGB32);
    size_t from_y;
    size_t to_y = 0;
    size_t const x_step = std::max(mandelbrot::MIN_STEP_VALUE, params.h / mandelbrot::THREADS_COUNT);
    for (size_t i = 0; i != mandelbrot::THREADS_COUNT; ++i) {
        from_y = to_y;
        to_y = std::min(from_y + x_step, params.h);
        threads[i] = std::thread(f, from_y, to_y);
    }


    for (size_t i = 0; i != mandelbrot::THREADS_COUNT; ++i) {
        threads[i].join();
    }

    if (low_pixmap_requested && !cancel_operation.load() && !abort.load()) {
        emit update_low_pixmap(image);
    } else if (!cancel_operation.load() && !abort.load()){
        emit update_pixmap(image);
    }
}

template <bool downscaled>
void mandelbrot_renderer::render(size_t from_y, size_t to_y) {
    // variables
    double z_x;
    double z_y;

    double z_x2;
    double z_y2;
    double z_xy;

    double c_x;
    double c_y = params.center_y + (static_cast<int>(from_y) - static_cast<int>(params.h) / 2 - 1) * params.scale;

    size_t n_iterations;
    size_t x_step;
    size_t y_step;

    size_t y;
    size_t x;

    size_t i;

    auto pixel_pointer = reinterpret_cast<QRgb *>(image.scanLine(from_y));
    for (y = from_y; y != to_y; ++y) {
        c_y += params.scale;
        c_x = params.center_x - (params.w / 2 + 1) * params.scale;
        for (x = 0; x != params.w; ++x) {
            // looks ugly, but performs well
            z_x = z_y = 0.;
            c_x += params.scale;

            z_x2 = z_x * z_x;
            z_y2 = z_y * z_y;
            z_xy = z_x * z_y;
            n_iterations = 0;
            // abs(z) < 2.0
            while ((z_x2 + z_y2) < 4.0 && n_iterations < mandelbrot::ITERATIONS_NUM) {
                z_x = z_x2 - z_y2 + c_x;
                z_y = z_xy + z_xy + c_y;

                z_x2 = z_x * z_x;
                z_y2 = z_y * z_y;
                z_xy = z_x * z_y;

                ++n_iterations;
            }

            *pixel_pointer++ = mandelbrot::colors[n_iterations];

            if constexpr (downscaled) {
                x_step = std::min(params.w - x - 1, mandelbrot::SKIP_X_DOWNSCALED);
                x += x_step;
                c_x += x_step * params.scale;
                for (i = 0; i < x_step; ++i) {
                    *pixel_pointer++ = mandelbrot::colors[n_iterations];
                }
            }

            if (cancel_operation.load() || abort.load()) {
                return;
            }
        }

        if constexpr (downscaled) {
            y_step = std::min(to_y - y - 1, mandelbrot::SKIP_Y_DOWNSCALED);
            y += y_step;
            c_y += y_step * params.scale;
            auto prev_line_p = pixel_pointer - params.w;
            for (i = 0; i < y_step * params.w; ++i) {
                *pixel_pointer++ = *prev_line_p++;
                if (cancel_operation.load() || abort.load()) {
                    return;
                }
            }
        }
    }
}
