#include "userlogin.h"
#include "CCMainWindow.h"
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

QString gLoginEmployeeID;	//登录者账号

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

	if (!connectMysql()) {
		QMessageBox::information(NULL, QString::fromUtf8("提示"),
			QString::fromUtf8("连接数据库失败"));
		close();
	}
}

bool UserLogin::connectMysql()
{
	QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
	db.setDatabaseName("qt_qq");
	db.setHostName("localhost");
	db.setUserName("root");
	db.setPassword("kzk");
	db.setPort(3306);

	if (db.open()) {
		return true;
	}
	else {
		qDebug() << db.lastError().text();
		return false;
	}
}

bool UserLogin::veryfyAccountCode(bool& isAccountLogin, QString& strAccount)
{
	QString strAccountInput = ui.editUserAccount->text();
	QString strCodeInput = ui.editPassword->text();

	//员工号登录
	QString strSqlCode = QString("SELECT code FROM tab_accounts WHERE employeeID = %1").arg(strAccountInput);
	QSqlQuery queryEmployeeID(strSqlCode);
	queryEmployeeID.exec();
	if (queryEmployeeID.first()) {	//输入的账号在数据库中能找到
		QString strCode = queryEmployeeID.value(0).toString();
		if (strCode == strCodeInput) {	//密码正确
			gLoginEmployeeID = strAccountInput;
			isAccountLogin = false;
			strAccount = strAccountInput;
			return true;
		}
		else {
			return false;
		}
	}

	//账号登录
	strSqlCode = QString("SELECT code, employeeID FROM tab_accounts WHERE account = '%1'").arg(strAccountInput);
	QSqlQuery queryAccount(strSqlCode);
	queryAccount.exec();
	if (queryAccount.first()) {
		QString strCode = queryAccount.value(0).toString();

		if (strCode == strCodeInput) {
			gLoginEmployeeID = queryAccount.value(1).toString();
			isAccountLogin = true;
			strAccount = strAccountInput;
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}

void UserLogin::onLoginBtnClicked()
{
	bool isAccountLogin;
	QString strAccount;

	//点击登录按钮，登录窗口关闭，创建聊天主窗口
	if (!veryfyAccountCode(isAccountLogin, strAccount)) {
		QMessageBox::information(NULL, QString::fromUtf8("提示"),
			QString::fromUtf8("您输入的账号或密码有误，请重新输入！"));
		return;
	}

	QSqlQuery sqlUpdate;
	sqlUpdate.prepare("UPDATE tab_employees SET online = 2 WHERE employeeID = ?");
	sqlUpdate.addBindValue(gLoginEmployeeID);
	sqlUpdate.exec();

	close();
	CCMainWindow* mainWindow = new CCMainWindow(strAccount, isAccountLogin);
	mainWindow->show();
}
