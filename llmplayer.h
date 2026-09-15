#ifndef LLMPLAYER_H
#define LLMPLAYER_H
#include "gameboard.h"
#include <QObject>
#include <QString>
#include <string>
// 大模型玩家：负责 - LLM -> JSON -> 校验 -> GameBoard - 这条链路。
// 协议层（棋盘序列化 + 回复解析校验）已实现，
// 传输层（HTTP / 本地文件）通过 sendRequest() 留作接口，接入方式后续再定。
class LlmPlayer : public QObject {
  Q_OBJECT
 public:
  explicit LlmPlayer(QObject* parent = nullptr);
  // 请求一步：序列化棋盘并触发一次发送；结果异步经 moveReady 返回
  void requestMove(const GameBoard& board);
 signals:
  // 校验通过（或校验失败后已随机回退）的方向
  void moveReady(GameBoard::Direction direction);
  // 需要提示用户的信息：传输层未接入、非法 JSON、非法按键等
  void warningShown(const QString& message);

 private:
  // —— 协议层（与传输方式无关）——
  // 用 json 库把棋盘序列化为 JSON 文本
  std::string buildBoardPayload(const GameBoard& board) const;
  // 用 json 库解析模型回复并校验按键；非法时随机回退
  GameBoard::Direction parseAndValidate(const std::string& replyText);
  // 收到模型回复后的统一入口：任何传输层拿到文本后都应调用它
  void handleReplyText(const std::string& replyText);
  // —— 传输层（待接入 HTTP / 本地文件）——
  void sendRequest(const std::string& payload);
  bool m_requestInFlight = false;
  bool m_transportWarned = false;
};
#endif  // LLMPLAYER_H
