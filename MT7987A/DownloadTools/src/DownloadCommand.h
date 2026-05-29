#pragma once

#include "BoardModel.h"

#include <QString>
#include <QStringList>

struct DownloadOptions {
    QString serialPort;
    QString deviceIp = "192.168.1.1";
    QString startAt = "BL2";
    int waitDeviceSeconds = 120;
    bool skipUartBoot = false;
    bool dryRun = false;
};

struct DownloadCommand {
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QString displayText() const;
};

class DownloadCommandBuilder {
public:
    explicit DownloadCommandBuilder(QString mt7987aRoot);

    DownloadCommand build(const BoardModelInfo &model, const DownloadOptions &options) const;

private:
    QString m_root;
};
