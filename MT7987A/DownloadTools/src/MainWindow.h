#pragma once

#include "BoardModel.h"
#include "DownloadCommand.h"
#include "DownloadProcess.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QProcess>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QVector>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onModelChanged();
    void onStartDownload();
    void onStopDownload();
    void refreshSerialPorts();
    void scanDevices();
    void openPackage();
    void showSettings();
    void manageBoards();
    void showAbout();
    void clearLog();
    void copyLog();
    void saveLog();
    void appendLog(const QString &line);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessFailedToStart(const QString &message);

private:
    QString mt7987aRoot() const;
    BoardModelInfo selectedModel() const;
    DownloadOptions currentOptions() const;
    void buildUi();
    void populateModels();
    void updateModelDetails();
    void updateRunningState(bool running);
    void updateDownloadControls(bool running, const QString &statusText);
    void applyTheme();
    bool isDarkTheme() const;
    void setStepState(const QString &stepName);
    void selectStartStep(const QString &stepName);
    QWidget *makeStepNode(const QString &stepName, int number);
    QLabel *makeDetailLabel(const QString &text);
    QLabel *makeDetailValue(const QString &text, const QString &objectName = QString());
    QString boardPreviewPath() const;
    QString boardPreviewPath(const BoardModelInfo &model) const;
    QString firmwareSizeText(const BoardModelInfo &model) const;
    QString firmwareModifiedText(const BoardModelInfo &model) const;
    QString packageVersionText() const;

    BoardModelRepository m_repository;
    DownloadCommandBuilder m_commandBuilder;
    DownloadProcess m_process;

    QListWidget *m_modelList = nullptr;
    QLabel *m_modelCount = nullptr;
    QComboBox *m_serialPort = nullptr;
    QLineEdit *m_deviceIp = nullptr;
    QSpinBox *m_waitSeconds = nullptr;
    QCheckBox *m_skipUartBoot = nullptr;
    QLabel *m_modelName = nullptr;
    QLabel *m_firmwareName = nullptr;
    QLabel *m_firmwarePath = nullptr;
    QLabel *m_firmwareSize = nullptr;
    QLabel *m_firmwareModified = nullptr;
    QLabel *m_componentStatus = nullptr;
    QLabel *m_boardPreview = nullptr;
    QLabel *m_fileStatus = nullptr;
    QLabel *m_statusText = nullptr;
    QLabel *m_readyText = nullptr;
    QLabel *m_progressPercent = nullptr;
    QProgressBar *m_progress = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_refreshPortsButton = nullptr;
    QPushButton *m_clearLogButton = nullptr;
    QPushButton *m_copyLogButton = nullptr;
    QPushButton *m_saveLogButton = nullptr;
    QVector<QPushButton *> m_startStepButtons;
    QVector<QLabel *> m_stepCircles;
    QVector<QLabel *> m_stepStates;
    QString m_startStep = "FIRMWARE";
    bool m_applyingTheme = false;
};
