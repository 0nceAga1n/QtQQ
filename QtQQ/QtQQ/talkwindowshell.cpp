#include "talkwindowshell.h"
#include "commonutils.h"
#include "talkwindow.h"
#include <QSqlQueryModel>
#include <QMessageBox>
#include <QFile>	
#include <QSqlQuery>
#include "windowmanager.h"
#include "qmsgtextedit.h"
#include "sendfile.h"
#include "receivefile.h"

QString gfileName;
QString gfileData;

const int gtcpPort = 8888;	//通信端口
const int gudpPort = 6666;
extern QString gLoginEmployeeID;

TalkWindowShell::TalkWindowShell(QWidget *parent)
	: BasicWindow(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	initControl();

	initTcpSocket();	//初始化tcp通信
	initUdpSocket();	//初始化udp通信

	//首次打开聊天窗口时更新js脚本
	QFile file("Resources/MainWindow/MsgHtml/msgTmpl.js");
	if (!file.size()) {
		QStringList employeesIDList;
		getEmployeesID(employeesIDList);
		if (!createJSFile(employeesIDList)) {
			QMessageBox::information(this, QString::fromUtf8("提示"),
				QString::fromUtf8("更新js文件失败"));
		}
	}
}

TalkWindowShell::~TalkWindowShell()
{
	delete m_emotionWindow;
	m_emotionWindow = nullptr;
}

void TalkWindowShell::addTalkWindow(TalkWindow* talkWindow, TalkWindowItem* talkWindowItem, const QString uid)
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
	QSqlQueryModel sqlDepModel;
	QString strQuery = QString("SELECT picture FROM tab_department WHERE departmentID = %1").arg(uid);	//群聊
	sqlDepModel.setQuery(strQuery);
	int rows = sqlDepModel.rowCount();

	if (rows == 0) {	//单聊
		strQuery = QString("SELECT picture FROM tab_employees WHERE employeeID = %1").arg(uid);
		sqlDepModel.setQuery(strQuery);
	}
	QModelIndex index;
	index = sqlDepModel.index(0, 0);
	QImage img;
	img.load(sqlDepModel.data(index).toString());

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
	//建立客户端连接
	m_tcpClientSocket = new QTcpSocket(this);
	m_tcpClientSocket->connectToHost("127.0.0.1", gtcpPort);
}

void TalkWindowShell::initUdpSocket()
{
	m_udpReceiver = new QUdpSocket(this);
	for (quint16 port = gudpPort; port < gudpPort + 200; port++) {
		if (m_udpReceiver->bind(port, QUdpSocket::ShareAddress))
			break;
	}
	connect(m_udpReceiver, &QUdpSocket::readyRead, this, &TalkWindowShell::processPendingData);
}

void TalkWindowShell::getEmployeesID(QStringList& employeeIDList)
{
	//获取所有员工id
	QSqlQueryModel queryModel;
	queryModel.setQuery("SELECT employeeID FROM tab_employees WHERE status = 1");
	int employeesNum = queryModel.rowCount();
	QModelIndex index;
	for (int i = 0; i < employeesNum; i++) {
		index = queryModel.index(i, 0);
		employeeIDList << queryModel.data(index).toString();
	}
}

