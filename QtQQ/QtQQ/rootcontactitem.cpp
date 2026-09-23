#include "rootcontactitem.h"
#include <QPainter>

RootContactItem::RootContactItem(bool hasArrow, QWidget* parent)
	: QLabel(parent), m_rotation(0), m_hasArrow(hasArrow)
{
	setFixedHeight(32);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	
	//初始化动画，和动态属性绑定，动画根据字符串找到属性
	m_animation = new QPropertyAnimation(this, "rotation");
	m_animation->setDuration(50);	//动画时间
	m_animation->setEasingCurve(QEasingCurve::InQuad);	//动画速度曲线
}

RootContactItem::~RootContactItem()
{}

void RootContactItem::setText(const QString & title)
{
	m_titleText = title;
	update();
}

void RootContactItem::setExpanded(bool expand)
{
	//展开或折叠根项，则开始动画
	if (expand) {
		m_animation->setEndValue(90);
	}
	else {
		m_animation->setEndValue(0);
	}
	m_animation->start();
}

int RootContactItem::rotation()
{
	return m_rotation;
}

void RootContactItem::setRotation(int rotation)
{
	m_rotation = rotation;
	update();
}

void RootContactItem::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::TextAntialiasing, true);

	//绘制字体
	QFont font;
	font.setPointSize(10);
	painter.setFont(font);
	painter.drawText(24, 0, width() - 24, height(), Qt::AlignLeft | Qt::AlignVCenter, m_titleText);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.save();

	//绘制箭头
	if (m_hasArrow) {
		QPixmap pixmap;
		pixmap.load(":/Resources/MainWindow/arrow.png");

		QPixmap tmpPixmap(pixmap.size());
		tmpPixmap.fill(Qt::transparent);
		//先在临时画布上绘制指定角度的箭头
		QPainter p(&tmpPixmap);
		p.setRenderHint(QPainter::SmoothPixmapTransform, true);

		p.translate(pixmap.width() / 2, pixmap.height() / 2);
		p.rotate(m_rotation);
		p.drawPixmap(0 - pixmap.width() / 2, 0 - pixmap.height() / 2, pixmap);
		//再将绘制好的箭头绘制到RootContactItem
		painter.drawPixmap(6, (height() - pixmap.height()) / 2, tmpPixmap);
		painter.restore();
	}
	QLabel::paintEvent(event);
}

