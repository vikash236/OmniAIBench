#pragma once
#include <QWidget>

class NPUBenchmarkWidget : public QWidget {
  Q_OBJECT
public:
  explicit NPUBenchmarkWidget(QWidget *parent = nullptr) : QWidget(parent) {}
};
