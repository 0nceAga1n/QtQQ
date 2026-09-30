#include "CCMainWindow.h"
#include "skinwindow.h"
#include "systray.h"
#include "notifymanager.h"
#include "rootcontactitem.h"
#include "contactitem.h"
#include "talkwindowitem.h"
#include "talkwindowshell.h"
#include "windowmanager.h"

#include <QProxyStyle>
#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QJsonObject>

QString gstrLoginHeadPath;	//定义全局变量获取登录者头像路径

class CustomProxyStyle : public QProxyStyle
{		//自定义代理样式类，重写drawPrimitive虚函数
public:
	virtual void drawPrimitive(PrimitiveElement element, const QStyleOption* opt, QPainter* painter, const QWidget* widget = 0) const {
		if (PE_FrameFocusRect == element) {
			return;	//画到焦点框时，直接返回，什么都不画
		}                             
		else {
			QProxyStyle::drawPrimitive(element, opt, painter, widget);	//其他元素正常画
		}
	}
};

CCMainWindow::CCMainWindow(QString loginPicture, QJsonArray departments, QWidget* parent)
	: BasicWindow(parent), m_loginPicture(loginPicture), m_departments(departments)
{
    ui.setupUi(this);

    setWindowFlags(windowFlags() | Qt::Tool);
	activateWindow();
    loadStyleSheet("CCMainWindow");
    initControl();
	initTimer();
}

CCMainWindow::~CCMainWindow()
{
}

void CCMainWindow::initTimer()
{
	QTimer* timer = new QTimer(this);
	timer->setInterval(500);
	connect(timer, &QTimer::timeout, [this]() {
		static int level = 0;
		if (level == 99)
			level = 0;
		level++;
		this->setLevelPixmap(level);
	});
	timer->start();
}

void CCMainWindow::initControl()
{
	ui.treeWidget->setStyle(new CustomProxyStyle);	//设置树控件没有边框
	setLevelPixmap(0);	//设置等级
	setHeadPixmap(getHeadPicturePath());	//设置头像
	setStatusMenuIcon(":/Resources/MainWindow/StatusSucceeded.png");	//设置在线状态

	//初始化app图标
	QHBoxLayout* appupLayout = new QHBoxLayout;
	appupLayout->setContentsMargins(0, 0, 0, 0);
	appupLayout->setSpacing(2);
	appupLayout->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_7.png", "app_7"));
	appupLayout->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_2.png", "app_2"));
	appupLayout->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_3.png", "app_3"));
	appupLayout->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_4.png", "app_4"));
	appupLayout->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_5.png", "app_5"));
	appupLayout->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_6.png", "app_6"));
	appupLayout->addStretch();
	appupLayout->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/skin.png", "app_skin"));
	ui.appWidget->setLayout(appupLayout);

	ui.bottomLayout_up->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_8.png", "app_8"));
	ui.bottomLayout_up->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_9.png", "app_9"));
	ui.bottomLayout_up->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_10.png", "app_10"));
	ui.bottomLayout_up->addWidget(addOtherAppExtension(":/Resources/MainWindow/app/app_11.png", "app_11"));
	ui.bottomLayout_up->addStretch();

	initContactTree();	//初始化聊天树

	//个性签名，搜索栏
	ui.lineEdit->installEventFilter(this);
	ui.searchLineEdit->installEventFilter(this);

	//最小化按钮，关闭按钮
	connect(ui.sysmin, &QPushButton::clicked, this, &BasicWindow::onShowHide);
	connect(ui.sysclose, &QPushButton::clicked, this, &BasicWindow::onShowClose);

	//窗口皮肤样式改变时，搜索栏的样式更新
	connect(NotifyManager::getInstance(), &NotifyManager::signalSkinChanged, [this]() {
		updateSearchStyle();
	});

	//初始化自定义系统托盘
	SysTray* systray = new SysTray(this);
}

void CCMainWindow::updateSearchStyle()
{
	ui.searchWidget->setStyleSheet(QString("QWidget#searchWidget{background-color:rgba(%1,%2,%3,50);border-bottom:1px solid rgba(%1,%2,%3,30)}\
								QPushButton#searchBtn{border-image:url(:/Resources/MainWindow/search/search_icon.png)}")
								.arg(m_colorBackGround.red()).arg(m_colorBackGround.green()).arg(m_colorBackGround.blue()));
}

