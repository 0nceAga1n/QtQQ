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
	emit signalClientDisconnect(m_descriptor);	//触发信号告诉服务端TcpServer当前客户端断开链接额
}

void TcpSocket::onReceiveData()
{
	QByteArray buffer = this->readAll();	//读取客户端发来的消息
	if (!buffer.isEmpty()) {
		QString strData = QString::fromUtf8(buffer);

		emit signalGetDataFromClient(buffer, m_descriptor);	//触发信号将数据传递到服务端TcpServer
	}
}