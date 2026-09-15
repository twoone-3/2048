#include "solver.h"
#include <cmath>
#include <cstring>
namespace {
// 空白格数量：越多越从容，期望值越高
int countEmpty(const int grid[GameBoard::SIZE][GameBoard::SIZE]) {
  int n = 0;
  for (int i = 0; i < GameBoard::SIZE; ++i)
    for (int j = 0; j < GameBoard::SIZE; ++j)
      if (grid[i][j] == 0) ++n;
  return n;
}

double tileLog(int v) {
  return v <= 0 ? 0.0 : std::log2(static_cast<double>(v));
}
// 行列方向相邻元素对数值之差的绝对值之和，越小代表数字排列越顺滑
// （越接近"大小相邻连成片"，越方便继续合并）
double smoothness(const int grid[GameBoard::SIZE][GameBoard::SIZE]) {
  double total = 0.0;
  for (int i = 0; i < GameBoard::SIZE; ++i) {
    for (int j = 0; j < GameBoard::SIZE - 1; ++j) {
      total += std::fabs(tileLog(grid[i][j]) - tileLog(grid[i][j + 1]));
      total += std::fabs(tileLog(grid[j][i]) - tileLog(grid[j + 1][i]));
    }
  }
  return total;
}

double maxTile(const int grid[GameBoard::SIZE][GameBoard::SIZE]) {
  int mx = 0;
  for (int i = 0; i < GameBoard::SIZE; ++i)
    for (int j = 0; j < GameBoard::SIZE; ++j)
      if (grid[i][j] > mx) mx = grid[i][j];
  return tileLog(mx);
}
}  // namespace
std::optional<GameBoard::Direction> Solver::chooseMove(
    const GameBoard& board) const {
  int current[GameBoard::SIZE][GameBoard::SIZE];
  for (int i = 0; i < GameBoard::SIZE; ++i)
    for (int j = 0; j < GameBoard::SIZE; ++j) current[i][j] = board.value(i, j);
  std::optional<GameBoard::Direction> best;
  double bestScore = -1e18;
  // 固定顺序保证结果稳定：优先向上、向左（便于把大块往左上角推）
  const GameBoard::Direction order[] = {
      GameBoard::Direction::Up,
      GameBoard::Direction::Left,
      GameBoard::Direction::Down,
      GameBoard::Direction::Right,
  };

  for (GameBoard::Direction direction : order) {
    int out[GameBoard::SIZE][GameBoard::SIZE];
    const int gained = board.previewMove(direction, out);
    if (std::memcmp(current, out, sizeof(current)) == 0)
      continue;  // 该方向没有任何格子移动 / 合并
    const double score = 30.0 * countEmpty(out) + 16.0 * tileLog(gained + 1) +
                         8.0 * maxTile(out) - 10.0 * smoothness(out);
    if (score > bestScore) {
      bestScore = score;
      best = direction;
    }
  }
  return best;
}
