#include "uistyles.h"

QString UiStyles::mainWindow()
{
    return QStringLiteral(R"(
QMainWindow {
    background-color: #F7F8FA;
    font-family: "Segoe UI Variable", "Segoe UI", "Microsoft YaHei UI", sans-serif;
}

QWidget#centralwidget {
    background: transparent;
}

QFrame#statusCard,
QFrame#infoCard,
QFrame#actionCard {
    background-color: #FFFFFF;
    border: 1px solid #E8EAED;
    border-radius: 8px;
}

QFrame#statusCard {
    border-left: 4px solid #F59E0B;
}

QFrame#statusCard[state="connected"] {
    border-left-color: #16A34A;
}

QFrame#statusCard[state="disconnected"] {
    border-left-color: #DC2626;
}

QFrame#statusCard[state="checking"] {
    border-left-color: #F59E0B;
}

QLabel {
    background: transparent;
    border: none;
    color: #1F2937;
}

QLabel#titleLabel {
    font-size: 26px;
    font-weight: 700;
    color: #111827;
}

QLabel#subtitleLabel {
    font-size: 13px;
    color: #667085;
}

QLabel#statusTitleLabel,
QLabel#infoTitleLabel,
QLabel#actionTitleLabel {
    font-size: 12px;
    font-weight: 700;
    color: #667085;
}

QLabel#statusValueLabel {
    font-size: 18px;
    font-weight: 700;
    color: #111827;
}

QLabel#statusDescriptionLabel {
    font-size: 13px;
    color: #667085;
}

QLabel#wifiNameLabel,
QLabel#userTypeLabel,
QLabel#lastLoginLabel {
    font-size: 12px;
    font-weight: 600;
    color: #667085;
}

QLabel#wifiNameValue,
QLabel#userTypeValue,
QLabel#lastLoginValue {
    background-color: #F8FAFC;
    border: 1px solid #EAECF0;
    border-radius: 6px;
    color: #111827;
    font-size: 14px;
    font-weight: 600;
    padding: 8px 10px;
    min-height: 18px;
}

QLabel#statusIndicator {
    min-width: 12px;
    max-width: 12px;
    min-height: 12px;
    max-height: 12px;
    border-radius: 6px;
    background-color: #F59E0B;
}

QLabel#statusIndicator[state="connected"] {
    background-color: #16A34A;
}

QLabel#statusIndicator[state="disconnected"] {
    background-color: #DC2626;
}

QLabel#statusIndicator[state="checking"] {
    background-color: #F59E0B;
}

QPushButton {
    border-radius: 6px;
    font-size: 14px;
    font-weight: 600;
    min-height: 38px;
    min-width: 118px;
    padding: 8px 18px;
}

QPushButton#configButton {
    background-color: #2563EB;
    border: 1px solid #2563EB;
    color: #FFFFFF;
}

QPushButton#configButton:hover {
    background-color: #1D4ED8;
    border-color: #1D4ED8;
}

QPushButton#configButton:pressed {
    background-color: #1E40AF;
    border-color: #1E40AF;
}

QPushButton#toggleButton {
    background-color: #16A34A;
    border: 1px solid #16A34A;
    color: #FFFFFF;
}

QPushButton#toggleButton:hover {
    background-color: #15803D;
    border-color: #15803D;
}

QPushButton#toggleButton:pressed {
    background-color: #166534;
    border-color: #166534;
}

QPushButton#toggleButton.stopped {
    background-color: #FFFFFF;
    border: 1px solid #D0D5DD;
    color: #344054;
}

QPushButton#toggleButton.stopped:hover {
    background-color: #F9FAFB;
    border-color: #98A2B3;
}

QPushButton#toggleButton.stopped:pressed {
    background-color: #F2F4F7;
}

QFrame#divider,
QFrame#divider2 {
    background-color: #EAECF0;
    border: none;
    max-height: 1px;
    min-height: 1px;
}

QStatusBar {
    background-color: #FFFFFF;
    border-top: 1px solid #EAECF0;
    color: #667085;
    font-size: 12px;
    padding: 4px 8px;
}
)");
}

