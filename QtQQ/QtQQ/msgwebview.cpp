#include "msgwebview.h"
#include "talkwindowshell.h"
#include "windowmanager.h"

#include <QFile>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileInfo>
#include <QBuffer>
#include <QDir>
#include <QUrl>
#include <QDesktopServices>

extern QString gstrLoginHeadPath;

MsgHtmlObj::MsgHtmlObj(QObject* parent, QString msgLPicPath)
{
	m_msgLPicPath = msgLPicPath;
	initHtmlTmpl();
}

QString MsgHtmlObj::buildLeftBubble(const QString& content)
{
	QString html = m_msgLHtmlTmpl;
	html.replace("{{MSG}}", content);
	return html;
}

QString MsgHtmlObj::buildRightBubble(const QString& content)
{
	QString html = m_msgRHtmlTmpl;
	html.replace("{{MSG}}", content);
	return html;
}

void MsgHtmlObj::initHtmlTmpl()
{
	//分别加载左侧和右侧模板
	m_msgLHtmlTmpl = getMsgTmplHtml("msgleftTmpl");
	m_msgLHtmlTmpl.replace("%1", toWebUrl(m_msgLPicPath));	//将左侧模板里的头像地址替换为消息发送者头像

	m_msgRHtmlTmpl = getMsgTmplHtml("msgrightTmpl");
	m_msgRHtmlTmpl.replace("%1", toWebUrl(gstrLoginHeadPath));	//将右侧模板里的头像地址替换为当前登录者头像
}

QString MsgHtmlObj::getMsgTmplHtml(const QString& code)
{
	QFile file(":/Resources/MainWindow/MsgHtml/" + code + ".html");
	file.open(QFile::ReadOnly);
	QString strData;
	if (file.isOpen()) {
		strData = QLatin1String(file.readAll());
	}
	else {
		QMessageBox::information(nullptr, "Tips", "Failed to init html");
		return QString("");
	}
	file.close();
	return strData;
}

QString MsgHtmlObj::toWebUrl(const QString& path)
{
	if (path.startsWith(":/")) {
		return "qrc" + path;
	}
	else {
		// 读取本地图片文件，转为 data URL 嵌入 HTML
		QImage image(path);
		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		buffer.open(QIODevice::WriteOnly);
		image.save(&buffer, "PNG");
		return QString("data:image/png;base64,%1").arg(QString(byteArray.toBase64()));
	}
}

bool MsgWebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
	if (url.scheme() == QString("qrc")) {
		return true;
	}
	if (url.scheme() == QString("download")) {
		emit signalDownloadFile(url.path().toInt());
		return false;
	}
	return false;
}

MsgWebView::MsgWebView(QWidget *parent)
	: QWebEngineView(parent)
{
	//将格式化后的数据传递给talkwindowshell，发生tcp通信
	TalkWindowShell* talkWindowShell = WindowManager::getInstance()->getTalkWindowShell();
	connect(this, &MsgWebView::signalSendMsg, talkWindowShell, &TalkWindowShell::updateSendTcpMsg);

	//初始化自定义页面
	MsgWebPage* page = new MsgWebPage(this);
	setPage(page);	//设置页面

	//用户点击文件气泡时进行处理
	connect(page, &MsgWebPage::signalDownloadFile, this, &MsgWebView::onDownloadFile);

	/*------初始化 HTML 模板管理对象，用于右侧气泡------*/
	m_selfHtmlObj = new MsgHtmlObj(this);

	/*------初始化 HTML 模板管理对象，用于左侧气泡------*/
	QJsonObject talkInfo = WindowManager::getInstance()->getCreatingTalkInfo();
	bool isGroup = talkInfo.value("is_group").toInt() == 1;
	QJsonArray members = talkInfo.value("members").toArray();

	if (isGroup) {
		for (const QJsonValue& v : members) {
			QJsonObject m = v.toObject();
			QString employeeID = m.value("employeeID").toString();
			QString picture = m.value("picture").toString();
			m_memberHtmlObjs.insert(employeeID, new MsgHtmlObj(this, picture));
		}
	}
	else {
		QString picture = talkInfo.value("picture").toString();
		m_memberHtmlObjs.insert(talkInfo.value("uid").toString(), new MsgHtmlObj(this, picture));
	}

	//加载基础页面 msgTmpl.html
	connect(this, &QWebEngineView::loadFinished, this, [this](bool ok) {
		if (!ok) return;
		m_pageLoaded = true;
		QList<PendingMsg> pending = m_pendingMsgs;
		m_pendingMsgs.clear();
		for (const PendingMsg& m : pending) {
			appendMsg(m.html, m.strObj, m.shouldSend, m.fileSegments);
		}
	});
	this->load(QUrl("qrc:/Resources/MainWindow/MsgHtml/msgTmpl.html"));
}

MsgWebView::~MsgWebView()
{}

