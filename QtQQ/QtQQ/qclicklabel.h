#pragma once

#include <QLabel>
#include <QMouseEvent>

//自定义能发送点击信号的标签
class QClickLabel  : public QLabel
{
	Q_OBJECT

public:
	QClickLabel(QWidget *parent);
	~QClickLabel();

protected:
	void mousePressEvent(QMouseEvent* event);

signals:
	void clicked();
};

