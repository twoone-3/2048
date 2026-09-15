#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextOption>
#include <QTimer>
#include <optional>
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_started(false) {
  ui->setupUi(this);
  setFixedSize(500, 710);
  setStyleSheet(QStringLiteral(
      "QMainWindow { background: #faf7f0; }"
      "QLabel#score { color: #776e65; font-size: 28px; font-weight: 700; }"
      "QPushButton { background: #8f7a66; color: white; border: none;"
      "border-radius: 10px; padding: 0 12px; font-size: 14px; }"
      "QPushButton:hover { background: #a58b73; }"
      "QPushButton:pressed { background: #756352; }"
      "QPushButton:disabled { background: #d8cfc5; color: #9b9187; }"
      "QComboBox, QSpinBox { background: white; color: #5b5148;"
      "border: 1px solid #ddd2c5; border-radius: 9px; padding: 0 10px;"
      "font-size: 13px; }"
      "QToolButton { background: transparent; color: #8f7a66; border: none;"
      "padding: 0; font-size: 10px; }"
      "QToolButton:hover { background: #f3ece3; }"
      "QToolButton:pressed { background: #e8dccd; }"
      "QComboBox::drop-down { border: none; width: 24px; }"
      "QComboBox QAbstractItemView { background: white;"
      "selection-background-color: #ead8c3; }"
      "QStatusBar { color: #8a7d70; background: transparent; }"));
  connect(ui->start, &QPushButton::clicked, this, &MainWindow::startGame);
  // ---- 模式选择：手动 / 算法 / LLM ----
  m_modeCombo = new QComboBox(this);
  m_modeCombo->setGeometry(32, 96, 120, 38);
  m_modeCombo->addItem(QStringLiteral("手动"), static_cast<int>(Mode::Manual));
  m_modeCombo->addItem(QStringLiteral("算法"), static_cast<int>(Mode::Solver));
  m_modeCombo->addItem(QStringLiteral("LLM"), static_cast<int>(Mode::Llm));
  connect(m_modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
          &MainWindow::onModeChanged);
  // ---- 自动间隔 / 单步 / LLM 设置 ----
  m_intervalSpin = new QSpinBox(this);
  m_intervalSpin->setGeometry(164, 96, 112, 38);
  m_intervalSpin->setRange(50, 2000);
  m_intervalSpin->setValue(400);
  m_intervalSpin->setSuffix(QStringLiteral(" ms"));
  m_intervalSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
  m_intervalSpin->setFocusPolicy(Qt::StrongFocus);
  m_intervalSpin->setReadOnly(false);
  m_intervalSpin->setKeyboardTracking(false);

  m_intervalUpBtn = new QToolButton(this);
  m_intervalUpBtn->setText(QStringLiteral("▲"));
  m_intervalUpBtn->setGeometry(254, 97, 20, 17);
  m_intervalUpBtn->setAutoRepeat(true);
  m_intervalDownBtn = new QToolButton(this);
  m_intervalDownBtn->setText(QStringLiteral("▼"));
  m_intervalDownBtn->setGeometry(254, 116, 20, 17);
  m_intervalDownBtn->setAutoRepeat(true);
  connect(m_intervalUpBtn, &QToolButton::clicked, this, [this]() {
    m_intervalSpin->setValue(m_intervalSpin->value() + 50);
  });
  connect(m_intervalDownBtn, &QToolButton::clicked, this, [this]() {
    m_intervalSpin->setValue(m_intervalSpin->value() - 50);
  });
  connect(m_intervalSpin, qOverload<int>(&QSpinBox::valueChanged), this,
          &MainWindow::onIntervalChanged);
  const auto commitInterval = [this]() {
    QString text = m_intervalSpin->text().trimmed();
    text.remove(QStringLiteral("ms"), Qt::CaseInsensitive);
    bool ok = false;
    const int value = text.trimmed().toInt(&ok);
    if (!ok) return;
    m_intervalSpin->setValue(qBound(50, value, 2000));
    m_autoTimer.setInterval(m_intervalSpin->value());
  };
  connect(m_intervalSpin, &QAbstractSpinBox::editingFinished, this,
          commitInterval);

  m_autoBtn = new QPushButton(QStringLiteral("开始自动解题"), this);
  m_autoBtn->setGeometry(32, 145, 208, 38);
  m_autoBtn->setEnabled(false);
  connect(m_autoBtn, &QPushButton::clicked, this,
      &MainWindow::onAutoToggleClicked);

  m_stepBtn = new QPushButton(QStringLiteral("单步"), this);
  m_stepBtn->setGeometry(252, 145, 108, 38);
  m_stepBtn->setEnabled(false);
  connect(m_stepBtn, &QPushButton::clicked, this, &MainWindow::onStepClicked);

  m_llmSetupBtn = new QPushButton(QStringLiteral("LLM设置"), this);
  m_llmSetupBtn->setGeometry(372, 96, 96, 38);
  connect(m_llmSetupBtn, &QPushButton::clicked, this,
          &MainWindow::onLlmSetupClicked);

  // LLM 接口配置：默认从环境变量读取，可在界面“LLM设置”中修改
  m_llmUrl = qEnvironmentVariable("LLM_API_URL",
                                  "https://api.deepseek.com/chat/completions");
  m_llmKey = qEnvironmentVariable("LLM_API_KEY");
  m_llmModel = qEnvironmentVariable("LLM_MODEL", "deepseek-chat");
  m_llm.setConfig(m_llmUrl, m_llmKey, m_llmModel);
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
  m_autoTimer.stop();
  m_board.reset();
  m_started = true;
  ui->start->setText("重新开始");
  ui->score->setText("score: " + QString::number(m_board.score()));
  if (m_autoBtn && m_mode != Mode::Manual)
    m_autoBtn->setText(QStringLiteral("开始自动解题"));
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
    if (m_autoBtn) m_autoBtn->setText(QStringLiteral("开始自动解题"));
  }
  update();
}

