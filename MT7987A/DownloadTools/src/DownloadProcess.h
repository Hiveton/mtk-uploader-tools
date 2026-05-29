#pragma once

#include "DownloadCommand.h"

#include <QObject>
#include <QProcess>

class DownloadProcess : public QObject {
    Q_OBJECT

public:
    explicit DownloadProcess(QObject *parent = nullptr);

    bool isRunning() const;

public slots:
    void start(const DownloadCommand &command);
    void stop();

signals:
    void outputLine(const QString &line);
    void stateChanged(bool running);
    void finished(int exitCode, QProcess::ExitStatus status);
    void failedToStart(const QString &message);

private:
    QProcess m_process;
};
