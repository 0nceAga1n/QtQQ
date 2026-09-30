#include "windowmanager.h"
#include "talkwindow.h"
#include "talkwindowitem.h"

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
	if (!widget) {	//没有找到uid对应的talkwindow时，发请求拉取会话信息（窗口在响应回调里创建）
		m_talkwindowshell->sendFetchTalkInfo(uid);
	}
	else {	//找到uid对应的talkwindow时，设为当前窗口，talkwidowitem设为选中
		m_talkwindowshell->setCurrentWidget(widget);
		QListWidgetItem* item = m_talkwindowshell->getTalkWindowItemMap().key(widget);
		item->setSelected(true);

		//显示唯一的talkwindowshell
		m_talkwindowshell->show();
		m_talkwindowshell->activateWindow();
	}
}

void WindowManager::createTalkWindow(const QJsonObject& talkInfo)
{
	QString uid = talkInfo.value("uid").toString();
	QString name = talkInfo.value("name").toString();
	QString sign = talkInfo.value("sign").toString();
	QString picture = talkInfo.value("picture").toString();

	m_creatingTalkInfo = talkInfo;
	TalkWindow* talkwindow = new TalkWindow(m_talkwindowshell, uid, talkInfo);
	TalkWindowItem* talkwindowItem = new TalkWindowItem(talkwindow);

	talkwindow->setWindowName(sign);
	talkwindowItem->setMsgLabelContent(name);
	m_talkwindowshell->addTalkWindow(talkwindow, talkwindowItem, uid, picture);

	//显示唯一的talkwindowshell
	m_talkwindowshell->show();
	m_talkwindowshell->activateWindow();
}


QJsonObject WindowManager::getCreatingTalkInfo()
{
	return m_creatingTalkInfo;
}

TalkWindowShell* WindowManager::getTalkWindowShell()
{
	return m_talkwindowshell;
}
