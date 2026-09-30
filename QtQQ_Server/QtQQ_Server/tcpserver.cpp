#include "tcpserver.h"
#include "tcpsocket.h"
#include "msgprotocol.h"
#include "passwordutils.h"

#include <QTcpSocket>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

TcpServer::TcpServer(int port)
	: m_port(port)
{}

TcpServer::~TcpServer()
{}

bool TcpServer::run()
{
	if (this->listen(QHostAddress::AnyIPv4, m_port)) {
		qDebug() << QString::fromUtf8("服务端监听端口 %1 成功").arg(m_port);
		return true;
	}
	else {
		qDebug() << QString::fromUtf8("服务端监听端口 %1 失败").arg(m_port);
		return false;
	}
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
	qDebug() << QString::fromUtf8("新的连接：") << socketDescriptor;

	//创建连接
	TcpSocket* tcpsocket = new TcpSocket();
	tcpsocket->setSocketDescriptor(socketDescriptor);	//设置描述符用于区分不同客户端连接
	tcpsocket->run();

	//套接字收到消息时服务端进行处理
	connect(tcpsocket, SIGNAL(signalMsgFrame(const QJsonObject&, int)), this, SLOT(SocketDataProcessing(const QJsonObject&, int)));
	//套接字断开连接时服务端进行清理
	connect(tcpsocket, SIGNAL(signalClientDisconnect(int)), this, SLOT(SocketDisconnected(int)));

	m_socketMap.insert(socketDescriptor, tcpsocket);	//连接保存到服务端
}

void TcpServer::SocketDisconnected(int descriptor)
{
	if (m_socketMap.contains(descriptor)) {
		//清理连接映射
		QTcpSocket* item = m_socketMap.value(descriptor);
		m_socketMap.remove(descriptor);
		item->deleteLater();

		//清理所有身份映射
		if (m_descriptorToId.contains(descriptor)) {
			QString id = m_descriptorToId.value(descriptor);
			m_descriptorToId.remove(descriptor);
			m_idToDescriptor.remove(id);
		}
		qDebug() << "TcpSocket断开连接" << descriptor;
	}
}

void TcpServer::handleLogin(const QJsonObject& obj, int descriptor)
{
	//获取登录者id
	QString token = obj.value("token").toString();
	QString employeeID = m_tokenToId.value(token);
	if (employeeID.isEmpty()) {
		qDebug() << "无效 token，拒绝会话连接";
		return;
	}

	//同一员工重复登录，踢掉旧连接
	if (m_idToDescriptor.contains(employeeID)) {
		int oldDesc = m_idToDescriptor.value(employeeID);
		if (m_socketMap.contains(oldDesc)) {
			m_socketMap.value(oldDesc)->disconnectFromHost();
		}
	}

	//保存登录者
	m_idToDescriptor.insert(employeeID, descriptor);
	m_descriptorToId.insert(descriptor, employeeID);
	qDebug() << "员工" << employeeID << "登录，描述符" << descriptor;
}

void TcpServer::handleChatMsg(const QJsonObject& obj, int descriptor)
{
	// 用服务端记录的真实身份覆盖 sender，防止伪造
	QString realSender = m_descriptorToId.value(descriptor);
	if (realSender.isEmpty()) {
		return;
	}
	QJsonObject msg = obj;
	msg.insert("sender", realSender);

	if (!MsgProtocol::isValidMsg(msg)) {
		qDebug() << "非法消息";
		return;
	}

	saveMessage(msg);	//先落库（所有消息都存，用于历史记录 + 离线消息）

	bool isGroup = msg.value("group").toInt() == 1;
	QString receiver = msg.value("receiver").toString();

	if (isGroup) {	//群聊：只发送给在线的群成员（跳过发送者自己）
		QStringList members = getGroupMembers(receiver);
		QByteArray frame = MsgProtocol::pack(msg);
		for (const QString& memberID : members) {
			if (memberID == realSender) continue;	// 跳过发送者自己
			if (m_idToDescriptor.contains(memberID)) {	// 成员在线才发
				int targetDesc = m_idToDescriptor.value(memberID);
				if (m_socketMap.contains(targetDesc)) {
					m_socketMap.value(targetDesc)->write(frame);
				}
			}
		}
	}
	else {	//单聊，只发送给接收者
		//接收者在线
		if (m_idToDescriptor.contains(receiver)) {
			int targetDesc = m_idToDescriptor.value(receiver);
			if (m_socketMap.contains(targetDesc)) {
				m_socketMap.value(targetDesc)->write(MsgProtocol::pack(msg));
			}
		}
		//接收者离线，消息落库saveMessage()，等对方上线再拉取
	}
}

