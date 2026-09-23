#include "tcpserver.h"
#include <QTcpSocket>
#include "tcpsocket.h"

TcpServer::TcpServer(int port)
	: m_port(port)
{}

TcpServer::~TcpServer()
{}

bool TcpServer::run()
{
	if (this->listen(QHostAddress::AnyIPv4, m_port)) {
		qDebug() << QString::fromUtf8("服务端监听端口 %1 成功").arg(m_port);
		return true;
	}
	else {
		qDebug() << QString::fromUtf8("服务端监听端口 %1 失败").arg(m_port);
		return false;
	}
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
	qDebug() << QString::fromUtf8("新的连接：") << socketDescriptor;

	//创建连接
	TcpSocket* tcpsocket = new TcpSocket();
	tcpsocket->setSocketDescriptor(socketDescriptor);	//设置描述符用于区分不同客户端连接
	tcpsocket->run();

	connect(tcpsocket, SIGNAL(signalGetDataFromClient(QByteArray&, int)), this, SLOT(SocketDataProcessing(QByteArray&, int)));
	connect(tcpsocket, SIGNAL(signalClientDisconnect(int)), this, SLOT(SocketDisconnected(int)));

	m_tcpSocketConnectList.append(tcpsocket);	//连接保存到服务端
}

void TcpServer::SocketDisconnected(int descriptor)
{
	//遍历所有客户端连接，根据描述符找到要断开的客户端
	for (int i = 0; i < m_tcpSocketConnectList.count(); i++) {
		QTcpSocket* item = m_tcpSocketConnectList.at(i);
		int itemDescriptor = item->socketDescriptor();
		if (itemDescriptor == descriptor || itemDescriptor == -1) {
			m_tcpSocketConnectList.removeAt(i);	//清理链表
			item->deleteLater();	//安全销毁
			qDebug() << "TcpSocket断开连接：" << descriptor;
			return;
		}
	}
}

void TcpServer::SocketDataProcessing(QByteArray& SendData, int descriptor)
{
	//遍历所有客户端连接，根据描述符找到发送数据的客户端
	for (int i = 0; i < m_tcpSocketConnectList.count(); i++) {
		QTcpSocket* item = m_tcpSocketConnectList.at(i);
		if (item->socketDescriptor() == descriptor) {
			qDebug() << "来自IP：" << item->peerAddress().toString()
				<< "发来的数据：" << QString(SendData);
			emit signalTcpMsgComes(SendData);	//将数据传递到QtQQ_Server主窗口
		}
	}
}