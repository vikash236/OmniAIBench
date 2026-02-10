/*
 * Leaderboard Widget Implementation
 * SQLite-backed score tracking with filter and export
 * License: MIT
 */

#include "leaderboard_widget.h"
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTextStream>

LeaderboardWidget::LeaderboardWidget(QWidget *parent) : QWidget(parent) {
  initDatabase();
  setupUI();
  applyStyles();
  loadScores();
  refreshTable();
}

LeaderboardWidget::~LeaderboardWidget() {
  if (db) {
    sqlite3_close(db);
    db = nullptr;
  }
}

void LeaderboardWidget::initDatabase() {
  // Store in app data directory
  QString dataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(dataDir);
  dbPath = dataDir + "/benchmarks.db";

  int rc = sqlite3_open(dbPath.toUtf8().constData(), &db);
  if (rc != SQLITE_OK) {
    return;
  }

  // Create table
  const char *createSQL = "CREATE TABLE IF NOT EXISTS benchmarks ("
                          "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "  test_type TEXT NOT NULL,"
                          "  score INTEGER NOT NULL,"
                          "  hardware TEXT,"
                          "  duration REAL,"
                          "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
                          ");";

  sqlite3_exec(db, createSQL, nullptr, nullptr, nullptr);
}

void LeaderboardWidget::setupUI() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(15);

  // Title
  auto *titleLabel = new QLabel("Benchmark Leaderboard");
  QFont titleFont = titleLabel->font();
  titleFont.setPointSize(18);
  titleFont.setBold(true);
  titleLabel->setFont(titleFont);
  titleLabel->setStyleSheet("color: #6A9955;"); // Green for Leaderboard
  mainLayout->addWidget(titleLabel);

  // Toolbar: filter + buttons
  auto *toolbar = new QWidget(this);
  auto *toolbarLayout = new QHBoxLayout(toolbar);
  toolbarLayout->setContentsMargins(0, 0, 0, 0);

  auto *filterLabel = new QLabel("Filter:");
  filterLabel->setStyleSheet("color: #CCCCCC; font-size: 13px;");
  toolbarLayout->addWidget(filterLabel);

  filterCombo = new QComboBox(this);
  filterCombo->addItems({"All", "CPU", "GPU", "NPU"});
  filterCombo->setMinimumWidth(120);
  connect(filterCombo, &QComboBox::currentTextChanged, this,
          &LeaderboardWidget::onFilterChanged);
  toolbarLayout->addWidget(filterCombo);

  toolbarLayout->addStretch();

  totalLabel = new QLabel("0 results", this);
  totalLabel->setStyleSheet(
      "color: #808080; font-size: 12px; font-style: italic;");
  toolbarLayout->addWidget(totalLabel);

  exportButton = new QPushButton("Export JSON", this);
  exportButton->setMinimumWidth(100);
  connect(exportButton, &QPushButton::clicked, this,
          &LeaderboardWidget::onExportClicked);
  toolbarLayout->addWidget(exportButton);

  clearButton = new QPushButton("Clear All", this);
  clearButton->setMinimumWidth(80);
  clearButton->setStyleSheet(
      "QPushButton { background-color: #CE4444; color: white; "
      "border: none; border-radius: 4px; padding: 6px 12px; }"
      "QPushButton:hover { background-color: #B33A3A; }");
  connect(clearButton, &QPushButton::clicked, this,
          &LeaderboardWidget::onClearClicked);
  toolbarLayout->addWidget(clearButton);

  mainLayout->addWidget(toolbar);

  // Score table
  scoreTable = new QTableWidget(this);
  scoreTable->setColumnCount(6);
  scoreTable->setHorizontalHeaderLabels(
      {"#", "Date", "Test", "Score", "Hardware", "Duration"});
  scoreTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
  scoreTable->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  scoreTable->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  scoreTable->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);
  scoreTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
  scoreTable->horizontalHeader()->setSectionResizeMode(
      5, QHeaderView::ResizeToContents);
  scoreTable->setColumnWidth(0, 40);
  scoreTable->verticalHeader()->setVisible(false);
  scoreTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  scoreTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  scoreTable->setAlternatingRowColors(true);
  scoreTable->setShowGrid(false);
  scoreTable->verticalHeader()->setDefaultSectionSize(28);
  scoreTable->setSortingEnabled(true);

  mainLayout->addWidget(scoreTable);
}

