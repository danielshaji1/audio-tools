// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import SourceModel 1.0

Rectangle {
    id: panel
    color: "#101214"
    property bool decay: false
    property var result: ({})
    function measure() {
        result = input.currentIndex >= 0 ? serviceDesk.room(input.model.get(input.currentIndex)) : ({message: "Select an impulse response."})
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 2
        RowLayout {
            Layout.rightMargin: 150
            ComboBox {
                id: input
                Layout.preferredWidth: 155
                enabled: panel.decay || !serviceDesk.recording
                textRole: "title"
                model: SourceModel { list: sourceList; unrollGroups: true }
                onCurrentIndexChanged: panel.result = ({})
                Accessible.name: "Measurement source"
            }
            TextField {
                id: title
                visible: !panel.decay
                Layout.preferredWidth: 140
                text: "Sunday service"
                enabled: !serviceDesk.recording
                Accessible.name: "Service name"
            }
            Button {
                text: panel.decay ? "Measure RT60" : serviceDesk.recording ? "Finish service" : "Record service"
                enabled: input.currentIndex >= 0 || (!panel.decay && serviceDesk.recording)
                onClicked: {
                    if (panel.decay) panel.measure()
                    else if (serviceDesk.recording) serviceDesk.finish()
                    else serviceDesk.start(title.text, input.model.get(input.currentIndex))
                }
            }
            ToolButton {
                visible: !panel.decay
                text: "Reports ▾"
                onClicked: archiveMenu.open()
                Menu {
                    id: archiveMenu
                    Repeater {
                        model: serviceDesk.archives
                        MenuItem { text: modelData; onTriggered: serviceDesk.openArchive(modelData) }
                    }
                    MenuItem { text: "Retry pending email"; onTriggered: serviceDesk.retry() }
                }
            }
        }
        Label {
            Layout.fillWidth: true
            color: "#c1c7cb"
            elide: Text.ElideRight
            text: panel.decay ? panel.result.message || "Measure the selected source’s impulse response to fit broadband T20." :
                serviceDesk.status || (reportSettings.automatic ? "Reports send automatically when recording stops." : "Set up automatic delivery in Reporting Preferences.")
            ToolTip.visible: hover.containsMouse
            ToolTip.text: text
            MouseArea {id: hover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton}
        }
        Canvas {
            id: graph
            Layout.fillWidth: true
            Layout.fillHeight: true
            property var points: panel.decay ? panel.result.points || [] : serviceDesk.levels
            onPointsChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                var c = getContext("2d"), left = 45, right = width - 28, top = 15, bottom = height - 25
                c.clearRect(0,0,width,height)
                if (right <= left || bottom <= top) return
                var lo = panel.decay ? -60 : 50, hi = panel.decay ? 0 : 100
                for (var k=0;k<points.length;++k) {
                    lo = Math.min(lo,Math.floor(points[k].y/10)*10)
                    hi = Math.max(hi,Math.ceil(points[k].y/10)*10)
                }
                var end = points.length ? Math.max(0.001,points[points.length-1].x) : 1
                c.font = "11px sans-serif"; c.lineWidth=1
                for(var j=0;j<=4;++j) {
                    var y=bottom-(bottom-top)*j/4, x=left+(right-left)*j/4
                    c.strokeStyle="#363b3e"; c.beginPath(); c.moveTo(left,y); c.lineTo(right,y); c.moveTo(x,top);c.lineTo(x,bottom);c.stroke()
                    c.fillStyle="#b8c0c2"; c.fillText((lo+(hi-lo)*j/4).toFixed(0),4,y)
                    c.fillText((end*j/4/(panel.decay?1:60)).toFixed(panel.decay?2:1),x-9,bottom+15)
                }
                c.fillText(panel.decay?"dB":"dBA",2,10)
                c.fillText(panel.decay?"s":"min",width-22,bottom+15)
                c.strokeStyle=panel.decay?"#c7a0ff":"#90d564";c.lineWidth=2;c.beginPath()
                for(var i=0;i<points.length;++i) {
                    var px=left+points[i].x/end*(right-left), py=bottom-(points[i].y-lo)/(hi-lo)*(bottom-top)
                    if(i)c.lineTo(px,py);else c.moveTo(px,py)
                }
                c.stroke()
            }
        }
    }
}
