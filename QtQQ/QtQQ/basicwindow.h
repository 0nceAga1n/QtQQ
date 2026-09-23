#pragma once

#include <QDialog>
#include "titlebar.h"

//窗口基类，派生登录窗口，聊天窗口等
class BasicWindow  : public QDialog
{
	Q_OBJECT

public:
	BasicWindow(QWidget *parent = nullptr);
	virtual ~BasicWindow();

public:
	void loadStyleSheet(const QString& sheetName);	//加载样式表
	QPixmap getRoundImage(const QPixmap& src, QPixmap& mask, QSize masksize = QSize(0, 0));	//获取圆头像

private:
	void initBackGroundColor();	//初始化背景

protected:
	void paintEvent(QPaintEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);

protected:
	void initTitleBar(ButtonType buttontype = MIN_BUTTON);	//初始化标题栏
	void setTitleBarTitle(const QString& title, const QString& icon = "");	//设置标题栏内容和图标

public slots:
	void onShowClose(bool);
	void onShowMin(bool);
	void onShowHide(bool);
	void onShowNormal(bool);
	void onShowQuit(bool);
	void onSignalSkinChanged(const QColor& color);

	void onButtonMinClicked();
	void onButtonMaxClicked();
	void onButtonCloseClicked();
	void onButtonRestoreClicked();

protected:
	QPoint m_mousePoint;	//鼠标位置
	bool m_mousePressed;	//鼠标是否按下
	QColor m_colorBackGround;	//背景色
	QString m_styleName;	//样式文件名
	TitleBar* _titleBar;	//窗口标题栏
};

