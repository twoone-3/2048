#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTextOption>
#include <QTimer>
#include <optional>
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_started(false) {
  ui->setupUi(this);
  setFixedSize(400, 440);
  connect(ui->start, &QPushButton::clicked, this, &MainWindow::startGame);
  // ---- 模式选择：手动 / 算法 / LLM ----
  m_modeCombo = new QComboBox(this);
  m_modeCombo->setGeometry(8, 8, 96, 22);
  m_modeCombo->addItem(QStringLiteral("手动"), static_cast<int>(Mode::Manual));
  m_modeCombo->addItem(QStringLiteral("算法"), static_cast<int>(Mode::Solver));
  m_modeCombo->addItem(QStringLiteral("LLM"), static_cast<int>(Mode::Llm));
  connect(m_modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &MainWindow::onModeChanged);
  // ---- 自动落子节奏（算法 / LLM 模式使用）----
  m_autoTimer.setInterval(400);
  connect(&m_autoTimer, &QTimer::timeout, this, &MainWindow::onAutoTurn);
  connect(&m_llm, &LlmPlayer::moveReady, this, &MainWindow::onLlmMove);
  connect(&m_llm, &LlmPlayer::warningShown, this, &MainWindow::onLlmWarning);
  // 避免按钮长期占用键盘焦点，保证方向键事件能到达主窗口
  ui->start->setFocusPolicy(Qt::NoFocus);
  setFocusPolicy(Qt::StrongFocus);
  // 启动即开局：方向键 / WASD 立即可用
  startGame();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::startGame() {
  m_board.reset();
  m_started = true;
  ui->start->setText("重新开始");
  ui->score->setText("score: " + QString::number(m_board.score()));
  // 确保窗口持有键盘焦点，方向键事件不会被其他部件截获
  setFocus();
  update();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
  if (!m_started) return;
  // 非手动模式下键盘不参与，由算法 / LLM 驱动
  if (m_mode != Mode::Manual) return;
  GameBoard::Direction direction;
  switch (event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up:
      direction = GameBoard::Direction::Up;
      break;
    case Qt::Key_S:
    case Qt::Key_Down:
      direction = GameBoard::Direction::Down;
      break;
    case Qt::Key_A:
    case Qt::Key_Left:
      direction = GameBoard::Direction::Left;
      break;
    case Qt::Key_D:
    case Qt::Key_Right:
      direction = GameBoard::Direction::Right;
      break;
    default:
      QMainWindow::keyPressEvent(event);
      return;
  }
  handleMove(m_board.move(direction));
  event->accept();
}

void MainWindow::handleMove(bool changed) {
  if (!changed) return;
  ui->score->setText("score: " + QString::number(m_board.score()));
  if (m_board.hasWon())
    QMessageBox::information(this, "恭喜！", "你合成了 2048，游戏胜利！");
  if (m_board.isGameOver()) {
    QMessageBox::warning(this, "Game Over！", "游戏结束！");
    m_autoTimer.stop();
    m_board.reset();
    m_started = false;
    ui->start->setText("开始游戏");
    ui->score->setText("score: 0");
  }
  update();
}

void MainWindow::applyMove(GameBoard::Direction direction) {
  handleMove(m_board.move(direction));
}

void MainWindow::onModeChanged(int index) {
  m_mode = static_cast<Mode>(index);
  m_autoTimer.stop();
  if (m_mode == Mode::Manual) {
    statusBar()->clearMessage();
    return;
  }
  if (!m_started) startGame();
  m_autoTimer.setInterval(m_mode == Mode::Solver ? 400 : 1200);
  statusBar()->showMessage(
      m_mode == Mode::Solver
          ? QStringLiteral("算法模式：AI 自动落子中…")
          : QStringLiteral("LLM 模式：等待模型返回 JSON 指令…"),
      2500);
  m_autoTimer.start();
}

void MainWindow::onAutoTurn() {
  if (!m_started) {
    m_autoTimer.stop();
    return;
  }

  if (m_mode == Mode::Solver) {
    const std::optional<GameBoard::Direction> direction =
        m_solver.chooseMove(m_board);
    if (direction) applyMove(*direction);
  } else if (m_mode == Mode::Llm) {
    m_llm.requestMove(m_board);
  }
}

void MainWindow::onLlmMove(GameBoard::Direction direction) {
  applyMove(direction);
}

void MainWindow::onLlmWarning(const QString& message) {
  statusBar()->showMessage(message, 3000);
}

void MainWindow::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  // 绘制棋盘区域
  for (int row = 0; row < GameBoard::SIZE; ++row) {
    for (int col = 0; col < GameBoard::SIZE; ++col) {
      const int value = m_board.value(row, col);
      // 根据数值设置格子颜色
      painter.setPen(Qt::transparent);
      switch (value) {
        case 0:
          painter.setBrush(QBrush(QColor(204, 202, 202, 80)));
          break;
        case 2:
          painter.setBrush(QBrush(QColor(255, 252, 227, 210)));
          break;
        case 4:
          painter.setBrush(QBrush(QColor(255, 241, 140, 210)));
          break;
        case 8:
          painter.setBrush(QBrush(QColor(255, 243, 26, 210)));
          break;
        case 16:
          painter.setBrush(QBrush(QColor(128, 255, 252, 210)));
          break;
        case 32:
          painter.setBrush(QBrush(QColor(38, 255, 250, 210)));
          break;
        case 64:
          painter.setBrush(QBrush(QColor(35, 254, 150, 210)));
          break;
        case 128:
          painter.setBrush(QBrush(QColor(0, 200, 60, 210)));
          break;
        case 256:
          painter.setBrush(QBrush(QColor(255, 51, 51, 210)));
          break;
        case 512:
          painter.setBrush(QBrush(QColor(255, 2, 2, 210)));
          break;
        case 1024:
          painter.setBrush(QBrush(QColor(96, 78, 254, 210)));
          break;
        case 2048:
          painter.setBrush(QBrush(QColor(48, 24, 254, 210)));
          break;
        default:
          painter.setBrush(QBrush(QColor(254, 1, 159, 210)));
          break;
      }
      QRect b(50 + 75 * col, 50 + 75 * row, 70, 70);
      painter.drawRoundedRect(b, 15, 15);
      if (value != 0) {
        painter.setPen(Qt::black);
        painter.setFont(QFont("微软雅黑", 20, 700, false));
        // 在方块中央绘制数值
        painter.drawText(b, QString::number(value),
                         QTextOption(Qt::AlignCenter));
      }
    }
  }
}
