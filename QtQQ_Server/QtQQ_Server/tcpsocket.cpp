#include "tcpsocket.h"

TcpSocket::TcpSocket()
{}

TcpSocket::~TcpSocket()
{}

void TcpSocket::run()
{
	m_descriptor = this->socketDescriptor();	//保存描述符
	
	connect(this, SIGNAL(readyRead()), this, SLOT(onReceiveData()));
	connect(this, SIGNAL(disconnected()), this, SLOT(onClientDisconnect()));
}

void TcpSocket::onClientDisconnect()
{
	emit signalClientDisconnect(m_descriptor);	//触发信号告诉服务端TcpServer当前客户端断开连接
}

void TcpSocket::onReceiveData()
{
	QByteArray buffer = this->readAll();	//读取客户端发来的消息
	const QList<QJsonObject>& frames = m_decoder.push(buffer);	//分帧
	for (const QJsonObject& obj : frames) {
		emit signalMsgFrame(obj, m_descriptor);	// 每帧单独处理
	}
}