QString UiStyles::configDialog()
{
    return QStringLiteral(R"(
QDialog {
    background-color: #F7F8FA;
    font-family: "Segoe UI Variable", "Segoe UI", "Microsoft YaHei UI", sans-serif;
}

QFrame#headerCard,
QFrame#contentCard {
    background-color: #FFFFFF;
    border: 1px solid #E8EAED;
    border-radius: 8px;
}

QLabel {
    background: transparent;
    border: none;
    color: #1F2937;
    font-size: 13px;
    font-weight: 600;
}

QLabel#dialogTitleLabel {
    color: #111827;
    font-size: 24px;
    font-weight: 700;
}

QLabel#dialogSubtitleLabel {
    color: #667085;
    font-size: 13px;
    font-weight: 400;
}

QLabel#studentHintLabel,
QLabel#teacherHintLabel,
QLabel#settingsHintLabel {
    background-color: #F8FAFC;
    border: 1px solid #EAECF0;
    border-radius: 6px;
    color: #667085;
    font-size: 12px;
    font-weight: 400;
    padding: 10px 12px;
}

QTabWidget::pane {
    background-color: #FFFFFF;
    border: none;
    margin-top: 8px;
}

QTabBar::tab {
    background-color: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    color: #667085;
    font-size: 14px;
    font-weight: 600;
    margin: 6px 4px 0 0;
    min-width: 110px;
    padding: 9px 18px;
}

QTabBar::tab:selected {
    background-color: #EFF6FF;
    border-color: #BFDBFE;
    color: #1D4ED8;
}

QTabBar::tab:hover:!selected {
    background-color: #F2F4F7;
    color: #344054;
}

QGroupBox {
    background-color: #F8FAFC;
    border: 1px solid #EAECF0;
    border-radius: 8px;
    color: #111827;
    font-size: 14px;
    font-weight: 700;
    margin-top: 10px;
    padding-top: 22px;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 14px;
    padding: 0 8px;
    background-color: #F8FAFC;
}

QLineEdit,
QComboBox {
    background-color: #FFFFFF;
    border: 1px solid #D0D5DD;
    border-radius: 6px;
    color: #111827;
    font-size: 14px;
    min-height: 22px;
    padding: 9px 11px;
    selection-background-color: #2563EB;
    selection-color: #FFFFFF;
}

QLineEdit:hover,
QComboBox:hover {
    border-color: #98A2B3;
}

QLineEdit:focus,
QComboBox:focus {
    border: 2px solid #2563EB;
    padding: 8px 10px;
}

QLineEdit:disabled,
QComboBox:disabled {
    background-color: #F2F4F7;
    border-color: #EAECF0;
    color: #98A2B3;
}

QComboBox::drop-down {
    background-color: transparent;
    border: none;
    width: 28px;
}

QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    border: 1px solid #D0D5DD;
    border-radius: 6px;
    outline: none;
    padding: 4px;
    selection-background-color: #EFF6FF;
    selection-color: #1D4ED8;
}

QCheckBox {
    color: #344054;
    font-size: 14px;
    font-weight: 500;
    spacing: 10px;
}

QCheckBox::indicator {
    background-color: #FFFFFF;
    border: 1px solid #98A2B3;
    border-radius: 4px;
    height: 18px;
    width: 18px;
}

QCheckBox::indicator:hover {
    border-color: #2563EB;
}

QCheckBox::indicator:checked {
    background-color: #2563EB;
    border-color: #2563EB;
}

QPushButton {
    border-radius: 6px;
    font-size: 14px;
    font-weight: 600;
    min-height: 38px;
    min-width: 100px;
    padding: 8px 18px;
}

QPushButton#saveButton {
    background-color: #2563EB;
    border: 1px solid #2563EB;
    color: #FFFFFF;
}

QPushButton#saveButton:hover {
    background-color: #1D4ED8;
    border-color: #1D4ED8;
}

QPushButton#saveButton:pressed {
    background-color: #1E40AF;
    border-color: #1E40AF;
}

QPushButton#cancelButton {
    background-color: #FFFFFF;
    border: 1px solid #D0D5DD;
    color: #344054;
}

QPushButton#cancelButton:hover {
    background-color: #F9FAFB;
    border-color: #98A2B3;
}

QPushButton#cancelButton:pressed {
    background-color: #F2F4F7;
}
)");
}
