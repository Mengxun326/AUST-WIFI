#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H

#include <QDialog>
#include <QSettings>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class ConfigDialog;
}
QT_END_NAMESPACE

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();
    
    void loadConfig();
    void saveConfig();

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupComboBoxData();  // 设置下拉框数据
    
    Ui::ConfigDialog *ui;
    QSettings *m_settings;
};

#endif // CONFIG_DIALOG_H 