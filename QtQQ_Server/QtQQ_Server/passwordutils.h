#pragma once
#include <QString>
#include <QCryptographicHash>

inline QString hashPassword(const QString& password) {
	const QString salt = QStringLiteral("QtQQ_Salt_2024");
	QString salted = password + salt;
	QByteArray hash = QCryptographicHash::hash(salted.toUtf8(), QCryptographicHash::Sha256);
	return QString::fromLatin1(hash.toHex());
}