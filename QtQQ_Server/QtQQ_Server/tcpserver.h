#pragma once

#include <QTcpServer>
#include <QMap>
#include <QJsonObject>

//tcp服务端，管理所有客户端连接
class TcpServer  : public QTcpServer
{
	Q_OBJECT

public:
	TcpServer(int port);
	~TcpServer();

public:
	bool run();	//启动监听

protected:
	void incomingConnection(qintptr socketDescriptor);	//有客户端连接时，创建连接

private slots:
	void SocketDataProcessing(const QJsonObject& obj, int descriptor);	//客户端发来数据时调用，接收完整JSON帧
	void SocketDisconnected(int descriptor);	//客户端断开连接时调用

private:
	void handleLogin(const QJsonObject& obj, int descriptor); // 打开聊天窗口时绑定身份
	void handleChatMsg(const QJsonObject& obj, int descriptor); // 定向转发聊天消息
	void handleFetchHistory(const QJsonObject& obj, int descriptor); // 拉取历史消息
	void saveMessage(const QJsonObject& obj);	//消息落库
	void handleAuth(const QJsonObject& obj, int descriptor);	// 登录校验
	bool verifyAccount(const QString& account, const QString& code, QString& employeeID);
	void handleFetchTalkInfo(const QJsonObject& obj, int descriptor); // 拉取会话信息
	QStringList getGroupMembers(const QString& groupID); // 获取群成员ID列表

private:
	int m_port;	//服务端监听的端口
	QMap<int, QTcpSocket*> m_socketMap;		// 描述符 -> 套接字
	QMap<QString, int> m_idToDescriptor;	// 员工ID -> 描述符（用于单聊路由）
	QMap<int, QString> m_descriptorToId;	// 描述符 -> 员工ID（用于断线清理）

	QMap<QString, QString> m_tokenToId;	// token -> 员工ID
};

