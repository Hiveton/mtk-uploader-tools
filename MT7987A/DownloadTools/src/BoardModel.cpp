#include "BoardModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

#include <utility>

namespace {
QJsonObject componentToJson(const FirmwareComponentInfo &component)
{
    QJsonObject object;
    object["name"] = component.name;
    object["file"] = component.file;
    object["required"] = component.required;
    return object;
}

FirmwareComponentInfo componentFromJson(const QJsonObject &object)
{
    FirmwareComponentInfo component;
    component.name = object.value("name").toString();
    component.file = object.value("file").toString();
    component.required = object.value("required").toBool(true);
    return component;
}

QJsonObject modelToJson(const BoardModelInfo &model)
{
    QJsonArray components;
    for (const auto &component : model.firmwareComponents) {
        components.append(componentToJson(component));
    }

    QJsonObject object;
    object["id"] = model.id;
    object["displayName"] = model.displayName;
    object["directoryName"] = model.directoryName;
    object["firmwareFile"] = model.firmwareFile;
    object["defaultIp"] = model.defaultIp;
    object["defaultStartStep"] = model.defaultStartStep;
    object["boardImage"] = model.boardImage;
    object["waitDeviceSeconds"] = model.waitDeviceSeconds;
    object["firmwareComponents"] = components;
    return object;
}

BoardModelInfo modelFromJson(const QJsonObject &object)
{
    BoardModelInfo model;
    model.id = object.value("id").toString();
    model.displayName = object.value("displayName").toString(model.id);
    model.directoryName = object.value("directoryName").toString(model.id);
    model.firmwareFile = object.value("firmwareFile").toString();
    model.defaultIp = object.value("defaultIp").toString("192.168.1.1");
    model.defaultStartStep = object.value("defaultStartStep").toString("FIRMWARE");
    model.boardImage = object.value("boardImage").toString("assets/board-preview.png");
    model.waitDeviceSeconds = object.value("waitDeviceSeconds").toInt(120);

    const auto components = object.value("firmwareComponents").toArray();
    for (const auto &value : components) {
        const auto component = componentFromJson(value.toObject());
        if (!component.name.trimmed().isEmpty() && !component.file.trimmed().isEmpty()) {
            model.firmwareComponents.append(component);
        }
    }
    if (model.firmwareComponents.isEmpty() && !model.firmwareFile.trimmed().isEmpty()) {
        model.firmwareComponents.append({"FIRMWARE", model.firmwareFile, true});
    }
    return model;
}
}

BoardModelRepository::BoardModelRepository(QString mt7987aRoot)
    : m_root(std::move(mt7987aRoot))
{
    loadOrCreateConfig();
}

QVector<BoardModelInfo> BoardModelRepository::defaultModels() const
{
    return {
        {"H87Pro", "H87Pro", "87pro.bin", "H87Pro", "192.168.1.1", "FIRMWARE", "assets/board-preview.png", 120, {{"BL2", "bl2.img", true}, {"GPT", "gpt.bin", true}, {"FIP", "fip.bin", true}, {"FIRMWARE", "87pro.bin", true}}},
        {"H87AM", "H87AM", "HiGoROS-H5000AM-1-26-05-29-02.bin", "H87AM", "192.168.1.1", "FIRMWARE", "assets/board-preview.png", 120, {{"BL2", "bl2.img", true}, {"GPT", "gpt.bin", true}, {"FIP", "fip.bin", true}, {"FIRMWARE", "HiGoROS-H5000AM-1-26-05-29-02.bin", true}}},
        {"H5MIFI", "H5MIFI", "87pro.bin", "H5MIFI", "192.168.1.1", "FIRMWARE", "assets/board-preview.png", 120, {{"BL2", "bl2.img", true}, {"GPT", "gpt.bin", true}, {"FIP", "fip.bin", true}, {"FIRMWARE", "87pro.bin", true}}},
        {"H5000M", "H5000M", "HiGoROS-H5000M-1-26-04-29-09.bin", "H5000M", "192.168.1.1", "FIRMWARE", "assets/board-preview.png", 120, {{"BL2", "bl2.img", true}, {"GPT", "gpt.bin", true}, {"FIP", "fip.bin", true}, {"FIRMWARE", "HiGoROS-H5000M-1-26-04-29-09.bin", true}}},
        {"H5000W", "H5000W", "HiGoROS-H5000M-1-26-04-29-09.bin", "H5000W", "192.168.1.1", "FIRMWARE", "assets/board-preview.png", 120, {{"BL2", "bl2.img", true}, {"GPT", "gpt.bin", true}, {"FIP", "fip.bin", true}, {"FIRMWARE", "HiGoROS-H5000M-1-26-04-29-09.bin", true}}},
        {"E87N", "E87N", "HiGoROS-E87N-1-26-05-09-02.bin", "E87N", "192.168.1.1", "FIRMWARE", "assets/board-preview.png", 120, {{"BL2", "bl2.img", true}, {"GPT", "gpt.bin", true}, {"FIP", "fip.bin", true}, {"FIRMWARE", "HiGoROS-E87N-1-26-05-09-02.bin", true}}},
    };
}

void BoardModelRepository::loadOrCreateConfig()
{
    QString errorMessage;
    if (load(&errorMessage)) {
        return;
    }

    m_models = defaultModels();
    save(m_models);
}

