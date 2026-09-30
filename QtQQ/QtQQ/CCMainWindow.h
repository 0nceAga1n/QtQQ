#pragma once

#include "basicwindow.h"
#include "ui_CCMainWindow.h"
#include <QJsonArray>

class CCMainWindow : public BasicWindow
{
    Q_OBJECT

public:
    CCMainWindow(QString loginPicture, QJsonArray departments, QWidget *parent = nullptr);  //参一为登录者头像，参二为联系人树
    ~CCMainWindow();

public:
    void setUserName(const QString& username);  //设置用户名
    void setLevelPixmap(int level); //设置等级

    void setHeadPixmap(const QString& headPath);    //设置头像
    void setStatusMenuIcon(const QString& statusPath);  //设置状态
    QWidget* addOtherAppExtension(const QString& appPath, const QString& appName);  //添加app图标
    void initContactTree(); //初始化聊天树

private:
    void initTimer();   //初始化定时器，模拟等级升级
    void initControl(); //初始化控件
    void updateSearchStyle();   //  更新搜索样式
    void addCompanyDeps(QTreeWidgetItem* pRootGroupItem, const QJsonObject& dep); //添加聊天树里的子项
    QString getHeadPicturePath();   //获取头像路径

private:
    void resizeEvent(QResizeEvent* event);
    bool eventFilter(QObject* obj, QEvent* event);
    void mousePressEvent(QMouseEvent* event);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);  //单击聊天树的项
    void onItemExpanded(QTreeWidgetItem* item); //聊天树的项展开
    void onItemCollapsed(QTreeWidgetItem* item);    //项折叠
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);    //双击聊天树的项

    void onAppIconClicked();    //app图标点击，只完成了皮肤图标

private:
    Ui::CCMainWindowClass ui;
    QString m_loginPicture; //登录者头像
    QJsonArray m_departments;   //联系人树
    //QMap<QTreeWidgetItem*, QString> m_groupMap; //映射聊天项->群名，从数据库中直接获取id后再得到群名即可
};

