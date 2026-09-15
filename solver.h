#ifndef SOLVER_H
#define SOLVER_H
#include "gameboard.h"
#include <optional>
// 内置算法玩家：对四个方向分别做一次"无随机数的模拟"（previewMove），
// 用启发式函数评估后继局面，选出对当前棋盘最有利的一步。
class Solver {
 public:
  // 返回建议方向；四个方向都无法移动时返回空值
  std::optional<GameBoard::Direction> chooseMove(const GameBoard& board) const;
};
#endif  // SOLVER_H
