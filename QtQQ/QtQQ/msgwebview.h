#pragma once

#include <QWebEngineView>
#include <QDomNode>

//HTML 模板管理器
//将消息气泡的 HTML 模板字符串通过 Q_PROPERTY 暴露给 JavaScript，让 JS 端能直接访问 C++ 对象的属性。
class MsgHtmlObj : public QObject
{
	Q_OBJECT
	//MEMBER 表示直接关联成员变量，NOTIFY 指定属性变化时发出的信号
	Q_PROPERTY(QString msgLHtmlTmpl MEMBER m_msgLHtmlTmpl NOTIFY signalMsgHtml)
	Q_PROPERTY(QString msgRHtmlTmpl MEMBER m_msgRHtmlTmpl NOTIFY signalMsgHtml)

public:
	MsgHtmlObj(QObject* parent, QString msgLPicPath = "");	//参二为左侧头像地址

signals:
	void signalMsgHtml(const QString html);
	
private:
	void initHtmlTmpl();	//初始化消息气泡的html模板
	QString getMsgTmplHtml(const QString& code);	//传入文件名 code（不含扩展名），返回文件内容
	QString toWebUrl(const QString& path);	//地址转换

private:
	QString m_msgLPicPath;	//左侧头像
	QString m_msgLHtmlTmpl;
	QString m_msgRHtmlTmpl;
};

//自定义页面
//重写导航请求过滤，阻止所有外部 URL 导航，只允许 qrc:// 内部资源。
class MsgWebPage : public QWebEnginePage
{
	Q_OBJECT

public:
	MsgWebPage(QObject* parent = nullptr)
		: QWebEnginePage(parent) 
	{}
	
protected:
	// Chromium 内核每次尝试导航（页面跳转、链接点击等）前都会调用这个回调。
	// 返回 true 允许导航，false 拒绝。
	// 这里只放行 qrc 协议（Qt 内部资源），其他一律拒绝——防止用户点击消息中的恶意链接导致浏览器跳转或加载外部资源。
	bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame);
};

//自定义消息窗口，本质是一个内嵌的迷你浏览器
//接收 HTML 消息并渲染成聊天气泡
class MsgWebView  : public QWebEngineView
{
	Q_OBJECT

public:
	MsgWebView(QWidget *parent);
	~MsgWebView();

	void appendMsg(const QString& html, QString strObj = "0");	//参二用于区分是接收数据还是发送数据

private:
	//解析html，返回值为解析后的列表，
	//如果html标签为img，即发送的表情包，则解析为QStringList["img", src]，src对应表情包url
	//如果html标签为span，即发送的文本，则解析为QStringList["text", content]，content对应发送的文本
	QList<QStringList> parseHtml(const QString& html);
	QList<QStringList> parseDocNode(const QDomNode& node);

signals:
	void signalSendMsg(QString& strData, int& msgType, QString sfile = "");	//发送消息时（调用appendMsg()函数）触发信号，将解析后的数据传递给talkwindowshell

private:
	MsgHtmlObj* m_msgHtmlObj;	//HTML 模板管理对象，通过 QWebChannel 注册给 JS
	QWebChannel* m_channel;
};