void MsgWebView::appendMsg(const QString & html, QString strObj, bool shouldSend, const QJsonArray& fileSegments)
{
	if (!m_pageLoaded) {	//页面脚本还未加载完成，先保存消息
		m_pendingMsgs.append(PendingMsg{ html, strObj, shouldSend, fileSegments });
		return;
	}

	QJsonArray segments; // 结构化消息段落，发送给服务端
	QString qsMsg;	//气泡渲染内容，渲染在聊天窗口

	const QList<QStringList> msgLst = parseHtml(html);	//解析传入的html

	for (int i = 0; i < msgLst.size(); i++) {	//遍历解析后的列表
		if (msgLst.at(i).at(0) == "img") {	//当前项是img标签，即用户要发送的是表情包
			QString imagePath = msgLst.at(i).at(1);	//获取表情包url

			//格式化消息
			QJsonObject seg;
			seg.insert("type", QString("image"));
			seg.insert("data", QFileInfo(imagePath).baseName());	//表情编号
			segments.append(seg);

			//加载表情图片以获得宽高
			QPixmap pixmap;
			if (imagePath.left(3) == "qrc") {
				pixmap.load(imagePath.mid(3));
			}
			else {
				pixmap.load(imagePath);
			}

			//生成带宽高属性的img标签，确保表情尺寸正确
			QString imgPath = QString("<img src=\"%1\" width=\"%2\" height=\"%3\" />")
				.arg(imagePath).arg(pixmap.width()).arg(pixmap.height());

			qsMsg += imgPath;	//拼接气泡渲染内容
		}
		else if (msgLst.at(i).at(0) == "text") {	//当前项是text文本，即用户要发送的是文本
			//格式化消息
			QJsonObject seg;
			seg.insert("type", QString("text"));
			seg.insert("data", msgLst.at(i).at(1));
			segments.append(seg);
			
			qsMsg += msgLst.at(i).at(1).toHtmlEscaped();	//拼接气泡渲染内容
		}
		else if (msgLst.at(i).at(0) == "file") {	//当前项是文件链接
			QString index = msgLst.at(i).at(1);
			QString name = msgLst.at(i).at(2);
			qsMsg += QString("<a href=\"download:%1\">%2</a>").arg(index).arg(name);
		}
	}

	for (const QJsonValue& v : fileSegments) {
		//取出格式好的文件消息
		QJsonObject seg = v.toObject();
		segments.append(seg);

		qsMsg += QString("<span style=\"color:#0066cc;\">📎 %1</span>")
			.arg(seg.value("name").toString());	//拼接气泡渲染内容
	}

	if (strObj == "0") {	//发送数据
		QString bubble = m_selfHtmlObj->buildRightBubble(qsMsg);
		QJsonObject obj;
		obj.insert("HTML", bubble);
		QString arg = QJsonDocument(obj).toJson(QJsonDocument::Compact);
		this->page()->runJavaScript(QString("appendHtml(%1.HTML)").arg(arg));	//在页面中执行JS函数appendHtml，将消息渲染为右侧聊天气泡
		if (shouldSend && !segments.isEmpty()) {	//历史消息渲染时 shouldSend=false，不触发发送
			emit signalSendMsg(segments);	//将结构化消息段落传递出去进行tcp通信，构造函数里已经进行信号连接
		}
	}
	else {	//接收数据
		MsgHtmlObj* memberObj = m_memberHtmlObjs.value(strObj);
		if (memberObj) {
			QString bubble = memberObj->buildLeftBubble(qsMsg);
			QJsonObject obj;
			obj.insert("HTML", bubble);
			QString arg = QJsonDocument(obj).toJson(QJsonDocument::Compact);
			this->page()->runJavaScript(QString("appendHtml(%1.HTML)").arg(arg));	//在页面中执行JS函数appendHtml，将消息渲染为左侧聊天气泡
		}
	}
}

int MsgWebView::registerFile(const QJsonObject& seg)
{
	int index = m_nextFileIndex++;
	m_files.insert(index, seg);
	return index;
}

void MsgWebView::onDownloadFile(int index)
{
	if (!m_files.contains(index)) return;
	QJsonObject seg = m_files.value(index);

	// Base64 解码成二进制
	QByteArray content = QByteArray::fromBase64(seg.value("data").toString().toLatin1());

	// 保存到临时目录
	QString savePath = QDir::tempPath() + "/" + seg.value("name").toString();
	QFile file(savePath);
	if (!file.open(QIODevice::WriteOnly)) return;
	file.write(content);
	file.close();

	// 用系统默认程序打开
	QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
}

QList<QStringList> MsgWebView::parseHtml(const QString & html)
{
	QDomDocument doc;	//Qt 提供的 XML/HTML DOM 解析器
	doc.setContent(html);	//解析字符串为DOM树
	const QDomElement& root = doc.documentElement();	//获得<html>标签
	const QDomNode& node = root.firstChildElement("body");	//获得<body>标签，同时向上转型便于parseDocNode函数参数传递

	return parseDocNode(node);
}

QList<QStringList> MsgWebView::parseDocNode(const QDomNode& node)
{
	QList<QStringList> attribute;

	// 图片节点/文件链接：直接提取，不再往下递归
	if (node.isElement()) {
		const QDomElement& element = node.toElement();
		if (element.tagName() == "img") {
			QStringList attributeList;
			attributeList << "img" << element.attribute("src");
			attribute << attributeList;
			return attribute;
		}
		if (element.tagName() == "a") {
			QString href = element.attribute("href");
			if (href.startsWith("download:")) {
				QStringList l;
				l << "file" << href.mid(QString("download:").length()) << element.text();
				attribute << l;
				return attribute;
			}
		}
	}

	// 文本节点：提取纯文本（跳过纯空白）
	if (node.isText()) {
		QString text = node.toText().data().trimmed();
		if (!text.isEmpty()) {
			QStringList attributeList;
			attributeList << "text" << text;
			attribute << attributeList;
		}
		return attribute;
	}

	// 其他元素（html/body/p/span 等）：递归遍历子节点，保证图文顺序
	const QDomNodeList& list = node.childNodes();
	for (int i = 0; i < list.count(); i++) {
		attribute << parseDocNode(list.at(i));
	}
	return attribute;
}
