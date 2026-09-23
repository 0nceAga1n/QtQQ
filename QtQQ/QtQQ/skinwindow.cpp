#include "skinwindow.h"
#include "qclicklabel.h"
#include "notifymanager.h"
#include <QPalette>

SkinWindow::SkinWindow(QWidget *parent)
	: BasicWindow(parent)
{
	ui.setupUi(this);
	loadStyleSheet("SkinWindow");
	setAttribute(Qt::WA_DeleteOnClose);
	initControl();
}

SkinWindow::~SkinWindow()
{}

void SkinWindow::initControl()
{
	QList<QColor> colorList = {	//初始化皮肤列表
		QColor(22, 154, 218), QColor(40, 138, 221), QColor(49, 166, 107), QColor(218, 67, 68),
		QColor(177, 99, 158), QColor(107, 81, 92), QColor(89, 92, 160), QColor(21, 156, 199),
		QColor(79, 169, 172), QColor(155, 183, 154), QColor(128, 77, 77), QColor(240, 188, 189)
	};

	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 4; column++) {
			QClickLabel* label = new QClickLabel(this);	//自定义标签
			label->setCursor(Qt::PointingHandCursor);

			//点击标签时，由管理类通知所有窗口颜色改变
			connect(label, &QClickLabel::clicked, [row, column, colorList]() {
				NotifyManager::getInstance()->notifyOtherWindowChangeSkin(colorList.at(row * 4 + column));
			});

			label->setFixedSize(84, 84);

			//填充标签颜色
			QPalette palette;
			palette.setColor(QPalette::Window, colorList.at(row * 4 + column));
			label->setAutoFillBackground(true);
			label->setPalette(palette);

			//添加标签到布局
			ui.gridLayout->addWidget(label, row, column);
		}
	}

	connect(ui.sysmin, &QPushButton::clicked, this, &BasicWindow::onShowMin);
	//connect(ui.sysclose, SIGNAL(clicked()), this, SLOT(onShowClose()));
	connect(ui.sysclose, &QPushButton::clicked, this, &BasicWindow::onShowClose);
}

void SkinWindow::onShowClose()
{
	close();
}

