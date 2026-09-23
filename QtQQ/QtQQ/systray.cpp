#include "systray.h"
#include "custommenu.h"
#include "basicwindow.h"

SysTray::SysTray(QWidget *parent)
	: m_parent(parent), QSystemTrayIcon(parent)
{
	initSystemTray();
	show();
}

SysTray::~SysTray()
{}

void SysTray::initSystemTray()
{
	setToolTip(QStringLiteral("QQ"));
	setIcon(QIcon(":/Resources/MainWindow/app/logo.ico"));
	connect(this, &QSystemTrayIcon::activated, this, &SysTray::onIconActivated);
}

void SysTray::addSystrayMenu()
{
	CustomMenu* customMenu = new CustomMenu(m_parent);	//初始化自定义菜单
	//添加菜单项
	customMenu->addCustomMenu("onShow", ":/Resources/MainWindow/app/logo.ico", QStringLiteral("显示"));	
	customMenu->addCustomMenu("onQuit", ":/Resources/MainWindow/app/page_close_btn_hover.png", QStringLiteral("退出"));
	//菜单项信号连接
	connect(customMenu->getAction("onShow"), SIGNAL(triggered(bool)), m_parent, SLOT(onShowNormal(bool)));	
	//connect(customMenu->getAction("onQuit"), SIGNAL(triggered(bool)), m_parent, SLOT(onShowQuit(bool)));
	connect(customMenu->getAction("onQuit"), &QAction::triggered, static_cast<BasicWindow*>(m_parent), &BasicWindow::onShowQuit);

	customMenu->exec(QCursor::pos());	//模态显示
	delete customMenu;	//及时清除
	customMenu = nullptr;
}

void SysTray::onIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger) {	//左键单击
		m_parent->show();
		m_parent->activateWindow();
	}
	else if (reason == QSystemTrayIcon::Context) {	//右键点击
		addSystrayMenu();
	}
}

