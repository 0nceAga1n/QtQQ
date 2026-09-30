#include "talkwindow.h"
#include "rootcontactitem.h"
#include "contactitem.h"
#include "commonutils.h"
#include "windowmanager.h"

#include <QToolTip>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>


TalkWindow::TalkWindow(QWidget* parent, const QString& uid, const QJsonObject& talkInfo)
	: QWidget(parent), m_talkId(uid), m_talkInfo(talkInfo)
{
	ui.setupUi(this);
	WindowManager::getInstance()->addWindowName(m_talkId, this);	//注册到窗口管理类的映射里
	setAttribute(Qt::WA_DeleteOnClose);

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
	bool hasText = !ui.textEdit->toPlainText().isEmpty();
	bool hasFile = !m_pendingFiles.isEmpty();
	//消息为空直接返回
	if (!hasText && !hasFile) {
		QToolTip::showText(this->mapToGlobal(QPoint(630, 660)), 
			QString::fromUtf8("发送的信息不能为空！"), 
			this, 
			QRect(0, 0, 120, 100), 2000);
		return;
	}

	//构造文件segments
	QJsonArray fileSegments;
	for (const auto& f : m_pendingFiles) {
		QFile file(f.first);
		if (!file.open(QIODevice::ReadOnly)) continue;
		QByteArray content = file.readAll();
		file.close();

		QJsonObject seg;
		seg.insert("type", QString("file"));
		seg.insert("name", f.second);
		seg.insert("size", content.size());
		seg.insert("data", QString::fromLatin1(content.toBase64()));
		fileSegments.append(seg);
	}
	m_pendingFiles.clear();
	
	//获取 除文件链接外的 消息
	QString html = removeFileChips(ui.textEdit->document()->toHtml());

	ui.textEdit->clear();	//清空编辑区
	ui.textEdit->deleteAllEmotionImage();	//释放资源

	ui.msgWidget->appendMsg(html, "0", true, fileSegments);	//聊天窗口添加信息
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
	QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择文件"), "/", "所有文件 (*.*)");
	if (path.isEmpty()) return;

	//保存文件路径和文件名
	m_pendingFiles.append(qMakePair(path, QFileInfo(path).fileName()));

	// 在输入框里插入一个可视 chip（真实数据在 m_pendingFiles）
	ui.textEdit->insertHtml(QString("<a href=\"#file\">📎 %1</a> ").arg(QFileInfo(path).fileName()));
}

void TalkWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		onSendBtnClicked(1);
	}
	else {
		QWidget::keyPressEvent(event);
	}
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

	connect(ui.textEdit, SIGNAL(sendMsgSignal(bool)), this, SLOT(onSendBtnClicked(bool)));

	if (m_talkInfo.value("is_group").toInt() == 1) {	//部门群聊
		initTalkWindow();
	}
	else {	//单聊
		initPtoPTalk();
	}
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

	//获取部门名称、群聊人数
	QString strGroupName = m_talkInfo.value("name").toString();
	QJsonArray members = m_talkInfo.value("members").toArray();
	int nEmployeeNum = members.size();

	//设置根控件内容
	QString qsGroupName = QString::fromUtf8("%1 %2/%3").arg(strGroupName).arg(0).arg(nEmployeeNum);
	pItemName->setText(qsGroupName);

	//添加根项，根控件嵌入根项
	ui.treeWidget->addTopLevelItem(pRootItem);
	ui.treeWidget->setItemWidget(pRootItem, 0, pItemName);

	//群员列表始终展开
	pRootItem->setExpanded(true);

	//添加群员
	for (const QJsonValue& v : members) {
		addPeopInfo(pRootItem, v.toObject());
	}
}

void TalkWindow::addPeopInfo(QTreeWidgetItem* pRootGroupItem, const QJsonObject& member)
{
	QString employeeID = member.value("employeeID").toString();
	QString strName = member.value("employee_name").toString();
	QString strSign = member.value("employee_sign").toString();
	QString strPicturePath = member.value("picture").toString();

	//初始化子项
	QTreeWidgetItem* pChild = new QTreeWidgetItem();
	pChild->setData(0, Qt::UserRole, 1);	//子项标志为1
	pChild->setData(0, Qt::UserRole + 1, employeeID);	//用员工id区分不同子项

	//初始化子控件
	ContactItem* pContactItem = new ContactItem(ui.treeWidget);

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

QString TalkWindow::removeFileChips(const QString& html)
{
	QString result = html;
	QRegularExpression re("<a[^>]*href=\"#file\"[^>]*>.*?</a>",
		QRegularExpression::DotMatchesEverythingOption);
	result.remove(re);
	return result;
}