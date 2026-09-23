#include "commonutils.h"
#include <QPainter>
#include <QFile>
#include <QWidget>
#include <QApplication>
#include <QSettings>

CommonUtils::CommonUtils() {
}

QPixmap CommonUtils::getRoundImage(const QPixmap& src, QPixmap& mask, QSize masksize) {
	//初始化圆头像大小
	if (masksize == QSize(0, 0)) {	//未设置大小时，直接使用空白圆头像图片大小
		masksize = mask.size();
	}
	else {	//设置了大小时，将空白圆头像图片进行平滑抗锯齿缩放
		mask = mask.scaled(masksize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	QImage resultImage(masksize, QImage::Format_ARGB32_Premultiplied);	//初始化画布
	QPainter painter(&resultImage);
	painter.setCompositionMode(QPainter::CompositionMode_Source);	//绘制透明矩形
	painter.fillRect(resultImage.rect(), Qt::transparent);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);		//将不透明的空白圆头像覆盖在透明矩形上
	painter.drawPixmap(0, 0, mask);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);		//在目标不透明区域（也就是空白圆头像）绘制图片
	painter.drawPixmap(0, 0, src.scaled(masksize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	painter.end();

	return QPixmap::fromImage(resultImage);
}

void CommonUtils::loadStyleSheet(QWidget* widget, const QString& sheetName) {
	QFile file(":/Resources/QSS/" + sheetName + ".css");
	file.open(QFile::ReadOnly);
	if (file.isOpen()) {
		widget->setStyleSheet("");
		QString qsstyleSheet = QLatin1String(file.readAll());
		widget->setStyleSheet(qsstyleSheet);
	}
	file.close();
}

void CommonUtils::setDefaultSkinColor(const QColor& color) {
	const QString&& path = QApplication::applicationDirPath() + "/" + QString("tradeprintinfo.ini");
	QSettings settings(path, QSettings::IniFormat);	//写入配置文件
	settings.setValue("DefaultSkin/red", color.red());
	settings.setValue("DefaultSkin/green", color.green());
	settings.setValue("DefaultSkin/blue", color.blue());
}

QColor CommonUtils::getDefaultSkinColor() {
	QColor color;
	const QString&& path = QApplication::applicationDirPath() + "/" + QString("tradeprintinfo.ini");
	if (!QFile::exists(path)) {	//配置文件不存在
		setDefaultSkinColor(QColor(22, 154, 218));
	}

	//从配置文件中获取三原色
	QSettings settings(path, QSettings::IniFormat);
	color.setRed(settings.value("DefaultSkin/red").toInt());
	color.setGreen(settings.value("DefaultSkin/green").toInt());
	color.setBlue(settings.value("DefaultSkin/blue").toInt());

	return color;
}