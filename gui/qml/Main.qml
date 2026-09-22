import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root
    width: 960
    height: 420
    visible: true
    title: qsTr("Multieffect")
    color: "#1e1e1e"

    property var cpu: ({ totalPercent: 0, perSlotMicros: [] })

    Timer {
        interval: 200
        running: true
        repeat: true
        onTriggered: root.cpu = chainModel.cpuStats()
    }

    function slotCpuMicros(i) {
        return (root.cpu.perSlotMicros && i < root.cpu.perSlotMicros.length)
               ? root.cpu.perSlotMicros[i] : 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Efekty: " + chainModel.slotCount + " / " + chainModel.maxSlots
                color: "#aaaaaa"
            }
            Label {
                text: "CPU: " + root.cpu.totalPercent.toFixed(2) + " %"
                color: root.cpu.totalPercent > 50 ? "#e0a030" : "#4caf50"
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "+ slot"
                enabled: chainModel.slotCount < chainModel.maxSlots
                onClicked: chainModel.addSlot()
            }
            Button {
                text: "- slot"
                enabled: chainModel.slotCount > chainModel.minSlots
                onClicked: chainModel.removeSlot()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

        Repeater {
            model: chainModel.slotCount

            Rectangle {
                id: slotPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"
                radius: 6
                border.color: "#444444"

                property int slotIndex: index
                property var slotData: (chainModel.chainSlots.length > slotIndex)
                                        ? chainModel.chainSlots[slotIndex] : ({ name: "", empty: true, params: [] })
                property bool isTuner: slotData.name === "Tuner"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Slot " + (slotPanel.slotIndex + 1)
                            color: "#aaaaaa"
                            font.pixelSize: 12
                        }

                        Label {
                            visible: !slotPanel.slotData.empty
                            text: root.slotCpuMicros(slotPanel.slotIndex).toFixed(1) + " us"
                            color: "#666666"
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: combo
                            Layout.fillWidth: true
                            model: ["-"].concat(chainModel.availableEffects)
                            currentIndex: {
                                var i = model.indexOf(slotPanel.slotData.name)
                                return i >= 0 ? i : 0
                            }
                            onActivated: function(idx) {
                                var name = model[idx]
                                chainModel.setSlotEffect(slotPanel.slotIndex, name === "-" ? "" : name)
                            }
                        }
                    }

                    // --- zwykły efekt: 3 suwaki parametrów ---
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !slotPanel.slotData.empty && !slotPanel.isTuner
                        spacing: 4

                        Repeater {
                            model: slotPanel.slotData.params

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    text: modelData.name
                                    color: "white"
                                    Layout.preferredWidth: 90
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 100
                                    stepSize: 1
                                    value: modelData.value
                                    onMoved: chainModel.setSlotParam(slotPanel.slotIndex, index, value)
                                }

                                Label {
                                    text: modelData.value
                                    color: "#aaaaaa"
                                    Layout.preferredWidth: 30
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }

                    // --- tuner: animacja strojenia ---
                    Item {
                        id: tunerPanel
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: slotPanel.isTuner

                        property var reading: ({})
                        property bool haveReading: tunerPanel.reading && tunerPanel.reading.valid === true

                        Timer {
                            interval: 60
                            running: slotPanel.isTuner
                            repeat: true
                            onTriggered: tunerPanel.reading = chainModel.tunerReading(slotPanel.slotIndex)
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            Label {
                                text: tunerPanel.haveReading
                                      ? ("Struna: " + tunerPanel.reading.stringName
                                         + "   " + tunerPanel.reading.freq.toFixed(1) + " Hz")
                                      : "Brak sygnalu... zagraj na strunie"
                                color: "white"
                                font.pixelSize: 15
                            }

                            Rectangle {
                                id: meterTrack
                                Layout.fillWidth: true
                                height: 26
                                radius: 4
                                color: "#151515"
                                border.color: "#3a3a3a"

                                Rectangle {
                                    // znacznik "0 centow" na środku (cel strojenia)
                                    width: 2
                                    height: parent.height
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: "#666666"
                                }

                                Rectangle {
                                    width: 8
                                    height: parent.height
                                    radius: 4
                                    color: !tunerPanel.haveReading ? "#555555"
                                           : (tunerPanel.reading.inTune ? "#4caf50" : "#e0a030")

                                    x: {
                                        var cents = tunerPanel.haveReading
                                                    ? Math.max(-50, Math.min(50, tunerPanel.reading.cents))
                                                    : 0
                                        return (meterTrack.width - width) * ((cents + 50) / 100)
                                    }
                                    Behavior on x { NumberAnimation { duration: 80 } }
                                }
                            }

                            Label {
                                text: "-50                    0                    +50 (centow)"
                                color: "#777777"
                                font.pixelSize: 10
                            }

                            Label {
                                text: !tunerPanel.haveReading ? ""
                                      : (tunerPanel.reading.inTune ? "*** NASTROJONO! ***"
                                         : (tunerPanel.reading.cents > 0 ? "za wysoko - poluzuj strune"
                                                                          : "za nisko - naciagnij strune"))
                                color: tunerPanel.haveReading && tunerPanel.reading.inTune ? "#4caf50" : "#e0a030"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }

                    // --- pusty slot ---
                    Label {
                        visible: slotPanel.slotData.empty
                        text: "Pusty slot — wybierz efekt z listy powyzej"
                        color: "#666666"
                        font.italic: true
                    }
                }
            }
        }

        }

        Button {
            Layout.fillWidth: true
            text: chainModel.recording ? "Zatrzymaj nagrywanie" : "Nagrywaj"
            highlighted: chainModel.recording
            onClicked: chainModel.toggleRecording()
        }
    }
}
