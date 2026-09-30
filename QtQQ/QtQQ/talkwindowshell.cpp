#include "talkwindowshell.h"
#include "commonutils.h"
#include "talkwindow.h"
#include "windowmanager.h"
#include "qmsgtextedit.h"

#include <QMessageBox>
#include <QFile>	

extern QString gLoginEmployeeID;
extern QString gLoginToken;

TalkWindowShell::TalkWindowShell(QWidget *parent)
	: BasicWindow(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	initControl();

	initTcpSocket();	//初始化tcp通信
}

TalkWindowShell::~TalkWindowShell()
{
	delete m_emotionWindow;
	m_emotionWindow = nullptr;
}

void TalkWindowShell::sendFetchTalkInfo(const QString& uid)
{
	if (m_tcpClientSocket->state() != QAbstractSocket::ConnectedState) {
		m_pendingTalkInfo.append(uid);
		return;
	}

	QJsonObject req;
	req.insert("cmd", QString("fetch_talk_info"));
	req.insert("uid", uid);
	m_tcpClientSocket->write(MsgProtocol::pack(req));
}

void TalkWindowShell::addTalkWindow(TalkWindow* talkWindow, TalkWindowItem* talkWindowItem, const QString uid, const QString& picture)
{
	//添加右侧聊天窗口
	ui.rightStackedWidget->addWidget(talkWindow);
	//表情窗口隐藏了，让talkwindow设置表情按钮状态
	connect(m_emotionWindow, SIGNAL(signalEmotionWindowHide()), talkWindow, SLOT(onSetEmotionBtnStatus()));

	//初始化左侧聊天列表项
	QListWidgetItem* aItem = new QListWidgetItem(ui.listWidget);
	m_talkwindowItemMap.insert(aItem, talkWindow);	//建立映射

	aItem->setSelected(true);

	//设置左侧聊天列表项的头像
	QImage img;
	img.load(picture);
	talkWindowItem->setHeadPixmap(QPixmap::fromImage(img));

	//添加左侧聊天列表项，将传入的控件嵌入列表项
	ui.listWidget->addItem(aItem);
	ui.listWidget->setItemWidget(aItem, talkWindowItem);

	//激活对应聊天窗口
	onTalkWindowItemClicked(aItem);

	//聊天列表项关闭按钮信号连接
	connect(talkWindowItem, &TalkWindowItem::signalCloseClicked,
		[talkWindowItem, talkWindow, aItem, this]() {
			m_talkwindowItemMap.remove(aItem);	//映射表删除
			talkWindow->close();	//关闭窗口，即释放窗口
			ui.listWidget->takeItem(ui.listWidget->row(aItem));	//左侧列表项删除
			delete talkWindowItem;	//释放列表项
			ui.rightStackedWidget->removeWidget(talkWindow);	//右侧窗口删除
			if (ui.rightStackedWidget->count() < 1) {	//右侧没有窗口时关闭整个talkwindowshell
				close();
			}
		});

	// 拉取该窗口的历史消息（若尚未连接则先缓存）
	if (m_tcpClientSocket->state() == QAbstractSocket::ConnectedState) {
		sendFetchHistory(uid);
	}
	else {
		m_pendingHistory.append(uid);
	}
}

void TalkWindowShell::setCurrentWidget(QWidget* widget)
{
	ui.rightStackedWidget->setCurrentWidget(widget);	//右侧聊天窗口选择
	//ui.listWidget->setCurrentItem(m_talkwindowItemMap.key(widget));	//左侧聊天列表选择，在WindowManager中实现
}

const QMap<QListWidgetItem*, QWidget*>& TalkWindowShell::getTalkWindowItemMap() const
{
	return m_talkwindowItemMap;
}

void TalkWindowShell::initControl()
{
	loadStyleSheet("TalkWindow");
	setWindowTitle(QString::fromUtf8("人道巅峰"));

	m_emotionWindow = new EmotionWindow;
	m_emotionWindow->hide();

	QList<int> leftWidgetSize;
	leftWidgetSize << 154 << width() - 154;
	ui.splitter->setSizes(leftWidgetSize);

	ui.listWidget->setStyle(new CustomProxyStyle(this));

	//左侧聊天列表项点击时激活对应聊天窗口
	connect(ui.listWidget, &QListWidget::itemClicked, this, &TalkWindowShell::onTalkWindowItemClicked);
	//点击表情窗口中表情时，在当前聊天窗口添加表情
	connect(m_emotionWindow, SIGNAL(signalEmotionItemClicked(int)), this, SLOT(onEmotionItemClicked(int)));
}