bool BoardModelRepository::load(QString *errorMessage)
{
    QFile file(configPath());
    if (!file.exists()) {
        if (errorMessage) {
            *errorMessage = "配置文件不存在。";
        }
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return false;
    }

    QVector<BoardModelInfo> loaded;
    const auto boards = document.object().value("boards").toArray();
    for (const auto &value : boards) {
        const auto model = modelFromJson(value.toObject());
        if (!model.id.trimmed().isEmpty() && !model.directoryName.trimmed().isEmpty()) {
            loaded.append(model);
        }
    }
    if (loaded.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "配置中没有板卡。";
        }
        return false;
    }

    m_models = loaded;
    return true;
}

bool BoardModelRepository::save(const QVector<BoardModelInfo> &models, QString *errorMessage) const
{
    if (!validate(models, errorMessage)) {
        return false;
    }

    QJsonArray boards;
    for (const auto &model : models) {
        boards.append(modelToJson(model));
    }

    QJsonObject root;
    root["schemaVersion"] = 1;
    root["defaultBoard"] = models.isEmpty() ? QString() : models.first().id;
    root["boards"] = boards;

    if (QFileInfo::exists(configPath())) {
        const QString backupPath = configPath() + ".backup";
        QFile::remove(backupPath);
        QFile::copy(configPath(), backupPath);
    }

    QSaveFile file(configPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

bool BoardModelRepository::validate(const QVector<BoardModelInfo> &models, QString *errorMessage) const
{
    if (models.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "至少需要保留一块板卡。";
        }
        return false;
    }

    QSet<QString> ids;
    for (const auto &model : models) {
        const QString id = model.id.trimmed();
        if (id.isEmpty()) {
            if (errorMessage) {
                *errorMessage = "板卡 ID 不能为空。";
            }
            return false;
        }
        if (ids.contains(id.toLower())) {
            if (errorMessage) {
                *errorMessage = "板卡 ID 重复：" + id;
            }
            return false;
        }
        ids.insert(id.toLower());

        if (model.directoryName.trimmed().isEmpty()) {
            if (errorMessage) {
                *errorMessage = "板卡目录不能为空：" + id;
            }
            return false;
        }
        if (model.firmwareComponents.isEmpty()) {
            if (errorMessage) {
                *errorMessage = "板卡至少需要一个固件组件：" + id;
            }
            return false;
        }

        QSet<QString> componentNames;
        bool hasFirmware = false;
        for (const auto &component : model.firmwareComponents) {
            const QString name = component.name.trimmed().toUpper();
            if (name.isEmpty() || component.file.trimmed().isEmpty()) {
                if (errorMessage) {
                    *errorMessage = "组件名称和文件不能为空：" + id;
                }
                return false;
            }
            if (componentNames.contains(name)) {
                if (errorMessage) {
                    *errorMessage = "组件重复：" + name + "，板卡：" + id;
                }
                return false;
            }
            componentNames.insert(name);
            hasFirmware = hasFirmware || name == "FIRMWARE";
        }
        if (!hasFirmware) {
            if (errorMessage) {
                *errorMessage = "板卡需要 FIRMWARE 固件组件：" + id;
            }
            return false;
        }
    }
    return true;
}

bool BoardModelRepository::exportConfig(const QString &targetPath, QString *errorMessage) const
{
    QFile::remove(targetPath);
    if (!QFile::copy(configPath(), targetPath)) {
        if (errorMessage) {
            *errorMessage = "无法导出配置到：" + targetPath;
        }
        return false;
    }
    return true;
}

bool BoardModelRepository::importConfig(const QString &sourcePath, QString *errorMessage)
{
    QFile file(sourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = "配置无效：" + parseError.errorString();
        }
        return false;
    }

    QVector<BoardModelInfo> imported;
    const auto boards = document.object().value("boards").toArray();
    for (const auto &value : boards) {
        imported.append(modelFromJson(value.toObject()));
    }
    if (!save(imported, errorMessage)) {
        return false;
    }
    reload();
    return true;
}

void BoardModelRepository::reload()
{
    loadOrCreateConfig();
}

QVector<BoardModelInfo> BoardModelRepository::models() const
{
    return m_models;
}

BoardModelInfo BoardModelRepository::modelById(const QString &id) const
{
    for (const auto &model : m_models) {
        if (model.id.compare(id, Qt::CaseInsensitive) == 0) {
            return model;
        }
    }
    return m_models.isEmpty() ? BoardModelInfo{} : m_models.first();
}

QString BoardModelRepository::boardDirectory(const BoardModelInfo &model) const
{
    return QDir(m_root).filePath(model.directoryName);
}

QString BoardModelRepository::firmwarePath(const BoardModelInfo &model) const
{
    return QDir(boardDirectory(model)).filePath(model.firmwareFile);
}

QString BoardModelRepository::componentPath(const BoardModelInfo &model, const FirmwareComponentInfo &component) const
{
    return QDir(boardDirectory(model)).filePath(component.file);
}

bool BoardModelRepository::firmwareExists(const BoardModelInfo &model) const
{
    return QFileInfo::exists(firmwarePath(model));
}

bool BoardModelRepository::componentExists(const BoardModelInfo &model, const FirmwareComponentInfo &component) const
{
    return QFileInfo::exists(componentPath(model, component));
}

QString BoardModelRepository::configPath() const
{
    return QDir(m_root).filePath("boards.json");
}
