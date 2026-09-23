#pragma once

#include <QTcpServer>

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

signals:
	void signalTcpMsgComes(QByteArray&);	//调用SocketDataProcessing()时触发信号，传给QtQQ_Server主窗口

private slots:
	void SocketDataProcessing(QByteArray& SendData, int descriptor);	//客户端发来数据时调用
	void SocketDisconnected(int descriptor);	//客户端断开连接时调用

private:
	int m_port;	//服务端监听的端口
	QList<QTcpSocket*> m_tcpSocketConnectList;	//保存所有客户端连接
};

