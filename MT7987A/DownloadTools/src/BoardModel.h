#pragma once

#include <QString>
#include <QVector>

struct FirmwareComponentInfo {
    QString name;
    QString file;
    bool required = true;
};

struct BoardModelInfo {
    QString id;
    QString displayName;
    QString firmwareFile;
    QString directoryName;
    QString defaultIp = "192.168.1.1";
    QString defaultStartStep = "FIRMWARE";
    QString boardImage;
    int waitDeviceSeconds = 120;
    QVector<FirmwareComponentInfo> firmwareComponents;
};

class BoardModelRepository {
public:
    explicit BoardModelRepository(QString mt7987aRoot);

    QVector<BoardModelInfo> models() const;
    BoardModelInfo modelById(const QString &id) const;
    QString boardDirectory(const BoardModelInfo &model) const;
    QString firmwarePath(const BoardModelInfo &model) const;
    QString componentPath(const BoardModelInfo &model, const FirmwareComponentInfo &component) const;
    bool firmwareExists(const BoardModelInfo &model) const;
    bool componentExists(const BoardModelInfo &model, const FirmwareComponentInfo &component) const;
    QString configPath() const;
    bool validate(const QVector<BoardModelInfo> &models, QString *errorMessage = nullptr) const;
    bool save(const QVector<BoardModelInfo> &models, QString *errorMessage = nullptr) const;
    bool exportConfig(const QString &targetPath, QString *errorMessage = nullptr) const;
    bool importConfig(const QString &sourcePath, QString *errorMessage = nullptr);
    void reload();

private:
    QVector<BoardModelInfo> defaultModels() const;
    void loadOrCreateConfig();
    bool load(QString *errorMessage = nullptr);
    QString m_root;
    QVector<BoardModelInfo> m_models;
};
