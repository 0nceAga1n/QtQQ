#pragma once

#include "basicwindow.h"
#include "ui_userlogin.h"
#include "msgprotocol.h"
#include <QTcpSocket>


//登录窗口类
class UserLogin : public BasicWindow
{
	Q_OBJECT

public:
	UserLogin(QWidget *parent = nullptr);
	~UserLogin();

private slots:
	void onLoginBtnClicked();	//登录按钮点击

private:
	void initControl();	//初始化控件
	void initLoginSocket(); //建立登录连接
	void handleAuthResult(const QJsonObject& obj);	//处理服务端发来的登录结果

protected:
	virtual void keyPressEvent(QKeyEvent* event) override;

private:
	Ui::UserLoginClass ui;
	QTcpSocket* m_loginSocket;	//登录用 TCP连接向服务端发请求
	MsgProtocol::Decoder m_decoder;	//分帧解码器
};

