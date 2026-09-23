#pragma once

#include <QWidget>
#include <QMap>
#include <QTcpSocket>
#include <QUdpSocket>

#include "ui_talkwindowshell.h"
#include "basicwindow.h"
#include "emotionwindow.h"
#include "talkwindowitem.h"

class TalkWindow;	//前向声明

//聊天组件，分左侧聊天列表talkWindowItem，右侧聊天窗口talkWindow
class TalkWindowShell : public BasicWindow
{
	Q_OBJECT

public:
	TalkWindowShell(QWidget *parent = nullptr);
	~TalkWindowShell();

public:
	void addTalkWindow(TalkWindow* talkWindow, TalkWindowItem* talkWindowItem, const QString uid);	//添加聊天窗口talkwindow
	void setCurrentWidget(QWidget* widget);	//设置当前显示的窗口

	const QMap<QListWidgetItem*, QWidget*>& getTalkWindowItemMap() const;	//获取映射

private:
	void initControl();
	void initTcpSocket();	//初始化tcp套接字
	void initUdpSocket();	//初始化udp套接字

	void getEmployeesID(QStringList& employeeIDList);	//获取所有员工id，传出参数
	bool createJSFile(QStringList& employeeList);	//更新js文件

	void handleReceivedMsg(int senderEmployeeID, int msgType, QString strMsg);	//添加接收到的消息到聊天窗口

public slots:
	void onEmotionBtnClicked(bool);	//点击表情按钮
	void updateSendTcpMsg(QString& strData, int& msgType, QString fileName = "");	//更新客户端要发送的消息

private slots:
	void onTalkWindowItemClicked(QListWidgetItem* item);	//左侧聊天列表项点击
	void onEmotionItemClicked(int emotionNum);	//表情窗口里点击表情

	void processPendingData();	//解析接收到的数据

private:
	Ui::TalkWindowClass ui;
	QMap<QListWidgetItem*, QWidget*> m_talkwindowItemMap;	//映射 左侧聊天列表项->右侧聊天窗口
	EmotionWindow* m_emotionWindow;	//表情窗口

private:
	QTcpSocket* m_tcpClientSocket;	//客户端tcp套接字，用于发数据到服务端
	QUdpSocket* m_udpReceiver;	//客户端udp套接字，用于从服务端收数据
};