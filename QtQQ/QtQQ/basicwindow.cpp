#include "basicwindow.h"
#include "commonutils.h"
#include "notifymanager.h"

#include <QFile>
#include <QStyleOption>
#include <QPainter>
#include <QApplication>
#include <QSqlQuery>

extern QString gLoginEmployeeID;

BasicWindow::BasicWindow(QWidget *parent)
	: QDialog(parent)
{
	m_colorBackGround = CommonUtils::getDefaultSkinColor();	//获取默认皮肤颜色
	setWindowFlags(Qt::FramelessWindowHint);	//去掉系统默认边框和标题栏
	setAttribute(Qt::WA_TranslucentBackground, true);	//启用半透明/透明背景

	//利用管理类统一管理所有窗口皮肤颜色
	connect(NotifyManager::getInstance(), SIGNAL(signalSkinChanged(const QColor&)), this, SLOT(onSignalSkinChanged(const QColor&)));
}

BasicWindow::~BasicWindow()
{
}

void BasicWindow::onSignalSkinChanged(const QColor& color)
{
	m_colorBackGround = color;
	loadStyleSheet(m_styleName);
}

void BasicWindow::setTitleBarTitle(const QString& title, const QString& icon)
{
	_titleBar->setTitleContent(title);
	_titleBar->setTitleIcon(icon);
}

void BasicWindow::initTitleBar(ButtonType buttontype)
{
	_titleBar = new TitleBar(this);
	_titleBar->setButtonType(buttontype);
	_titleBar->move(0, 0);

	connect(_titleBar, &TitleBar::signalButtonMinClicked, this, &BasicWindow::onButtonMinClicked);
	connect(_titleBar, &TitleBar::signalButtonMaxClicked, this, &BasicWindow::onButtonMaxClicked);
	connect(_titleBar, &TitleBar::signalButtonRestoreClicked, this, &BasicWindow::onButtonRestoreClicked);
	connect(_titleBar, &TitleBar::signalButtonCloseClicked, this, &BasicWindow::onButtonCloseClicked);
}

void BasicWindow::loadStyleSheet(const QString& sheetName)
{
	m_styleName = sheetName;
	QFile file(":/Resources/QSS/" + sheetName + ".css");
	file.open(QFile::ReadOnly);

	if (file.isOpen()) {
		setStyleSheet("");
		QString qsstyleSheet = QLatin1String(file.readAll());

		QString r = QString::number(m_colorBackGround.red());
		QString g = QString::number(m_colorBackGround.green());
		QString b = QString::number(m_colorBackGround.blue());

		qsstyleSheet += QString("QWidget[titleskin=true]\
								{background-color:rgb(%1,%2,%3);\
								border-top-left-radius:4px;}\
								QWidget[bottomskin=true]\
								{border-top:1px solid rgba(%1,%2,%3,100);\
								background-color:rgba(%1,%2,%3,50);\
								border-bottom-left-radius:4px;\
								border-bottom-right-radius:4px;}")
								.arg(r).arg(g).arg(b);
		setStyleSheet(qsstyleSheet);
	}
	file.close();
}

void BasicWindow::initBackGroundColor()
{
	//确保控件不会丢失系统主题、样式表或默认背景
	QStyleOption opt;
	opt.initFrom(this);

	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);	//调用当前系统/应用风格，绘制 PE_Widget（即 QWidget 的默认背景）
}

//子类化部件时要重写绘图事件设置背景图
void BasicWindow::paintEvent(QPaintEvent* event)
{
	initBackGroundColor();
	QDialog::paintEvent(event);
}

//图像 转 圆头像
QPixmap BasicWindow::getRoundImage(const QPixmap& src, QPixmap& mask, QSize maskSize)
{
	if (maskSize == QSize(0, 0)) {
		maskSize = mask.size();
	}
	else {
		mask = mask.scaled(maskSize, Qt::KeepAspectRatio, Qt::SmoothTransformation); //图像等比缩放，不变形，平滑抗锯齿，不模糊
	}

	//保存转换后的头像
	QImage resultImage(maskSize, QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&resultImage);
	painter.setCompositionMode(QPainter::CompositionMode_Source);	//绘制透明矩形
	painter.fillRect(resultImage.rect(), Qt::transparent);	
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);	//将不透明的空白圆头像覆盖在透明矩形上
	painter.drawPixmap(0, 0, mask);		
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);	//在目标不透明区域（也就是空白圆头像）绘制图片
	painter.drawPixmap(0, 0, src.scaled(maskSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));	
	painter.end();

	return QPixmap::fromImage(resultImage);
}

void BasicWindow::onShowClose(bool)
{
	close();
}

void BasicWindow::onShowMin(bool)
{
	showMinimized();	//最小化到任务栏
}

void BasicWindow::onShowHide(bool)
{
	hide();
}

void BasicWindow::onShowNormal(bool)
{
	show();
	activateWindow();
}

void BasicWindow::onShowQuit(bool)
{
	QSqlQuery sqlUpdate;
	sqlUpdate.prepare("UPDATE tab_employees SET online_status = 1 WHERE employeeID = ?");
	sqlUpdate.addBindValue(gLoginEmployeeID);
	sqlUpdate.exec();

	QApplication::quit();
}

void BasicWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (m_mousePressed && (event->buttons() & Qt::LeftButton)) {
		move(event->globalPosition().toPoint() - m_mousePoint);		//移动后的位置 = 当前鼠标位置 - 按下时鼠标相对窗口位置
		event->accept();
	}
}

void BasicWindow::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_mousePressed = true;
		m_mousePoint = event->globalPosition().toPoint() - pos();	//记录鼠标相对窗口位置
		event->accept();
	}
}

void BasicWindow::mouseReleaseEvent(QMouseEvent* event)
{
	m_mousePressed = false;
}

void BasicWindow::onButtonMinClicked()
{
	if (Qt::Tool == (windowFlags() & Qt::Tool)) {	//工具窗口直接隐藏
		hide();
	}
	else {	//普通窗口最小化到任务栏
		showMinimized();
	}
}

void BasicWindow::onButtonRestoreClicked()
{
	QPoint windowPos;
	QSize windowSize;
	_titleBar->getRestoreInfo(windowPos, windowSize);
	setGeometry(QRect(windowPos, windowSize));
}

void BasicWindow::onButtonMaxClicked()
{
	_titleBar->saveRestoreInfo(pos(), QSize(width(), height()));
	QRect desktopRect = QApplication::primaryScreen()->availableGeometry();	//获取屏幕可用区域（排除任务栏等系统占用）
	QRect factRect = QRect(desktopRect.x() - 3, desktopRect.y() - 3, desktopRect.width() + 6, desktopRect.height() + 6);	//向外扩展 3 像素：左上偏移 -3，宽高各 +6
	setGeometry(factRect);

	// 推荐：直接最大化，让 Qt/系统处理边缘
	//showMaximized();
}

void BasicWindow::onButtonCloseClicked()
{
	close();
}