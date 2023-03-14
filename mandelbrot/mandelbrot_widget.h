#pragma once 

#include "mandelbrot_renderer.h"

#include <QWidget>
#include <QPaintEvent>
#include <QPixmap>

class mandelbrot_widget : public QWidget {
    Q_OBJECT
public:
    mandelbrot_widget(QWidget* parent = nullptr);

    void paintEvent(QPaintEvent *event) override;

    void move_center(double x, double y);
    void zoom(double zoom_value);
    void resize(size_t new_w, size_t new_h);
    void set_center(double x, double y);

private slots:
    void update_pixmap(QImage const& image);
    void update_low_pixmap(QImage const& image);

signals:
    void send_updates(double center_x_, double center_y_, double scale_, size_t w_, size_t h_);

private:
    QPixmap pixmap;
    QPixmap low_pixmap;

    parameters params;

    mandelbrot_renderer renderer;

    bool pixmap_updated;
    bool low_pixmap_updated;

    void request_updates();
};
