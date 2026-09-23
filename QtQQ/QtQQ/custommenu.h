#pragma once

#include <QMenu>
#include <QMap>

//自定义菜单，用于系统托盘
class CustomMenu  : public QMenu
{
	Q_OBJECT

public:
	CustomMenu(QWidget *parent = nullptr);
	~CustomMenu();

public:
	void addCustomMenu(const QString& text, const QString& icon, const QString& name);	//添加菜单项，参一为菜单项标记，参二为图标，参三为文本
	QAction* getAction(const QString& text);	//获取菜单项，利用菜单项标记从映射中获取

private:
	QMap<QString, QAction*> m_menuActionMap;	//映射 菜单项标记->菜单项
};

