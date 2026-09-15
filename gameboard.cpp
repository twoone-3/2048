#include "gameboard.h"

#include <QRandomGenerator>
#include <utility>

namespace {
// 随机生成一个数字：90% 概率为 2，10% 概率为 4
int randomTileValue() {
    return QRandomGenerator::global()->bounded(10) < 9 ? 2 : 4;
}
}  // namespace

void GameBoard::reset() {
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            m_slots[i][j] = 0;
        }
    }
    m_score = 0;
    m_won = false;
    addRandomTile();
    addRandomTile();
}

bool GameBoard::move(Direction direction) {
    bool changed = false;
    // 所有方向都转换为"向左移动"：通过转置 / 翻转完成坐标变换
    switch (direction) {
    case Direction::Up:
        transpose();
        changed = moveLeft();
        transpose();
        break;
    case Direction::Down:
        transpose();
        reverseRows();
        changed = moveLeft();
        reverseRows();
        transpose();
        break;
    case Direction::Left:
        changed = moveLeft();
        break;
    case Direction::Right:
        reverseRows();
        changed = moveLeft();
        reverseRows();
        break;
    }

    if (changed)
        addRandomTile();
    return changed;
}

bool GameBoard::isGameOver() const {
    // 还有空格就还能落新数字，游戏未结束
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (m_slots[i][j] == 0)
                return false;
        }
    }
    // 左右或上下相邻存在相等方块时仍可合并，游戏未结束
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE - 1; ++j) {
            if (m_slots[i][j] == m_slots[i][j + 1] || m_slots[j][i] == m_slots[j + 1][i])
                return false;
        }
    }
    return true;
}

// 向左滑动：压缩 -> 相邻相等合并 -> 再压缩
bool GameBoard::moveLeft() {
    bool changed = false;
    for (int row = 0; row < SIZE; ++row) {
        // 第一步：把这一行的非零数字向左压缩
        int write = 0;
        for (int col = 0; col < SIZE; ++col) {
            if (m_slots[row][col] != 0) {
                if (write != col) {
                    std::swap(m_slots[row][write], m_slots[row][col]);
                    changed = true;
                }
                ++write;
            }
        }
        // 第二步：从左到右合并相等且相邻的数字
        for (int col = 0; col + 1 < SIZE; ++col) {
            if (m_slots[row][col] != 0 && m_slots[row][col] == m_slots[row][col + 1]) {
                m_slots[row][col] *= 2;
                m_slots[row][col + 1] = 0;
                m_score += m_slots[row][col];
                changed = true;
                if (m_slots[row][col] == WIN_VALUE)
                    m_won = true;
                ++col; // 跳过合并产生的空位
            }
        }
        // 第三步：合并产生的空洞再次压缩
        write = 0;
        for (int col = 0; col < SIZE; ++col) {
            if (m_slots[row][col] != 0) {
                if (write != col) {
                    std::swap(m_slots[row][write], m_slots[row][col]);
                    changed = true;
                }
                ++write;
            }
        }
    }
    return changed;
}

void GameBoard::addRandomTile() {
    struct Cell {
        int row;
        int col;
    };
    Cell empty[SIZE * SIZE];
    int count = 0;
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (m_slots[i][j] == 0) {
                empty[count].row = i;
                empty[count].col = j;
                ++count;
            }
        }
    }
    if (count == 0)
        return;
    const int pos = QRandomGenerator::global()->bounded(count);
    m_slots[empty[pos].row][empty[pos].col] = randomTileValue();
}

void GameBoard::transpose() {
    for (int i = 0; i < SIZE; ++i) {
        for (int j = i + 1; j < SIZE; ++j) {
            std::swap(m_slots[i][j], m_slots[j][i]);
        }
    }
}

void GameBoard::reverseRows() {
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE / 2; ++j) {
            std::swap(m_slots[i][j], m_slots[i][SIZE - 1 - j]);
        }
    }
}