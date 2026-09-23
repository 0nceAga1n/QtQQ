#include "msgwebview.h"
#include <QFile>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QWebChannel>
#include "talkwindowshell.h"
#include "windowmanager.h"
#include <QFileInfo>
#include <QSqlQueryModel>
#include <QBuffer>

extern QString gstrLoginHeadPath;

MsgHtmlObj::MsgHtmlObj(QObject* parent, QString msgLPicPath)
{
	m_msgLPicPath = msgLPicPath;
	initHtmlTmpl();
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
	if (url.scheme() == QString("qrc") || url.scheme() == QString("file")) {
		return true;
	}
	return false;
}

MsgWebView::MsgWebView(QWidget *parent)
	: QWebEngineView(parent)
	, m_channel(new QWebChannel(this))
{
	//将格式化后的数据传递给talkwindowshell，发生tcp通信
	TalkWindowShell* talkWindowShell = WindowManager::getInstance()->getTalkWindowShell();
	connect(this, &MsgWebView::signalSendMsg, talkWindowShell, &TalkWindowShell::updateSendTcpMsg);

	//初始化自定义页面
	MsgWebPage* page = new MsgWebPage(this);
	setPage(page);	//设置页面

	/*------初始化 HTML 模板管理对象，用于右侧气泡------*/
	m_msgHtmlObj = new MsgHtmlObj(this);

	//QWebEngineView 里的 Chromium 网页和 C++ 程序是两个隔离的运行环境，互相访问不到对方的数据。
	//创建 QWebChannel，注册 MsgHtmlObj 为 "external0"，
	//JS 端可通过 channel.objects.external 访问模板字符串
	m_channel->registerObject("external0", m_msgHtmlObj);


	/*------初始化 HTML 模板管理对象，用于左侧气泡------*/
	//2种情况：群聊，单聊。公司群QQ号 2000，属于特殊情况
	QString strTalkID = WindowManager::getInstance()->getCreatingTalkID();	//获取当前窗口id
	
	QSqlQueryModel queryEmployeeModel;
	QString strEmployeeID, strPicturePath;
	QString strExternal;
	bool isGroupTalk = false;		// 群聊标志，初始为 false

	// 获取 公司群id
	queryEmployeeModel.setQuery(QString("SELECT departmentID FROM tab_department WHERE department_name = '%1'")
		.arg(QStringLiteral("公司群")));
	QModelIndex companyIndex = queryEmployeeModel.index(0, 0);
	QString strCompanyID = queryEmployeeModel.data(companyIndex).toString();

	// 判断，当前窗口ID，是不是公司群
	if (strTalkID == strCompanyID)	//是公司群
	{
		isGroupTalk = true;
		// 把公司群没有注销的【员工ID和头像】，全提取出来
		queryEmployeeModel.setQuery("SELECT employeeID,picture FROM tab_employees WHERE status = 1");
	}
	else	//不是公司群
	{
		if (strTalkID.length() == 4)	// 部门群
		{
			isGroupTalk = true;
			// 把部门群没有注销的【员工ID和头像】，全提取出来
			queryEmployeeModel.setQuery(QString("SELECT employeeID,picture FROM tab_employees WHERE status = 1 AND departmentID = %1").arg(strTalkID));
		}
		else  // 单聊
		{
			// 获取 对方头像
			queryEmployeeModel.setQuery(QString("SELECT picture FROM tab_employees WHERE status = 1 AND employeeID = %1").arg(strTalkID));
			// 通过索引，找出图片路径，并转成 字符串
			QModelIndex index = queryEmployeeModel.index(0, 0);
			strPicturePath = queryEmployeeModel.data(index).toString();

			// 构建网页对象
			MsgHtmlObj* msgHtmlObj = new MsgHtmlObj(this, strPicturePath);
			// 注册
			strExternal = "external_" + strTalkID;
			m_channel->registerObject(strExternal, msgHtmlObj);
		}
	}

	// 进行群聊处理
	if (isGroupTalk)
	{
		QModelIndex employeeModelIndex, pictureModelIndex;
		// 模型总行数
		int rows = queryEmployeeModel.rowCount();
		// 遍历群聊，每个员工的气泡都要初始化
		for (int i = 0; i < rows; i++)
		{
			employeeModelIndex = queryEmployeeModel.index(i, 0);	// 群成员QQ号/ID，索引
			pictureModelIndex = queryEmployeeModel.index(i, 1);		// 群成员头像路径，索引

			// 获取群成员 ID
			strEmployeeID = queryEmployeeModel.data(employeeModelIndex).toString();
			// 获取群成员 头像路径
			strPicturePath = queryEmployeeModel.data(pictureModelIndex).toString();

			// 构建网页对象
			MsgHtmlObj* msgHtmlObj = new MsgHtmlObj(this, strPicturePath);
			// 注册
			strExternal = "external_" + strEmployeeID;
			m_channel->registerObject(strExternal, msgHtmlObj);
		}
	}

	/*----气泡模板设置完成，将通道设置到页面----*/
	this->page()->setWebChannel(m_channel);

	//加载基础页面 msgTmpl.html
	this->load(QUrl("qrc:/Resources/MainWindow/MsgHtml/msgTmpl.html"));
}

