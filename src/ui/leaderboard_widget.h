/*
 * Leaderboard Widget - Benchmark score tracking with SQLite persistence
 * License: MIT
 */

#pragma once

#include <QComboBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <sqlite3.h>
#include <vector>

struct BenchmarkEntry {
  int id = 0;
  QString testType; // "CPU", "GPU", "NPU"
  int score = 0;
  QString hardware; // e.g., "AMD Ryzen 7 7840HS"
  double duration = 0;
  QString timestamp;
};

class LeaderboardWidget : public QWidget {
  Q_OBJECT

public:
  explicit LeaderboardWidget(QWidget *parent = nullptr);
  ~LeaderboardWidget();

  // Public API for other widgets to add scores
  void addScore(const QString &testType, int score, const QString &hardware,
                double duration);

private:
  void setupUI();
  void applyStyles();
  void initDatabase();
  void loadScores();
  void refreshTable();

  // UI
  QTableWidget *scoreTable = nullptr;
  QComboBox *filterCombo = nullptr;
  QPushButton *exportButton = nullptr;
  QPushButton *clearButton = nullptr;
  QLabel *totalLabel = nullptr;

  // Database
  sqlite3 *db = nullptr;
  QString dbPath;

  // Data
  QVector<BenchmarkEntry> allEntries;
  QString currentFilter = "All";

private slots:
  void onFilterChanged(const QString &filter);
  void onExportClicked();
  void onClearClicked();
};
