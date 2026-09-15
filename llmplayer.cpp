#include "llmplayer.h"
#include "json.h"
#include <QRandomGenerator>
#include <cctype>
#include <optional>
#include <string>
namespace {
std::string lower(std::string s) {
  for (char& c : s)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::optional<GameBoard::Direction> parseDirection(const std::string& key) {
  const std::string k = lower(key);
  if (k == "up" || k == "上") return GameBoard::Direction::Up;
  if (k == "left" || k == "左") return GameBoard::Direction::Left;
  if (k == "down" || k == "下") return GameBoard::Direction::Down;
  if (k == "right" || k == "右") return GameBoard::Direction::Right;
  return std::nullopt;
}

GameBoard::Direction randomFallback() {
  switch (QRandomGenerator::global()->bounded(4)) {
    case 0:
      return GameBoard::Direction::Up;
    case 1:
      return GameBoard::Direction::Left;
    case 2:
      return GameBoard::Direction::Down;
    default:
      return GameBoard::Direction::Right;
  }
}
}  // namespace

LlmPlayer::LlmPlayer(QObject* parent) : QObject(parent) {}

void LlmPlayer::requestMove(const GameBoard& board) {
  if (m_requestInFlight) return;
  const std::string payload = buildBoardPayload(board);
  // 传输层未接入前只提示一次，避免反复刷屏
  if (!m_transportWarned) {
    m_transportWarned = true;
    emit warningShown(
        QStringLiteral("LLM 传输层尚未接入（待定 HTTP / "
                       "本地文件），当前用占位回复走完整校验链路"));
  }
  m_requestInFlight = true;
  sendRequest(payload);
}

std::string LlmPlayer::buildBoardPayload(const GameBoard& board) const {
  json::Value root;
  json::Value grid;  // 4x4 二维数组
  for (int i = 0; i < GameBoard::SIZE; ++i) {
    json::Value row;
    for (int j = 0; j < GameBoard::SIZE; ++j)
      row.append(json::Value(static_cast<double>(board.value(i, j))));
    grid.append(row);
  }
  root["grid"] = grid;
  root["score"] = json::Value(static_cast<double>(board.score()));
  return root.dump(false, "");
}

GameBoard::Direction LlmPlayer::parseAndValidate(const std::string& replyText) {
  // 剥离 ```json ... ``` 等裹挟文本，只保留最外层 JSON 大括号
  std::string text = replyText;
  const std::size_t firstBrace = text.find('{');
  const std::size_t lastBrace = text.rfind('}');
  if (firstBrace != std::string::npos && lastBrace != std::string::npos &&
      firstBrace < lastBrace) {
    text = text.substr(firstBrace, lastBrace - firstBrace + 1);
  }
  json::Value root;
  json::Reader reader;
  if (!reader.parse(text, root) || !root.isObject()) {
    emit warningShown(QStringLiteral("模型回复不是合法 JSON，已随机回退一步"));
    return randomFallback();
  }
  // 兼容 move / direction 两种字段名
  std::string key;
  const json::Value moveValue = root["move"];
  const json::Value directionValue = root["direction"];
  if (moveValue.isString())
    key = moveValue.asString();
  else if (directionValue.isString())
    key = directionValue.asString();
  const std::optional<GameBoard::Direction> parsed = parseDirection(key);
  if (!parsed) {
    emit warningShown(QStringLiteral(
        "模型回复中未找到合法按键（up/down/left/right），已随机回退一步"));
    return randomFallback();
  }
  return *parsed;
}

void LlmPlayer::handleReplyText(const std::string& replyText) {
  m_requestInFlight = false;
  emit moveReady(parseAndValidate(replyText));
}

void LlmPlayer::sendRequest(const std::string& payload) {
  // TODO: 接入待定的传输方式（OpenAI 兼容 HTTP 或本地 JSON 文件）。
  // 当前占位：把一条固定合法回复喂给校验链路，便于先行联调整条管线。
  Q_UNUSED(payload);
  handleReplyText("{\"move\": \"left\"}");
}