void TcpServer::handleFetchHistory(const QJsonObject& obj, int descriptor)
{
	// 请求者身份：用服务端记录的，不信任客户端自报
	QString requester = m_descriptorToId.value(descriptor);
	if (requester.isEmpty()) {
		return;
	}

	QString peer = obj.value("peer").toString();
	bool isGroup = obj.value("is_group").toInt() == 1;

	QSqlQuery query;
	if (isGroup) {
		// 群聊：查发给这个群的所有消息
		query.prepare("SELECT sender, segments FROM tab_message "
			"WHERE is_group = 1 AND receiver = ? ORDER BY msg_id ASC");
		query.addBindValue(peer);
	}
	else {
		// 单聊：查双方互发的消息
		query.prepare("SELECT sender, segments FROM tab_message "
			"WHERE is_group = 0 AND ((sender = ? AND receiver = ?) OR (sender = ? AND receiver = ?)) "
			"ORDER BY msg_id ASC");
		query.addBindValue(requester);
		query.addBindValue(peer);
		query.addBindValue(peer);
		query.addBindValue(requester);
	}

	if (!query.exec()) {
		qDebug() << "查询历史失败:" << query.lastError().text();
		return;
	}

	QJsonArray messages;
	while (query.next()) {
		QJsonObject msg;
		msg.insert("sender", query.value(0).toString());
		// segments 存的是 JSON 字符串，还原成数组
		QJsonArray segs = QJsonDocument::fromJson(query.value(1).toString().toUtf8()).array();

		qDebug() << query.value(0).toString() << " " << query.value(1).toString();

		msg.insert("segments", segs);
		messages.append(msg);
	}

	QJsonObject resp;
	resp.insert("cmd", QString("history"));
	resp.insert("peer", peer);
	resp.insert("is_group", isGroup ? 1 : 0);
	resp.insert("messages", messages);

	if (m_socketMap.contains(descriptor)) {
		m_socketMap.value(descriptor)->write(MsgProtocol::pack(resp));
	}
}

void TcpServer::saveMessage(const QJsonObject& obj)
{
	QString sender = obj.value("sender").toString();
	QString receiver = obj.value("receiver").toString();
	int isGroup = obj.value("group").toInt() == 1 ? 1 : 0;

	// segments 数组序列化成 JSON 字符串存进 TEXT 字段
	QJsonArray segments = obj.value("segments").toArray();
	QString segmentsJson = QString::fromUtf8(QJsonDocument(segments).toJson(QJsonDocument::Compact));

	QSqlQuery query;
	query.prepare("INSERT INTO tab_message(sender, receiver, is_group, segments, send_time, is_read) VALUES(?, ?, ?, ?, NOW(), 0)");
	query.addBindValue(sender);
	query.addBindValue(receiver);
	query.addBindValue(isGroup);
	query.addBindValue(segmentsJson);
	if (!query.exec()) {
		qDebug() << "消息落库失败:" << query.lastError().text();
	}
}

void TcpServer::handleAuth(const QJsonObject& obj, int descriptor)
{
	QString account = obj.value("account").toString();
	QString code = obj.value("code").toString();

	QString employeeID;
	bool ok = verifyAccount(account, code, employeeID);

	QJsonObject resp;
	resp.insert("cmd", QString("auth_result"));
	resp.insert("ok", ok);
	if (ok) {
		resp.insert("employeeID", employeeID);

		QString loginPicture;	//登录者头像路径
		int selfDepID = 0;	//登录者所在部门号
		QSqlQuery q1;
		q1.prepare("SELECT picture, departmentID FROM tab_employees WHERE employeeID = ?");
		q1.addBindValue(employeeID);
		if (q1.exec() && q1.first()) {
			loginPicture = q1.value(0).toString();
			selfDepID = q1.value(1).toInt();
		}
		resp.insert("picture", loginPicture);

		QJsonArray departments;
		QSqlQuery q2;
		q2.prepare("SELECT departmentID, picture FROM tab_department WHERE department_name = ?");
		q2.addBindValue(QString::fromUtf8("公司群"));
		if (q2.exec() && q2.first()) {
			QJsonObject dep;
			dep.insert("departmentID", q2.value(0).toString());
			dep.insert("department_name", QString::fromUtf8("公司群"));
			dep.insert("picture", q2.value(1).toString());
			departments.append(dep);
		}
		q2.prepare("SELECT departmentID, department_name, picture FROM tab_department WHERE departmentID = ?");
		q2.addBindValue(selfDepID);
		if (q2.exec() && q2.first()) {
			QJsonObject dep;
			dep.insert("departmentID", q2.value(0).toString());
			dep.insert("department_name", q2.value(1).toString());
			dep.insert("picture", q2.value(2).toString());
			departments.append(dep);
		}
		resp.insert("departments", departments);

		// 登录成功，生成随机 token 并返回
		QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
		m_tokenToId.insert(token, employeeID);
		resp.insert("token", token);
	}

	if (m_socketMap.contains(descriptor)) {
		m_socketMap.value(descriptor)->write(MsgProtocol::pack(resp));
	}
}

