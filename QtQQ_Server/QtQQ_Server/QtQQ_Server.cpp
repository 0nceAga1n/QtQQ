#include "QtQQ_Server.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDebug>
#include <QFileDialog>

const int gtcpPort = 8888;
const int gudpPort = 6666;

QtQQ_Server::QtQQ_Server(QWidget *parent)
    : QDialog(parent)
    , m_queryMode(All)  //初始查询模式为查询所有员工
    , m_queryDepID(0)
    , m_queryEmployeeID(0)
    , m_timer(nullptr)
    , m_tcpServer(nullptr)
    , m_udpSender(nullptr)
    , m_pixPath("")
{
    ui.setupUi(this);

    //连接数据库
    if (!connectMySql()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("连接数据库失败"));
        close();
        return;
    }

    loadDepartments();  //初始化（部门id->部门名称）映射，同时初始化下拉框
    initTableWidget();  //初始化表格
    refreshTable(); //设置表格初始数据

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &QtQQ_Server::onRefresh);  //每隔一秒刷新一次表格
    m_timer->start();

    initUdpSocket();
    initTcpServer();
}

QtQQ_Server::~QtQQ_Server()
{
}

bool QtQQ_Server::connectMySql()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setDatabaseName("qt_qq");
    db.setHostName("localhost");
    db.setUserName("root");
    db.setPassword("kzk");
    db.setPort(3306);

    if (!db.open()) {
        qDebug() << db.lastError().text();
        return false;
    }
    return true;
}

void QtQQ_Server::loadDepartments()
{
    //从部门表中获取部门id和部门名称
    QSqlQuery query;
    if (!query.exec("SELECT departmentID, department_name FROM tab_department")) {
        qDebug() << "loadDepartments failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        m_depNameMap.insert(id, name);  //建立映射
    }

    // 查询用下拉框："公司群"表示显示全部员工
    ui.departmentBox->clear();
    ui.departmentBox->addItem(QStringLiteral("公司群"), 0);
    for (auto it = m_depNameMap.constBegin(); it != m_depNameMap.constEnd(); ++it) {
        if (it.value() != QStringLiteral("公司群"))
            ui.departmentBox->addItem(it.value(), it.key());
    }

    // 新增员工用下拉框：不含"公司群"
    ui.employeeDepBox->clear();
    for (auto it = m_depNameMap.constBegin(); it != m_depNameMap.constEnd(); ++it) {
        if (it.value() != QStringLiteral("公司群"))
            ui.employeeDepBox->addItem(it.value(), it.key());
    }
}

void QtQQ_Server::initTableWidget()
{
    ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QStringList headers;
    headers << QStringLiteral("部门")
            << QStringLiteral("员工号")
            << QStringLiteral("员工姓名")
            << QStringLiteral("员工签名")
            << QStringLiteral("员工状态")
            << QStringLiteral("员工照片")
            << QStringLiteral("在线状态");
    ui.tableWidget->setColumnCount(headers.size());
    ui.tableWidget->setHorizontalHeaderLabels(headers);
    ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

QString QtQQ_Server::buildQuery() const
{
    switch (m_queryMode) {
    case ByDepartment:  //部门查询
        if (m_queryDepID > 0)
            return QString("SELECT * FROM tab_employees WHERE departmentID = %1").arg(m_queryDepID);    //其他部门
        return "SELECT * FROM tab_employees";   //公司群
    case ByEmployeeID:  //id查询
        if (m_queryEmployeeID > 0)
            return QString("SELECT * FROM tab_employees WHERE employeeID = %1").arg(m_queryEmployeeID);
        return "SELECT * FROM tab_employees";
    default:    //查询所有
        return "SELECT * FROM tab_employees";
    }
}

QString QtQQ_Server::mapDepartmentID(const QString& id) const
{
    int depID = id.toInt();
    auto it = m_depNameMap.constFind(depID);
    if (it != m_depNameMap.constEnd())
        return it.value();
    return id;  //
}

QString QtQQ_Server::mapStatus(const QString& status) const
{
    if (status == "1") return QStringLiteral("有效");
    if (status == "0") return QStringLiteral("注销");
    return status;
}

QString QtQQ_Server::mapOnline(const QString& online) const
{
    if (online == "1") return QStringLiteral("离线");
    if (online == "2") return QStringLiteral("在线");
    if (online == "3") return QStringLiteral("隐身");
    return online;
}

void QtQQ_Server::refreshTable()
{
    QSqlQuery query;
    if (!query.exec(buildQuery())) {
        qDebug() << "refreshTable query failed:" << query.lastError().text();
        return;
    }

    QList<QSqlRecord> rows; //获取所有字段
    while (query.next())
        rows.append(query.record());

    int columns = rows.isEmpty() ? 0 : rows.first().count();    //获取列数，员工表字段列数为7

    ui.tableWidget->setRowCount(rows.size());   //设置表格行数

    for (int i = 0; i < rows.size(); i++) {
        const QSqlRecord& row = rows[i];
        for (int j = 0; j < columns; j++) { //遍历所有项，设置到表格里
            QString fieldName = row.fieldName(j);
            QString value = row.value(j).toString();
            QString displayText = value;

            //根据字段名将数据人性化
            if (fieldName == "departmentID")
                displayText = mapDepartmentID(value);
            else if (fieldName == "status")
                displayText = mapStatus(value);
            else if (fieldName == "online")
                displayText = mapOnline(value);

            QTableWidgetItem* item = ui.tableWidget->item(i, j);    //获取表格中的项
            if (!item) {    //项为空，新创建并设置到表格里
                ui.tableWidget->setItem(i, j, new QTableWidgetItem(displayText));
            } else if (item->text() != displayText) {   //项存在但内容不对，重新设置内容
                item->setText(displayText);
            }
            //项存在且内容一致，不做操作
        }
    }
}

void QtQQ_Server::on_queryDepartmentBtn_clicked()
{
    m_queryMode = ByDepartment; //按部门id号查询
    m_queryDepID = ui.departmentBox->currentData().toInt(); //获取管理员选择的下拉框对应的部门id
    refreshTable(); //刷新表格
}

void QtQQ_Server::on_queryIDBtn_clicked()
{
    QString text = ui.queryIDLineEdit->text().trimmed();    //获取管理员输入的员工id
    if (text.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入员工ID号"));
        return;
    }

    bool ok = false;
    int employeeID = text.toInt(&ok);
    if (!ok || employeeID <= 0) {   //输入的id是不小于零的整数才有效
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("员工ID号无效"));
        return;
    }

    m_queryMode = ByEmployeeID;     //设置查询模式为按员工id号查询
    m_queryEmployeeID = employeeID; //设置要查询的员工id
    refreshTable(); //刷新表格
}