void CCMainWindow::addCompanyDeps(QTreeWidgetItem* pRootGroupItem, const QJsonObject& dep)
{
	//创建子项
	QTreeWidgetItem* pChild = new QTreeWidgetItem;
	pChild->setData(0, Qt::UserRole, 1);	//设置子项标志为1
	pChild->setData(0, Qt::UserRole + 1, dep.value("departmentID").toString());	//用部门ID区分不同子项

	//自定义聊天控件初始化
	ContactItem* pContactItem = new ContactItem(ui.treeWidget);

	//设置群组头像
	QPixmap pix;
	pix.load(":/Resources/MainWindow/head_mask.png");
	QPixmap groupPix;
	groupPix.load(dep.value("picture").toString());
	pContactItem->setHeadPixmap(getRoundImage(groupPix, pix, pContactItem->getHeadLabelSize()));
	
	//设置群组名称
	QString strDepName;
	pContactItem->setUserName(dep.value("department_name").toString());

	pRootGroupItem->addChild(pChild);	//根项添加子项
	ui.treeWidget->setItemWidget(pChild, 0, pContactItem);	//控件嵌入子项
}

QString CCMainWindow::getHeadPicturePath()
{
	gstrLoginHeadPath = m_loginPicture;
	return m_loginPicture;
}

void CCMainWindow::setUserName(const QString& username)
{
	ui.nameLabel->adjustSize();

	//名字过长时省略
	QString name = ui.nameLabel->fontMetrics().elidedText(username, Qt::ElideRight, ui.nameLabel->width());	
	ui.nameLabel->setText(name);
}

void CCMainWindow::setLevelPixmap(int level)
{
	QPixmap levelPixmap(ui.levelBtn->size());
	levelPixmap.fill(Qt::transparent);

	QPainter painter(&levelPixmap);
	painter.drawPixmap(0, 4, QPixmap(":/Resources/MainWindow/lv.png"));

	int unitNum = level % 10;
	int tenNum = level / 10;
	painter.drawPixmap(10, 4, QPixmap(":/Resources/MainWindow/levelvalue.png"), tenNum * 6, 0, 6, 7);
	painter.drawPixmap(16, 4, QPixmap(":/Resources/MainWindow/levelvalue.png"), unitNum * 6, 0, 6, 7);

	ui.levelBtn->setIcon(levelPixmap);
	ui.levelBtn->setIconSize(ui.levelBtn->size());
}

void CCMainWindow::setHeadPixmap(const QString& headPath)
{
	QPixmap pix;
	pix.load(":/Resources/MainWindow/head_mask.png");
	ui.headLabel->setPixmap(getRoundImage(QPixmap(headPath), pix, ui.headLabel->size()));
}

void CCMainWindow::setStatusMenuIcon(const QString& statusPath)
{
	QPixmap statusBtnPixmap(ui.stausBtn->size());
	statusBtnPixmap.fill(Qt::transparent);

	QPainter painter(&statusBtnPixmap);
	painter.drawPixmap(4, 2, QPixmap(statusPath));

	ui.stausBtn->setIcon(statusBtnPixmap);
	ui.stausBtn->setIconSize(ui.stausBtn->size());
}

QWidget* CCMainWindow::addOtherAppExtension(const QString& appPath, const QString& appName)
{
	QPushButton* btn = new QPushButton;
	btn->setFixedSize(20, 20);

	QPixmap pixmap(btn->size());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	QPixmap appPixmap(appPath);
	painter.drawPixmap((btn->width() - appPixmap.width()) / 2, (btn->height() - appPixmap.height()) / 2, appPixmap);
	btn->setIcon(pixmap);
	btn->setIconSize(btn->size());
	btn->setObjectName(appName);
	btn->setProperty("hasborder", true);

	connect(btn, &QPushButton::clicked, this, &CCMainWindow::onAppIconClicked);	//点击app图标按钮，只做了皮肤图标
	return btn;
}

