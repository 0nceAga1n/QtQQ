#pragma once

#include <QWidget>
#include "ui_contactitem.h"

//自定义聊天项，作为群聊项
class ContactItem : public QWidget
{
	Q_OBJECT

public:
	ContactItem(QWidget *parent = nullptr);
	~ContactItem();

	void setUserName(const QString& userName);	//设置用户名
	void setSignName(const QString& signName);	//设置个性签名
	void setHeadPixmap(const QPixmap& headPath);	//设置头像
	QString getUserName() const;	//获取用户名
	QSize getHeadLabelSize() const;	//获取头像图标大小

private:
	void initControl();

private:
	Ui::ContactItemClass ui;
};

