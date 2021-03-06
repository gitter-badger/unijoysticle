/****************************************************************************
Copyright 2016 Ricardo Quesada
http://github.com/ricardoquesada/unijoysticle

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QMdiSubWindow>
#include <QHostInfo>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QTimer>

#include "aboutdialog.h"
#include "preferences.h"
#include "commandowidget.h"
#include "dpadwidget.h"
#include "commodorehomeform.h"
#include "qMDNS.h"

static const int STATE_VERSION = 1;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _lastServerName("")
{
    ui->setupUi(this);
    QPixmap pixmap(1,1);
    pixmap.fill(QColor(0,0,0,0));
    QIcon icon(pixmap);

    _dpadWidget = new DpadWidget;
    auto subWindowDpad = ui->mdiArea->addSubWindow(_dpadWidget, Qt::Widget);
    subWindowDpad->setWindowTitle(tr("D-Pad mode"));
    subWindowDpad->setWindowIcon(icon);
    subWindowDpad->showMaximized();
    subWindowDpad->layout()->setContentsMargins(2, 2, 2, 2);

    _commandoWidget = new CommandoWidget;
    auto subWindow = ui->mdiArea->addSubWindow(_commandoWidget, Qt::Widget);
    subWindow->setWindowTitle(tr("Commando mode"));
    subWindow->setWindowIcon(icon);
    subWindow->showMaximized();
    subWindow->layout()->setContentsMargins(2, 2, 2, 2);

    _commodoreHomeForm = new CommodoreHomeForm;
    subWindow = ui->mdiArea->addSubWindow(_commodoreHomeForm, Qt::Widget);
    subWindow->setWindowTitle(tr("Commodore Home mode"));
    subWindow->setWindowIcon(icon);
    subWindow->showMaximized();
    subWindow->layout()->setContentsMargins(2, 2, 2, 2);

    // enable tab #1
//    ui->mdiArea->setActiveSubWindow(subWindowDpad);
//    ui->mdiArea->setFocus();
//    subWindowDpad->show();
//    subWindowDpad->setFocus();


    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);
    connect(qMDNS::getInstance(), &qMDNS::hostFound, this, &MainWindow::onDeviceDiscovered);
    connect(ui->lineEdit_server, &QLineEdit::editingFinished, this, &MainWindow::onResolveTriggered);
    connect(ui->pushButton_stats, &QPushButton::clicked, [&](){
        QDesktopServices::openUrl(QUrl(QString("http://") + ui->lineEdit_server->text()));
    });

    // dpad settings
    connect(ui->radioButton_joy1, &QRadioButton::clicked, _dpadWidget, &DpadWidget::onJoy1Clicked);
    connect(ui->radioButton_joy2, &QRadioButton::clicked, _dpadWidget, &DpadWidget::onJoy2Clicked);
    connect(ui->checkBox_swapAB, &QCheckBox::clicked, _dpadWidget, &DpadWidget::onSwapABChecked);
    connect(ui->checkBox_jumpB, &QCheckBox::clicked, _dpadWidget, &DpadWidget::onJumpBChecked);
    connect(ui->checkBox_jumpB, &QCheckBox::clicked, [&](bool checked){
        this->ui->checkBox_swapAB->setEnabled(checked);
    });

    setUnifiedTitleAndToolBarOnMac(true);

    restoreSettings();

    QTimer::singleShot(60, [&](){
        statusBar()->showMessage(tr("UniJoystiCle Controller %1").arg(VERSION));
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

MainWindow* MainWindow::getInstance()
{
    for (auto &widget: qApp->topLevelWidgets()) {
        if (dynamic_cast<MainWindow*>(widget))
            return static_cast<MainWindow*>(widget);
    }
    qDebug() << "MainWindow not found";
    return nullptr;
}

void MainWindow::onSubWindowActivated(QMdiSubWindow* subwindow)
{
    // subwindow can be nullptr when closing the app
    if (subwindow)
    {
        auto widget = static_cast<BaseJoyMode*>(subwindow->widget());

        if (widget == _dpadWidget) {
            ui->groupBox_dpad->show();
            _commandoWidget->enable(false);
            _commodoreHomeForm->enable(false);
            _dpadWidget->enable(true);
        } else if (widget == _commandoWidget) {
            ui->groupBox_dpad->hide();
            _commodoreHomeForm->enable(false);
            _dpadWidget->enable(false);
            _commandoWidget->enable(true);
        } else if (widget == _commodoreHomeForm) {
            ui->groupBox_dpad->hide();
            _dpadWidget->enable(false);
            _commandoWidget->enable(false);
            _commodoreHomeForm->enable(true);
        }
    }
}

void MainWindow::onDeviceDiscovered (const QHostInfo& info)
{
    qDebug() << "";
    qDebug() << info.hostName()
             << "has the following IPs:";
    foreach (QHostAddress address, info.addresses())
        qDebug() << "  -" << address.toString();
    qDebug() << "";

    if(info.hostName() == ui->lineEdit_server->text() && info.addresses().length() > 0) {
        QHostAddress serverAddr(info.addresses().at(0));
        setServerAddress(serverAddr);
        setEnableTabs(true);
    }
}

void MainWindow::onResolveTriggered()
{
    QString newServerName = ui->lineEdit_server->text();

    if (_lastServerName != newServerName) {
        setEnableTabs(false);
        qDebug() << "Resolving: " << newServerName;
        qMDNS::getInstance()->lookup(newServerName);

        _lastServerName = newServerName;
    }
}

void MainWindow::setEnableTabs(bool enabled)
{
    for(auto& subwindow: ui->mdiArea->subWindowList()) {
        subwindow->setEnabled(enabled);
    }
    ui->mdiArea->setEnabled(enabled);
    ui->pushButton_stats->setEnabled(enabled);
}

void MainWindow::setServerAddress(const QHostAddress& address)
{
    for(auto& subwindow: ui->mdiArea->subWindowList()) {
        BaseJoyMode *widget = static_cast<BaseJoyMode*>(subwindow->widget());
        widget->setServerAddress(address);
    }
}

void MainWindow::on_actionQuit_triggered()
{
    saveSettings();
    QApplication::exit();
}

void MainWindow::restoreSettings()
{
    // before restoring settings, save the current layout
    // needed for "reset layout"
    auto& preferences = Preferences::getInstance();
    preferences.setMainWindowDefaultGeometry(saveGeometry());
    preferences.setMainWindowDefaultState(saveState(STATE_VERSION));

    auto geom = preferences.getMainWindowGeometry();
    auto state = preferences.getMainWindowState();

    restoreState(state, STATE_VERSION);
    restoreGeometry(geom);

    auto serverAddress = preferences.getServerIPAddress();
    ui->lineEdit_server->setText(serverAddress);

    // dpad
    bool jumpB = preferences.getDpadJumpWithB();
    ui->checkBox_jumpB->setChecked(jumpB);
    _dpadWidget->setJumpWithB(jumpB);

    bool swapAB = preferences.getDpadSwapAB();
    ui->checkBox_swapAB->setChecked(swapAB);
    ui->checkBox_swapAB->setEnabled(jumpB);
    _dpadWidget->setSwapAB(swapAB);

    int joy = preferences.getDpadJoystick();
    if (joy == 1)
        ui->radioButton_joy1->setChecked(true);
    else ui->radioButton_joy2->setChecked(true);
    _dpadWidget->selectJoystick(joy);
}

void MainWindow::saveSettings()
{
    auto& preferences = Preferences::getInstance();
    preferences.setMainWindowGeometry(saveGeometry());
    preferences.setMainWindowState(saveState(STATE_VERSION));

    auto serverAddress = ui->lineEdit_server->text();
    preferences.setServerIPAddress(serverAddress);

    preferences.setDpadJumpWithB(ui->checkBox_jumpB->isChecked());
    preferences.setDpadSwapAB(ui->checkBox_swapAB->isChecked());
    preferences.setDpadJoystick(ui->radioButton_joy1->isChecked() ? 1 : 2);
}

void MainWindow::showEvent( QShowEvent* event ) {
    QMainWindow::showEvent( event );

    // execute this after the main window has been created
    onResolveTriggered();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    saveSettings();
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}
