#pragma once
#include <QWidget>

class SensorPanelWidget : public QWidget {
  Q_OBJECT
public:
  explicit SensorPanelWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  void updateData() {}
};
