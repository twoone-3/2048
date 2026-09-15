#ifndef GAMEBOARD_H
#define GAMEBOARD_H

// 2048 核心逻辑类：维护棋盘状态、移动合并、得分与胜负判定。
// 不依赖任何界面代码，便于后续单独复用与单元测试。
class GameBoard {
public:
	// 四个滑动方向：方向键 / WASD 分别对应 Up/Left/Down/Right
	enum class Direction { Up, Left, Down, Right };

	// 合成出该数字即达成胜利（2048 为游戏主题数字）
	static constexpr int WIN_VALUE = 2048;
	static constexpr int SIZE = 4;

	// 重置棋盘并开始一局新游戏（随机生成两个初始数字）
	void reset();
	// 朝指定方向滑动一帧；棋盘发生变化时自动生成一个新数字
	bool move(Direction direction);
	// 无空格且上下左右均无法合并时为游戏结束
	bool isGameOver() const;
	// 是否合出过 2048
	bool hasWon() const { return m_won; }
	int score() const { return m_score; }
	int value(int row, int col) const { return m_slots[row][col]; }

private:
	// 所有方向统一转换为一次"向左移动"来实现
	bool moveLeft();
	// 在空白格子随机生成 2 或 4
	void addRandomTile();
	// 矩阵转置
	void transpose();
	// 每行左右翻转
	void reverseRows();

	int m_slots[SIZE][SIZE] = {};
	int m_score = 0;
	bool m_won = false;
};

#endif // GAMEBOARD_H