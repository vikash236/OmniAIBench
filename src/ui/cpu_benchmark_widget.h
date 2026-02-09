#pragma once
#include <QWidget>

class CPUBenchmarkWidget : public QWidget {
  Q_OBJECT
public:
  explicit CPUBenchmarkWidget(QWidget *parent = nullptr) : QWidget(parent) {}
};
