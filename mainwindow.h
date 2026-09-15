#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "gameboard.h"
#include "llmplayer.h"
#include "solver.h"
#include <QComboBox>
#include <QKeyEvent>
#include <QMainWindow>
#include <QPainter>
#include <QTimer>
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
  void handleMove(bool changed);
  void applyMove(GameBoard::Direction direction);
  void onModeChanged(int index);
  void onAutoTurn();
  void onLlmMove(GameBoard::Direction direction);
  void onLlmWarning(const QString& message);
  Ui::MainWindow* ui;
  GameBoard m_board;
  Solver m_solver;
  LlmPlayer m_llm;
  QTimer m_autoTimer;
  QComboBox* m_modeCombo = nullptr;
  Mode m_mode = Mode::Manual;
  bool m_started = false;
};
#endif  // MAINWINDOW_H
