import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 420
    height: 820
    visible: true
    title: "AUST WiFi"
    color: "#F7F8FA"

    Material.theme: Material.Light
    Material.accent: "#2563EB"
    Material.primary: "#2563EB"

    property bool showPasswords: false

    Connections {
        target: backend
        function onLoginSucceeded(message) {
            resultDialog.title = "登录成功"
            resultDialog.text = message
            resultDialog.open()
        }
        function onLoginFailed(message) {
            resultDialog.title = "登录失败"
            resultDialog.text = message
            resultDialog.open()
        }
    }

    Dialog {
        id: resultDialog
        modal: true
        standardButtons: Dialog.Ok
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 360)
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 18
            anchors.margins: 20

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
            }

            Label {
                text: "AUST WiFi"
                color: "#111827"
                font.pixelSize: 30
                font.bold: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
            }

            Label {
                text: "手机端 MVP：保存账号，可手动或启动后自动登录校园网。教师账号填写后会优先使用教师登录。"
                color: "#667085"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                radius: 14
                color: "#FFFFFF"
                border.color: "#E8EAED"
                implicitHeight: statusColumn.implicitHeight + 32

                ColumnLayout {
                    id: statusColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    Label {
                        text: "当前状态"
                        color: "#667085"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Label {
                        text: backend.statusText
                        color: backend.busy ? "#B45309" : "#111827"
                        font.pixelSize: 18
                        font.bold: true
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: backend.activeAccountText
                        color: "#667085"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: backend.credentialBackendText
                        color: "#667085"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: backend.networkStatusText
                        color: backend.campusWifiDetected ? "#047857" : "#667085"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            text: "授权识别 WiFi"
                            enabled: !backend.busy
                            Layout.fillWidth: true
                            onClicked: backend.requestNetworkPermissions()
                        }

                        Button {
                            text: "刷新"
                            enabled: !backend.busy
                            Layout.fillWidth: true
                            onClicked: backend.refreshNetworkState()
                        }
                    }

                    ProgressBar {
                        visible: backend.busy
                        indeterminate: true
                        Layout.fillWidth: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                radius: 14
                color: "#FFFFFF"
                border.color: "#E8EAED"
                implicitHeight: formColumn.implicitHeight + 32

                ColumnLayout {
                    id: formColumn
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    Label {
                        text: "学生账号"
                        color: "#111827"
                        font.pixelSize: 17
                        font.bold: true
                    }

                    TextField {
                        text: backend.studentUser
                        placeholderText: "学号"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onTextEdited: backend.studentUser = text
                    }

                    TextField {
                        text: backend.studentPassword
                        placeholderText: "学生密码"
                        echoMode: window.showPasswords ? TextInput.Normal : TextInput.Password
                        Layout.fillWidth: true
                        onTextEdited: backend.studentPassword = text
                    }

                    ComboBox {
                        id: serverCombo
                        textRole: "label"
                        valueRole: "value"
                        Layout.fillWidth: true
                        model: [
                            { "label": "电信", "value": "aust" },
                            { "label": "联通", "value": "unicom" },
                            { "label": "移动", "value": "cmcc" }
                        ]
                        Component.onCompleted: {
                            var index = 0
                            for (var i = 0; i < model.length; ++i) {
                                if (model[i].value === backend.studentServer) {
                                    index = i
                                    break
                                }
                            }
                            currentIndex = index
                        }
                        onActivated: backend.studentServer = currentValue
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#EAECF0"
                    }

                    Label {
                        text: "教师账号"
                        color: "#111827"
                        font.pixelSize: 17
                        font.bold: true
                    }

                    TextField {
                        text: backend.teacherUser
                        placeholderText: "工号"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.fillWidth: true
                        onTextEdited: backend.teacherUser = text
                    }

                    TextField {
                        text: backend.teacherPassword
                        placeholderText: "教师密码"
                        echoMode: window.showPasswords ? TextInput.Normal : TextInput.Password
                        Layout.fillWidth: true
                        onTextEdited: backend.teacherPassword = text
                    }

                    CheckBox {
                        text: "显示密码"
                        checked: window.showPasswords
                        onToggled: window.showPasswords = checked
                    }

                    Switch {
                        text: "启动后自动登录"
                        checked: backend.autoLoginOnLaunch
                        enabled: !backend.busy
                        onToggled: backend.autoLoginOnLaunch = checked
                    }

                    Switch {
                        text: "仅校园 WiFi 自动登录"
                        checked: backend.autoLoginOnlyOnCampusWifi
                        enabled: !backend.busy && backend.autoLoginOnLaunch
                        onToggled: backend.autoLoginOnlyOnCampusWifi = checked
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                spacing: 12

                Button {
                    text: "保存"
                    enabled: !backend.busy
                    Layout.fillWidth: true
                    onClicked: backend.saveConfig()
                }

                Button {
                    text: backend.busy ? "登录中..." : "登录"
                    enabled: !backend.busy
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: backend.login()
                }
            }

            Label {
                text: "当前版本会在前台定时识别 WiFi 并自动登录；后续会继续加入前台服务和 APK 更新下载。"
                color: "#667085"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 24
            }
        }
    }
}
