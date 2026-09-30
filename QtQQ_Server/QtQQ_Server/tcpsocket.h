#pragma once

#include <QTcpSocket>
#include "msgprotocol.h"

//自定义客户端连接套接字
class TcpSocket  : public QTcpSocket
{
	Q_OBJECT

public:
	TcpSocket();
	~TcpSocket();

	void run();	//初始化

private slots:
	void onReceiveData();	//客户端发来消息时调用
	void onClientDisconnect();	//客户端断开连接时调用

signals:
	void signalMsgFrame(const QJsonObject& obj, int descriptor);	//调用onReceiveData()时触发信号，将数据传递给服务端
	void signalClientDisconnect(int);	//调用onClientDisconnect()时触发信号，传递给服务端

private:
	int m_descriptor;	//区分不同客户端
	MsgProtocol::Decoder m_decoder;	//分帧解码器
};

