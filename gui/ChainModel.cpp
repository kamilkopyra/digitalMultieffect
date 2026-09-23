#include "ChainModel.h"

#include "Fuzz.h"
#include "Tremolo.h"
#include "Tuner.h"
#include "NoiseGate.h"
#include "Rat.h"
#include "TubeScreamer.h"
#include "Delay.h"
#include "Compressor.h"
#include "Vibrato.h"
#include "Flanger.h"
#include "Phaser.h"
#include "AutoWah.h"
#include "Equalizer.h"
#include "Boost.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QUrl>
#include <filesystem>

ChainModel::ChainModel(QObject* parent) : QObject(parent) {}

ChainModel::~ChainModel() {
    engine.stop();
}

void ChainModel::start() {
    if (engine.init_single_effect(16) != 0) {
        qWarning() << "Nie udalo sie uruchomic PortAudio (brak interfejsu audio?) "
                       "- GUI dziala, ale bez dzwieku.";
    }
}

QStringList ChainModel::availableEffects() const {
    return {
        "Fuzz", "Tremolo", "Tuner", "NoiseGate", "Rat", "TubeScreamer",
        "Delay", "Compressor", "Vibrato", "Flanger", "Phaser", "AutoWah",
        "Equalizer", "Boost"
    };
}

Effect* ChainModel::createEffect(const QString& name) const {
    if (name == "Fuzz")         return new Fuzz();
    if (name == "Tremolo")      return new Tremolo();
    if (name == "Tuner")        return new Tuner();
    if (name == "NoiseGate")    return new NoiseGate();
    if (name == "Rat")          return new Rat();
    if (name == "TubeScreamer") return new TubeScreamer();
    if (name == "Delay")        return new Delay();
    if (name == "Compressor")   return new Compressor();
    if (name == "Vibrato")      return new Vibrato();
    if (name == "Flanger")      return new Flanger();
    if (name == "Phaser")       return new Phaser();
    if (name == "AutoWah")      return new AutoWah();
    if (name == "Equalizer")    return new Equalizer();
    if (name == "Boost")        return new Boost();
    return nullptr;   // "" -> pusty slot
}

void ChainModel::setSlotEffect(int index, const QString& name) {
    engine.getChain().setSlot(index, createEffect(name));
    emit chainChanged();
}

void ChainModel::setSlotParam(int index, int paramIdx, int value) {
    engine.getChain().setSlotParam(index, paramIdx, value);
    emit chainChanged();
}

void ChainModel::addSlot() {
    engine.getChain().addSlot();
    emit chainChanged();
}

void ChainModel::removeSlot() {
    engine.getChain().removeSlot();
    emit chainChanged();
}

void ChainModel::resetChain() {
    EffectChain& chain = engine.getChain();
    chain.clear();   // opróżnia wszystkie aktywne sloty (removeSlot i tak by to zrobił, ale jawnie)
    // wraca do domyślnych 3 slotów (nie do minimum 1) — "wyczyść" ma dać
    // pusty punkt startowy, nie najmniejszy możliwy
    while (chain.slotCount() > EffectChain::defaultSlots)
        chain.removeSlot();
    while (chain.slotCount() < EffectChain::defaultSlots)
        chain.addSlot();
    qInfo() << "Lancuch wyczyszczony.";
    emit chainChanged();
}

int ChainModel::slotCount() {
    return engine.getChain().slotCount();
}

int ChainModel::minSlots() const {
    return EffectChain::minSlots;
}

int ChainModel::maxSlots() const {
    return EffectChain::maxSlots;
}

void ChainModel::toggleRecording() {
    if (engine.wavWriter.isRecording()) {
        engine.wavWriter.stop();
        qInfo() << "Nagrywanie zatrzymane.";
    }
    else {
        std::string nazwa = engine.getChain().getName();
        std::filesystem::create_directories("./recordings");
        std::string path = "./recordings/" + nazwa + ".wav";
        bool ok = engine.wavWriter.start(path);
        if (ok) {
            std::string abs = std::filesystem::absolute(path).string();
            qInfo() << "Nagrywanie rozpoczete:" << QString::fromStdString(abs);
        }
        else {
            qWarning() << "Nie udalo sie otworzyc pliku do nagrywania:" << QString::fromStdString(path);
        }
    }
    emit recordingChanged();
}

QVariantList ChainModel::chainSlots() {
    QVariantList result;
    for (const auto& info : engine.getChain().listEffects()) {
        QVariantMap m;
        m["name"] = QString::fromStdString(info.name);
        m["empty"] = info.empty;

        QVariantList params;
        for (int p = 0; p < 3; ++p) {
            QVariantMap pm;
            pm["name"] = QString::fromStdString(info.paramNames[p]);
            pm["value"] = info.params[p];
            params.append(pm);
        }
        m["params"] = params;

        result.append(m);
    }
    return result;
}

bool ChainModel::isRecording() {
    return engine.wavWriter.isRecording();
}

QVariantMap ChainModel::cpuStats() {
    QVariantMap result;
    EffectChain& chain = engine.getChain();

    result["totalPercent"] = chain.cpuLoadPercent();

    QVariantList perSlot;
    int n = chain.slotCount();
    for (int i = 0; i < n; ++i)
        perSlot.append(chain.slotCpuMicros(i));
    result["perSlotMicros"] = perSlot;

    return result;
}

