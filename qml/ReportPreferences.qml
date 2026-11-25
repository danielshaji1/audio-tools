// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
Item {
    property var dataObject
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Label { text: "Report recipient" }
            TextField { id: email; Layout.fillWidth: true; text: reportSettings.email; placeholderText: "leader@example.org" }
            CheckBox { id: automatic; text: "Send on stop"; checked: reportSettings.automatic }
        }
        RowLayout {
            TextField { id: pin; Layout.preferredWidth: 148; placeholderText: reportSettings.configured ? "Leader PIN" : "Create leader PIN"; echoMode: TextInput.Password; maximumLength: 12 }
            TextField { id: confirm; visible: !reportSettings.configured; Layout.preferredWidth: 145; placeholderText: "Confirm PIN"; echoMode: TextInput.Password; maximumLength: 12 }
            Button {
                text: "Save preferences"
                onClicked: {
                    status.text = reportSettings.save(email.text, automatic.checked, pin.text, confirm.text)
                    pin.clear(); confirm.clear()
                }
            }
            Label { id: status; Layout.fillWidth: true; wrapMode: Text.WordWrap; text: reportSettings.configured ? "Changes require the leader PIN." : "Choose a 6–12 digit PIN. Delivery uses Apple Mail." }
        }
    }
}
