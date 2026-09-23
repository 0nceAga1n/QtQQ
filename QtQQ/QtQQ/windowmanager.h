#pragma once

#include <QObject>
#include "talkwindowshell.h"

//窗口管理类，单例模式，保证只有一个talkwindowshell
class WindowManager  : public QObject
{
	Q_OBJECT

public:
	WindowManager();
	~WindowManager();

public:
	QWidget* findWindowName(const QString& qsWindowName);	//从映射中寻找窗口talkwindow
	void deleteWindowName(const QString& qsWindowName);	//从映射中删除窗口talkwindow
	void addWindowName(const QString& qsWindowName, QWidget* qWidget);	//在映射中添加窗口talkwindow

	static WindowManager* getInstance();	//获取唯一实例WindowManager
	void addNewTalkWindow(const QString& uid);	//添加talkwindow

	QString getCreatingTalkID();	//获取创建的窗口id

	TalkWindowShell* getTalkWindowShell();	//获取唯一talkwindowshell
private:
	TalkWindowShell* m_talkwindowshell;	//唯一talkwindowshell
	QMap<QString, QWidget*> m_windowMap;	//映射 talkwindow名->talkwindow，用来保存当前talkwindowshell有哪些talkwindow
	QString m_strCreatingTalkID = "";	//创建的窗口id
};

