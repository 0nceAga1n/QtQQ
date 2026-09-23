#pragma once

#include <QWidget>
#include "ui_talkwindow.h"
#include "talkwindowshell.h"

//自定义聊天窗口，分3个模块
/*---------------------------*/
/*			信息模块		 */
/*---------------------------*/
/*             |             */
/*  消息窗口   |  群员列表   */
/*  信息编辑   |             */
/*             |			 */
/*---------------------------*/

class TalkWindow : public QWidget
{
	Q_OBJECT

public:
	TalkWindow(QWidget *parent, const QString& uid);	//参二为窗口id（即数据库中的员工id和部门id）
	~TalkWindow();

public:
	void addEmotionImage(int emotionNum);	//添加表情图片到textedit
	void setWindowName(const QString& name);	//设置窗口名
	QString getTalkId();	//获取m_talkId

private slots:
	void onSendBtnClicked(bool);	//发送信息按钮点击时触发
	void onItemDoubleClicked(QTreeWidgetItem* item); //群员列表项双击
	void onFileOpenBtnClicked(bool);	//点击打开文件按钮

private:
	void initControl();
	void initGroupTalkStatus();	//判断窗口是单聊还是部门群聊
	int getComDepID();	//从数据库中获取公司群id

	void initPtoPTalk();		//初始化单聊群聊
	void initTalkWindow();		//初始化部门群聊

	void addPeopInfo(QTreeWidgetItem* pRootGroupItem, int employeeID);	//添加群员信息

private:
	Ui::TalkWindow ui;
	QString m_talkId;	//区分不同talkwindow
	bool m_isGroupTalk;	//区分单聊窗口还是部门窗口

	friend class TalkWindowShell;	//声明友元类
};

