#include "emotionwindow.h"
#include "commonutils.h"
#include "emotionlabelitem.h"
#include <QStyleOption>
#include <QPainter>

//表情包14列12行
const int emotionColumn = 14;
const int emotionRow = 12;

EmotionWindow::EmotionWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_DeleteOnClose);

	initControl();
}

EmotionWindow::~EmotionWindow()
{}

void EmotionWindow::initControl()
{
	CommonUtils::loadStyleSheet(this, "EmotionWindow");
	for (int row = 0; row < emotionRow; row++) {	//表情添加到表情窗口布局
		for (int column = 0; column < emotionColumn; column++) {
			EmotionLabelItem* label = new EmotionLabelItem(this);
			label->setEmotionName(row * emotionColumn + column);

			//每个表情都进行信号连接
			connect(label, &EmotionLabelItem::emotionClicked, this, &EmotionWindow::addEmotion);
			ui.gridLayout->addWidget(label, row, column);
		}
	}
}

void EmotionWindow::paintEvent(QPaintEvent* event)
{
	QStyleOption opt;
	opt.initFrom(this);
	QPainter painter(this);

	style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
	
	__super::paintEvent(event);
}

void EmotionWindow::addEmotion(int emotionNum)
{
	hide();
	emit signalEmotionWindowHide();	//通知聊天窗口talkwindowshell表情窗口隐藏了
	emit signalEmotionItemClicked(emotionNum);	//通知聊天窗口talkwindowshell点击了表情
}