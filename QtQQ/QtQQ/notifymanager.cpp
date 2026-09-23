#include "notifymanager.h"
#include "commonutils.h"

NotifyManager* NotifyManager::instance = nullptr;

NotifyManager::NotifyManager() : QObject(nullptr)
{}

NotifyManager::~NotifyManager()
{}

NotifyManager* NotifyManager::getInstance()
{
	if (instance == nullptr) {
		instance = new NotifyManager();
	}
	return instance;
}

void NotifyManager::notifyOtherWindowChangeSkin(const QColor& color)
{
	emit signalSkinChanged(color);	//触发信号
	CommonUtils::setDefaultSkinColor(color);	//将改变后的颜色写入配置文件
}
