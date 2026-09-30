#pragma once

#include <QWidget>
#include <QMap>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>

#include "ui_talkwindowshell.h"
#include "basicwindow.h"
#include "emotionwindow.h"
#include "talkwindowitem.h"
#include "msgprotocol.h"
#include "msgwebview.h"

class TalkWindow;	//前向声明

//聊天组件，分左侧聊天列表talkWindowItem，右侧聊天窗口talkWindow
class TalkWindowShell : public BasicWindow
{
	Q_OBJECT

public:
	TalkWindowShell(QWidget *parent = nullptr);
	~TalkWindowShell();

public:
	void sendFetchTalkInfo(const QString& uid);	//向服务端获取当前窗口信息
	void addTalkWindow(TalkWindow* talkWindow, TalkWindowItem* talkWindowItem, const QString uid, const QString& picture);	//添加聊天窗口talkwindow
	void setCurrentWidget(QWidget* widget);	//设置当前显示的窗口

	const QMap<QListWidgetItem*, QWidget*>& getTalkWindowItemMap() const;	//获取映射

private:
	void initControl();
	void initTcpSocket();	//初始化tcp套接字

	void handleReceivedMsg(int senderEmployeeID, const QJsonArray& segments);	//添加接收到的消息到聊天窗口
	void processMsgFrame(const QJsonObject& obj);	//处理单帧（服务器发来的消息）

	void sendFetchHistory(const QString& uid);	// 发送拉取历史请求

	QString buildSegmentsHtml(const QJsonArray& segments, MsgWebView* view);// 把消息段落转成 HTML
	void handleHistoryMsg(const QJsonObject& obj);	// 渲染历史消息

public slots:
	void onEmotionBtnClicked(bool);	//点击表情按钮
	void updateSendTcpMsg(const QJsonArray& segments);	//更新客户端要发送的消息

private slots:
	void onTalkWindowItemClicked(QListWidgetItem* item);	//左侧聊天列表项点击
	void onEmotionItemClicked(int emotionNum);	//表情窗口里点击表情

	void onTcpReadyRead();	//解析接收到的数据

private:
	Ui::TalkWindowClass ui;
	QMap<QListWidgetItem*, QWidget*> m_talkwindowItemMap;	//映射 左侧聊天列表项->右侧聊天窗口
	EmotionWindow* m_emotionWindow;	//表情窗口

private:
	QTcpSocket* m_tcpClientSocket;	//客户端tcp套接字，用于发数据到服务端
	MsgProtocol::Decoder m_decoder;	//解码器
	QStringList m_pendingHistory;	//连接建立前缓存的待拉取窗口
	QStringList m_pendingTalkInfo;	// 连接建立前缓存的待拉取会话
};