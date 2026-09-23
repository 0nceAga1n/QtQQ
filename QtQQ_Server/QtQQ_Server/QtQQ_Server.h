#pragma once

#include <QtWidgets/QDialog>
#include "ui_QtQQ_Server.h"
#include "tcpserver.h"
#include <QTimer>
#include <QMap>
#include <QUdpSocket>

class QtQQ_Server : public QDialog
{
    Q_OBJECT

public:
    QtQQ_Server(QWidget *parent = nullptr);
    ~QtQQ_Server();

private:
    bool connectMySql();    //连接数据库
    void initTcpServer();   //初始化tcp服务端
    void initUdpSocket();
    void initTableWidget(); //初始化表格
    void loadDepartments(); //加载部门
    void refreshTable();    //刷新表格
    QString buildQuery() const; //根据查询模式，利用查询的部门id或查询的员工id建立查询语句
    QString mapDepartmentID(const QString& id) const;   //获取部门名称
    QString mapStatus(const QString& status) const; //获取账号状态
    QString mapOnline(const QString& online) const; //获取在线状态

private slots:
    void onUDPbroadMsg(QByteArray& btData); //udp广播消息
    void onRefresh();   //定时刷新表格
    void on_queryDepartmentBtn_clicked();   //按部门查询按钮
    void on_queryIDBtn_clicked();   //按员工id查询
    void on_logoutBtn_clicked();    //注销员工
    void on_addBtn_clicked();   //新增员工
    void on_selectPictureBtn_clicked(); //选择员工寸照

private:
    Ui::QtQQ_ServerClass ui;

    QMap<int, QString> m_depNameMap;    //部门id->部门名称

    QTimer* m_timer;    //计时器，实现表格刷新
    TcpServer* m_tcpServer; //tcp服务端
    QUdpSocket* m_udpSender;

    enum QueryMode { All, ByDepartment, ByEmployeeID }; //查询模式枚举
    QueryMode m_queryMode;  //查询模式
    int m_queryDepID;   //查询的部门id
    int m_queryEmployeeID;  //查询的员工id
    QString m_pixPath; //员工寸照路径
};
