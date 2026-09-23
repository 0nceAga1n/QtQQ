#pragma once

#include "basicwindow.h"
#include "ui_userlogin.h"

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
	bool connectMysql();	//连接数据库
	bool veryfyAccountCode(bool& isAccountLogin, QString& strAccount);	//判断账号密码是否正确，同时传出参数：参一：账号登录还是员工登录；参二：用户输入的账号

private:
	Ui::UserLoginClass ui;
};