void TalkWindowShell::initTcpSocket()
{
	//建立客户端套接字
	m_tcpClientSocket = new QTcpSocket(this);
	
	//收到服务端的消息时进行处理
	connect(m_tcpClientSocket, &QTcpSocket::readyRead, this, &TalkWindowShell::onTcpReadyRead);
	//成功连接到服务端时
	connect(m_tcpClientSocket, &QTcpSocket::connected, this, [this]() {
		// 连接成功后向服务端注册身份（对应服务端 handleLogin）
		QJsonObject login;
		login.insert("cmd", QString("login"));
		login.insert("token", gLoginToken);
		m_tcpClientSocket->write(MsgProtocol::pack(login));

		// 连接建立后，拉取之前缓存的窗口信息
		for (const QString& uid : m_pendingTalkInfo) {
			sendFetchTalkInfo(uid);
		}
		m_pendingTalkInfo.clear();

		// 连接建立后，拉取之前缓存的窗口历史
		for (const QString& uid : m_pendingHistory) {
			sendFetchHistory(uid);
		}
		m_pendingHistory.clear();
	});

	//连接服务端
	m_tcpClientSocket->connectToHost("127.0.0.1", MsgProtocol::TCP_PORT);
}

void TalkWindowShell::processMsgFrame(const QJsonObject& obj)
{
	QString cmd = obj.value("cmd").toString();
	if (cmd == "talk_info") {
		WindowManager::getInstance()->createTalkWindow(obj);
		return;
	}

	if (cmd == "history") {	//历史消息响应
		handleHistoryMsg(obj);
		return;
	}

	if (cmd != "msg") {
		return;
	}
	if (!MsgProtocol::isValidMsg(obj)) {	//非法消息不处理
		return;
	}

	QString sender = obj.value("sender").toString();
	QString receiver = obj.value("receiver").toString();
	bool isGroup = obj.value("group").toInt() == 1;
	QJsonArray segments = obj.value("segments").toArray();	//聊天消息

	if (sender == gLoginEmployeeID) {
		return;		// 自己的消息（服务端已排除，这里双保险）
	}

	QString windowID = isGroup ? receiver : sender;
	QWidget* widget = WindowManager::getInstance()->findWindowName(windowID);
	if (!widget) {
		//TODO:窗口未打开，消息落库 + 未读标记，暂先丢弃
		return;
	}

	this->setCurrentWidget(widget);
	QListWidgetItem* item = m_talkwindowItemMap.key(widget);
	if (item) {
		item->setSelected(true);
	}
	handleReceivedMsg(sender.toInt(), segments);
}

void TalkWindowShell::sendFetchHistory(const QString& uid)
{
	QJsonObject req;
	req.insert("cmd", QString("fetch_history"));
	req.insert("peer", uid);
	req.insert("is_group", (uid.length() == 4) ? 1 : 0);
	m_tcpClientSocket->write(MsgProtocol::pack(req));
}

QString TalkWindowShell::buildSegmentsHtml(const QJsonArray& segments, MsgWebView* view)
{
	QString content;
	for (const QJsonValue& val : segments) {
		QJsonObject seg = val.toObject();
		QString type = seg.value("type").toString();
		QString data = seg.value("data").toString();
		if (type == "image") {	//表情包
			content += QString("<img src=\"qrc:/Resources/MainWindow/emotion/%1.png\" />").arg(data);
		}
		else if (type == "file") {	//文件链接
			int index = view ? view->registerFile(seg) : -1;	//文件编号，用于查找文件信息

			//计算文件大小
			int size = seg.value("size").toInt();
			QString sizeStr;
			if (size < 1024) sizeStr = QString("%1 B").arg(size);
			else if (size < 1024 * 1024) sizeStr = QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
			else sizeStr = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);

			content += QString("<a href=\"download:%1\">📎 %2 (%3)</a>")
				.arg(index).arg(seg.value("name").toString()).arg(sizeStr);
		}
		else {	//文本
			content += QString("<span style=\"font-size:10pt;\">%1</span>").arg(data.toHtmlEscaped());
		}
	}
	return content;
}

