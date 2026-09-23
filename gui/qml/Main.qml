import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 960
    height: 520
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

    // Domyślny styl QtQuick.Controls sam nie wie nic o naszym motywie
    // (dark/light) — bez tego przyciski/pola miały białe tło niezależnie od
    // reszty aplikacji, przez co tekst na nich bywał nieczytelny. `palette`
    // ustawiona raz tutaj spływa w dół do wszystkich Buttonów/ComboBoxów/
    // TextFieldów automatycznie.
    palette.window: bgColor
    palette.windowText: textColor
    palette.button: panelColor
    palette.buttonText: textColor
    palette.text: textColor
    palette.base: panelColor
    palette.highlight: "#4caf50"
    palette.highlightedText: "#ffffff"

    // grupa "disabled" osobno — bez tego przyciski z enabled:false (np. +/-
    // slot na granicy min/max) wyglądały identycznie jak aktywne, bo płaskie
    // przypisanie wyżej ustawia ten sam kolor we wszystkich grupach naraz
    palette.disabled.windowText: dimTextColor
    palette.disabled.buttonText: dimTextColor
    palette.disabled.text: dimTextColor
    palette.disabled.button: Qt.darker(panelColor, 1.15)

    // ==================== język (prosty słownik EN/PL) ====================
    property string language: "en"

    readonly property var strings: ({
        en: {
            title: "Digital MultiEffect",
            effects: "Effects",
            cpu: "CPU",
            addSlot: "+ slot",
            removeSlot: "- slot",
            clear: "Clear",
            audioIn: "Input:",
            audioOut: "Output:",
            applyAudio: "Apply",
            menuPresets: "Presets",
            menuSave: "Save...",
            menuLoad: "Load",
            menuDelete: "Delete",
            menuManage: "Manage presets...",
            managePresetsTitle: "Presets",
            close: "Close",
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
            title: "Digital MultiEffect",
            effects: "Efekty",
            cpu: "CPU",
            addSlot: "+ slot",
            removeSlot: "- slot",
            clear: "Wyczyść",
            audioIn: "Wejscie:",
            audioOut: "Wyjscie:",
            applyAudio: "Zastosuj",
            menuPresets: "Presety",
            menuSave: "Zapisz...",
            menuLoad: "Wczytaj",
            menuDelete: "Usuń",
            menuManage: "Zarzadzaj presetami...",
            managePresetsTitle: "Presety",
            close: "Zamknij",
            menuLanguage: "Język",
            menuTheme: "Motyw",
            light: "Jasny",
            dark: "Ciemny",
            slot: "Slot",
            emptySlot: "Pusty slot — wybierz efekt powyżej",
            record: "Nagrywaj",
            stopRecording: "Zatrzymaj nagrywanie",
            noSignal: "Brak sygnału... zagraj na strunie",
            inTune: "*** NASTROJONO! ***",
            tooHigh: "za wysoko - poluzuj strunę",
            tooLow: "za nisko - naciagnij strunę",
            stringLabel: "Struna",
            savePresetTitle: "Zapisz preset",
            presetName: "nazwa presetu",
            noPresets: "(brak presetów)",
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

    // ==================== urządzenia audio (dwie osobne listy — patrz komentarz w AudioEngine.h) ====================
    property var allAudioDevices: chainModel.listAudioDevices()
    function byRecommendedFirst(a, b) {
        return (b.recommended ? 1 : 0) - (a.recommended ? 1 : 0)
    }
    property var inputDevices: allAudioDevices.filter(function(d) { return d.maxInputChannels > 0 }).sort(byRecommendedFirst)
    property var outputDevices: allAudioDevices.filter(function(d) { return d.maxOutputChannels > 0 }).sort(byRecommendedFirst)

    // odświeżanie listy — podłączenie/odłączenie USB dzieje się poza appką,
    // bez żadnej akcji użytkownika do której mógłbym to podpiąć, więc trzeba
    // odpytywać cyklicznie (PortAudio samo nie powiadamia o zmianach)
    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: root.allAudioDevices = chainModel.listAudioDevices()
    }

    // ==================== menu ====================
    menuBar: MenuBar {
        Menu {
            title: root.tr("menuPresets")

            MenuItem {
                text: root.tr("menuSave")
                onTriggered: saveDialog.open()
            }
            MenuItem {
                text: root.tr("menuManage")
                onTriggered: {
                    root.presetNames = chainModel.listPresets()
                    managePresetsDialog.open()
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

    // ==================== dialog zarzadzania presetami ====================
    // Zwykła lista z przyciskami Wczytaj/Usuń w każdym wierszu — bez
    // zagnieżdżonych Menu z nawigacją po hover, która potrafiła się gubić
    // (submenu zamykało się zanim zdążyło się do niego dojechać po skosie).
    Dialog {
        id: managePresetsDialog
        title: root.tr("managePresetsTitle")
        modal: true
        anchors.centerIn: parent
        width: 360
        height: 320

        footer: DialogButtonBox {
            Button {
                text: root.tr("close")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            Label {
                Layout.fillWidth: true
                visible: root.presetNames.length === 0
                text: root.tr("noPresets")
                color: root.dimTextColor
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.presetNames
                spacing: 4

                delegate: RowLayout {
                    width: ListView.view.width
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: modelData
                        color: root.textColor
                        elide: Text.ElideRight
                    }
                    Button {
                        text: root.tr("menuLoad")
                        onClicked: {
                            chainModel.loadPreset(modelData)
                            managePresetsDialog.close()
                        }
                    }
                    Button {
                        text: root.tr("menuDelete")
                        onClicked: {
                            chainModel.deletePreset(modelData)
                            root.presetNames = chainModel.listPresets()
                        }
                    }
                }
            }
        }
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
                opacity: enabled ? 1.0 : 0.35
                onClicked: chainModel.addSlot()
            }
            Button {
                text: root.tr("removeSlot")
                enabled: chainModel.slotCount > chainModel.minSlots
                opacity: enabled ? 1.0 : 0.35
                onClicked: chainModel.removeSlot()
            }
            Button {
                text: root.tr("clear")
                onClicked: chainModel.resetChain()
            }
        }

        // ==================== wybór karty dźwiękowej ====================
        // Windows traktuje wejście i wyjście jako osobne urządzenia (jedno
        // fizyczne urządzenie = dwa wpisy), stąd dwa osobne dropdowny, nie jeden.
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label { text: root.tr("audioIn"); color: root.mutedTextColor }
            ComboBox {
                id: inputDeviceCombo
                Layout.fillWidth: true
                model: root.inputDevices
                textRole: "name"
                currentIndex: {
                    for (var i = 0; i < model.length; ++i)
                        if (model[i].index === chainModel.currentInputDevice) return i
                    return -1
                }
            }
            Label { text: root.tr("audioOut"); color: root.mutedTextColor }
            ComboBox {
                id: outputDeviceCombo
                Layout.fillWidth: true
                model: root.outputDevices
                textRole: "name"
                currentIndex: {
                    for (var i = 0; i < model.length; ++i)
                        if (model[i].index === chainModel.currentOutputDevice) return i
                    return -1
                }
            }
            Button {
                text: root.tr("applyAudio")
                enabled: inputDeviceCombo.currentIndex >= 0 && outputDeviceCombo.currentIndex >= 0
                onClicked: chainModel.selectAudioDevices(
                    root.inputDevices[inputDeviceCombo.currentIndex].index,
                    root.outputDevices[outputDeviceCombo.currentIndex].index
                )
            }
        }

        // ==================== oscyloskop (prosta wizualizacja wyjścia) ====================
        Rectangle {
            id: wavePanel
            Layout.fillWidth: true
            height: 70
            color: root.trackColor
            border.color: root.trackBorderColor
            radius: 4

            property var samples: []

            Timer {
                interval: 33   // ~30 fps, wystarczy dla płynnego oscyloskopu
                running: true
                repeat: true
                onTriggered: {
                    wavePanel.samples = chainModel.waveform()
                    waveCanvas.requestPaint()
                }
            }

            Canvas {
                id: waveCanvas
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    var mid = height / 2

                    // linia zerowa
                    ctx.strokeStyle = root.trackBorderColor
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(0, mid)
                    ctx.lineTo(width, mid)
                    ctx.stroke()

                    var s = wavePanel.samples
                    if (!s || s.length < 2) return

                    // wzmocnienie tylko na potrzeby rysowania (nie dotyka dźwięku) —
                    // realny sygnał gitarowy rzadko sięga pełnego zakresu ±1.0,
                    // więc bez tego wykres wygląda na prawie płaski
                    var visualGain = 4.0

                    ctx.strokeStyle = "#4caf50"
                    ctx.lineWidth = 1.5
                    ctx.beginPath()
                    for (var i = 0; i < s.length; ++i) {
                        var x = (i / (s.length - 1)) * width
                        var v = Math.max(-1, Math.min(1, s[i] * visualGain))
                        var y = mid - v * mid * 0.9   // *0.9, zeby fala nie docinala sie na krawedziach
                        if (i === 0) ctx.moveTo(x, y)
                        else ctx.lineTo(x, y)
                    }
                    ctx.stroke()
                }
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

                        // grafika efektu — prawdziwy plik z assets/effects/, w wolnym miejscu pod suwakami
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 60

                            Image {
                                id: iconImage
                                anchors.centerIn: parent
                                // bazuje na wymiarach całego slotPanel (te same dla każdego
                                // typu panelu), nie na lokalnie zostającym miejscu — inaczej
                                // panel Tunera (więcej elementów nad ikoną) zawsze wychodził mniejszy
                                width: Math.min(Math.min(slotPanel.width, slotPanel.height) * 0.75, parent.height * 0.98)
                                height: width
                                fillMode: Image.PreserveAspectFit
                                source: slotPanel.slotData.empty ? ""
                                        : chainModel.effectIconUrl(slotPanel.slotData.name)
                                visible: status === Image.Ready
                                asynchronous: true
                            }

                            Label {
                                anchors.centerIn: parent
                                width: parent.width
                                visible: !slotPanel.slotData.empty && iconImage.status !== Image.Ready
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                font.pixelSize: 10
                                color: root.dimTextColor
                                text: "assets/effects/" + slotPanel.slotData.name.toLowerCase() + ".png"
                            }
                        }
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
                            spacing: 5

                            Label {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                text: tunerPanel.haveReading
                                      ? (root.tr("stringLabel") + ": " + tunerPanel.reading.stringName
                                         + "   " + tunerPanel.reading.freq.toFixed(1) + " Hz")
                                      : root.tr("noSignal")
                                color: root.textColor
                                font.pixelSize: 13
                            }

                            Rectangle {
                                id: meterTrack
                                Layout.fillWidth: true
                                height: 16
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

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label { text: "-50"; color: root.dimTextColor; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }
                                Label { text: "0"; color: root.dimTextColor; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }
                                Label { text: "+50"; color: root.dimTextColor; font.pixelSize: 10 }
                            }

                            Label {
                                text: !tunerPanel.haveReading ? ""
                                      : (tunerPanel.reading.inTune ? root.tr("inTune")
                                         : (tunerPanel.reading.cents > 0 ? root.tr("tooHigh") : root.tr("tooLow")))
                                color: tunerPanel.haveReading && tunerPanel.reading.inTune ? "#4caf50" : "#e0a030"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            // grafika efektu — ten sam mechanizm co w panelu zwykłego efektu
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 60

                                Image {
                                    id: tunerIconImage
                                    anchors.centerIn: parent
                                    // ta sama reguła co iconImage w zwykłym panelu — bazuje na
                                    // wymiarach slotPanel, nie na lokalnie zostającym miejscu
                                    width: Math.min(Math.min(slotPanel.width, slotPanel.height) * 0.75, parent.height * 0.98)
                                    height: width
                                    fillMode: Image.PreserveAspectFit
                                    source: chainModel.effectIconUrl("Tuner")
                                    visible: status === Image.Ready
                                    asynchronous: true
                                }

                                Label {
                                    anchors.centerIn: parent
                                    width: parent.width
                                    visible: tunerIconImage.status !== Image.Ready
                                    horizontalAlignment: Text.AlignHCenter
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 10
                                    color: root.dimTextColor
                                    text: "assets/effects/tuner.png"
                                }
                            }
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