void LeaderboardWidget::applyStyles() {
  setStyleSheet(R"(
    QComboBox {
        background-color: #3E3E42;
        color: #CCCCCC;
        border: 1px solid #3E3E42;
        border-radius: 4px;
        padding: 5px 10px;
        font-size: 12px;
    }
    QComboBox::drop-down {
        border: none;
        width: 20px;
    }
    QComboBox QAbstractItemView {
        background-color: #2D2D30;
        color: #CCCCCC;
        selection-background-color: #094771;
        border: 1px solid #3E3E42;
    }

    QPushButton {
        background-color: #3E3E42;
        color: #CCCCCC;
        border: 1px solid #3E3E42;
        border-radius: 4px;
        padding: 6px 12px;
        font-size: 12px;
    }
    QPushButton:hover {
        background-color: #505050;
    }

    QTableWidget {
        background-color: #1E1E1E;
        color: #CCCCCC;
        border: 1px solid #3E3E42;
        border-radius: 4px;
        gridline-color: #2D2D30;
        font-size: 12px;
        font-family: 'Consolas', monospace;
        outline: none;
        selection-background-color: #094771;
    }
    QTableWidget::item {
        padding: 4px 8px;
    }
    QTableWidget::item:alternate {
        background-color: #1A1A1A;
    }
    QHeaderView::section {
        background-color: #252526;
        color: #CCCCCC;
        border: none;
        border-bottom: 1px solid #3E3E42;
        border-right: 1px solid #2D2D30;
        padding: 6px 8px;
        font-size: 12px;
        font-weight: bold;
    }

    QScrollBar:vertical {
        background-color: #1E1E1E;
        width: 10px;
        border: none;
    }
    QScrollBar::handle:vertical {
        background-color: #424242;
        border-radius: 4px;
        min-height: 30px;
    }
  )");
}

void LeaderboardWidget::loadScores() {
  allEntries.clear();
  if (!db)
    return;

  const char *selectSQL =
      "SELECT id, test_type, score, hardware, duration, timestamp "
      "FROM benchmarks ORDER BY timestamp DESC;";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) != SQLITE_OK)
    return;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    BenchmarkEntry entry;
    entry.id = sqlite3_column_int(stmt, 0);
    entry.testType =
        QString::fromUtf8((const char *)sqlite3_column_text(stmt, 1));
    entry.score = sqlite3_column_int(stmt, 2);
    entry.hardware =
        QString::fromUtf8((const char *)sqlite3_column_text(stmt, 3));
    entry.duration = sqlite3_column_double(stmt, 4);
    entry.timestamp =
        QString::fromUtf8((const char *)sqlite3_column_text(stmt, 5));
    allEntries.append(entry);
  }
  sqlite3_finalize(stmt);
}