QVariantList ChainModel::waveform() {
    QVariantList result;
    for (float v : engine.snapshotWaveform())
        result.append(v);
    return result;
}

QStringList ChainModel::listPresets() {
    QStringList result;
    std::filesystem::create_directories("./presets");
    for (const auto& entry : std::filesystem::directory_iterator("./presets")) {
        if (entry.path().extension() == ".json")
            result.append(QString::fromStdString(entry.path().stem().string()));
    }
    return result;
}

bool ChainModel::savePreset(const QString& name) {
    if (name.trimmed().isEmpty()) return false;

    QJsonArray slotsArr;
    for (const auto& info : engine.getChain().listEffects()) {
        QJsonObject o;
        o["name"] = QString::fromStdString(info.name);
        QJsonArray params;
        for (int p = 0; p < 3; ++p) params.append(info.params[p]);
        o["params"] = params;
        slotsArr.append(o);
    }
    QJsonObject root;
    root["slots"] = slotsArr;

    std::filesystem::create_directories("./presets");
    QFile f("./presets/" + name + ".json");
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "Nie udalo sie zapisac presetu:" << f.fileName();
        return false;
    }
    f.write(QJsonDocument(root).toJson());
    f.close();
    qInfo() << "Zapisano preset:" << f.fileName();
    return true;
}

bool ChainModel::loadPreset(const QString& name) {
    QFile f("./presets/" + name + ".json");
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Nie udalo sie wczytac presetu:" << f.fileName();
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return false;

    QJsonArray slotsArr = doc.object()["slots"].toArray();
    EffectChain& chain = engine.getChain();
    chain.clear();

    // dopasuj liczbę slotów do zapisanej w presecie (w granicach min/max)
    int wanted = slotsArr.size();
    while (chain.slotCount() < wanted && chain.slotCount() < EffectChain::maxSlots) chain.addSlot();
    while (chain.slotCount() > wanted && chain.slotCount() > EffectChain::minSlots) chain.removeSlot();

    for (int i = 0; i < slotsArr.size() && i < chain.slotCount(); ++i) {
        QJsonObject o = slotsArr[i].toObject();
        QString effectName = o["name"].toString();
        if (effectName.isEmpty()) continue;   // pusty slot w presecie

        Effect* e = createEffect(effectName);
        if (!e) continue;
        chain.setSlot(i, e);

        QJsonArray params = o["params"].toArray();
        for (int p = 0; p < 3 && p < params.size(); ++p)
            chain.setSlotParam(i, p, params[p].toInt());
    }

    emit chainChanged();
    qInfo() << "Wczytano preset:" << f.fileName();
    return true;
}

QString ChainModel::effectIconUrl(const QString& name) const {
    QString path = QCoreApplication::applicationDirPath() + "/assets/effects/" + name.toLower() + ".png";
    return QUrl::fromLocalFile(path).toString();
}

QVariantList ChainModel::listAudioDevices() {
    QVariantList result;
    for (const auto& d : AudioEngine::listDevices()) {
        QVariantMap m;
        m["index"] = d.index;
        m["name"] = (d.recommended ? QString::fromUtf8("★ ") : QString())
                    + QString::fromStdString(d.name);
        m["maxInputChannels"] = d.maxInputChannels;
        m["maxOutputChannels"] = d.maxOutputChannels;
        m["recommended"] = d.recommended;
        result.append(m);
    }
    return result;
}

bool ChainModel::selectAudioDevices(int inputIndex, int outputIndex) {
    engine.stop();
    bool ok = engine.init_single_effect(16, inputIndex, outputIndex) == 0;
    if (ok)
        qInfo() << "Przelaczono audio - wejscie:" << inputIndex << "wyjscie:" << outputIndex;
    else
        qWarning() << "Nie udalo sie uruchomic audio - wejscie:" << inputIndex << "wyjscie:" << outputIndex;
    emit audioDeviceChanged();
    return ok;
}

int ChainModel::currentInputDevice() {
    return engine.currentInputDevice();
}

int ChainModel::currentOutputDevice() {
    return engine.currentOutputDevice();
}

bool ChainModel::deletePreset(const QString& name) {
    QFile f("./presets/" + name + ".json");
    if (!f.exists()) {
        qWarning() << "Preset nie istnieje:" << f.fileName();
        return false;
    }
    bool ok = f.remove();
    if (ok)
        qInfo() << "Usunieto preset:" << f.fileName();
    else
        qWarning() << "Nie udalo sie usunac presetu:" << f.fileName();
    return ok;
}

QVariantMap ChainModel::tunerReading(int index) {
    QVariantMap result;

    Effect* e = engine.getChain().slot(index);
    Tuner* tuner = dynamic_cast<Tuner*>(e);
    if (!tuner) return result;   // pusta mapa = "nie ma tu tunera"

    TunerReading r = tuner->getReading();
    if (!r.valid) return result;

    result["freq"] = r.freq;
    result["target"] = r.target;
    result["cents"] = r.cents;
    result["stringName"] = QString::fromStdString(r.stringName);
    result["inTune"] = r.inTune;
    result["valid"] = true;
    return result;
}
