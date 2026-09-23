#pragma once

#include <QWidget>
#include "ui_talkwindowitem.h"

//自定义聊天列表项
class TalkWindowItem : public QWidget
{
	Q_OBJECT

public:
	TalkWindowItem(QWidget *parent = nullptr);
	~TalkWindowItem();

	void setHeadPixmap(const QPixmap& pixmap);	//设置头像
	void setMsgLabelContent(const QString& msg);	//设置聊天对象信息
	QString getMsgLabelText();	//获取聊天对象信息

private:
	void initControl();

signals:
	void signalCloseClicked();	//提供关闭按钮点击信号给其他类使用

private:
	void enterEvent(QEnterEvent* event);
	void leaveEvent(QEvent* event);
	void resizeEvent(QResizeEvent* event);

private:
	Ui::TalkWindowItemClass ui;
};

