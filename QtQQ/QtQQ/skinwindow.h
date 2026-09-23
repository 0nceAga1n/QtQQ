#pragma once

#include <QWidget>
#include "basicwindow.h"
#include "ui_skinwindow.h"

//皮肤窗口
class SkinWindow : public BasicWindow
{
	Q_OBJECT

public:
	SkinWindow(QWidget *parent = nullptr);
	~SkinWindow();

	void initControl();

public slots:
	void onShowClose();
private:
	Ui::SkinWindowClass ui;
};

