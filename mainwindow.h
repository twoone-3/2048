#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "gameboard.h"
#include "llmplayer.h"
#include "solver.h"
#include <QComboBox>
#include <QKeyEvent>
#include <QMainWindow>
#include <QPainter>
#include <QVector>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>

class QPushButton;
class QPlainTextEdit;
class QSpinBox;
class QToolButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE
class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  // 三种输入来源：键盘手动 / 内置算法 / 大模型 LLM
  enum class Mode { Manual, Solver, Llm };
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();
  void startGame();

 protected:
  void paintEvent(QPaintEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  struct AnimatedTile {
    int value;
    int fromRow;
    int fromCol;
    int toRow;
    int toCol;
    bool merged;
  };

  void handleMove(bool changed);
  void applyMove(GameBoard::Direction direction);
  void onModeChanged(int index);
  void onAutoToggleClicked();
  void onAutoTurn();
  void onStepClicked();
  void onIntervalChanged(int ms);
  void onLlmSetupClicked();
  void onLlmMove(GameBoard::Direction direction);
  void onLlmWarning(const QString& message);
  void appendLog(const QString& message);
  void finishAnimation();
  void drawTile(QPainter& painter, int row, int col, int value,
                qreal scale = 1.0, qreal opacity = 1.0) const;
  Ui::MainWindow* ui;
  GameBoard m_board;
  Solver m_solver;
  LlmPlayer m_llm;
  QTimer m_autoTimer;
  QComboBox* m_modeCombo = nullptr;
  QSpinBox* m_intervalSpin = nullptr;
  QToolButton* m_intervalUpBtn = nullptr;
  QToolButton* m_intervalDownBtn = nullptr;
  QPushButton* m_autoBtn = nullptr;
  QPushButton* m_stepBtn = nullptr;
  QPushButton* m_llmSetupBtn = nullptr;
  QPlainTextEdit* m_logEdit = nullptr;
  QString m_llmUrl;
  QString m_llmKey;
  QString m_llmModel;
  Mode m_mode = Mode::Manual;
  bool m_started = false;
  QTimer m_animationTimer;
  QElapsedTimer m_animationClock;
  int m_animationBefore[GameBoard::SIZE][GameBoard::SIZE] = {};
  int m_animationAfter[GameBoard::SIZE][GameBoard::SIZE] = {};
  QVector<AnimatedTile> m_animationTiles;
  int m_newTileRow = -1;
  int m_newTileCol = -1;
  int m_newTileValue = 0;
  bool m_animating = false;
  qreal m_animationProgress = 1.0;
};
#endif  // MAINWINDOW_H
