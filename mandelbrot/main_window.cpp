#include "main_window.h"
#include "./ui_main_window.h"

#include <iostream>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::main_window)
{
    ui->setupUi(this);
    setWindowTitle("Mandelbrot");
    setFocus();
}

main_window::~main_window()
{
    delete ui;
}

void main_window::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_Left:
        ui->centralwidget->move_center(-mandelbrot::MOVE_STEP, 0);
        break;
    case Qt::Key_Right:
        ui->centralwidget->move_center(mandelbrot::MOVE_STEP, 0);
        break;
    case Qt::Key_Up:
        ui->centralwidget->move_center(0, -mandelbrot::MOVE_STEP);
        break;
    case Qt::Key_Down:
        ui->centralwidget->move_center(0, mandelbrot::MOVE_STEP);
        break;
    case Qt::Key_Plus:
        ui->centralwidget->zoom(mandelbrot::ZOOM_VALUE);
        break;
    case Qt::Key_Minus:
        ui->centralwidget->zoom(1 / mandelbrot::ZOOM_VALUE);
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void main_window::resizeEvent(QResizeEvent *event) {
    ui->centralwidget->resize(event->size().width(), event->size().height());
}

void main_window::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        ui->centralwidget->set_center(event->position().x(), event->position().y());
    }
}
