#include "userlogin.h"
#include "CCMainWindow.h"
#include <QMessageBox>

QString gLoginEmployeeID;	//登录者账号
QString gLoginToken;	//登录token

UserLogin::UserLogin(QWidget *parent)
	: BasicWindow(parent)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	//初始化标题栏
	initTitleBar();
	setTitleBarTitle("", ":/Resources/MainWindow/qqlogoclassic.png");
	//加载登录窗口样式
	loadStyleSheet("UserLogin");
	//初始化控件
	initControl();
}

UserLogin::~UserLogin()
{}

void UserLogin::initControl()
{
	QLabel* headlabel = new QLabel(this);	//圆头像
	headlabel->setFixedSize(68, 68);
	QPixmap pix(":/Resources/MainWindow/head_mask.png");
	headlabel->setPixmap(getRoundImage(QPixmap(":/Resources/MainWindow/app/logo.ico"), pix, headlabel->size()));
	headlabel->move(width() / 2 - 34, ui.titleWidget->height() - 34);

	connect(ui.loginBtn, &QPushButton::clicked, this, &UserLogin::onLoginBtnClicked);

	initLoginSocket();	//建立登录连接
}

void UserLogin::initLoginSocket()
{
	m_loginSocket = new QTcpSocket(this);
	//收到登录反馈时进行处理
	connect(m_loginSocket, &QTcpSocket::readyRead, this, [this]() {
		QByteArray buffer = m_loginSocket->readAll();
		const QList<QJsonObject>& frames = m_decoder.push(buffer);
		for (const QJsonObject& obj : frames) {
			if (obj.value("cmd").toString() == "auth_result") {
				handleAuthResult(obj);
			}
		}
	});

	m_loginSocket->connectToHost("127.0.0.1", MsgProtocol::TCP_PORT);
}

void UserLogin::handleAuthResult(const QJsonObject& obj)
{
	bool ok = obj.value("ok").toBool();
	if (!ok) {
		QMessageBox::information(NULL, QString::fromUtf8("提示"), QString::fromUtf8("您输入的账号或密码有误，请重新输入！"));
		ui.loginBtn->setEnabled(true);
		return;
	}

	gLoginToken = obj.value("token").toString();
	gLoginEmployeeID = obj.value("employeeID").toString();	//登录者ID
	QString loginPicture = obj.value("picture").toString();	//登录者头像路径
	QJsonArray departments = obj.value("departments").toArray();	//CCMainWindow的联系树

	close();
	CCMainWindow* mainWindow = new CCMainWindow(loginPicture, departments);
	mainWindow->show();
}

void UserLogin::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		onLoginBtnClicked();
	}
	else {
		BasicWindow::keyPressEvent(event);
	}
}

void UserLogin::onLoginBtnClicked()
{
	QString account = ui.editUserAccount->text().trimmed();
	QString code = ui.editPassword->text();

	if (account.isEmpty() || code.isEmpty()) {
		return;
	}

	if (m_loginSocket->state() != QAbstractSocket::ConnectedState) {
		QMessageBox::information(NULL, QString::fromUtf8("提示"), QString::fromUtf8("正在连接服务器，请稍后重试"));
		return;
	}

	QJsonObject obj;
	obj.insert("cmd", "auth");
	obj.insert("account", account);
	obj.insert("code", code);
	m_loginSocket->write(MsgProtocol::pack(obj));

	ui.loginBtn->setEnabled(false);
}
