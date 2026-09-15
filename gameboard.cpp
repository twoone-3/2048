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

// 在真实棋盘上执行一次移动；移动或合并发生后补充一个新数字
bool GameBoard::move(Direction direction) {
  bool changed = false;
  const int gained = applyDirection(m_slots, direction, changed);
  if (changed) {
    m_score += gained;
    // 成功合成的数字可能达到胜利阈值
    for (int i = 0; i < SIZE; ++i) {
      for (int j = 0; j < SIZE; ++j) {
        if (m_slots[i][j] == WIN_VALUE) m_won = true;
      }
    }
    // 移动成功才生成随机新数字
    addRandomTile();
  }
  return changed;
}

// 在副本上模拟一次移动，不生成新数字、不产生随机数
int GameBoard::previewMove(Direction direction, int out[SIZE][SIZE]) const {
  for (int i = 0; i < SIZE; ++i) {
    for (int j = 0; j < SIZE; ++j) {
      out[i][j] = m_slots[i][j];
    }
  }
  bool changed = false;
  return applyDirection(out, direction, changed);
}

bool GameBoard::isGameOver() const {
  // 还有空格就还能落新数字，游戏未结束
  for (int i = 0; i < SIZE; ++i) {
    for (int j = 0; j < SIZE; ++j) {
      if (m_slots[i][j] == 0) return false;
    }
  }
  // 左右或上下相邻存在相等方块时仍可合并，游戏未结束
  for (int i = 0; i < SIZE; ++i) {
    for (int j = 0; j < SIZE - 1; ++j) {
      if (m_slots[i][j] == m_slots[i][j + 1] ||
          m_slots[j][i] == m_slots[j + 1][i])
        return false;
    }
  }
  return true;
}

// ---- 以下为纯网格运算，不访问成员状态，move / previewMove 共用 ----
int GameBoard::applyDirection(int grid[SIZE][SIZE], Direction direction,
                              bool& changed) {
  int gained = 0;
  // 所有方向都转换为"向左滑动"：通过转置 / 翻转完成坐标变换
  switch (direction) {
    case Direction::Up:
      transposeGrid(grid);
      gained = moveLeftGrid(grid, changed);
      transposeGrid(grid);
      break;
    case Direction::Down:
      transposeGrid(grid);
      reverseGridRows(grid);
      gained = moveLeftGrid(grid, changed);
      reverseGridRows(grid);
      transposeGrid(grid);
      break;
    case Direction::Left:
      gained = moveLeftGrid(grid, changed);
      break;
    case Direction::Right:
      reverseGridRows(grid);
      gained = moveLeftGrid(grid, changed);
      reverseGridRows(grid);
      break;
  }
  return gained;
}

// 向左滑动：压缩 -> 相邻相等合并 -> 再压缩
int GameBoard::moveLeftGrid(int grid[SIZE][SIZE], bool& changed) {
  int gained = 0;
  for (int row = 0; row < SIZE; ++row) {
    // 第一步：把这一行的非零数字向左压缩
    int write = 0;
    for (int col = 0; col < SIZE; ++col) {
      if (grid[row][col] != 0) {
        if (write != col) {
          std::swap(grid[row][write], grid[row][col]);
          changed = true;
        }
        ++write;
      }
    }
    // 第二步：从左到右合并相邻相等数字，每对只合并一次
    for (int col = 0; col + 1 < SIZE; ++col) {
      if (grid[row][col] != 0 && grid[row][col] == grid[row][col + 1]) {
        grid[row][col] *= 2;
        grid[row][col + 1] = 0;
        gained += grid[row][col];
        changed = true;
        ++col;  // 跳过合并产生的空位
      }
    }
    // 第三步：合并产生的空洞再次压缩
    write = 0;
    for (int col = 0; col < SIZE; ++col) {
      if (grid[row][col] != 0) {
        if (write != col) {
          std::swap(grid[row][write], grid[row][col]);
          changed = true;
        }
        ++write;
      }
    }
  }
  return gained;
}

void GameBoard::transposeGrid(int grid[SIZE][SIZE]) {
  for (int i = 0; i < SIZE; ++i) {
    for (int j = i + 1; j < SIZE; ++j) {
      std::swap(grid[i][j], grid[j][i]);
    }
  }
}

void GameBoard::reverseGridRows(int grid[SIZE][SIZE]) {
  for (int i = 0; i < SIZE; ++i) {
    for (int j = 0; j < SIZE / 2; ++j) {
      std::swap(grid[i][j], grid[i][SIZE - 1 - j]);
    }
  }
}

void GameBoard::addRandomTile() {
  struct Cell {
    int row;
    int col;
  };
  // 先收集空白格子，再随机选一个生成新数字
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
  if (count == 0) return;
  const int pos = QRandomGenerator::global()->bounded(count);
  m_slots[empty[pos].row][empty[pos].col] = randomTileValue();
}
