#pragma once

#include <QLabel>
#include <QPropertyAnimation>

//自定义根项控件
class RootContactItem  : public QLabel
{
	Q_OBJECT

	Q_PROPERTY(int rotation READ rotation WRITE setRotation)	//动态属性，将m_rotation与rotation绑定
public:
	RootContactItem(bool hasArrow = true, QWidget *parent = nullptr);
	~RootContactItem();

public:
	void setText(const QString& title);	//设置文本
	void setExpanded(bool expand);	//设置展开

private:
	int rotation();	//获取角度
	void setRotation(int rotation);	//设置角度

protected:
	void paintEvent(QPaintEvent* event);

private:
	QPropertyAnimation* m_animation;
	QString m_titleText;	//显示的文本
	int m_rotation;			//箭头的角度
	bool m_hasArrow;		//是否有箭头
};