void CCMainWindow::initContactTree()
{
	connect(ui.treeWidget, &QTreeWidget::itemClicked, this, &CCMainWindow::onItemClicked);			//点击
	connect(ui.treeWidget, &QTreeWidget::itemExpanded, this, &CCMainWindow::onItemExpanded);		//展开
	connect(ui.treeWidget, &QTreeWidget::itemCollapsed, this, &CCMainWindow::onItemCollapsed);		//折叠
	connect(ui.treeWidget, &QTreeWidget::itemDoubleClicked, this, &CCMainWindow::onItemDoubleClicked);	//双击

	//初始化根项
	QTreeWidgetItem* pRootGroupItem = new QTreeWidgetItem;
	pRootGroupItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator); //即使没有子项，此项也会显示展开和折叠的控件。
	pRootGroupItem->setData(0, Qt::UserRole, 0);	//给这个树节点附加一个隐藏的整数值 0，供后续读取使用

	//初始化自定义根项，本质标签控件
	RootContactItem* pItemName = new RootContactItem(true, ui.treeWidget);
	QString strGroupName = QString::fromUtf8("人道巅峰");
	pItemName->setText(strGroupName);

	ui.treeWidget->addTopLevelItem(pRootGroupItem);	//根项添加到treeWidget
	ui.treeWidget->setItemWidget(pRootGroupItem, 0, pItemName);	//将标签控件设置到根项第0列

	//给根项添加子项
	for (const QJsonValue& v : m_departments) {
		addCompanyDeps(pRootGroupItem, v.toObject());
	}
}

void CCMainWindow::resizeEvent(QResizeEvent* event)
{
	setUserName(QString::fromUtf8("学IT 月薪过万 就来黑马程序员"));
	BasicWindow::resizeEvent(event);
}

bool CCMainWindow::eventFilter(QObject* obj, QEvent* event)
{
	if (ui.searchLineEdit == obj) {
		if (event->type() == QEvent::FocusIn) {	//点击搜索栏时设置搜索栏样式
			ui.searchWidget->setStyleSheet(QString("QWidget#searchWidget{background-color:rgb(255,255,255);border-bottom:1px solid rgba(%1,%2,%3,100)}\
				QPushButton#searchBtn{border-image:url(:/Resources/MainWindow/search/main_search_deldown.png)}\
				QPushButton#searchBtn:hover{border-image:url(:/Resources/MainWindow/search/main_search_delhighlight.png)}\
				QPushButton#searchBtn:pressed{border-image:url(:/Resources/MainWindow/search/main_search_delhighdown.png)}")
				.arg(m_colorBackGround.red()).arg(m_colorBackGround.green()).arg(m_colorBackGround.blue()));
		}
		else if (event->type() == QEvent::FocusOut) {
			updateSearchStyle();	//离开搜索栏时更新样式
		}
	}
	return false;
}

void CCMainWindow::mousePressEvent(QMouseEvent* event)
{
	//鼠标点击搜索栏或个性签名后，再点击其他位置时清除光标焦点
	if (qApp->widgetAt(event->pos()) != ui.searchLineEdit && ui.searchLineEdit->hasFocus()) {
		ui.searchLineEdit->clearFocus();
	}
	else if (qApp->widgetAt(event->pos()) != ui.lineEdit && ui.lineEdit->hasFocus()) {
		ui.lineEdit->clearFocus();
	}
	BasicWindow::mousePressEvent(event);
}

void CCMainWindow::onItemClicked(QTreeWidgetItem* item, int column)
{
	//qDebug() << "单击了项";
	//单击项
	bool bIsChild = item->data(0, Qt::UserRole).toBool();	//根项返回0，子项返回1
	if (!bIsChild) {//根项
		item->setExpanded(!item->isExpanded());	//未展开则展开，展开了则折叠
	}
}

void CCMainWindow::onItemExpanded(QTreeWidgetItem* item)
{
	//qDebug() << "项展开了";
	bool bIsChild = item->data(0, Qt::UserRole).toBool();
	if (!bIsChild) {
		RootContactItem* prootItem = dynamic_cast<RootContactItem*>(ui.treeWidget->itemWidget(item, 0));	//返回由item和给定列指定的单元格中显示的控件
		if (prootItem) {
			prootItem->setExpanded(true);	//实现根项展开时箭头旋转
		}
	}
}

void CCMainWindow::onItemCollapsed(QTreeWidgetItem* item)
{
	bool bIsChild = item->data(0, Qt::UserRole).toBool();
	if (!bIsChild) {
		RootContactItem* prootItem = dynamic_cast<RootContactItem*>(ui.treeWidget->itemWidget(item, 0));
		if (prootItem) {
			prootItem->setExpanded(false);	//实现根项折叠时箭头旋转
		}
	}
}

void CCMainWindow::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
	//双击子项时创建聊天窗口
	bool bIsChild = item->data(0, Qt::UserRole).toBool();
	if (bIsChild) {
		WindowManager::getInstance()->addNewTalkWindow(item->data(0, Qt::UserRole + 1).toString());
	}
}

void CCMainWindow::onAppIconClicked()
{
	if (sender()->objectName() == "app_skin") {
		SkinWindow* skinWindow = new SkinWindow;
		skinWindow->show();
	}
}