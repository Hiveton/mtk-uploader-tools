#include "DownloadProcess.h"

#include <QDateTime>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStringDecoder>

namespace {
QString decodeProcessOutput(const QByteArray &bytes)
{
    QStringDecoder utf8Decoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    QString text = utf8Decoder.decode(bytes);
    if (!utf8Decoder.hasError()) {
        return text;
    }
    return QString::fromLocal8Bit(bytes);
}
}

DownloadProcess::DownloadProcess(QObject *parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);

    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString text = decodeProcessOutput(m_process.readAllStandardOutput());
        const auto lines = text.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            emit outputLine(line);
        }
    });

    connect(&m_process, &QProcess::started, this, [this]() {
        emit stateChanged(true);
    });

    connect(&m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        emit stateChanged(false);
        emit finished(exitCode, status);
    });

    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit stateChanged(false);
            emit failedToStart(m_process.errorString());
        }
    });
}

bool DownloadProcess::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void DownloadProcess::start(const DownloadCommand &command)
{
    if (isRunning()) {
        emit outputLine("已有下载任务正在运行。");
        return;
    }

    emit outputLine(QString("[%1] RUN %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), command.displayText()));
    m_process.setWorkingDirectory(command.workingDirectory);
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("PYTHONIOENCODING", "utf-8");
    environment.insert("POWERSHELL_OUTPUT_ENCODING", "utf-8");
    environment.insert("RUST_BACKTRACE", "0");
#ifdef Q_OS_WIN
    environment.insert("POWERSHELL_TELEMETRY_OPTOUT", "1");
#endif
    m_process.setProcessEnvironment(environment);
    m_process.start(command.program, command.arguments);
}

void DownloadProcess::stop()
{
    if (!isRunning()) {
        return;
    }

    emit outputLine("正在停止下载进程...");
    m_process.terminate();
    if (!m_process.waitForFinished(3000)) {
        m_process.kill();
    }
}
