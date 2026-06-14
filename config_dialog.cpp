#include "config_dialog.h"
#include "ui_config_dialog.h"
#include "credentialstore.h"
#include "uistyles.h"

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
    , m_settings(new QSettings("AUST_WIFI", "Config", this))
{
    ui->setupUi(this);
    setStyleSheet(UiStyles::configDialog());
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    ui->dialogSubtitleLabel->setWordWrap(true);
    ui->studentUserEdit->setClearButtonEnabled(true);
    ui->studentPasswordEdit->setClearButtonEnabled(true);
    ui->teacherUserEdit->setClearButtonEnabled(true);
    ui->teacherPasswordEdit->setClearButtonEnabled(true);
    ui->saveButton->setDefault(true);
    ui->saveButton->setCursor(Qt::PointingHandCursor);
    ui->cancelButton->setAutoDefault(false);
    ui->cancelButton->setCursor(Qt::PointingHandCursor);
    ui->tabWidget->setDocumentMode(true);
    setTabOrder(ui->studentUserEdit, ui->studentPasswordEdit);
    setTabOrder(ui->studentPasswordEdit, ui->studentServerCombo);
    setTabOrder(ui->studentServerCombo, ui->teacherUserEdit);
    setTabOrder(ui->teacherUserEdit, ui->teacherPasswordEdit);
    setTabOrder(ui->teacherPasswordEdit, ui->autoStartCheck);
    setTabOrder(ui->autoStartCheck, ui->minimizeToTrayCheck);
    setTabOrder(ui->minimizeToTrayCheck, ui->saveButton);
    
    // 设置下拉框数据
    setupComboBoxData();
    
    // 连接信号和槽
    connect(ui->saveButton, &QPushButton::clicked, this, &ConfigDialog::onSaveClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ConfigDialog::onCancelClicked);
    
    // 加载配置
    loadConfig();
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::setupComboBoxData()
{
    // 清空并重新设置运营商下拉框
    ui->studentServerCombo->clear();
    ui->studentServerCombo->addItem("电信", "aust");
    ui->studentServerCombo->addItem("联通", "unicom");
    ui->studentServerCombo->addItem("移动", "cmcc");
}

void ConfigDialog::loadConfig()
{
    CredentialStore credentials(m_settings);
    credentials.migrateLegacyPassword("student");
    credentials.migrateLegacyPassword("teacher");

    // 加载学生配置
    ui->studentUserEdit->setText(m_settings->value("student/user").toString());
    ui->studentPasswordEdit->setText(credentials.password("student"));
    
    QString studentServer = m_settings->value("student/server").toString();
    for (int i = 0; i < ui->studentServerCombo->count(); ++i) {
        if (ui->studentServerCombo->itemData(i).toString() == studentServer) {
            ui->studentServerCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // 加载教师配置
    ui->teacherUserEdit->setText(m_settings->value("teacher/user").toString());
    ui->teacherPasswordEdit->setText(credentials.password("teacher"));
    
    // 加载公共设置
    ui->autoStartCheck->setChecked(m_settings->value("autoStart", false).toBool());
    ui->minimizeToTrayCheck->setChecked(m_settings->value("minimizeToTray", true).toBool());
}

bool ConfigDialog::saveConfig()
{
    CredentialStore credentials(m_settings);

    // 保存学生配置
    m_settings->setValue("student/user", ui->studentUserEdit->text());
    if (!credentials.setPassword("student", ui->studentPasswordEdit->text())) {
        QMessageBox::critical(this, "保存失败", "学生账号密码无法安全保存，请检查系统权限。");
        return false;
    }
    m_settings->setValue("student/server", ui->studentServerCombo->currentData().toString());
    
    // 保存教师配置
    m_settings->setValue("teacher/user", ui->teacherUserEdit->text());
    if (!credentials.setPassword("teacher", ui->teacherPasswordEdit->text())) {
        QMessageBox::critical(this, "保存失败", "教师账号密码无法安全保存，请检查系统权限。");
        return false;
    }
    m_settings->setValue("teacher/server", "jzg");  // 教师固定使用jzg
    
    // 保存公共设置
    m_settings->setValue("autoStart", ui->autoStartCheck->isChecked());
    m_settings->setValue("minimizeToTray", ui->minimizeToTrayCheck->isChecked());
    m_settings->sync();
    return true;
}

void ConfigDialog::onSaveClicked()
{
    // 检查是否至少配置了一套账号信息
    bool hasStudentConfig = !ui->studentUserEdit->text().isEmpty() && !ui->studentPasswordEdit->text().isEmpty();
    bool hasTeacherConfig = !ui->teacherUserEdit->text().isEmpty() && !ui->teacherPasswordEdit->text().isEmpty();
    
    if (!hasStudentConfig && !hasTeacherConfig) {
        QMessageBox::warning(this, "警告", "请至少配置一套账号信息（学生或教师）！");
        return;
    }
    
    // 如果配置了学生信息，检查运营商选择
    if (hasStudentConfig && ui->studentServerCombo->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, "警告", "学生账号需要选择网络运营商！");
        return;
    }
    
    // 验证账号格式
    if (hasStudentConfig) {
        QString studentUser = ui->studentUserEdit->text();
        if (studentUser.length() < 8) {
            QMessageBox::warning(this, "警告", "学号格式不正确，请输入完整学号！");
            return;
        }
    }
    
    if (hasTeacherConfig) {
        QString teacherUser = ui->teacherUserEdit->text();
        if (teacherUser.length() > 8) {
            QMessageBox::warning(this, "警告", "工号格式不正确，请检查输入！");
            return;
        }
    }
    
    if (!saveConfig()) {
        return;
    }
    
    QString message = "配置保存成功！\n";
    if (hasStudentConfig) {
        message += "✓ 学生配置已保存\n";
    }
    if (hasTeacherConfig) {
        message += "✓ 教师配置已保存\n";
    }
    message += "\n系统将优先使用教师账号登录；未填写教师账号密码时，再根据WiFi网络选择对应配置。";
    
    QMessageBox::information(this, "保存成功", message);
    accept();
}

void ConfigDialog::onCancelClicked()
{
    reject();
} 
