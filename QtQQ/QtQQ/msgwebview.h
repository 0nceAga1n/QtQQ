#pragma once

#include <QWebEngineView>
#include <QDomNode>
#include <QJsonArray>
#include <QMap>

//HTML 模板管理器
class MsgHtmlObj : public QObject
{
	Q_OBJECT

public:
	MsgHtmlObj(QObject* parent, QString msgLPicPath = "");	//参二为左侧头像地址
	QString buildLeftBubble(const QString& content); // 拼完整左气泡 HTML 
	QString buildRightBubble(const QString& content); // 拼完整右气泡 HTML

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

signals:
	void signalDownloadFile(int index);	//用户点击了文件气泡
	
protected:
	// Chromium 内核每次尝试导航（页面跳转、链接点击等）前都会调用这个回调。
	// 返回 true 允许导航，false 拒绝。
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

	void appendMsg(const QString& html, QString strObj = "0", bool shouldSend = true, const QJsonArray& fileSegments = {});	//参二区分收发；参三控制是否发送；参四为格式好的文件消息，用于发送
	
	int registerFile(const QJsonObject& seg);

private slots:
	void onDownloadFile(int index);	//点击页面文件链接时，将文件打开

private:
	//解析html，返回值为解析后的列表，
	//如果html标签为img，即发送的表情包，则解析为QStringList["img", src]，src对应表情包url
	//如果html标签为span，即发送的文本，则解析为QStringList["text", content]，content对应发送的文本
	QList<QStringList> parseHtml(const QString& html);
	QList<QStringList> parseDocNode(const QDomNode& node);

signals:
	void signalSendMsg(const QJsonArray& segments);	//发送消息时（调用appendMsg()函数）触发信号，将解析后的数据传递给talkwindowshell

private:
	MsgHtmlObj* m_selfHtmlObj; // 自己的右气泡模板 
	QMap<QString, MsgHtmlObj*> m_memberHtmlObjs; // 成员ID -> 左气泡模板

	struct PendingMsg {	//缓存的信息
		QString html;
		QString strObj;
		bool shouldSend;
		QJsonArray fileSegments;
	};
	QList<PendingMsg> m_pendingMsgs;
	bool m_pageLoaded = false;	//页面加载是否完成

	QMap<int, QJsonObject> m_files; // 序号 -> 文件信息 
	int m_nextFileIndex = 0; // 文件序号计数
};

