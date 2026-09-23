#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>

//标题栏拥有的按钮
enum ButtonType {
	MIN_BUTTON = 0,	//最小化和关闭按钮
	MIN_MAX_BUTTON,		//最小化、最大化和关闭按钮
	ONLY_CLOSE_BUTTON,	//只有关闭按钮
};

//自定义标题栏
class TitleBar  : public QWidget
{
	Q_OBJECT

public:
	TitleBar(QWidget *parent = nullptr);
	~TitleBar();

	void setTitleIcon(const QString& filePath);		//设置标题栏图标
	void setTitleContent(const QString& titleContent);//设置标题栏内容
	void setTitleWidth(int width);				//设置标题栏宽度
	void setButtonType(ButtonType buttonType);	//设置标题栏按钮类型

	void saveRestoreInfo(const QPoint& point, const QSize& size);	//保存还原时标题栏位置信息
	void getRestoreInfo(QPoint& point, QSize& size);				//获得还原时标题栏位置信息

private:
	//重写事件
	void paintEvent(QPaintEvent* event);
	void mouseDoubleClickEvent(QMouseEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);

	//初始化
	void initControl();		//初始化控件
	void initConnections();	//初始化信号和槽
	void loadStyleSheet(const QString& sheetName);	//加载样式表

signals:
	void signalButtonMinClicked();
	void signalButtonRestoreClicked();
	void signalButtonMaxClicked();
	void signalButtonCloseClicked();

private slots:
	void onButtonMinClicked();
	void onButtonMaxClicked();
	void onButtonCloseClicked();
	void onButtonRestoreClicked();

private:
	QLabel* m_pIcon;				//标题栏图标
	QLabel* m_pTitleContent;		//标题栏内容
	QPushButton* m_pButtonMin;		//最小化按钮
	QPushButton* m_pButtonMax;		//最大化按钮
	QPushButton* m_pButtonRestore;	//还原按钮
	QPushButton* m_pButtonClose;	//关闭按钮

	QPoint m_restorePos;	//还原位置
	QSize m_restoreSize;	//还原大小

	bool m_isPressed;		//左键是否按下
	QPoint m_startMovePos;	//鼠标起始移动位置

	QString m_titleContent;		//标题内容
	ButtonType m_buttonType;	//按钮类型
};