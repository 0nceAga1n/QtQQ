#include "talkwindow.h"
#include "rootcontactitem.h"
#include "contactitem.h"
#include "commonutils.h"
#include "windowmanager.h"
#include <QToolTip>
#include <QFile>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include "sendfile.h"

TalkWindow::TalkWindow(QWidget* parent, const QString& uid)
	: QWidget(parent), m_talkId(uid)
{
	ui.setupUi(this);
	WindowManager::getInstance()->addWindowName(m_talkId, this);	//注册到窗口管理类的映射里
	setAttribute(Qt::WA_DeleteOnClose);

	initGroupTalkStatus();
	initControl();
}

TalkWindow::~TalkWindow()
{
	//关闭聊天窗口时，注销掉该窗口在窗口管理类的映射
	//避免下次再打开聊天窗口时，映射中已经存在聊天窗口的uid，而造成对空指针的调用报错
	WindowManager::getInstance()->deleteWindowName(m_talkId);
}

void TalkWindow::addEmotionImage(int emotionNum)
{
	ui.textEdit->setFocus();
	ui.textEdit->addEmotionUrl(emotionNum);
}

void TalkWindow::setWindowName(const QString& name)
{
	ui.nameLabel->setText(name);
}

QString TalkWindow::getTalkId()
{
	return m_talkId;
}

void TalkWindow::onSendBtnClicked(bool)
{
	//消息为空直接返回
	if (ui.textEdit->toPlainText().isEmpty()) {
		QToolTip::showText(this->mapToGlobal(QPoint(630, 660)), 
			QString::fromUtf8("发送的信息不能为空！"), 
			this, 
			QRect(0, 0, 120, 100), 2000);
		return;
	}
	
	//获取消息
	QString html = ui.textEdit->document()->toHtml();
	if (!html.contains("</span>")) {	//发送的是文字且文字没有样式
		QString fontHtml;
		QString text = ui.textEdit->toPlainText();	//获取文本
		text.remove(QChar::ObjectReplacementCharacter);  //去掉表情占位符
		if (!text.isEmpty()) {
			// 只剩表情，没有文字，跳过包装，有文字则进入该分支
			QFile file(":/Resources/MainWindow/MsgHtml/msgFont.txt");	//加载html
			if (file.open(QIODevice::ReadOnly)) {
				fontHtml = file.readAll();
				fontHtml.replace("%1", text);	//将文本写进带样式的html
				file.close();
			}
			else {
				QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("文件不存在"));
				return;
			}

			if (!html.contains(fontHtml)) {	//将原消息中的文本替换为带样式的文本
				html.replace(text, fontHtml);
			}
		}
	}
	ui.textEdit->clear();	//清空编辑区
	ui.textEdit->deleteAllEmotionImage();	//释放资源

	ui.msgWidget->appendMsg(html);	//聊天窗口添加信息
}

void TalkWindow::onItemDoubleClicked(QTreeWidgetItem* item)
{
	//双击群员列表项时，判断是否为子项（双击的是否为群员）
	bool bIsChild = item->data(0, Qt::UserRole).toBool();
	if (bIsChild) {
		//添加单聊窗口
		WindowManager::getInstance()->addNewTalkWindow(item->data(0, Qt::UserRole + 1).toString());
	}
}

void TalkWindow::onFileOpenBtnClicked(bool)
{
	SendFile* sendFile = new SendFile(this);
	sendFile->show();
}

void TalkWindow::initControl()
{
	//初始化控件
	QList<int> rightWidgetSize;
	rightWidgetSize << 600 << 138;
	ui.bodySplitter->setSizes(rightWidgetSize);

	ui.textEdit->setFontPointSize(10);
	ui.textEdit->setFocus();

	//信号连接
	connect(ui.sysmin, SIGNAL(clicked(bool)), parent(), SLOT(onShowMin(bool)));
	connect(ui.sysclose, SIGNAL(clicked(bool)), parent(), SLOT(onShowClose(bool)));
	connect(ui.closeBtn, SIGNAL(clicked(bool)), parent(), SLOT(onShowClose(bool)));

	connect(ui.faceBtn, SIGNAL(clicked(bool)), parent(), SLOT(onEmotionBtnClicked(bool)));
	connect(ui.sendBtn, SIGNAL(clicked(bool)), this, SLOT(onSendBtnClicked(bool)));
	connect(ui.fileopenBtn, SIGNAL(clicked(bool)), this, SLOT(onFileOpenBtnClicked(bool)));

	connect(ui.treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*)));

	if (m_isGroupTalk) {	//部门群聊
		initTalkWindow();
	}
	else {	//单聊
		initPtoPTalk();
	}
}

void TalkWindow::initGroupTalkStatus()
{
	//在部门表中查找uid，不存在说明窗口为单聊窗口
	QSqlQueryModel sqlDepModel;
	QString strSql = QString("SELECT * FROM tab_department WHERE departmentID = %1").arg(m_talkId);
	sqlDepModel.setQuery(strSql);
	int rows = sqlDepModel.rowCount();
	if (rows == 0) {	//单聊窗口
		m_isGroupTalk = false;
	}
	else {	//部门窗口
		m_isGroupTalk = true;
	}
}