void TalkWindowShell::handleHistoryMsg(const QJsonObject& obj)
{
	QString peer = obj.value("peer").toString();
	QJsonArray messages = obj.value("messages").toArray();

	// 找到 peer 对应的聊天窗口（单聊=对方ID，群聊=群ID，正好是窗口映射的 key）
	QWidget* widget = WindowManager::getInstance()->findWindowName(peer);
	if (!widget) return;
	TalkWindow* talkWindow = dynamic_cast<TalkWindow*>(widget);
	if (!talkWindow) return;

	// 服务端已按 msg_id 升序返回，直接按顺序渲染，就是从旧到新
	for (const QJsonValue& v : messages) {
		QJsonObject msg = v.toObject();
		QString sender = msg.value("sender").toString();
		QJsonArray segments = msg.value("segments").toArray();

		QString htmlText = QString("<html><body>%1</body></html>").arg(buildSegmentsHtml(segments, talkWindow->ui.msgWidget));

		bool isSelf = (sender == gLoginEmployeeID);
		if (isSelf) {
			// 自己发的历史消息：右气泡，但不触发发送
			talkWindow->ui.msgWidget->appendMsg(htmlText, "0", false);
		}
		else {
			// 对方发的历史消息：左气泡
			talkWindow->ui.msgWidget->appendMsg(htmlText, sender, false);
		}
	}
}

void TalkWindowShell::handleReceivedMsg(int senderEmployeeID, const QJsonArray& segments)
{
	TalkWindow* talkWindow = dynamic_cast<TalkWindow*>(ui.rightStackedWidget->currentWidget());
	if (!talkWindow) return;
	QString htmlText = QString("<html><body>%1</body></html>").arg(buildSegmentsHtml(segments, talkWindow->ui.msgWidget));

	//在当前窗口的网页上添加消息
	talkWindow->ui.msgWidget->appendMsg(htmlText, QString::number(senderEmployeeID));

}

void TalkWindowShell::updateSendTcpMsg(const QJsonArray& segments)
{
	if (segments.isEmpty()) {
		return;
	}

	//获取发送消息的是哪个窗口
	TalkWindow* curTalkWindow = dynamic_cast<TalkWindow*>(ui.rightStackedWidget->currentWidget());
	if (!curTalkWindow) {
		return;
	}

	/*-----------将要发送的消息进行封装以便于解析--------------*/
	QString talkId = curTalkWindow->getTalkId();
	bool isGroup = (talkId.length() == 4);
	QJsonObject obj = MsgProtocol::buildChatMsg(gLoginEmployeeID, talkId, isGroup, segments);

	//将消息转化类型后发送到服务端
	m_tcpClientSocket->write(MsgProtocol::pack(obj));
}

void TalkWindowShell::onEmotionBtnClicked(bool)
{
	//点击表情按钮时显示表情窗口
	m_emotionWindow->setVisible(!m_emotionWindow->isVisible());
	QPoint emotionPoint = this->mapToGlobal(QPoint(0, 0));

	emotionPoint.setX(emotionPoint.x() + 170);
	emotionPoint.setY(emotionPoint.y() + 220);
	m_emotionWindow->move(emotionPoint);
}

void TalkWindowShell::onTalkWindowItemClicked(QListWidgetItem* item)
{
	//左侧聊天列表项点击时激活对应聊天窗口
	QWidget* talkwindowWidget = m_talkwindowItemMap.find(item).value();
	ui.rightStackedWidget->setCurrentWidget(talkwindowWidget);
}

void TalkWindowShell::onEmotionItemClicked(int emotionNum)
{
	//点击表情窗口中表情时，在当前聊天窗口添加表情
	TalkWindow* curTalkWindow = dynamic_cast<TalkWindow*>(ui.rightStackedWidget->currentWidget());
	if (curTalkWindow) {
		curTalkWindow->addEmotionImage(emotionNum);
	}
}

void TalkWindowShell::onTcpReadyRead()
{
	QByteArray buffer = m_tcpClientSocket->readAll();	//读取服务端发来的消息
	const QList<QJsonObject>& frames = m_decoder.push(buffer);		//分帧
	for (const QJsonObject& obj : frames) {		//每帧单独处理
		processMsgFrame(obj);
	}
}