#include "talkwindowitem.h"
#include "commonutils.h"

TalkWindowItem::TalkWindowItem(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	initControl();
}

TalkWindowItem::~TalkWindowItem()
{}

void TalkWindowItem::setHeadPixmap(const QPixmap& pixmap)
{
	//设置头像
	QPixmap mask = QPixmap(":/Resources/MainWindow/head_mask.png");

	const QPixmap& headpixmap = CommonUtils::getRoundImage(pixmap, mask, ui.headlabel->size());
	ui.headlabel->setPixmap(headpixmap);
}

void TalkWindowItem::setMsgLabelContent(const QString& msg)
{
	ui.msgLabel->setText(msg);
}

QString TalkWindowItem::getMsgLabelText()
{
	return ui.msgLabel->text();
}

void TalkWindowItem::initControl()
{
	ui.tclosebtn->setVisible(false);	//初始按钮不可见，鼠标进入控件后按钮可见
	connect(ui.tclosebtn, SIGNAL(clicked(bool)), this, SIGNAL(signalCloseClicked()));	//点击关闭按钮时发出信号
}

void TalkWindowItem::enterEvent(QEnterEvent* event)
{
	//鼠标进入控件后按钮可见
	ui.tclosebtn->setVisible(true);
	__super::enterEvent(event);
}

void TalkWindowItem::leaveEvent(QEvent* event)
{
	//鼠标离开控件后按钮不可见
	ui.tclosebtn->setVisible(false);
	__super::leaveEvent(event);
}

void TalkWindowItem::resizeEvent(QResizeEvent* event)
{
	__super::resizeEvent(event);
}