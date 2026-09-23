#include "windowmanager.h"
#include "talkwindow.h"
#include "talkwindowitem.h"
#include <QSqlQuery>
#include <QSqlQueryModel>

Q_GLOBAL_STATIC(WindowManager, theInstance)	//唯一的WindowManager

WindowManager::WindowManager()
	:QObject(nullptr), m_talkwindowshell(nullptr)
{}

WindowManager::~WindowManager()
{}

QWidget* WindowManager::findWindowName(const QString & qsWindowName)
{
	if (m_windowMap.contains(qsWindowName)) {
		return m_windowMap.value(qsWindowName);
	}
	return nullptr;
}

void WindowManager::deleteWindowName(const QString& qsWindowName)
{
	m_windowMap.remove(qsWindowName);
}

void WindowManager::addWindowName(const QString& qsWindowName, QWidget* qWidget)
{
	if (!m_windowMap.contains(qsWindowName)) {
		m_windowMap.insert(qsWindowName, qWidget);
	}
}

WindowManager* WindowManager::getInstance()
{
	return theInstance();
}

void WindowManager::addNewTalkWindow(const QString& uid)
{
	if (m_talkwindowshell == nullptr) {	//第一次调用时，talkwindowshell不存在
		m_talkwindowshell = new TalkWindowShell;
		connect(m_talkwindowshell, &TalkWindowShell::destroyed, [this](QObject* obj) {
			m_talkwindowshell = nullptr;
		});
	}

	QWidget* widget = findWindowName(uid);	//利用uid查找talkwindow
	if (!widget) {	//没有找到uid对应的talkwindow时，创建talkwindow
		m_strCreatingTalkID = uid;	//只在构建窗口时使用
		
		TalkWindow* talkwindow = new TalkWindow(m_talkwindowshell, uid);
		TalkWindowItem* talkwindowItem = new TalkWindowItem(talkwindow);

		m_strCreatingTalkID = "";

		//从部门表中获取部门名称和部门标语
		QSqlQueryModel sqlDepModel;
		QString strSql = QString("SELECT department_name, sign FROM tab_department WHERE departmentID = %1").arg(uid);
		sqlDepModel.setQuery(strSql);
		int rows = sqlDepModel.rowCount();	//用于判断要添加的窗口是单聊窗口还是部门窗口

		if (rows == 0) {	//要添加的窗口是单聊窗口
			QString sql = QString("SELECT employee_name, employee_sign FROM tab_employees WHERE employeeID = %1").arg(uid);
			sqlDepModel.setQuery(sql);
		}

		//设置聊天窗口名，聊天列表项内容
		QString strWindowName, strMsgLabel;
		QModelIndex nameIndex, signIndex;
		nameIndex = sqlDepModel.index(0, 0);
		signIndex = sqlDepModel.index(0, 1);
		strWindowName = sqlDepModel.data(signIndex).toString();
		strMsgLabel = sqlDepModel.data(nameIndex).toString();

		talkwindow->setWindowName(strWindowName);
		talkwindowItem->setMsgLabelContent(strMsgLabel);

		m_talkwindowshell->addTalkWindow(talkwindow, talkwindowItem, uid);
	}
	else {	//找到uid对应的talkwindow时，设为当前窗口，talkwidowitem设为选中
		m_talkwindowshell->setCurrentWidget(widget);
		QListWidgetItem* item = m_talkwindowshell->getTalkWindowItemMap().key(widget);
		item->setSelected(true);
	}

	//显示唯一的talkwindowshell
	m_talkwindowshell->show();
	m_talkwindowshell->activateWindow();
}

QString WindowManager::getCreatingTalkID()
{
	return m_strCreatingTalkID;
}

TalkWindowShell* WindowManager::getTalkWindowShell()
{
	return m_talkwindowshell;
}
