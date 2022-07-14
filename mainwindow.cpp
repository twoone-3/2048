#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>

int Rand24() {
	// 生成随机数 2 4
	int rand_number = 2;
	while (true) {
		rand_number = rand() % 5;
		if (rand_number == 0 || rand_number == 1 || rand_number == 3) // 除去0
			continue;
		else
			break;
	}
	return rand_number;
}

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow),
	  m_score(0), m_slots(), m_started(false) {
	ui->setupUi(this);
	setFixedSize(400, 400);
	connect(ui->start, &QPushButton::clicked, this, &MainWindow::startGame);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::startGame() {
	if (m_started){
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				m_slots[i][j] = 0;
			}
		}
		m_score = 0;
	}
	int rand_number = Rand24();
	m_slots[rand() % 4][rand() % 4] = rand_number;

	m_started = true;
	ui->start->setText("重新开始");
	update();
}

void MainWindow::CheckFailed() {
	// 找出空白格子
	int number = 0;	  // 记录空白格子个数

	// 储存空白格子位置
	struct Position {
		int i;
		int j;
	};
	Position site_4[4 * 4];

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (m_slots[i][j] == 0) {
				site_4[number].i = i;
				site_4[number].j = j;
				number++;
			}
		}
	}
	//判断游戏是否结束
	if (number == 0) {
		// 上下相邻是否有相等的
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < (4 - 1); j++) {
				if (m_slots[i][j] == m_slots[i][j + 1])
					return;
			}
		}
		// 左右相邻是否有相等的
		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < (4 - 1); i++) {
				if (m_slots[i][j] == m_slots[i + 1][j]) {
					return;
				}
			}
		}
		QMessageBox::warning(this, "Game Over！", "游戏结束！");
		return;
	}
	// 随机获取空白格子
	int site_rand = rand() % number;

	// 随机生成2，4
	int rand_number = Rand24();

	int x, y;
	x = site_4[site_rand].i;
	y = site_4[site_rand].j;
	m_slots[x][y] = rand_number;
}

void MainWindow::paintEvent(QPaintEvent* event) {
	QPainter painter(this);
	ui->score->setText("score: " + QString::number(m_score));

	// draw region
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			int value = m_slots[i][j];
			// 根据值设置格子刷子颜色
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
			QRect b(100 + 50 * i, 100 + 50 * j, 48, 48);
			painter.drawRect(b);
			if (value != 0) {
				painter.setPen(Qt::black);
				painter.setFont(QFont("微软雅黑", 10, 700, false));
				//在方块中 绘制Text， 并且文本位置位于中部
				painter.drawText(b, QString::number(value), QTextOption(Qt::AlignCenter));
			}
		}
	}
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
	if (m_started == false)
		return;
	int key = event->key();
	switch (key) {
	case Qt::Key_W:
		pressUp();
		break;
	case Qt::Key_S:
		pressDown();
		break;
	case Qt::Key_A:
		pressLeft();
		break;
	case Qt::Key_D:
		pressRight();
		break;
	default:
		break;
	}
	CheckFailed();
	update();
}

void MainWindow::pressUp() {
	for (int i = 0; i < 4; ++i) {
		for (int j = 1; j < 4; ++j) {
			if (m_slots[i][j] == 0)
				continue;
			for (int k = 0; k < j; ++k) {
				if (m_slots[i][k] == 0) { // 查看是否有格子可以移动
					std::swap(m_slots[i][j], m_slots[i][k]);
					break;
				}
			}
		}
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 1; j < 4; ++j) {
			if (m_slots[i][j] == m_slots[i][j - 1]) {
				m_slots[i][j] = 0;
				m_slots[i][j - 1] *= 2;
				m_score += m_slots[i][j - 1];
				for (int k = j + 1; k < 4; ++k) {
					std::swap(m_slots[i][k - 1], m_slots[i][k]);
				}
			}
		}
	}
}

void MainWindow::pressDown() {
	for (int i = 0; i < 4; ++i) {
		for (int j = 2; j > -1; --j) {
			if (m_slots[i][j] == 0)
				continue;
			for (int k = 3; k > j; --k) {
				if (m_slots[i][k] == 0) { // 查看是否有格子可以移动
					std::swap(m_slots[i][j], m_slots[i][k]);
					break;
				}
			}
		}
	}
	for (int i = 0; i < 4; ++i) {
		for (int j = 2; j > -1; --j) {
			if (m_slots[i][j] == m_slots[i][j + 1]) {
				m_slots[i][j] = 0;
				m_slots[i][j + 1] *= 2;
				m_score += m_slots[i][j + 1];
				for (int k = j - 1; k > -1; --k) {
					std::swap(m_slots[i][k + 1], m_slots[i][k]);
				}
			}
		}
	}
}

void MainWindow::pressLeft() {
	for (int j = 0; j < 4; ++j) {
		for (int i = 1; i < 4; ++i) {
			if (m_slots[i][j] == 0)
				continue;
			for (int k = 0; k < i; ++k) {
				if (m_slots[k][j] == 0) { // 查看是否有格子可以移动
					std::swap(m_slots[i][j], m_slots[k][j]);
					break;
				}
			}
		}
	}
	for (int j = 0; j < 4; ++j) {
		for (int i = 1; i < 4; ++i) {
			if (m_slots[i][j] == m_slots[i - 1][j]) {
				m_slots[i][j] = 0;
				m_slots[i - 1][j] *= 2;
				m_score += m_slots[i - 1][j];
				for (int k = i + 1; k < 4; ++k) {
					std::swap(m_slots[k - 1][j], m_slots[k][j]);
				}
			}
		}
	}
}

void MainWindow::pressRight() {
	for (int j = 0; j < 4; ++j) {
		for (int i = 2; i > -1; --i) {
			if (m_slots[i][j] == 0)
				continue;
			for (int k = 3; k > i; --k) {
				if (m_slots[k][j] == 0) { // 查看是否有格子可以移动
					std::swap(m_slots[i][j], m_slots[k][j]);
					break;
				}
			}
		}
	}
	for (int j = 0; j < 4; ++j) {
		for (int i = 2; i > -1; --i) {
			if (m_slots[i][j] == m_slots[i + 1][j]) {
				m_slots[i][j] = 0;
				m_slots[i + 1][j] *= 2;
				m_score += m_slots[i + 1][j];
				for (int k = i - 1; k > -1; --k) {
					std::swap(m_slots[k + 1][j], m_slots[k][j]);
				}
			}
		}
	}
}
