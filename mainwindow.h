#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QKeyEvent>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct Position {
    int i;
    int j;
};

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
		MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	void startGame();
    void checkFailed();
    void genNumber();

protected:
	void paintEvent(QPaintEvent* event);
	void keyPressEvent(QKeyEvent* event);

private:
	bool pressUp();
	bool pressDown();
	bool pressLeft();
	bool pressRight();
	Ui::MainWindow* ui;
	int m_score;
	int m_slots[4][4];
	bool m_started;
};
#endif // MAINWINDOW_H
