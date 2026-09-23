#include "qclicklabel.h"

QClickLabel::QClickLabel(QWidget *parent)
	: QLabel(parent)
{}

QClickLabel::~QClickLabel()
{}

void QClickLabel::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {	//左键点击标签时发出信号
		emit clicked();
	}
}

