#pragma once

#include "basicwindow.h"
#include "ui_CCMainWindow.h"

class CCMainWindow : public BasicWindow
{
    Q_OBJECT

public:
    CCMainWindow(QString account, bool isAccountLogin, QWidget *parent = nullptr);  //参一为登录时账号输入，参二为判断输入的账号是员工id还是员工账号
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
    void addCompanyDeps(QTreeWidgetItem* pRootGroupItem, int DepID); //添加聊天树里的子项
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
    bool m_isAccountLogin;  //判断输入的账号是员工账号(true)还是员工id(false)
    QString m_account;  //登录时输入的账号
    //QMap<QTreeWidgetItem*, QString> m_groupMap; //映射聊天项->群名，从数据库中直接获取id后再得到群名即可
};