bool TcpServer::verifyAccount(const QString& account, const QString& code, QString& employeeID)
{
	// 员工ID登录
	QSqlQuery query;
	query.prepare("SELECT ta.code FROM tab_accounts ta JOIN tab_employees te ON ta.employeeID = te.employeeID WHERE ta.employeeID = ? AND te.status = 1");
	query.addBindValue(account);
	if (query.exec() && query.first()) {
		if (query.value(0).toString() == hashPassword(code)) {
			employeeID = account;
			return true;
		}
		return false;
	}

	// 账号登录
	query.prepare("SELECT ta.code, ta.employeeID FROM tab_accounts ta JOIN tab_employees te ON ta.employeeID = te.employeeID WHERE ta.account = ? AND te.status = 1");
	query.addBindValue(account);
	if (query.exec() && query.first()) {
		if (query.value(0).toString() == hashPassword(code)) {
			employeeID = query.value(1).toString();
			return true;
		}
		return false;
	}
	return false;
}

void TcpServer::handleFetchTalkInfo(const QJsonObject& obj, int descriptor)
{
	QString uid = obj.value("uid").toString();

	QJsonObject resp;
	resp.insert("cmd", QString("talk_info"));
	resp.insert("uid", uid);

	// 先查是不是部门（有部门记录 = 群聊，否则 = 单聊）
	QSqlQuery qDep;
	qDep.prepare("SELECT department_name, sign, picture FROM tab_department WHERE departmentID = ?");
	qDep.addBindValue(uid);
	if (qDep.exec() && qDep.first()) {
		// 群聊
		resp.insert("is_group", 1);
		resp.insert("name", qDep.value(0).toString());
		resp.insert("sign", qDep.value(1).toString());
		resp.insert("picture", qDep.value(2).toString());

		// 成员列表：公司群 = 所有有效员工，部门群 = 该部门员工
		QString depName = qDep.value(0).toString();
		QSqlQuery qMembers;
		if (depName == QString::fromUtf8("公司群")) {
			qMembers.prepare("SELECT employeeID, employee_name, employee_sign, picture FROM tab_employees WHERE status = 1");
		}
		else {
			qMembers.prepare("SELECT employeeID, employee_name, employee_sign, picture FROM tab_employees WHERE status = 1 AND departmentID = ?");
			qMembers.addBindValue(uid);
		}

		QJsonArray members;
		if (qMembers.exec()) {
			while (qMembers.next()) {
				QJsonObject m;
				m.insert("employeeID", qMembers.value(0).toString());
				m.insert("employee_name", qMembers.value(1).toString());
				m.insert("employee_sign", qMembers.value(2).toString());
				m.insert("picture", qMembers.value(3).toString());
				members.append(m);
			}
		}
		resp.insert("members", members);
	}
	else {
		// 单聊
		QSqlQuery qEmp;
		qEmp.prepare("SELECT employee_name, employee_sign, picture FROM tab_employees WHERE employeeID = ?");
		qEmp.addBindValue(uid);
		if (qEmp.exec() && qEmp.first()) {
			resp.insert("is_group", 0);
			resp.insert("name", qEmp.value(0).toString());
			resp.insert("sign", qEmp.value(1).toString());
			resp.insert("picture", qEmp.value(2).toString());
			resp.insert("members", QJsonArray());
		}
	}

	if (m_socketMap.contains(descriptor)) {
		m_socketMap.value(descriptor)->write(MsgProtocol::pack(resp));
	}
}

QStringList TcpServer::getGroupMembers(const QString& groupID)
{
	QStringList members;

	// 查部门名，判断是公司群还是部门群
	QSqlQuery qDep;
	qDep.prepare("SELECT department_name FROM tab_department WHERE departmentID = ?");
	qDep.addBindValue(groupID);
	if (!qDep.exec() || !qDep.first()) {
		return members;
	}
	QString depName = qDep.value(0).toString();

	// 公司群 = 所有有效员工，部门群 = 该部门员工
	QSqlQuery qMembers;
	if (depName == QString::fromUtf8("公司群")) {
		qMembers.prepare("SELECT employeeID FROM tab_employees WHERE status = 1");
	}
	else {
		qMembers.prepare("SELECT employeeID FROM tab_employees WHERE status = 1 AND departmentID = ?");
		qMembers.addBindValue(groupID);
	}
	if (qMembers.exec()) {
		while (qMembers.next()) {
			members.append(qMembers.value(0).toString());
		}
	}
	return members;
}

void TcpServer::SocketDataProcessing(const QJsonObject& obj, int descriptor)
{
	QString cmd = obj.value("cmd").toString();
	if (cmd == "login") {	//连接请求
		handleLogin(obj, descriptor);
	}
	else if (cmd == "msg") {	//发送消息请求
		handleChatMsg(obj, descriptor);
	}
	else if (cmd == "fetch_history") {	//抓取历史消息
		handleFetchHistory(obj, descriptor);
	}
	else if (cmd == "auth") {	//登录账号校验请求
		handleAuth(obj, descriptor);
	}
	else if (cmd == "fetch_talk_info") {	//拉取会话信息
		handleFetchTalkInfo(obj, descriptor);
	}
}