void LeaderboardWidget::refreshTable() {
  scoreTable->setSortingEnabled(false);
  scoreTable->setRowCount(0);

  int visibleCount = 0;
  for (const auto &entry : allEntries) {
    if (currentFilter != "All" && entry.testType != currentFilter)
      continue;

    int row = scoreTable->rowCount();
    scoreTable->insertRow(row);
    visibleCount++;

    // Rank
    auto *rankItem = new QTableWidgetItem(QString::number(visibleCount));
    rankItem->setTextAlignment(Qt::AlignCenter);
    rankItem->setForeground(QColor("#808080"));
    scoreTable->setItem(row, 0, rankItem);

    // Date
    auto *dateItem = new QTableWidgetItem(entry.timestamp);
    dateItem->setForeground(QColor("#808080"));
    scoreTable->setItem(row, 1, dateItem);

    // Test type (color-coded)
    auto *typeItem = new QTableWidgetItem(entry.testType);
    typeItem->setTextAlignment(Qt::AlignCenter);
    if (entry.testType == "CPU")
      typeItem->setForeground(QColor("#569CD6"));
    else if (entry.testType == "GPU")
      typeItem->setForeground(QColor("#CE9178"));
    else if (entry.testType == "NPU")
      typeItem->setForeground(QColor("#DCDCAA"));
    scoreTable->setItem(row, 2, typeItem);

    // Score (highlighted)
    auto *scoreItem = new QTableWidgetItem(QString::number(entry.score));
    scoreItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    scoreItem->setForeground(QColor("#6A9955"));
    QFont scoreFont;
    scoreFont.setBold(true);
    scoreFont.setPointSize(11);
    scoreItem->setFont(scoreFont);
    scoreTable->setItem(row, 3, scoreItem);

    // Hardware
    auto *hwItem = new QTableWidgetItem(entry.hardware);
    hwItem->setForeground(QColor("#CCCCCC"));
    scoreTable->setItem(row, 4, hwItem);

    // Duration
    auto *durItem =
        new QTableWidgetItem(QString("%1s").arg(entry.duration, 0, 'f', 2));
    durItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    durItem->setForeground(QColor("#808080"));
    scoreTable->setItem(row, 5, durItem);
  }

  scoreTable->setSortingEnabled(true);
  totalLabel->setText(QString("%1 results").arg(visibleCount));
}

void LeaderboardWidget::addScore(const QString &testType, int score,
                                 const QString &hardware, double duration) {
  if (!db)
    return;

  const char *insertSQL =
      "INSERT INTO benchmarks (test_type, score, hardware, duration) "
      "VALUES (?, ?, ?, ?);";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK)
    return;

  sqlite3_bind_text(stmt, 1, testType.toUtf8().constData(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, score);
  sqlite3_bind_text(stmt, 3, hardware.toUtf8().constData(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 4, duration);

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  // Reload and refresh
  loadScores();
  refreshTable();
}

void LeaderboardWidget::onFilterChanged(const QString &filter) {
  currentFilter = filter;
  refreshTable();
}

void LeaderboardWidget::onExportClicked() {
  QString fileName = QFileDialog::getSaveFileName(
      this, "Export Benchmark Results", "benchmarks.json",
      "JSON Files (*.json);;CSV Files (*.csv)");

  if (fileName.isEmpty())
    return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QTextStream stream(&file);

  if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
    // CSV export
    stream << "Date,Test,Score,Hardware,Duration\n";
    for (const auto &entry : allEntries) {
      if (currentFilter != "All" && entry.testType != currentFilter)
        continue;
      stream << entry.timestamp << "," << entry.testType << "," << entry.score
             << "," << entry.hardware << ","
             << QString::number(entry.duration, 'f', 2) << "\n";
    }
  } else {
    // JSON export
    stream << "[\n";
    bool first = true;
    for (const auto &entry : allEntries) {
      if (currentFilter != "All" && entry.testType != currentFilter)
        continue;
      if (!first)
        stream << ",\n";
      first = false;
      stream << "  {\n"
             << "    \"date\": \"" << entry.timestamp << "\",\n"
             << "    \"test\": \"" << entry.testType << "\",\n"
             << "    \"score\": " << entry.score << ",\n"
             << "    \"hardware\": \"" << entry.hardware << "\",\n"
             << "    \"duration\": " << entry.duration << "\n"
             << "  }";
    }
    stream << "\n]\n";
  }

  file.close();
  QMessageBox::information(this, "Export Complete",
                           QString("Results exported to:\n%1").arg(fileName));
}

void LeaderboardWidget::onClearClicked() {
  auto reply = QMessageBox::question(
      this, "Clear All Results",
      "Are you sure you want to delete all benchmark results?\n"
      "This action cannot be undone.",
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

  if (reply != QMessageBox::Yes)
    return;

  if (db) {
    sqlite3_exec(db, "DELETE FROM benchmarks;", nullptr, nullptr, nullptr);
  }

  loadScores();
  refreshTable();
}
