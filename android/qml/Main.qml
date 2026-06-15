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
    color: "#F5F7FB"

    Material.theme: Material.Light
    Material.accent: "#2563EB"
    Material.primary: "#2563EB"

    property bool showPasswords: false
    readonly property color ink: "#111827"
    readonly property color muted: "#667085"
    readonly property color line: "#E6EAF0"
    readonly property color card: "#FFFFFF"
    readonly property color blue: "#2563EB"
    readonly property color green: "#047857"
    readonly property color amber: "#B45309"

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
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: Math.min(window.width, 520)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 16

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 12
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                radius: 22
                implicitHeight: heroLayout.implicitHeight + 32
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#EAF1FF" }
                    GradientStop { position: 1.0; color: "#F8FBFF" }
                }
                border.color: "#D8E3F7"

                RowLayout {
                    id: heroLayout
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 72
                        radius: 20
                        color: "#FFFFFF"
                        border.color: "#DDE7F8"

                        Image {
                            anchors.fill: parent
                            anchors.margins: 7
                            source: "qrc:/assets/logo.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: "AUST WiFi"
                            color: window.ink
                            font.pixelSize: 28
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "校园网登录助手"
                            color: window.muted
                            font.pixelSize: 14
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            radius: 999
                            color: backend.busy ? "#FEF3C7" : (backend.campusWifiDetected ? "#DCFCE7" : "#E8F0FF")
                            implicitWidth: statusChip.implicitWidth + 22
                            implicitHeight: 30

                            Label {
                                id: statusChip
                                anchors.centerIn: parent
                                text: backend.busy ? "正在登录" : (backend.campusWifiDetected ? "校园 WiFi" : "待连接")
                                color: backend.busy ? window.amber : (backend.campusWifiDetected ? window.green : window.blue)
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                radius: 16
                color: window.card
                border.color: window.line
                implicitHeight: statusLayout.implicitHeight + 28

                ColumnLayout {
                    id: statusLayout
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 9

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "连接状态"
                            color: window.ink
                            font.pixelSize: 16
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        Label {
                            text: backend.credentialBackendText
                            color: window.muted
                            font.pixelSize: 12
                        }
                    }

                    Label {
                        text: backend.statusText
                        color: backend.busy ? window.amber : window.ink
                        font.pixelSize: 18
                        font.bold: true
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: backend.activeAccountText
                        color: window.muted
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
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
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                radius: 16
                color: window.card
                border.color: backend.backgroundServiceEnabled ? "#BFDBFE" : window.line
                implicitHeight: guardLayout.implicitHeight + 28

                ColumnLayout {
                    id: guardLayout
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Label {
                                text: "后台守护"
                                color: window.ink
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Label {
                                text: backend.backgroundServiceStatusText
                                color: backend.backgroundServiceEnabled ? window.blue : window.muted
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        Rectangle {
                            radius: 999
                            color: backend.notificationPermissionGranted ? "#ECFDF3" : "#FEF3C7"
                            implicitWidth: noticeChip.implicitWidth + 18
                            implicitHeight: 28

                            Label {
                                id: noticeChip
                                anchors.centerIn: parent
                                text: backend.notificationPermissionGranted ? "通知已允许" : "需通知权限"
                                color: backend.notificationPermissionGranted ? window.green : window.amber
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            text: "通知授权"
                            enabled: !backend.notificationPermissionGranted
                            Layout.fillWidth: true
                            onClicked: backend.requestNotificationPermission()
                        }

                        Switch {
                            text: "开启守护"
                            checked: backend.backgroundServiceEnabled
                            Layout.fillWidth: true
                            onToggled: backend.backgroundServiceEnabled = checked
                        }
                    }

                    Label {
                        text: "当前阶段先保持前台服务运行；后台自动登录逻辑会在下一阶段接入服务侧。"
                        color: window.muted
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                radius: 16
                color: window.card
                border.color: backend.campusWifiDetected ? "#A7F3D0" : window.line
                implicitHeight: networkLayout.implicitHeight + 28

                ColumnLayout {
                    id: networkLayout
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Label {
                                text: "WiFi 识别"
                                color: window.ink
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Label {
                                text: backend.networkStatusText
                                color: backend.campusWifiDetected ? window.green : window.muted
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        Rectangle {
                            radius: 999
                            color: backend.wifiConnected ? "#ECFDF3" : "#F2F4F7"
                            implicitWidth: wifiChip.implicitWidth + 18
                            implicitHeight: 28

                            Label {
                                id: wifiChip
                                anchors.centerIn: parent
                                text: backend.wifiConnected ? "WiFi 已连接" : "无 WiFi"
                                color: backend.wifiConnected ? window.green : window.muted
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            text: "授权识别"
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
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                radius: 16
                color: window.card
                border.color: window.line
                implicitHeight: formLayout.implicitHeight + 28

                ColumnLayout {
                    id: formLayout
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 14

                    Label {
                        text: "账号配置"
                        color: window.ink
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Label {
                        text: "教师账号填写后会优先使用教师登录。"
                        color: window.muted
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: "学生账号"
                        color: window.ink
                        font.pixelSize: 14
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
                        color: window.line
                    }

                    Label {
                        text: "教师账号"
                        color: window.ink
                        font.pixelSize: 14
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
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 12

                Button {
                    text: "保存"
                    enabled: !backend.busy
                    Layout.fillWidth: true
                    onClicked: backend.saveConfig()
                }

                Button {
                    text: backend.busy ? "登录中..." : "立即登录"
                    enabled: !backend.busy
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: backend.login()
                }
            }

            Label {
                text: "当前版本会在前台定时识别 WiFi 并自动登录；后续会继续加入前台服务和 APK 更新下载。"
                color: window.muted
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.leftMargin: 18
                Layout.rightMargin: 18
                Layout.bottomMargin: 24
            }
        }
    }
}
