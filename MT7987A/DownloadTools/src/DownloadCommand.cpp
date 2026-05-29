#include "DownloadCommand.h"

#include <QDir>
#include <QOperatingSystemVersion>

#include <utility>

DownloadCommandBuilder::DownloadCommandBuilder(QString mt7987aRoot)
    : m_root(std::move(mt7987aRoot))
{
}

DownloadCommand DownloadCommandBuilder::build(const BoardModelInfo &model, const DownloadOptions &options) const
{
    DownloadCommand command;

#ifdef Q_OS_WIN
    command.program = "powershell.exe";
    command.workingDirectory = m_root;
    command.arguments = {
        "-ExecutionPolicy", "Bypass",
        "-NoProfile",
        "-File", QDir(m_root).filePath("download.ps1"),
        "-Model", model.id,
    };
    if (!options.serialPort.trimmed().isEmpty()) {
        command.arguments << "-SerialPort" << options.serialPort.trimmed();
    }
    if (!options.deviceIp.trimmed().isEmpty()) {
        command.arguments << "-DeviceIp" << options.deviceIp.trimmed();
    }
    command.arguments << "-StartAt" << options.startAt;
    command.arguments << "-WaitDeviceSeconds" << QString::number(options.waitDeviceSeconds);
    if (options.skipUartBoot) {
        command.arguments << "-SkipUartBoot";
    }
    if (options.dryRun) {
        command.arguments << "-DryRun";
    }
#else
    command.program = "/bin/bash";
    command.workingDirectory = m_root;
    command.arguments = {
        QDir(m_root).filePath("download-mac.sh"),
        "--model", model.id,
    };
    if (!options.serialPort.trimmed().isEmpty()) {
        command.arguments << "--serial" << options.serialPort.trimmed();
    }
    if (!options.deviceIp.trimmed().isEmpty()) {
        command.arguments << "--ip" << options.deviceIp.trimmed();
    }
    command.arguments << "--start-at" << options.startAt;
    command.arguments << "--wait-device" << QString::number(options.waitDeviceSeconds);
    if (options.skipUartBoot) {
        command.arguments << "--skip-uart";
    }
    if (options.dryRun) {
        command.arguments << "--dry-run";
    }
#endif

    return command;
}

QString DownloadCommand::displayText() const
{
    QStringList escaped;
    escaped << program;
    for (const auto &arg : arguments) {
        if (arg.contains(' ')) {
            escaped << QString("\"%1\"").arg(arg);
        } else {
            escaped << arg;
        }
    }
    return escaped.join(' ');
}
