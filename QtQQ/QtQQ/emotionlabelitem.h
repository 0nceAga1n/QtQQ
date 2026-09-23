#pragma once

#include "qclicklabel.h"

//自定义表情标签，用来放表情包
class EmotionLabelItem  : public QClickLabel
{
	Q_OBJECT

public:
	EmotionLabelItem(QWidget *parent);
	~EmotionLabelItem();
	void setEmotionName(int emotionName);

private:
	void initControl();

signals:
	void emotionClicked(int emotionNum);	//表情包点击信号，传递到表情包窗口

private:
	int m_emotionName;
	QMovie* m_apngMovie;
};

