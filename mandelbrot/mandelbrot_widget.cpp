#include "mandelbrot_widget.h"
#include "utils.h"

#include <iostream>

#include <QSize>
#include <QPainter>
#include <QImage>

mandelbrot_widget::mandelbrot_widget(QWidget* parent)
    : QWidget(parent), params{mandelbrot::START_CENTER_X, mandelbrot::START_CENTER_Y, mandelbrot::START_SCALE, 0, 0},
        renderer(), pixmap_updated(false), low_pixmap_updated(false) {

    connect(&renderer, &mandelbrot_renderer::update_pixmap, this, &mandelbrot_widget::update_pixmap);
    connect(&renderer, &mandelbrot_renderer::update_low_pixmap, this, &mandelbrot_widget::update_low_pixmap);
}

void mandelbrot_widget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    if (!low_pixmap_updated && !pixmap_updated) {
        painter.fillRect(rect(), Qt::black);
    } else if (!pixmap_updated){
        painter.drawPixmap(rect(), low_pixmap);
    } else {
        painter.drawPixmap(rect(), pixmap);
    }
}

void mandelbrot_widget::update_pixmap(QImage const& image) {
    pixmap = QPixmap::fromImage(image);
    pixmap_updated = true;
    update();
}

void mandelbrot_widget::update_low_pixmap(QImage const& image) {
    low_pixmap = QPixmap::fromImage(image);
    low_pixmap_updated = true;
    update();
}

void mandelbrot_widget::move_center(double x, double y) {
    params.center_x += x * params.scale;
    params.center_y += y * params.scale;
    request_updates();
}

void mandelbrot_widget::set_center(double x, double y) {
    params.center_x += (x - static_cast<double>(params.w) / 2) * params.scale;
    params.center_y += (y - static_cast<double>(params.h) / 2) * params.scale;
    request_updates();
}

void mandelbrot_widget::resize(size_t new_w, size_t new_h) {
    params.w = new_w;
    params.h = new_h;
    request_updates();
}

void mandelbrot_widget::zoom(double zoom_value) {
    params.scale *= zoom_value;
    params.scale = std::min(params.scale, mandelbrot::MAX_SCALE);
    request_updates();
}

void mandelbrot_widget::request_updates() {
    pixmap_updated = false;
    low_pixmap_updated = false;
    renderer.recieve_updates(params);
}