void QtQQ_Server::on_logoutBtn_clicked()
{
    QString text = ui.logoutIDLineEdit->text().trimmed();    //获取管理员输入的员工id
    if (text.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入员工ID号"));
        return;
    }

    bool ok = false;
    int employeeID = text.toInt(&ok);
    if (!ok || employeeID <= 0) {   //输入的id是不小于零的整数才有效
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("员工ID号无效"));
        return;
    }

    //更新数据库
    QSqlQuery sqlUpdate;
    sqlUpdate.prepare("UPDATE tab_employees SET status = 0 WHERE employeeID = ?");
    sqlUpdate.addBindValue(employeeID);
    sqlUpdate.exec();
    QMessageBox::information(this, QStringLiteral("提示"), QString::fromUtf8("员工 %1 的企业QQ已被注销！").arg(employeeID));
}

void QtQQ_Server::on_addBtn_clicked()
{
    QString strName = ui.nameLineEdit->text().trimmed();    //获取管理员输入的员工姓名
    if (strName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入员工姓名号"));
        return;
    }

    if (!m_pixPath.size()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择员工寸照"));
        return;
    }

    //获取数据库里最大员工id
    QSqlQuery querySql;
    querySql.exec(QString("SELECT MAX(employeeID) FROM tab_employees"));
    querySql.next();
    int employeeID = querySql.value(0).toInt() + 1;

    //获取管理员选择的部门id
    int depID = ui.employeeDepBox->currentData().toInt();

    //插入员工到员工表
    QSqlQuery insertSql;
    insertSql.prepare("INSERT INTO tab_employees(departmentID, employeeID, employee_name, status, picture, online) VALUES(?, ?, ?, ?, ?, ?)");
    insertSql.addBindValue(depID);
    insertSql.addBindValue(employeeID);
    insertSql.addBindValue(strName);
    insertSql.addBindValue(1);
    insertSql.addBindValue(m_pixPath);
    insertSql.addBindValue(1);
    insertSql.exec();

    //插入员工账号到账号表
    insertSql.prepare("INSERT INTO tab_accounts(employeeID, account, code) VALUES(?, ?, ?)");
    insertSql.addBindValue(employeeID);
    insertSql.addBindValue(strName);
    insertSql.addBindValue(strName);
    insertSql.exec();

    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("新增员工成功"));
    m_pixPath = "";
    ui.nameLineEdit->clear();
    ui.headLabel->setText("员工寸照");
}

void QtQQ_Server::on_selectPictureBtn_clicked()
{
    m_pixPath = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择头像"), ".", "*.png;;*.jpg");

    if (!m_pixPath.size()) {
        return;
    }
    else {
        QPixmap pixmap;
        pixmap.load(m_pixPath);
        pixmap = pixmap.scaled(QSize(ui.headLabel->width(), ui.headLabel->height()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui.headLabel->setPixmap(pixmap);
    }
}

void QtQQ_Server::onRefresh()
{
    refreshTable();
}

void QtQQ_Server::initTcpServer()
{
    //初始化服务端
    m_tcpServer = new TcpServer(gtcpPort);
    m_tcpServer->run();

    //服务端收到数据时进行广播
    connect(m_tcpServer, &TcpServer::signalTcpMsgComes, this, &QtQQ_Server::onUDPbroadMsg);
}

void QtQQ_Server::initUdpSocket()
{
    m_udpSender = new QUdpSocket(this);
}

void QtQQ_Server::onUDPbroadMsg(QByteArray& btData)
{
    for (quint16 port = gudpPort; port < gudpPort + 200; ++port) {
        m_udpSender->writeDatagram(btData, btData.size(), QHostAddress::Broadcast, port);
    }
}
