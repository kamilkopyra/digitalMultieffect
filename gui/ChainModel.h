#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "AudioEngine.h"

// Most między silnikiem audio (AudioEngine/EffectChain, zwykły C++, bez
// wiedzy o Qt) a QML. Cała logika DSP zostaje nietknięta — ta klasa tylko
// tłumaczy jej stan na właściwości/wywołania Qt i wywołania z UI na te same
// metody EffectChain, których wcześniej używała klawiatura.
//
// GUI adresuje sloty (0-2) bezpośrednio — ma miejsce, żeby pokazać
// wszystkie naraz, więc nie potrzebuje koncepcji "focused" (ta zostaje
// tylko dla klawiatury/enkoderów, które mają wspólne 3 gałki).
class ChainModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableEffects READ availableEffects CONSTANT)
    Q_PROPERTY(QVariantList chainSlots READ chainSlots NOTIFY chainChanged)
    Q_PROPERTY(int slotCount READ slotCount NOTIFY chainChanged)
    Q_PROPERTY(int minSlots READ minSlots CONSTANT)
    Q_PROPERTY(int maxSlots READ maxSlots CONSTANT)
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)

public:
    explicit ChainModel(QObject* parent = nullptr);
    ~ChainModel() override;

    // uruchamia PortAudio — wywołać raz, po utworzeniu obiektu
    void start();

    QStringList availableEffects() const;
    QVariantList chainSlots();
    int slotCount();
    int minSlots() const;
    int maxSlots() const;
    bool isRecording();

public slots:
    // name == "" czyści slot
    void setSlotEffect(int index, const QString& name);
    void setSlotParam(int index, int paramIdx, int value);
    void addSlot();
    void removeSlot();
    // czyści wszystkie sloty i ścina ich liczbę z powrotem do domyślnych (minSlots)
    void resetChain();
    void toggleRecording();

    // {} (pusta mapa) jeśli w tym slocie nie ma Tunera albo nic jeszcze
    // nie wykryto — QML odpytuje to Timerem, bo wynik zmienia się z wątku
    // analizy, nie z akcji użytkownika (więc nie ma tu sensownego sygnału)
    QVariantMap tunerReading(int index);

    // statystyki obciążenia CPU — jak wyżej, odpytywane Timerem (dane
    // zmieniają się z wątku audio, nie z akcji użytkownika)
    QVariantMap cpuStats();

    // --- presety (zapisują/wczytują cały łańcuch: efekty + parametry) ---
    QStringList listPresets();
    bool savePreset(const QString& name);
    bool loadPreset(const QString& name);
    bool deletePreset(const QString& name);

signals:
    void chainChanged();
    void recordingChanged();

private:
    AudioEngine engine;
    Effect* createEffect(const QString& name) const;
};
