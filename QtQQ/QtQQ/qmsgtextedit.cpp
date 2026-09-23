#include "qmsgtextedit.h"
#include <QMovie>
#include <QUrl>

QMsgTextEdit::QMsgTextEdit(QWidget *parent)
	: QTextEdit(parent)
{}

QMsgTextEdit::~QMsgTextEdit()
{
	deleteAllEmotionImage();
}

void QMsgTextEdit::onEmotionImageFrameChange(int frame)
{
	QMovie* movie = qobject_cast<QMovie*>(sender());	//获取哪个动画
	document()->addResource(QTextDocument::ImageResource, QUrl(m_emotionMap.value(movie)), movie->currentPixmap());	//更新文档内部的图片资源缓存
	viewport()->update();	//重绘，刷新动画帧
}

void QMsgTextEdit::addEmotionUrl(int emotionNum)
{
	//获取表情路径
	const QString& imageName = QString("qrc:/Resources/MainWindow/emotion/%1.png").arg(emotionNum);
	const QString& flagName = QString("%1").arg(imageName);

	//表情插入到textedit里
	insertHtml(QString("<img src='%1' />").arg(flagName));

	if (m_listEmotionUrl.contains(imageName)) {
		return;	//相同表情共用一个动画
	}
	else {
		m_listEmotionUrl.append(imageName);
	}

	//初始化apng动画
	QMovie* apngMovie = new QMovie(imageName.mid(3), "apng", this);
	m_emotionMap.insert(apngMovie, flagName);	//建立映射

	connect(apngMovie, SIGNAL(frameChanged(int)), this, SLOT(onEmotionImageFrameChange(int)));
	apngMovie->start();	//启动动画，开始循环
	//updateGeometry();	//触发几何重算，确保新插入的图片布局正确
}

void QMsgTextEdit::deleteAllEmotionImage()
{
	for (auto itor = m_emotionMap.constBegin(); itor != m_emotionMap.constEnd(); ++itor) {
		delete itor.key();	//释放映射的键
	}
	m_emotionMap.clear();
	m_listEmotionUrl.clear();	//避免第二次添加相同表情时表情不会动
}

