#pragma once

#include <QObject>

//管理类，单例模式，统一管理所有窗口皮肤
class NotifyManager  : public QObject
{
	Q_OBJECT

public:
	NotifyManager();
	~NotifyManager();

signals:
	void signalSkinChanged(const QColor& color);

public:
	static NotifyManager* getInstance();

	void notifyOtherWindowChangeSkin(const QColor& color);	//通知其他窗口皮肤改变

private:
	static NotifyManager* instance;
};

