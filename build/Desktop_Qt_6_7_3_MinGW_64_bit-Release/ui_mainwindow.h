/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QLabel *titleLabel;
    QLabel *statusLabel;
    QLabel *infoLabel;
    QPushButton *configButton;
    QHBoxLayout *controlLayout;
    QPushButton *startButton;
    QPushButton *stopButton;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(420, 340);
        MainWindow->setMinimumSize(QSize(420, 340));
        MainWindow->setMaximumSize(QSize(420, 340));
        MainWindow->setStyleSheet(QString::fromUtf8("QMainWindow {\n"
"    background-color: #f5f5f5;\n"
"}\n"
"\n"
"QWidget#centralwidget {\n"
"    background: transparent;\n"
"}"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(15);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(20, 15, 20, 15);
        titleLabel = new QLabel(centralwidget);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    font-size: 16px;\n"
"    font-weight: bold;\n"
"    color: #333;\n"
"    padding: 10px;\n"
"    background-color: white;\n"
"    border: 1px solid #ddd;\n"
"    border-radius: 6px;\n"
"}"));
        titleLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout->addWidget(titleLabel);

        statusLabel = new QLabel(centralwidget);
        statusLabel->setObjectName("statusLabel");
        statusLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    font-size: 13px;\n"
"    color: #666;\n"
"    padding: 8px;\n"
"    background-color: white;\n"
"    border: 1px solid #ddd;\n"
"    border-radius: 4px;\n"
"}"));
        statusLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout->addWidget(statusLabel);

        infoLabel = new QLabel(centralwidget);
        infoLabel->setObjectName("infoLabel");
        infoLabel->setStyleSheet(QString::fromUtf8("QLabel {\n"
"    font-size: 11px;\n"
"    color: #888;\n"
"    padding: 6px;\n"
"}"));
        infoLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout->addWidget(infoLabel);

        configButton = new QPushButton(centralwidget);
        configButton->setObjectName("configButton");
        configButton->setMinimumSize(QSize(0, 35));
        configButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #0078d4;\n"
"    border: none;\n"
"    border-radius: 4px;\n"
"    color: white;\n"
"    font-size: 13px;\n"
"    font-weight: bold;\n"
"    padding: 8px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #106ebe;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #005a9e;\n"
"}"));

        verticalLayout->addWidget(configButton);

        controlLayout = new QHBoxLayout();
        controlLayout->setSpacing(10);
        controlLayout->setObjectName("controlLayout");
        startButton = new QPushButton(centralwidget);
        startButton->setObjectName("startButton");
        startButton->setMinimumSize(QSize(0, 32));
        startButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #107c10;\n"
"    border: none;\n"
"    border-radius: 4px;\n"
"    color: white;\n"
"    font-size: 12px;\n"
"    padding: 6px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #0e6e0e;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #0c5d0c;\n"
"}"));

        controlLayout->addWidget(startButton);

        stopButton = new QPushButton(centralwidget);
        stopButton->setObjectName("stopButton");
        stopButton->setMinimumSize(QSize(0, 32));
        stopButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    background-color: #d13438;\n"
"    border: none;\n"
"    border-radius: 4px;\n"
"    color: white;\n"
"    font-size: 12px;\n"
"    padding: 6px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: #b52d31;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: #9a272a;\n"
"}"));

        controlLayout->addWidget(stopButton);


        verticalLayout->addLayout(controlLayout);

        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        statusbar->setStyleSheet(QString::fromUtf8("QStatusBar {\n"
"    background-color: #f0f0f0;\n"
"    border-top: 1px solid #ddd;\n"
"    color: #666;\n"
"    font-size: 10px;\n"
"}"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "AUST WiFi \350\207\252\345\212\250\351\207\215\350\277\236\345\267\245\345\205\267", nullptr));
        titleLabel->setText(QCoreApplication::translate("MainWindow", "AUST WiFi \347\256\241\347\220\206\345\231\250", nullptr));
        statusLabel->setText(QCoreApplication::translate("MainWindow", "\347\212\266\346\200\201: \346\255\243\345\234\250\346\243\200\346\265\213\347\275\221\347\273\234\350\277\236\346\216\245...", nullptr));
        infoLabel->setText(QCoreApplication::translate("MainWindow", "\347\250\213\345\272\217\345\234\250\345\220\216\345\217\260\350\277\220\350\241\214\357\274\214\345\217\214\345\207\273\346\211\230\347\233\230\345\233\276\346\240\207\346\230\276\347\244\272/\351\232\220\350\227\217\347\252\227\345\217\243", nullptr));
        configButton->setText(QCoreApplication::translate("MainWindow", "\351\205\215\347\275\256\350\256\276\347\275\256", nullptr));
        startButton->setText(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213", nullptr));
        stopButton->setText(QCoreApplication::translate("MainWindow", "\345\201\234\346\255\242", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