MsgWebView::~MsgWebView()
{}

void MsgWebView::appendMsg(const QString & html, QString strObj)
{
	QJsonObject msgObj;	//json对象
	QString qsMsg;	//json对象中MSG对应的value值

	const QList<QStringList> msgLst = parseHtml(html);	//解析传入的html

	int imageNum = 0;	//表情包数量
	int msgType = 1;	//要发送的消息类型，0表示表情包，1表示文本，2表示文件
	bool isImageMsg = false;	//发送的是否为表情包
	QString strData;	//发送的消息格式化后的数据

	for (int i = 0; i < msgLst.size(); i++) {	//遍历解析后的列表
		if (msgLst.at(i).at(0) == "img") {	//当前项是img标签，即用户要发送的是表情包
			QString imagePath = msgLst.at(i).at(1);	//获取表情包url

			//表情消息初步格式化
			QString strEmotionName = QFileInfo(imagePath).baseName();
			strEmotionName = strEmotionName.rightJustified(3, '0');
			strData += strEmotionName;
			msgType = 0;	//消息类型，用于tcp通信传递给接收方
			imageNum++;		//记录表情包个数用于进一步格式化
			isImageMsg = true;	//标记

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

			qsMsg += imgPath;	//拼接value
		}
		else if (msgLst.at(i).at(0) == "text") {	//当前项是text文本，即用户要发送的是文本
			qsMsg += msgLst.at(i).at(1);	//拼接value

			strData = qsMsg;
		}
	}

	msgObj.insert("MSG", qsMsg);	//json对象插入键值对

	const QString& Msg = QJsonDocument(msgObj).toJson(QJsonDocument::Compact);	//把 C++ 的 QJsonObject序列化为紧凑的 JSON 字符串
	if (strObj == "0") {	//发送数据
		this->page()->runJavaScript(QString("appendHtml0(%1)").arg(Msg));	//在页面中执行JS函数appendHtml，将消息渲染为右侧聊天气泡
		if (isImageMsg) {	//表情消息进一步格式化
			strData = QString::number(imageNum) + "images" + strData;
		}
		emit signalSendMsg(strData, msgType);	//将格式化后的数据传递出去进行tcp通信
	}
	else {	//接收数据
		this->page()->runJavaScript(QString("recvHtml_%1(%2)").arg(strObj).arg(Msg));	//在页面中执行JS函数recvHtml，将消息渲染为左侧聊天气泡
	}
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
	const QDomNodeList& list = node.childNodes();	//获取node的子节点集合
	for (int i = 0; i < list.count(); i++) {	//遍历子节点
		const QDomNode& node = list.at(i);
		if (node.isElement()) {	//只处理元素节点，纯文本会被跳过
			const QDomElement& element = node.toElement();	//类型转换，便于判断标签类型
			if (element.tagName() == "img") {	//img标签，提取src属性
				QStringList attributeList;
				attributeList << "img" << element.attribute("src");
				attribute << attributeList;
			}
			if (element.tagName() == "span") {	//span标签，提取文本内容
				QStringList attributeList;
				attributeList << "text" << element.text();
				attribute << attributeList;
			}
			if (node.hasChildNodes()) {
				attribute << parseDocNode(node);	//递归处理
			}
		}
	}
	return attribute;
}
