// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
Item {
    property var dataObject
    property string folder: reportSettings.destination
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            ComboBox { id: mode; model: ["Local archive", "Shared folder", "Apple Mail"]; currentIndex: reportSettings.destination ? 1 : reportSettings.automatic ? 2 : 0; Accessible.name: "Report delivery" }
            Button { visible: mode.currentIndex === 1; text: "Choose folder…"; onClicked: { var path=reportSettings.pickFolder(folder); if(path) folder=path } }
            Label { visible: mode.currentIndex === 1; Layout.fillWidth: true; text: folder || "Select a mounted network share or synced folder"; elide: Text.ElideMiddle }
            TextField { id: email; visible: mode.currentIndex === 2; Layout.fillWidth: true; text: reportSettings.email; placeholderText: "leader@example.org" }
        }
        RowLayout {
            TextField { id: pin; Layout.preferredWidth: 145; placeholderText: reportSettings.configured ? "Leader PIN" : "Create leader PIN"; echoMode: TextInput.Password; maximumLength: 12 }
            TextField { id: confirm; visible: !reportSettings.configured; Layout.preferredWidth: 130; placeholderText: "Confirm PIN"; echoMode: TextInput.Password; maximumLength: 12 }
            Button {
                text: "Save preferences"
                onClicked: {
                    if(mode.currentIndex === 1 && !folder) { status.text="Choose a folder first."; return }
                    status.text = reportSettings.save(email.text,mode.currentIndex === 2,pin.text,confirm.text,mode.currentIndex === 1 ? folder : "")
                    pin.clear(); confirm.clear()
                }
            }
            Label { id: status; Layout.fillWidth: true; wrapMode: Text.WordWrap; text: reportSettings.configured ? "Changes require the leader PIN." : "Choose a 6–12 digit PIN." }
        }
    }
}
