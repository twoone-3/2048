#include "llmplayer.h"
#include "json.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QDateTime>
#include <QUrl>
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
// 剥离 Markdown 代码块等裹挟文本，只保留最外层 JSON 大括号
std::string stripJsonShell(const std::string& text) {
  const std::size_t firstBrace = text.find('{');
  const std::size_t lastBrace = text.rfind('}');
  if (firstBrace != std::string::npos && lastBrace != std::string::npos &&
      firstBrace < lastBrace) {
    return text.substr(firstBrace, lastBrace - firstBrace + 1);
  }
  return text;
}
}  // namespace

LlmPlayer::LlmPlayer(QObject* parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this)) {
  // 默认接入智谱 GLM（OpenAI 兼容）；也可在界面里修改
  m_url = qEnvironmentVariable(
      "LLM_API_URL", "https://open.bigmodel.cn/api/paas/v4/chat/completions");
  m_apiKey = qEnvironmentVariable("LLM_API_KEY");
  m_model = qEnvironmentVariable("LLM_MODEL", "glm-4.7-flash");
  connect(
      m_network, &QNetworkAccessManager::finished, this,
      [this](QNetworkReply* reply) {
        m_requestInFlight = false;
        const int statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray responseBody = reply->readAll();
        emit logMessage(QStringLiteral("[%1] HTTP %2\n响应：%3")
                            .arg(QDateTime::currentDateTime().toString(
                                QStringLiteral("HH:mm:ss")))
                            .arg(statusCode > 0 ? QString::number(statusCode)
                                                : QStringLiteral("无"))
                            .arg(QString::fromUtf8(responseBody)));
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
          emit warningShown(QStringLiteral("LLM 请求失败（HTTP %1）：%2")
                                .arg(statusCode > 0
                                         ? QString::number(statusCode)
                                         : QStringLiteral("无"))
                                .arg(reply->errorString()));
          return;
        }
        const std::string content = extractContent(responseBody.toStdString());
        if (content.empty()) {
          emit warningShown(
              QStringLiteral("LLM 响应中没有可用内容，跳过本回合"));
          return;
        }
        handleReplyText(content);
      });
}

void LlmPlayer::setConfig(const QString& url, const QString& apiKey,
                          const QString& model) {
  m_url = url.trimmed();
  m_apiKey = apiKey.trimmed();
  m_model = model.trimmed();
}

void LlmPlayer::requestMove(const GameBoard& board) {
  if (m_requestInFlight) return;
  if (m_apiKey.isEmpty()) {
    emit warningShown(QStringLiteral(
        "未配置 LLM 接口（请点击“LLM设置”填写 URL / Key / Model）"));
    return;
  }
  emit logMessage(QStringLiteral("[%1] 请求模型 %2：%3")
                      .arg(QDateTime::currentDateTime().toString(
                          QStringLiteral("HH:mm:ss")))
                      .arg(m_model, m_url));
  m_requestInFlight = true;
  sendRequest(buildBoardPayload(board));
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
  return root.dump(true, "");
}

std::string LlmPlayer::buildRequestBody(const std::string& boardPayload) const {
  json::Value root;
  root["model"] = json::Value(m_model.toStdString());
  root["temperature"] = json::Value(0.0);
  json::Value messages;
  json::Value system;
  system["role"] = json::Value("system");
  system["content"] = json::Value(
      "你正在玩 2048 游戏。我会给你当前棋盘 JSON，请只输出一步移动，格式为严格 "
      "JSON："
      "{\"move\": \"left\"}。move 只能是 up / down / left / right "
      "之一，不要输出任何其他文字。");
  messages.append(system);
  json::Value user;
  user["role"] = json::Value("user");
  user["content"] = json::Value(boardPayload);
  messages.append(user);
  root["messages"] = messages;
  return root.dump(true, "");
}

std::string LlmPlayer::extractContent(const std::string& replyText) const {
  json::Value root;
  json::Reader reader;
  if (!reader.parse(stripJsonShell(replyText), root) || !root.isObject())
    return {};
  json::Value choices = root["choices"];
  if (!choices.isArray() || choices.size() == 0) return {};
  json::Value message = choices[0]["message"];
  if (!message.isObject()) return {};
  json::Value content = message["content"];
  if (!content.isString()) return {};
  return content.asString();
}

GameBoard::Direction LlmPlayer::parseAndValidate(const std::string& replyText) {
  json::Value root;
  json::Reader reader;
  if (!reader.parse(stripJsonShell(replyText), root) || !root.isObject()) {
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
  QNetworkRequest request{QUrl(m_url)};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
  request.setTransferTimeout(30000);
  QNetworkReply* reply = m_network->post(
      request, QByteArray::fromStdString(buildRequestBody(payload)));
}