bool TalkWindowShell::createJSFile(QStringList& employeeList)
{
	//打开js脚本模板文件，用于替换
	QString strFileTxt = "Resources/MainWindow/MsgHtml/msgtmpl.txt";
	QFile fileRead(strFileTxt);
	QString strFile;	//js脚本模板内容
	if (fileRead.open(QIODevice::ReadOnly)) {	//打开成功并读取
		strFile = fileRead.readAll();
		fileRead.close();
	}
	else {	//打开失败
		QMessageBox::information(this, QString("Tips"), QString("msgtmpl.txt false"));
		return false;
	}

	//打开要写入的js脚本，将替换后的js脚本模板写入
	QFile fileWrite("Resources/MainWindow/MsgHtml/msgtmpl.js");
	if (fileWrite.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		//模板里要替换的4个部分
		QString strSourceInitNull = "var external = null;";
		QString strSourceInit = "external = channel.objects.external;";
		QString strSourceNew = "new QWebChannel(qt.webChannelTransport,\
			function(channel) {\
			external = channel.objects.external;\
		}\
			); ";
		QString strSourceRecvHtml;	//不好直接拼写，从txt文本中获取
		QFile fileRecvHtml("Resources/MainWindow/MsgHtml/recvHtml.txt");
		if (fileRecvHtml.open(QIODevice::ReadOnly)) {
			strSourceRecvHtml = fileRecvHtml.readAll();
			fileRecvHtml.close();
		}
		else {
			QMessageBox::information(this, QString("Tips"), QString("recvHtml.txt false"));
			return false;
		}

		//替换后的内容
		QString strReplaceInitNull;
		QString strReplaceInit;
		QString strReplaceNew;
		QString strReplaceRecvHtml;
		for (int i = 0; i < employeeList.length(); i++) {
			QString strInitNull = strSourceInitNull;
			strInitNull.replace("external", QString("external_%1").arg(employeeList.at(i)));
			strReplaceInitNull += strInitNull;
			strReplaceInitNull += "\n";

			QString strInit = strSourceInit;
			strInit.replace("external", QString("external_%1").arg(employeeList.at(i)));
			strReplaceInit += strInit;
			strReplaceInit += "\n";

			QString strRecvHtml = strSourceRecvHtml;
			strRecvHtml.replace("external", QString("external_%1").arg(employeeList.at(i)));
			strRecvHtml.replace("recvHtml", QString("recvHtml_%1").arg(employeeList.at(i)));
			strReplaceRecvHtml += strRecvHtml;
			strReplaceRecvHtml += "\n";
		}

		//进行替换
		strFile.replace(strSourceInitNull, strReplaceInitNull);
		strFile.replace(strSourceInit, strReplaceInit);
		strFile.replace(strSourceNew, strReplaceNew);
		strFile.replace(strSourceRecvHtml, strReplaceRecvHtml);

		//写入js脚本
		QTextStream stream(&fileWrite);
		stream << strFile;
		fileWrite.close();

		return true;
	}
	else {
		QMessageBox::information(this, QString("Tips"), QString("msgtmpl.js false"));
		return false;
	}
}

void TalkWindowShell::handleReceivedMsg(int senderEmployeeID, int msgType, QString strMsg)
{
	QMsgTextEdit msgTextEdit;	// 自定义的文本编辑器
	msgTextEdit.setText(strMsg);

	if (msgType == 1)	// 文本信息
	{
		msgTextEdit.document()->toHtml();
	}
	else if (msgType == 0)	// 表情信息
	{
		const int emotionWidth = 3; // 每个表情所占宽度
		int emotionNum = strMsg.length() / emotionWidth; // 计算表情数量，数据长度 除以 每个表情宽度

		// 遍历数据中的 表情，添加html里去
		for (int i = 0; i < emotionNum; i++)
		{
			msgTextEdit.addEmotionUrl(strMsg.mid(i * 3, emotionWidth).toInt());
		}
	}

	QString htmlText = msgTextEdit.document()->toHtml();
	if (!htmlText.contains(".png") && !htmlText.contains("</span>"))
	{
		QString fontHtml;
		QFile file(":/Resources/MainWindow/MsgHtml/msgFont.txt");
		if (file.open(QIODevice::ReadOnly))
		{
			fontHtml = file.readAll();
			// 将html文件里的 %1，用字符串 text 替换
			fontHtml.replace("%1", strMsg);
			file.close();
		}
		else
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"),
				QString::fromLocal8Bit("文件 msgFont.txt 不存在！"));
			return;
		}

		// 判断转换后，有没有包含 fontHtml
		if (!htmlText.contains(fontHtml))
		{
			htmlText.replace(strMsg, fontHtml);
		}
	}

	//在当前窗口的网页上添加消息
	TalkWindow* talkWindow = dynamic_cast<TalkWindow*>(ui.rightStackedWidget->currentWidget());
	talkWindow->ui.msgWidget->appendMsg(htmlText, QString::number(senderEmployeeID));
}

