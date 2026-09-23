#pragma once

#include <QWidget>
#include "ui_emotionwindow.h"

//自定义表情窗口
class EmotionWindow : public QWidget
{
	Q_OBJECT

public:
	EmotionWindow(QWidget *parent = nullptr);
	~EmotionWindow();

private:
	void initControl();

private slots:
	void addEmotion(int emotionNum);	//emotionlabelitem发来点击信号时发生该槽函数

signals:
	void signalEmotionWindowHide();	//表情窗口隐藏，传递信号给聊天窗口talkwindowshell
	void signalEmotionItemClicked(int emotionNum);	//表情选项点击了，传递信号给聊天窗口talkwindowshell

private:
	void paintEvent(QPaintEvent* event) override;

private:
	Ui::EmotionWindowClass ui;
};

