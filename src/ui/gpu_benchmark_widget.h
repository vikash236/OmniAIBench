#pragma once
#include <QWidget>

class GPUBenchmarkWidget : public QWidget {
  Q_OBJECT
public:
  explicit GPUBenchmarkWidget(QWidget *parent = nullptr) : QWidget(parent) {}
};