void TalkWindowShell::updateSendTcpMsg(QString& strData, int& msgType, QString fileName)
{
	//获取发送消息的是哪个窗口
	TalkWindow* curTalkWindow = dynamic_cast<TalkWindow*>(ui.rightStackedWidget->currentWidget());
	QString talkId = curTalkWindow->getTalkId();

	/*-----------将要发送的消息进行封装以便于解析--------------*/
	QString strGroupFlag;	//区分单聊还是群聊
	if (talkId.length() == 4) {	//群聊
		strGroupFlag = "1";
	}
	else {	//单聊
		strGroupFlag = "0";
	}

	QString strSend;	//封装后的消息
	if (msgType == 1) {		//文本信息
		int dataLength = strData.length();	//得到原信息的长度
		QString strDataLength = QString::number(dataLength).rightJustified(5, '0');	//格式化文本信息长度，不足五位数则在左侧补0

		strSend = strGroupFlag + gLoginEmployeeID + talkId + "1" + strDataLength + strData;
	}
	else if (msgType == 0) {	//表情包信息
		strSend = strGroupFlag + gLoginEmployeeID + talkId + "0" + strData;
	}
	else if (msgType == 2) {	//文件信息
		QString strLength = QString::number(strData.toUtf8().length());	//获取文件内容长度

		strSend = strGroupFlag + gLoginEmployeeID + talkId + "2" + strLength + "bytes" + fileName + "data_begin" + strData;
	}

	//将消息转化类型后发送到服务端
	QByteArray dataBt;
	dataBt.resize(strSend.length());
	dataBt = strSend.toUtf8();
	m_tcpClientSocket->write(dataBt);
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

void TalkWindowShell::processPendingData()
{
	// 端口中，有未处理的数据
	while (m_udpReceiver->hasPendingDatagrams())
	{
		const static int groupFlagWidth = 1;	// 群聊标志宽度，占1位
		const static int groupWidth = 4;		// 群QQ号宽度，占4位
		const static int employeeWidth = 5;		// 员工QQ号宽度
		const static int msgTypeWidth = 1;		// 信息类型宽度
		const static int msgLengthWidth = 5;	// 文本数据宽度，最多占5位
		const static int pictureWidth = 3;		// 表情图片宽度

		QByteArray btData;
		btData.resize(m_udpReceiver->pendingDatagramSize());	// 获取 即将要处理数据的大小
		m_udpReceiver->readDatagram(btData.data(), btData.size());	// 读取UDP数据（数据，大小）
		QString strData = btData.data();	// 将原数据保存到 QString类型中，用于解析

		QString strWindowID;	//聊天窗口ID
		QString strSendEmployeeID, strRecevieID;	// 解析后得到的 发送端 和 接收端的id
		QString strMsg;		// 解析后数据
		int msgType;		// 数据类型

		//获取 发送端的QQ号
		strSendEmployeeID = strData.mid(groupFlagWidth, employeeWidth);
		//自己发送的消息，服务器广播给了自己
		if (strSendEmployeeID == gLoginEmployeeID)
		{
			continue;		// 直接返回,处理下一条数据
		}

		// 单聊和群聊分开处理
		if (btData[0] == '1')	//群聊
		{
			// 获取群聊ID
			strRecevieID = strData.mid(groupFlagWidth + employeeWidth, groupWidth);
			strWindowID = strRecevieID;	//聊天窗口ID就是群聊ID

			//获取消息类型，不同消息类型做不同解析
			QChar cMsgType = btData[groupFlagWidth + employeeWidth + groupWidth];
			if (cMsgType == '1')	//文本信息
			{
				msgType = 1;
				// 获取文本信息 长度
				int msgLength = strData.mid(groupFlagWidth + employeeWidth + groupWidth + msgTypeWidth, msgLengthWidth).toInt();
				// 获取 数据包里的 文本数据
				strMsg = strData.mid(groupFlagWidth + employeeWidth + groupWidth + msgTypeWidth + msgLengthWidth, msgLength);
			}
			else if (cMsgType == '0')	//表情消息
			{
				msgType = 0;
				// 找到原数据里“images”位置
				int posImages = strData.indexOf("images");
				// 获取 数据包里的 表情包数据
				strMsg = strData.right(strData.length() - posImages - QString("images").length());
			}
			else if (cMsgType == '2')	//文件信息
			{
				msgType = 2;

				// 计算 bytes 的长度
				int bytesWidth = QString("bytes").length();
				// bytes，第一次出现的位置
				int posBytes = strData.indexOf("bytes");
				// data_begin，第一次出现的位置
				int posData_begin = strData.indexOf("data_begin");

				// 获取 文件名称
				QString fileName = strData.mid(posBytes + bytesWidth, posData_begin - posBytes - bytesWidth);
				// 将解析出来的 文件名称，赋值给全局变量
				gfileName = fileName;

				// 文件内容起始位置
				int posData = posData_begin + QString("data_begin").length();
				// 获取文件内容
				strMsg = strData.mid(posData);
				// 将解析出来的 文件内容，赋值给全局变量
				gfileData = strMsg;

				// 根据employeeID获取发送者姓名
				QString sender;
				int empID = strSendEmployeeID.toInt();
				QSqlQuery querySenderName(QString("SELECT employee_name FROM tab_employees WHERE employeeID = %1").arg(empID));
				querySenderName.exec();
				if (querySenderName.first())
				{
					sender = querySenderName.value(0).toString();
				}

				// 接收文件的后续操作...
				ReceiveFile* recvFile = new ReceiveFile(this);

				// 用了点了取消，发送 返回信号
				connect(recvFile, &ReceiveFile::refuseFile, [this]() {
						return;
				});

				// 收到xxx的信息，将文本字符串，设置到标签
				QString msgLabel = QString::fromUtf8("收到") + sender + QString::fromUtf8("发来的文件，是否接收？");
				recvFile->setMsg(msgLabel);
				recvFile->show();
			}
		}
		else // 单聊
		{
			// 获取接收者的QQ号
			strRecevieID = strData.mid(groupFlagWidth + employeeWidth, employeeWidth);
			strWindowID = strSendEmployeeID;	//聊天窗口ID是发送者的id

			// 不是我的信息，不做处理
			// 接收者的ID 和 登陆者的ID ，不是一样的，则直接返回
			if (strRecevieID != gLoginEmployeeID)
			{
				continue;
			}

			// 获取信息的类型
			QChar cMsgType = btData[groupFlagWidth + employeeWidth + employeeWidth];
			// 判断信息类型
			if (cMsgType == '1')	//文本信息
			{
				msgType = 1;
				// 提取，文本信息的长度
				int msgLength = strData.mid(groupFlagWidth + employeeWidth + employeeWidth + msgTypeWidth, msgLengthWidth).toInt();
				// 文本信息
				strMsg = strData.mid(groupFlagWidth + employeeWidth + employeeWidth + msgTypeWidth + msgLengthWidth, msgLength);
			}
			else if (cMsgType == '0')	// 表情信息
			{
				msgType = 0;
				int posImages = strData.indexOf("images");
				// 获取 数据包里的 表情包数据
				strMsg = strData.mid(posImages + QString("images").length());
			}
			else if (cMsgType == '2')	// 文件信息
			{
				msgType = 2;

				int bytesWidth = QString("bytes").length();
				int posBytes = strData.indexOf("bytes");
				int data_beginWidth = QString("data_begin").length();
				int posData_begin = strData.indexOf("data_begin");

				// 文件名称
				QString fileName = strData.mid(posBytes + bytesWidth, posData_begin - posBytes - bytesWidth);
				gfileName = fileName;

				// 文件内容
				strMsg = strData.mid(posData_begin + data_beginWidth);
				gfileData = strMsg;

				// 根据employeeID获取发送者姓名
				QString sender;
				int empID = strSendEmployeeID.toInt();	// 转换成整形
				QSqlQuery querySenderName(QString("SELECT employee_name FROM tab_employees WHERE employeeID = %1").arg(empID));
				querySenderName.exec();
				if (querySenderName.first())
				{
					sender = querySenderName.value(0).toString();
				}

				// 接收文件的后续操作...
				ReceiveFile* recvFile = new ReceiveFile(this);

				// 用了点了取消，发送 返回信号
				connect(recvFile, &ReceiveFile::refuseFile, [this]() {
						return;
				});

				// 收到xxx的信息，将文本字符串，设置到标签上
				QString msgLabel = QString::fromUtf8("收到") + sender + QString::fromUtf8("发来的文件，是否接收？");
				recvFile->setMsg(msgLabel);
				recvFile->show();
			}
		}


		/*---------------用户没打开窗口就收不到消息，bug，待优化----------------*/
		// 将聊天窗口，设为活动的窗口
		QWidget* widget = WindowManager::getInstance()->findWindowName(strWindowID);
		// 判断窗口是否打开
		if (widget)
		{
			// 已存在，就设为活动窗口
			this->setCurrentWidget(widget);

			// 将左侧聊天列表，同步激活
			QListWidgetItem* item = m_talkwindowItemMap.key(widget);
			item->setSelected(true);	// 设为选中，活动状态
		}
		else
		{
			return;		// 不存在，直接返回
		}
		/*---------------用户没打开窗口就收不到消息，bug，待优化----------------*/


		// 对信息类型做判断，如果是文件类型，则不调用 handleReceivedMsg()
		if (msgType != 2)
		{
			int sendEmployeeID = strSendEmployeeID.toInt();
			// "网页"上追加数据
			handleReceivedMsg(sendEmployeeID, msgType, strMsg);
		}
	}
}