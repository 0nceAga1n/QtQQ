#include "custommenu.h"
#include "commonutils.h"

CustomMenu::CustomMenu(QWidget *parent)
	: QMenu(parent)
{
	setAttribute(Qt::WA_TranslucentBackground);
	CommonUtils::loadStyleSheet(this, "Menu");	//加载样式
}

CustomMenu::~CustomMenu()
{}

void CustomMenu::addCustomMenu(const QString& text, const QString& icon, const QString& name)
{
	QAction* pAction = addAction(QIcon(icon), name);	//添加菜单项
	m_menuActionMap.insert(text, pAction);	//添加映射
}

QAction* CustomMenu::getAction(const QString& text)
{
	return m_menuActionMap[text];	//利用映射获取菜单项
}

