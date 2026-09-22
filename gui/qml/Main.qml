import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 960
    height: 440
    visible: true
    title: tr("title")

    // ==================== motyw (light/dark) ====================
    property bool darkMode: true

    readonly property color bgColor: darkMode ? "#1e1e1e" : "#f0f0f0"
    readonly property color panelColor: darkMode ? "#2a2a2a" : "#ffffff"
    readonly property color borderColor: darkMode ? "#444444" : "#cccccc"
    readonly property color textColor: darkMode ? "#ffffff" : "#202020"
    readonly property color mutedTextColor: darkMode ? "#aaaaaa" : "#606060"
    readonly property color dimTextColor: darkMode ? "#666666" : "#909090"
    readonly property color trackColor: darkMode ? "#151515" : "#e0e0e0"
    readonly property color trackBorderColor: darkMode ? "#3a3a3a" : "#c0c0c0"

    color: bgColor

    // ==================== język (prosty słownik EN/PL) ====================
    property string language: "en"

    readonly property var strings: ({
        en: {
            title: "Multieffect",
            effects: "Effects",
            cpu: "CPU",
            addSlot: "+ slot",
            removeSlot: "- slot",
            clear: "Clear",
            menuPresets: "Presets",
            menuSave: "Save...",
            menuLoad: "Load",
            menuDelete: "Delete",
            menuLanguage: "Language",
            menuTheme: "Theme",
            light: "Light",
            dark: "Dark",
            slot: "Slot",
            emptySlot: "Empty slot — pick an effect above",
            record: "Record",
            stopRecording: "Stop recording",
            noSignal: "No signal... play a note",
            inTune: "*** IN TUNE! ***",
            tooHigh: "too high - loosen the string",
            tooLow: "too low - tighten the string",
            stringLabel: "String",
            savePresetTitle: "Save preset",
            presetName: "preset name",
            noPresets: "(no presets)",
            save: "Save",
            cancel: "Cancel",
            none: "-"
        },
        pl: {
            title: "Multieffect",
            effects: "Efekty",
            cpu: "CPU",
            addSlot: "+ slot",
            removeSlot: "- slot",
            clear: "Wyczysc",
            menuPresets: "Presety",
            menuSave: "Zapisz...",
            menuLoad: "Wczytaj",
            menuDelete: "Usun",
            menuLanguage: "Jezyk",
            menuTheme: "Motyw",
            light: "Jasny",
            dark: "Ciemny",
            slot: "Slot",
            emptySlot: "Pusty slot — wybierz efekt powyzej",
            record: "Nagrywaj",
            stopRecording: "Zatrzymaj nagrywanie",
            noSignal: "Brak sygnalu... zagraj na strunie",
            inTune: "*** NASTROJONO! ***",
            tooHigh: "za wysoko - poluzuj strune",
            tooLow: "za nisko - naciagnij strune",
            stringLabel: "Struna",
            savePresetTitle: "Zapisz preset",
            presetName: "nazwa presetu",
            noPresets: "(brak presetow)",
            save: "Zapisz",
            cancel: "Anuluj",
            none: "-"
        }
    })

    function tr(key) {
        var dict = strings[language]
        return (dict && dict[key] !== undefined) ? dict[key] : key
    }

    // ==================== statystyki CPU (odpytywane Timerem) ====================
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

    // ==================== presety (lista odświeżana ręcznie po zapisie) ====================
    property var presetNames: chainModel.listPresets()

    // ==================== menu ====================
    menuBar: MenuBar {
        Menu {
            title: root.tr("menuPresets")

            MenuItem {
                text: root.tr("menuSave")
                onTriggered: saveDialog.open()
            }

            Menu {
                id: presetLoadMenu
                title: root.tr("menuLoad")
                enabled: root.presetNames.length > 0

                Instantiator {
                    model: root.presetNames
                    delegate: MenuItem {
                        text: modelData
                        onTriggered: chainModel.loadPreset(modelData)
                    }
                    onObjectAdded: (index, object) => presetLoadMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => presetLoadMenu.removeItem(object)
                }
            }

            Menu {
                id: presetDeleteMenu
                title: root.tr("menuDelete")
                enabled: root.presetNames.length > 0

                Instantiator {
                    model: root.presetNames
                    delegate: MenuItem {
                        text: modelData
                        onTriggered: {
                            chainModel.deletePreset(modelData)
                            root.presetNames = chainModel.listPresets()
                        }
                    }
                    onObjectAdded: (index, object) => presetDeleteMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => presetDeleteMenu.removeItem(object)
                }
            }
        }

        Menu {
            title: root.tr("menuLanguage")
            MenuItem {
                text: "English"
                checkable: true
                checked: root.language === "en"
                onTriggered: root.language = "en"
            }
            MenuItem {
                text: "Polski"
                checkable: true
                checked: root.language === "pl"
                onTriggered: root.language = "pl"
            }
        }

        Menu {
            title: root.tr("menuTheme")
            MenuItem {
                text: root.tr("light")
                checkable: true
                checked: !root.darkMode
                onTriggered: root.darkMode = false
            }
            MenuItem {
                text: root.tr("dark")
                checkable: true
                checked: root.darkMode
                onTriggered: root.darkMode = true
            }
        }
    }

    // ==================== dialog zapisu presetu ====================
    Dialog {
        id: saveDialog
        title: root.tr("savePresetTitle")
        modal: true
        anchors.centerIn: parent
        width: Math.max(320, implicitWidth)

        // własne przyciski — standardButtons (Dialog.Save/Cancel) biorą etykiety
        // z lokalizacji systemowej Qt, nie z naszego słownika, więc np. przy
        // ustawionym English i polskim Windowsie i tak pokazywały "Zachowaj"/"Anuluj"
        footer: DialogButtonBox {
            Button {
                text: root.tr("save")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                text: root.tr("cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }

        TextField {
            id: saveNameField
            width: 280
            placeholderText: root.tr("presetName")
        }

        onAccepted: {
            var name = saveNameField.text.trim()
            if (name.length > 0) {
                chainModel.savePreset(name)
                root.presetNames = chainModel.listPresets()
                saveNameField.text = ""
            }
        }
        onOpened: saveNameField.forceActiveFocus()
    }

    // ==================== główny layout ====================
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: root.tr("effects") + ": " + chainModel.slotCount + " / " + chainModel.maxSlots
                color: root.mutedTextColor
            }
            Label {
                text: root.tr("cpu") + ": " + root.cpu.totalPercent.toFixed(2) + " %"
                color: root.cpu.totalPercent > 50 ? "#e0a030" : "#4caf50"
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Button {
                text: root.tr("addSlot")
                enabled: chainModel.slotCount < chainModel.maxSlots
                onClicked: chainModel.addSlot()
            }
            Button {
                text: root.tr("removeSlot")
                enabled: chainModel.slotCount > chainModel.minSlots
                onClicked: chainModel.removeSlot()
            }
            Button {
                text: root.tr("clear")
                onClicked: chainModel.resetChain()
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
                color: root.panelColor
                radius: 6
                border.color: root.borderColor

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
                            text: root.tr("slot") + " " + (slotPanel.slotIndex + 1)
                            color: root.mutedTextColor
                            font.pixelSize: 12
                        }

                        Label {
                            visible: !slotPanel.slotData.empty
                            text: root.slotCpuMicros(slotPanel.slotIndex).toFixed(1) + " us"
                            color: root.dimTextColor
                            font.pixelSize: 11
                        }

                        ComboBox {
                            id: combo
                            Layout.fillWidth: true
                            model: [root.tr("none")].concat(chainModel.availableEffects)
                            currentIndex: {
                                var i = model.indexOf(slotPanel.slotData.name)
                                return i >= 0 ? i : 0
                            }
                            onActivated: function(idx) {
                                var name = model[idx]
                                chainModel.setSlotEffect(slotPanel.slotIndex, name === root.tr("none") ? "" : name)
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
                                    color: root.textColor
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
                                    color: root.mutedTextColor
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
                                      ? (root.tr("stringLabel") + ": " + tunerPanel.reading.stringName
                                         + "   " + tunerPanel.reading.freq.toFixed(1) + " Hz")
                                      : root.tr("noSignal")
                                color: root.textColor
                                font.pixelSize: 15
                            }

                            Rectangle {
                                id: meterTrack
                                Layout.fillWidth: true
                                height: 26
                                radius: 4
                                color: root.trackColor
                                border.color: root.trackBorderColor

                                Rectangle {
                                    // znacznik "0 centow" na środku (cel strojenia)
                                    width: 2
                                    height: parent.height
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: root.dimTextColor
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
                                text: "-50                    0                    +50"
                                color: root.dimTextColor
                                font.pixelSize: 10
                            }

                            Label {
                                text: !tunerPanel.haveReading ? ""
                                      : (tunerPanel.reading.inTune ? root.tr("inTune")
                                         : (tunerPanel.reading.cents > 0 ? root.tr("tooHigh") : root.tr("tooLow")))
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
                        text: root.tr("emptySlot")
                        color: root.dimTextColor
                        font.italic: true
                    }
                }
            }
        }

        }

        Button {
            Layout.fillWidth: true
            text: chainModel.recording ? root.tr("stopRecording") : root.tr("record")
            highlighted: chainModel.recording
            onClicked: chainModel.toggleRecording()
        }
    }
}
