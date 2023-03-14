#pragma once

#pragma once

#include "utils.h"

#include <vector>
#include <functional>

#include <QSize>
#include <QThread>
#include <QColor>
#include <QImage>

class mandelbrot_renderer : public QThread {
    Q_OBJECT
public:
    mandelbrot_renderer();
    ~mandelbrot_renderer();

    template <bool downscaled>
    void render(size_t from_x, size_t to_x);

    void run_render_threads(std::function<void(size_t, size_t)>, bool);

    void recieve_updates(parameters const& requested_params);

signals:
    void update_pixmap(QImage const& image);
    void update_low_pixmap(QImage const& image);

protected:
    void run() override;

private:
    void stop_worker_threads();

    std::atomic<bool> cancel_operation;
    std::atomic<bool> abort;

    parameters params;
    std::vector<std::thread> threads{mandelbrot::THREADS_COUNT};
    QImage image;

    std::condition_variable cv;
    std::mutex m;
};