int TalkWindow::getComDepID()
{
	//获取公司群id
	QSqlQuery queryDepID(QString("SELECT departmentID FROM tab_department WHERE department_name = '%1'").arg(QString::fromUtf8("公司群")));
	queryDepID.exec();
	queryDepID.next();
	return queryDepID.value(0).toInt();
}

void TalkWindow::initPtoPTalk()
{
	//单聊窗口，没有群员列表项，直接显示一张图片
	QPixmap pixSkin;
	pixSkin.load(":/Resources/MainWindow/skin.png");

	ui.widget->setFixedSize(pixSkin.size());

	QLabel* skinLabel = new QLabel(ui.widget);
	skinLabel->setPixmap(pixSkin);
	skinLabel->setFixedSize(ui.widget->size());
}

void TalkWindow::initTalkWindow()
{
	//初始化根项
	QTreeWidgetItem* pRootItem = new QTreeWidgetItem();
	pRootItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);	//即使没有子项，此项也会显示展开和折叠的控件。
	pRootItem->setData(0, Qt::UserRole, 0);

	//初始化根控件
	RootContactItem* pItemName = new RootContactItem(false, ui.treeWidget);	//没有箭头的根控件
	ui.treeWidget->setFixedHeight(646);

	//获取部门名称
	QString strGroupName;	
	QSqlQuery queryGroupName(QString("SELECT department_name FROM tab_department WHERE departmentID = %1").arg(m_talkId));
	queryGroupName.exec();
	if (queryGroupName.next()) {
		strGroupName = queryGroupName.value(0).toString();
	}

	//获取群聊人数
	QSqlQueryModel queryEmployeeModel;
	if (getComDepID() == m_talkId.toInt()) {	//公司群
		queryEmployeeModel.setQuery("SELECT employeeID FROM tab_employees WHERE status = 1");
	}
	else {	//部门群
		queryEmployeeModel.setQuery(QString("SELECT employeeID FROM tab_employees WHERE status = 1 AND departmentID = %1").arg(m_talkId));
	}
	int nEmployeeNum = queryEmployeeModel.rowCount();

	//设置根控件内容
	QString qsGroupName = QString::fromUtf8("%1 %2/%3").arg(strGroupName).arg(0).arg(nEmployeeNum);
	pItemName->setText(qsGroupName);

	//添加根项，根控件嵌入根项
	ui.treeWidget->addTopLevelItem(pRootItem);
	ui.treeWidget->setItemWidget(pRootItem, 0, pItemName);

	//群员列表始终展开
	pRootItem->setExpanded(true);

	//添加群员
	for (int i = 0; i < nEmployeeNum; i++) {
		QModelIndex modelIndex = queryEmployeeModel.index(i, 0);
		int employeeID = queryEmployeeModel.data(modelIndex).toInt();
		addPeopInfo(pRootItem, employeeID);
	}
}

void TalkWindow::addPeopInfo(QTreeWidgetItem* pRootGroupItem, int employeeID)
{
	//初始化子项
	QTreeWidgetItem* pChild = new QTreeWidgetItem();
	pChild->setData(0, Qt::UserRole, 1);	//子项标志为1
	pChild->setData(0, Qt::UserRole + 1, employeeID);	//用员工id区分不同子项

	//初始化子控件
	ContactItem* pContactItem = new ContactItem(ui.treeWidget);

	//从数据库中获取头像路径，名字，个性签名
	QString strName, strSign, strPicturePath;
	QSqlQueryModel queryInfoModel;
	queryInfoModel.setQuery(QString("SELECT employee_name, employee_sign, picture FROM tab_employees WHERE employeeID = %1").arg(employeeID));
	QModelIndex nameIndex, signIndex, pictureIndex;
	nameIndex = queryInfoModel.index(0, 0);
	signIndex = queryInfoModel.index(0, 1);
	pictureIndex = queryInfoModel.index(0, 2);
	strName = queryInfoModel.data(nameIndex).toString();
	strSign = queryInfoModel.data(signIndex).toString();
	strPicturePath = queryInfoModel.data(pictureIndex).toString();

	//头像
	QPixmap pix1;
	pix1.load(":/Resources/MainWindow/head_mask.png");
	QImage imageHead;
	imageHead.load(strPicturePath);
	pContactItem->setHeadPixmap(CommonUtils::getRoundImage(QPixmap::fromImage(imageHead), pix1, pContactItem->getHeadLabelSize()));
	//名字，个性签名
	pContactItem->setUserName(strName);
	pContactItem->setSignName(strSign);

	//根项添加子项，子控件嵌入子项
	pRootGroupItem->addChild(pChild);
	ui.treeWidget->setItemWidget(pChild, 0, pContactItem);
}