void MainWindow::applyMove(GameBoard::Direction direction) {
  handleMove(m_board.move(direction));
}

void MainWindow::onModeChanged(int index) {
  m_mode = static_cast<Mode>(index);
  m_autoTimer.stop();
  m_stepBtn->setEnabled(m_mode != Mode::Manual);
  m_autoBtn->setEnabled(m_mode != Mode::Manual);
  m_autoBtn->setText(QStringLiteral("开始自动解题"));
  if (m_mode == Mode::Manual) {
    statusBar()->clearMessage();
    return;
  }
  if (!m_started) startGame();
  m_autoTimer.setInterval(m_intervalSpin->value());
  statusBar()->showMessage(
      m_mode == Mode::Solver
          ? QStringLiteral("算法模式：点击“开始自动解题”运行")
          : QStringLiteral("LLM 模式：点击“开始自动解题”运行"),
      2500);
}

void MainWindow::onAutoToggleClicked() {
  if (m_mode == Mode::Manual) return;
  if (!m_started) startGame();

  if (m_autoTimer.isActive()) {
    m_autoTimer.stop();
    m_autoBtn->setText(QStringLiteral("继续自动解题"));
    statusBar()->showMessage(QStringLiteral("自动解题已暂停"), 2500);
    return;
  }

  m_autoTimer.setInterval(m_intervalSpin->value());
  m_autoTimer.start();
  m_autoBtn->setText(QStringLiteral("暂停自动解题"));
  statusBar()->showMessage(
      m_mode == Mode::Solver ? QStringLiteral("算法模式：自动落子中")
                             : QStringLiteral("LLM 模式：等待模型返回指令"),
      2500);
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
  if (!m_started) return;
  applyMove(direction);
}

void MainWindow::onLlmWarning(const QString& message) {
  statusBar()->showMessage(message, 3000);
}

void MainWindow::onStepClicked() {
  if (m_mode == Mode::Manual) return;
  if (!m_started) startGame();
  // 单步走：暂停自动节奏，只执行一步
  m_autoTimer.stop();
  m_autoBtn->setText(QStringLiteral("继续自动解题"));
  onAutoTurn();
}

void MainWindow::onIntervalChanged(int ms) { m_autoTimer.setInterval(ms); }

void MainWindow::onLlmSetupClicked() {
  QDialog dialog(this);
  dialog.setWindowTitle(QStringLiteral("LLM 接口设置（OpenAI 兼容）"));

  QFormLayout form(&dialog);
  QLineEdit urlEdit(m_llmUrl);
  QLineEdit keyEdit(m_llmKey);
  keyEdit.setEchoMode(QLineEdit::Password);
  QLineEdit modelEdit(m_llmModel);
  form.addRow(QStringLiteral("API URL"), &urlEdit);
  form.addRow(QStringLiteral("API Key"), &keyEdit);
  form.addRow(QStringLiteral("Model"), &modelEdit);

  QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                           &dialog);
  connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  form.addRow(&buttons);

  if (dialog.exec() != QDialog::Accepted) return;
  m_llmUrl = urlEdit.text().trimmed();
  m_llmKey = keyEdit.text().trimmed();
  m_llmModel = modelEdit.text().trimmed();
  m_llm.setConfig(m_llmUrl, m_llmKey, m_llmModel);
  statusBar()->showMessage(QStringLiteral("LLM 接口已更新：%1").arg(m_llmModel),
                           3000);
}

void MainWindow::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.fillRect(rect(), QColor("#faf7f0"));
  // 绘制棋盘区域
  for (int row = 0; row < GameBoard::SIZE; ++row) {
    for (int col = 0; col < GameBoard::SIZE; ++col) {
      const int value = m_board.value(row, col);
      // 根据数值设置格子颜色
      painter.setPen(Qt::transparent);
      switch (value) {
        case 0:
          painter.setBrush(QColor("#e8dfd4"));
          break;
        case 2:
          painter.setBrush(QColor("#f2d39b"));
          break;
        case 4:
          painter.setBrush(QColor("#ead8b9"));
          break;
        case 8:
          painter.setBrush(QColor("#e9ad72"));
          break;
        case 16:
          painter.setBrush(QColor("#e88b62"));
          break;
        case 32:
          painter.setBrush(QColor("#d96b55"));
          break;
        case 64:
          painter.setBrush(QColor("#c9564c"));
          break;
        case 128:
          painter.setBrush(QColor("#b84b4b"));
          break;
        case 256:
          painter.setBrush(QColor("#9f3f4a"));
          break;
        case 512:
          painter.setBrush(QColor("#813646"));
          break;
        case 1024:
          painter.setBrush(QColor("#5e3d52"));
          break;
        case 2048:
          painter.setBrush(QColor("#432d45"));
          break;
        default:
          painter.setBrush(QColor("#30243a"));
          break;
      }
      QRect b(32 + 110 * col, 210 + 110 * row, 102, 102);
      painter.drawRoundedRect(b, 12, 12);
      if (value != 0) {
        painter.setPen(value > 4 ? Qt::white : QColor("#776e65"));
        painter.setFont(QFont("微软雅黑", value > 512 ? 24 : 28, 700, false));
        // 在方块中央绘制数值
        painter.drawText(b, QString::number(value),
                         QTextOption(Qt::AlignCenter));
      }
    }
  }
}
