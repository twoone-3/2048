#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "gameboard.h"
#include <QKeyEvent>
#include <QMainWindow>
#include <QPainter>
QT_BEGIN_NAMESPACE
namespace Ui {
	class MainWindow;
}
QT_END_NAMESPACE
class MainWindow : public QMainWindow {
	Q_OBJECT
	public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	void startGame();
	protected:
	void paintEvent(QPaintEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	private:
	void handleMove(bool changed);
	Ui::MainWindow* ui;
	GameBoard m_board;
	bool m_started;
};
#endif // MAINWINDOW_H
