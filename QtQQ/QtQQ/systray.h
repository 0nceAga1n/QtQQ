#pragma once

#include <QSystemTrayIcon>
#include <QWidget>

//系统托盘图标
class SysTray  : public QSystemTrayIcon
{
	Q_OBJECT

public:
	SysTray(QWidget *parent);
	~SysTray();

public slots:
	void onIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
	void initSystemTray();	//初始化系统托盘
	void addSystrayMenu();	//添加菜单

private:
	QWidget* m_parent;	//系统托盘父类，CCMainWindow
};

