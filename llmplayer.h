#ifndef LLMPLAYER_H
#define LLMPLAYER_H
#include "gameboard.h"
#include <QObject>
#include <QString>
#include <string>
class QNetworkAccessManager;
class QNetworkReply;
// 大模型玩家：负责 - LLM -> JSON -> 校验 -> GameBoard - 这条链路。
// 通过 OpenAI 兼容的 /chat/completions 接口调用模型（DeepSeek / GLM
// 等均兼容）， 棋盘用 json 库序列化，回复用 json
// 库解析并校验，非法输入随机回退。
class LlmPlayer : public QObject {
  Q_OBJECT
 public:
  explicit LlmPlayer(QObject* parent = nullptr);
  // 配置 OpenAI 兼容接口（URL / API Key / 模型名）
  void setConfig(const QString& url, const QString& apiKey,
                 const QString& model);
  // 请求一步：序列化棋盘并发送；结果异步经 moveReady 返回
  void requestMove(const GameBoard& board);
 signals:
  // 校验通过（或校验失败后已随机回退）的方向
  void moveReady(GameBoard::Direction direction);
  // 需要提示用户的信息：接口未配置、请求失败、非法 JSON、非法按键等
  void warningShown(const QString& message);

 private:
  // —— 协议层 ——
  // 用 json 库把棋盘序列化为 JSON 文本
  std::string buildBoardPayload(const GameBoard& board) const;
  // 用 json 库构造 OpenAI chat/completions 请求体
  std::string buildRequestBody(const std::string& boardPayload) const;
  // 用 json 库从响应中提取 choices[0].message.content
  std::string extractContent(const std::string& replyText) const;
  // 用 json 库解析模型回复并校验按键；非法时随机回退
  GameBoard::Direction parseAndValidate(const std::string& replyText);
  // 收到模型回复后的统一入口
  void handleReplyText(const std::string& replyText);
  // —— 传输层（OpenAI 兼容 HTTP）——
  void sendRequest(const std::string& payload);
  QNetworkAccessManager* m_network = nullptr;
  QString m_url;
  QString m_apiKey;
  QString m_model;
  bool m_requestInFlight = false;
};
#endif  // LLMPLAYER_H
