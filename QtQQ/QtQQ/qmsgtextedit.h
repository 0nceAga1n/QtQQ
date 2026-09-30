#pragma once

#include <QTextEdit>
#include <QKeyEvent>

//自定义聊天信息编辑区，以便支持 在Qt 的富文本编辑器中 插入与实时播放 APNG（动态 PNG）表情动画
class QMsgTextEdit  : public QTextEdit
{
	Q_OBJECT

public:
	QMsgTextEdit(QWidget *parent = nullptr);
	~QMsgTextEdit();

signals:
	void sendMsgSignal(bool);

private slots:
	void onEmotionImageFrameChange(int frame);	//动画帧改变时触发

public:
	void addEmotionUrl(int emotionNum);	//添加表情资源到文档（编辑区）中
	void deleteAllEmotionImage();	//清理所有QMovie动画对象，防止内存泄漏

protected:
	virtual void keyPressEvent(QKeyEvent* event) override;

private:
	QList<QString> m_listEmotionUrl;	//表情资源列表，用于去重
	QMap<QMovie*, QString> m_emotionMap; //QMovie指针→表情URL的映射，帧刷新时反查对应的文档资源URL
};

