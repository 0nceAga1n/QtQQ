#pragma once
#include <QProxyStyle>

class CustomProxyStyle : public QProxyStyle
{	//自定义代理样式类
public:
	CustomProxyStyle(QObject* parent) {
		setParent(parent);
	}
	//重写drawPrimitive函数，不画焦点框
	virtual void drawPrimitive(PrimitiveElement element, const QStyleOption* opt, QPainter* painter, const QWidget* widget = 0) const {
		if (PE_FrameFocusRect == element) {
			return;
		}
		else {
			QProxyStyle::drawPrimitive(element, opt, painter, widget);
		}
	}
};

//常用工具类
class CommonUtils
{
public:
	CommonUtils();
public:
	static QPixmap getRoundImage(const QPixmap& src, QPixmap& mask, QSize masksize = QSize(0, 0));	//获取圆头像
	static void loadStyleSheet(QWidget* widget, const QString& sheetName);	//加载样式表
	static void setDefaultSkinColor(const QColor& color);	//用配置文件设置默认皮肤颜色
	static QColor getDefaultSkinColor();	//从配置文件中加载默认皮肤颜色
};

