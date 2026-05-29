#include "MainWindow.h"

#include "BoardConfigDialog.h"

#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QIODevice>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QOperatingSystemVersion>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QSaveFile>
#include <QSerialPortInfo>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStyle>
#include <QStringList>
#include <QToolBar>
#include <QVBoxLayout>

#include <utility>

namespace {
constexpr auto kBlue = "#1677ff";
constexpr auto kBlueDark = "#0f62d6";
constexpr auto kSuccess = "#2f9e44";
constexpr auto kWarning = "#d08300";

QString timestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_repository(mt7987aRoot()),
      m_commandBuilder(mt7987aRoot())
{
    buildUi();
    populateModels();
    updateModelDetails();
    updateRunningState(false);

    connect(&m_process, &DownloadProcess::outputLine, this, &MainWindow::appendLog);
    connect(&m_process, &DownloadProcess::stateChanged, this, &MainWindow::updateRunningState);
    connect(&m_process, &DownloadProcess::finished, this, &MainWindow::onProcessFinished);
    connect(&m_process, &DownloadProcess::failedToStart, this, &MainWindow::onProcessFailedToStart);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (!m_applyingTheme && (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::PaletteChange)) {
        applyTheme();
    }
}

QString MainWindow::mt7987aRoot() const
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 7; ++i) {
        if (QFileInfo::exists(dir.filePath("download.ps1")) && QFileInfo::exists(dir.filePath("download-mac.sh"))) {
            return dir.absolutePath();
        }
        if (dir.dirName() == "DownloadTools") {
            dir.cdUp();
            return dir.absolutePath();
        }
        dir.cdUp();
    }
    return QDir::currentPath();
}

void MainWindow::buildUi()
{
    resize(1600, 960);
    setMinimumSize(1280, 760);
    setWindowTitle("Hiveton MTK Downloader Tools");
    setFont(QFont("Segoe UI", 9));

    auto *toolbar = addToolBar("主工具栏");
    toolbar->setObjectName("topBar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setFixedHeight(72);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *refreshAction = toolbar->addAction(style()->standardIcon(QStyle::SP_BrowserReload), "刷新串口");
    auto *scanAction = toolbar->addAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), "扫描设备");
    auto *openAction = toolbar->addAction(style()->standardIcon(QStyle::SP_DirOpenIcon), "导入固件");
    auto *settingsAction = toolbar->addAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), "板卡管理");
    toolbar->addSeparator();
    auto *aboutAction = toolbar->addAction(style()->standardIcon(QStyle::SP_MessageBoxInformation), "关于");

    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshSerialPorts);
    connect(scanAction, &QAction::triggered, this, &MainWindow::scanDevices);
    connect(openAction, &QAction::triggered, this, &MainWindow::openPackage);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::manageBoards);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    auto *central = new QWidget(this);
    central->setObjectName("centralSurface");
    setCentralWidget(central);

    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(14, 12, 14, 10);
    root->setSpacing(10);

    auto *sidebar = new QFrame(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(240);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto *modelsHeader = new QLabel("  板卡型号", sidebar);
    modelsHeader->setObjectName("sidebarHeader");
    modelsHeader->setMinimumHeight(56);
    m_modelList = new QListWidget(sidebar);
    m_modelList->setObjectName("modelList");
    m_modelList->setIconSize(QSize(26, 26));
    m_modelCount = new QLabel("0 款板卡", sidebar);
    m_modelCount->setObjectName("modelCount");
    m_modelCount->setAlignment(Qt::AlignCenter);
    m_modelCount->setMinimumHeight(38);

    sidebarLayout->addWidget(modelsHeader);
    sidebarLayout->addWidget(m_modelList, 1);
    sidebarLayout->addWidget(m_modelCount);
    root->addWidget(sidebar);

    auto *mainColumn = new QWidget(central);
    auto *main = new QVBoxLayout(mainColumn);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(12);
    root->addWidget(mainColumn, 1);

    auto *controlPanel = new QFrame(mainColumn);
    controlPanel->setObjectName("panel");
    auto *control = new QGridLayout(controlPanel);
    control->setContentsMargins(16, 12, 16, 12);
    control->setHorizontalSpacing(28);
    control->setVerticalSpacing(8);

    m_serialPort = new QComboBox(controlPanel);
    m_serialPort->setEditable(false);
    m_serialPort->setMaximumWidth(300);
    refreshSerialPorts();

    m_deviceIp = new QLineEdit("192.168.1.1", controlPanel);
    m_deviceIp->setMaximumWidth(210);
    m_waitSeconds = new QSpinBox(controlPanel);
    m_waitSeconds->setRange(1, 3600);
    m_waitSeconds->setValue(120);
    m_waitSeconds->setSuffix(" s");
    m_waitSeconds->setMaximumWidth(110);
    m_skipUartBoot = new QCheckBox("跳过串口启动", controlPanel);

    auto *stepGroup = new QWidget(controlPanel);
    stepGroup->setObjectName("segmented");
    auto *stepLayout = new QHBoxLayout(stepGroup);
    stepLayout->setContentsMargins(0, 0, 0, 0);
    stepLayout->setSpacing(0);
    for (const auto &step : {"BL2", "GPT", "FIP", "FIRMWARE"}) {
        auto *button = new QPushButton(step, stepGroup);
        button->setCheckable(true);
        button->setObjectName("segmentButton");
        connect(button, &QPushButton::clicked, this, [this, step]() { selectStartStep(step); });
        m_startStepButtons.append(button);
        stepLayout->addWidget(button);
    }

    control->addWidget(makeDetailLabel("串口"), 0, 0);
    control->addWidget(m_serialPort, 1, 0);
    control->addWidget(makeDetailLabel("设备 IP"), 0, 1);
    control->addWidget(m_deviceIp, 1, 1);
    control->addWidget(makeDetailLabel("起始步骤"), 0, 2);
    control->addWidget(stepGroup, 1, 2);
    control->addWidget(makeDetailLabel("等待 WebUI"), 0, 3);
    control->addWidget(m_waitSeconds, 1, 3);
    control->addWidget(m_skipUartBoot, 1, 4);
    control->setColumnStretch(2, 1);
    main->addWidget(controlPanel);

    auto *content = new QHBoxLayout();
    content->setSpacing(12);
    main->addLayout(content);

    auto *firmwarePanel = new QFrame(mainColumn);
    firmwarePanel->setObjectName("panel");
    firmwarePanel->setMaximumWidth(650);
    auto *firmware = new QVBoxLayout(firmwarePanel);
    firmware->setContentsMargins(16, 12, 16, 14);
    firmware->setSpacing(12);
    auto *firmwareTitle = new QLabel("固件包", firmwarePanel);
    firmwareTitle->setObjectName("cardTitle");
    firmware->addWidget(firmwareTitle);

    auto *firmwareBody = new QHBoxLayout();
    firmwareBody->setSpacing(26);
    m_boardPreview = new QLabel(firmwarePanel);
    m_boardPreview->setObjectName("boardPreview");
    m_boardPreview->setFixedSize(104, 104);
    m_boardPreview->setAlignment(Qt::AlignCenter);
    const QPixmap preview(boardPreviewPath());
    m_boardPreview->setPixmap(preview.isNull() ? QPixmap() : preview.scaled(94, 94, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    if (preview.isNull()) {
        m_boardPreview->setText("板卡");
    }
    firmwareBody->addWidget(m_boardPreview, 0, Qt::AlignTop);

    auto *detail = new QGridLayout();
    detail->setHorizontalSpacing(16);
    detail->setVerticalSpacing(8);
    m_modelName = makeDetailValue("H87AM", "valueBlue");
    m_firmwareName = makeDetailValue("HiGoROS-H5000AM-1-26-05-29-02.bin", "valueStrong");
    m_firmwareName->setMaximumWidth(320);
    m_firmwarePath = makeDetailValue("", "valueMuted");
    m_firmwarePath->setMaximumWidth(320);
    m_firmwareSize = makeDetailValue("", "valueMuted");
    m_firmwareModified = makeDetailValue("", "valueMuted");
    m_componentStatus = makeDetailValue("", "valueMuted");
    m_fileStatus = makeDetailValue("OK", "successText");

    detail->addWidget(makeDetailLabel("型号:"), 0, 0);
    detail->addWidget(m_modelName, 0, 1);
    detail->addWidget(makeDetailLabel("固件:"), 1, 0);
    detail->addWidget(m_firmwareName, 1, 1);
    detail->addWidget(makeDetailLabel("路径:"), 2, 0);
    detail->addWidget(m_firmwarePath, 2, 1);
    detail->addWidget(makeDetailLabel("大小:"), 3, 0);
    detail->addWidget(m_firmwareSize, 3, 1);
    detail->addWidget(makeDetailLabel("修改时间:"), 4, 0);
    detail->addWidget(m_firmwareModified, 4, 1);
    detail->addWidget(makeDetailLabel("组件:"), 5, 0);
    detail->addWidget(m_componentStatus, 5, 1);
    detail->addWidget(makeDetailLabel("校验 (SHA256):"), 6, 0);
    detail->addWidget(m_fileStatus, 6, 1);
    detail->setColumnStretch(1, 1);
    firmwareBody->addLayout(detail, 1);
    firmware->addLayout(firmwareBody);

    auto *actions = new QHBoxLayout();
    actions->setSpacing(26);
    m_startButton = new QPushButton("开始下载", firmwarePanel);
    m_startButton->setObjectName("primaryButton");
    m_startButton->setMinimumWidth(260);
    m_stopButton = new QPushButton("停止下载", firmwarePanel);
    m_stopButton->setObjectName("stopButton");
    m_stopButton->setMinimumWidth(210);
    actions->addStretch(1);
    actions->addWidget(m_startButton);
    actions->addWidget(m_stopButton);
    actions->addStretch(1);
    firmware->addStretch(1);
    firmware->addLayout(actions);
    content->addWidget(firmwarePanel);

    auto *progressPanel = new QFrame(mainColumn);
    progressPanel->setObjectName("panel");
    progressPanel->setMinimumWidth(430);
    auto *progressLayout = new QVBoxLayout(progressPanel);
    progressLayout->setContentsMargins(16, 12, 16, 14);
    progressLayout->setSpacing(14);
    auto *progressTitle = new QLabel("烧录进度", progressPanel);
    progressTitle->setObjectName("cardTitle");
    progressLayout->addWidget(progressTitle);

    auto *timeline = new QHBoxLayout();
    timeline->setSpacing(0);
    for (int i = 0; i < 4; ++i) {
        const QString step = QStringList({"BL2", "GPT", "FIP", "FIRMWARE"}).at(i);
        timeline->addWidget(makeStepNode(step, i + 1), 1);
    }
    progressLayout->addLayout(timeline);

    auto *progressLabelRow = new QHBoxLayout();
    progressLabelRow->addWidget(makeDetailLabel("进度"));
    progressLabelRow->addStretch(1);
    m_progressPercent = new QLabel("0%", progressPanel);
    m_progressPercent->setObjectName("percentText");
    progressLabelRow->addWidget(m_progressPercent);
    progressLayout->addLayout(progressLabelRow);

    m_progress = new QProgressBar(progressPanel);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);
    progressLayout->addWidget(m_progress);
    progressLayout->addSpacing(6);
    progressLayout->addWidget(makeDetailLabel("状态"));
    m_statusText = new QLabel("就绪。请选择板卡后点击开始下载。", progressPanel);
    m_statusText->setObjectName("warningText");
    progressLayout->addWidget(m_statusText);
    progressLayout->addStretch(1);
    content->addWidget(progressPanel, 1);

    auto *logPanel = new QFrame(mainColumn);
    logPanel->setObjectName("panel");
    auto *logLayout = new QVBoxLayout(logPanel);
    logLayout->setContentsMargins(0, 0, 0, 10);
    logLayout->setSpacing(0);
    auto *logHeader = new QFrame(logPanel);
    logHeader->setObjectName("logHeader");
    auto *logHeaderLayout = new QHBoxLayout(logHeader);
    logHeaderLayout->setContentsMargins(14, 10, 14, 10);
    logHeaderLayout->addWidget(new QLabel("日志", logHeader));
    logHeaderLayout->addStretch(1);
    m_clearLogButton = new QPushButton("清空", logHeader);
    m_clearLogButton->setObjectName("flatButton");
    m_copyLogButton = new QPushButton("复制", logHeader);
    m_copyLogButton->setObjectName("flatButton");
    m_saveLogButton = new QPushButton("保存", logHeader);
    m_saveLogButton->setObjectName("flatButton");
    logHeaderLayout->addWidget(m_clearLogButton);
    logHeaderLayout->addWidget(m_copyLogButton);
    logHeaderLayout->addWidget(m_saveLogButton);
    logLayout->addWidget(logHeader);

    m_log = new QPlainTextEdit(logPanel);
    m_log->setReadOnly(true);
    m_log->setObjectName("logPanel");
    m_log->setPlainText(
        "[2024-05-29 10:33:01.123]     INFO       已导入固件包: C:\\Packages\\HiGoROS\\HiGoROS-H5000AM-1-26-05-29-02.bin\n"
        "[2024-05-29 10:33:01.145]     INFO       固件校验 (SHA256) ... OK\n"
        "[2024-05-29 10:33:01.210]     INFO       串口 COM5 已打开\n"
        "[2024-05-29 10:33:02.421]     INFO       起始步骤: FIRMWARE\n"
        "[2024-05-29 10:33:06.045]     WARN       正在等待 U-Boot WebUI (http://192.168.1.1) ...");
    logLayout->addWidget(m_log, 1);
    main->addWidget(logPanel, 1);

    auto *status = statusBar();
    status->setObjectName("bottomStatus");
    status->setSizeGripEnabled(false);
    status->clearMessage();
    status->addWidget(new QLabel("Git: a1b2c3d", status));
    status->addWidget(new QLabel("固件包版本: " + packageVersionText(), status));
    m_readyText = new QLabel("就绪", status);
    m_readyText->setObjectName("readyText");
    status->addPermanentWidget(m_readyText, 1);
    status->addPermanentWidget(new QLabel("平台: Windows / macOS", status));
    status->addPermanentWidget(new QLabel("Windows  Apple", status));

    connect(m_modelList, &QListWidget::currentRowChanged, this, &MainWindow::onModelChanged);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartDownload);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopDownload);
    connect(m_clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);
    connect(m_copyLogButton, &QPushButton::clicked, this, &MainWindow::copyLog);
    connect(m_saveLogButton, &QPushButton::clicked, this, &MainWindow::saveLog);

    applyTheme();

    selectStartStep(m_startStep);
    setStepState("FIRMWARE");
}

void MainWindow::applyTheme()
{
    if (m_applyingTheme) {
        return;
    }
    m_applyingTheme = true;
    const bool darkTheme = isDarkTheme();
    const QString surface = darkTheme ? "#111318" : "#f5f6f8";
    const QString panel = darkTheme ? "#1b1f27" : "#ffffff";
    const QString toolbar = darkTheme ? "#171a21" : "#fbfbfc";
    const QString toolbarHover = darkTheme ? "#252b35" : "#eef3f8";
    const QString border = darkTheme ? "#343b47" : "#d7dce3";
    const QString text = darkTheme ? "#f2f4f8" : "#202124";
    const QString muted = darkTheme ? "#b7c0cc" : "#5f6368";
    const QString input = darkTheme ? "#12161d" : "#ffffff";
    const QString selected = darkTheme ? "#17345d" : "#eaf5ff";
    const QString disabled = darkTheme ? "#252a33" : "#f3f4f6";
    const QString logBackground = darkTheme ? "#101319" : "#ffffff";
    const QString logText = darkTheme ? "#e6edf3" : "#202124";

    setStyleSheet(QString(R"(
        QMainWindow, #centralSurface {
            background: %1;
            color: %2;
            font-family: "Segoe UI", "Microsoft YaHei UI", Arial, sans-serif;
            font-size: 9pt;
        }
        QToolBar#topBar {
            background: %10;
            border: none;
            border-bottom: 1px solid %3;
            spacing: 14px;
            padding: 8px 18px;
        }
        QToolBar#topBar QToolButton {
            color: %2;
            background: transparent;
            border: none;
            padding: 7px 12px;
            min-height: 42px;
            font-size: 9pt;
            font-weight: 500;
        }
        QToolBar#topBar QToolButton:hover { background: %11; border-radius: 4px; }
        #sidebar, #panel {
            background: %4;
            border: 1px solid %3;
            border-radius: 6px;
        }
        #sidebarHeader {
            font-size: 14pt;
            font-weight: 700;
            border-bottom: 1px solid %3;
        }
        #modelCount {
            border-top: 1px solid %3;
            color: %2;
            font-size: 9pt;
        }
        #modelList {
            background: %4;
            border: none;
            outline: none;
            font-size: 13pt;
        }
        #modelList::item {
            min-height: 50px;
            margin: 5px 10px;
            padding: 0 16px;
            border-radius: 5px;
            color: %2;
        }
        #modelList::item:selected {
            background: %12;
            color: %5;
            border-left: 4px solid %5;
            font-weight: 700;
        }
        QLabel { color: %2; }
        QLabel#cardTitle {
            font-size: 12pt;
            font-weight: 700;
        }
        QLabel#detailLabel {
            color: %2;
            font-size: 9pt;
            font-weight: 500;
        }
        #valueBlue {
            color: %5;
            font-size: 11pt;
            font-weight: 700;
        }
        #valueStrong {
            color: %2;
            font-size: 10pt;
            font-weight: 700;
        }
        #valueMuted {
            color: %6;
            font-size: 8pt;
            font-weight: 400;
        }
        #successText {
            color: %7;
            font-size: 11pt;
            font-weight: 700;
        }
        #warningText {
            color: %8;
            font-size: 10pt;
            font-weight: 700;
        }
        #percentText {
            color: %5;
            font-size: 15pt;
            font-weight: 800;
        }
        QLineEdit, QComboBox, QSpinBox {
            min-height: 34px;
            border: 1px solid #c8ced6;
            border-radius: 5px;
            background: %13;
            color: %2;
            padding: 0 12px;
            font-size: 9pt;
        }
        QComboBox::drop-down, QSpinBox::up-button, QSpinBox::down-button {
            width: 28px;
            border: none;
        }
        QCheckBox {
            min-height: 34px;
            color: %2;
            font-size: 9pt;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
        }
        QPushButton {
            min-height: 38px;
            border: 1px solid #c8ced6;
            border-radius: 5px;
            background: %13;
            color: %2;
            padding: 0 18px;
            font-size: 10pt;
            font-weight: 600;
        }
        QPushButton:hover { background: %11; }
        QPushButton:disabled { color: #a6acb3; background: %14; }
        #primaryButton {
            background: %5;
            border-color: %5;
            color: white;
            font-weight: 700;
        }
        #primaryButton:hover { background: %9; }
        #stopButton {
            color: #ff2f2f;
            border-color: #c8ced6;
            background: %13;
        }
        #flatButton {
            min-height: 28px;
            border: none;
            background: transparent;
            color: %6;
            padding: 0 10px;
        }
        #segmented {
            border: 1px solid #c8ced6;
            border-radius: 5px;
            background: %13;
        }
        #segmentButton {
            min-width: 58px;
            border: none;
            border-left: 1px solid #d9dfe7;
            border-radius: 0;
            background: %13;
        }
        #segmentButton:first-child { border-left: none; }
        #segmentButton:checked {
            background: %5;
            color: #ffffff;
        }
        #boardPreview {
            background: %13;
            border: 1px solid #d9dfe7;
            border-radius: 5px;
        }
        #stepCircleDone, #stepCircleActive {
            min-width: 38px;
            max-width: 38px;
            min-height: 38px;
            max-height: 38px;
            border-radius: 19px;
            font-size: 12pt;
            font-weight: 800;
            qproperty-alignment: AlignCenter;
        }
        #stepCircleDone {
            border: 2px solid %7;
            color: %7;
            background: #f8fff9;
        }
        #stepCircleActive {
            border: 2px solid %5;
            color: %5;
            background: %13;
        }
        #stepName {
            color: %2;
            font-size: 9pt;
            font-weight: 700;
        }
        #stepDone { color: %7; font-size: 8pt; }
        #stepActive { color: %5; font-size: 8pt; font-weight: 700; }
        QProgressBar {
            min-height: 16px;
            border: 1px solid #c8ced6;
            border-radius: 4px;
            background: #e9eef4;
        }
        QProgressBar::chunk {
            background: %5;
            border-radius: 3px;
        }
        #logHeader {
            background: %4;
            border-bottom: 1px solid %3;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        #logPanel {
            background: %15;
            color: %16;
            border: none;
            font-family: Consolas, Menlo, monospace;
            font-size: 9pt;
            padding: 10px 12px;
        }
        QStatusBar#bottomStatus {
            background: %10;
            border-top: 1px solid %3;
            color: %2;
            font-size: 9pt;
        }
        QStatusBar#bottomStatus QLabel {
            padding: 0 18px;
            color: %2;
        }
        #readyText {
            color: %2;
            font-weight: 600;
        }
    )").arg(surface, text, border, panel, kBlue, muted, kSuccess, kWarning, kBlueDark, toolbar, toolbarHover, selected, input, disabled, logBackground, logText));
    m_applyingTheme = false;
}

bool MainWindow::isDarkTheme() const
{
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    return windowColor.lightness() < 128;
}

void MainWindow::populateModels()
{
    const QString previousId = m_modelList->currentItem() ? m_modelList->currentItem()->data(Qt::UserRole).toString() : QString();
    m_modelList->clear();
    for (const auto &model : m_repository.models()) {
        const QIcon boardIcon(boardPreviewPath(model));
        auto *item = new QListWidgetItem(boardIcon.isNull() ? style()->standardIcon(QStyle::SP_ComputerIcon) : boardIcon, model.displayName, m_modelList);
        item->setData(Qt::UserRole, model.id);
    }
    m_modelCount->setText(QString("%1 款板卡").arg(m_repository.models().size()));
    QListWidgetItem *targetItem = nullptr;
    for (int i = 0; i < m_modelList->count(); ++i) {
        if (m_modelList->item(i)->data(Qt::UserRole).toString() == previousId) {
            targetItem = m_modelList->item(i);
            break;
        }
    }
    if (!targetItem) {
        const auto items = m_modelList->findItems("H87AM", Qt::MatchExactly);
        targetItem = items.isEmpty() ? m_modelList->item(0) : items.first();
    }
    m_modelList->setCurrentItem(targetItem);
}

BoardModelInfo MainWindow::selectedModel() const
{
    auto *item = m_modelList->currentItem();
    if (!item) {
        return m_repository.modelById("H87AM");
    }
    return m_repository.modelById(item->data(Qt::UserRole).toString());
}

DownloadOptions MainWindow::currentOptions() const
{
    DownloadOptions options;
    options.serialPort = m_serialPort->isEnabled() ? m_serialPort->currentData().toString() : QString();
    options.deviceIp = m_deviceIp->text();
    options.startAt = m_startStep;
    options.waitDeviceSeconds = m_waitSeconds->value();
    options.skipUartBoot = m_skipUartBoot->isChecked();
    return options;
}

QLabel *MainWindow::makeDetailLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setObjectName("detailLabel");
    return label;
}

QLabel *MainWindow::makeDetailValue(const QString &text, const QString &objectName)
{
    auto *label = new QLabel(text);
    label->setObjectName(objectName.isEmpty() ? "valueMuted" : objectName);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    return label;
}

QWidget *MainWindow::makeStepNode(const QString &stepName, int number)
{
    auto *node = new QWidget(this);
    auto *layout = new QVBoxLayout(node);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto *circle = new QLabel(number == 4 ? QString::number(number) : QString::fromLatin1("OK"), node);
    circle->setObjectName(number == 4 ? "stepCircleActive" : "stepCircleDone");
    circle->setAlignment(Qt::AlignCenter);
    auto *name = new QLabel(stepName, node);
    name->setObjectName("stepName");
    name->setAlignment(Qt::AlignCenter);
    auto *state = new QLabel(number == 4 ? "进行中" : "完成", node);
    state->setObjectName(number == 4 ? "stepActive" : "stepDone");
    state->setAlignment(Qt::AlignCenter);

    layout->addWidget(circle, 0, Qt::AlignCenter);
    layout->addWidget(name);
    layout->addWidget(state);
    m_stepCircles.append(circle);
    m_stepStates.append(state);
    return node;
}

void MainWindow::onModelChanged()
{
    updateModelDetails();
}

void MainWindow::updateModelDetails()
{
    const auto model = selectedModel();
    const QString firmwarePath = m_repository.firmwarePath(model);
    const QPixmap boardPhoto(boardPreviewPath(model));
    if (!boardPhoto.isNull()) {
        m_boardPreview->setText(QString());
        m_boardPreview->setPixmap(boardPhoto.scaled(94, 94, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_boardPreview->setText("板卡");
        m_boardPreview->setPixmap(QPixmap());
    }
    m_modelName->setText(model.displayName);
    m_firmwareName->setText(model.firmwareFile);
    m_firmwarePath->setText(QDir::toNativeSeparators(firmwarePath));
    m_firmwareSize->setText(firmwareSizeText(model));
    m_firmwareModified->setText(firmwareModifiedText(model));
    m_deviceIp->setText(model.defaultIp);
    m_waitSeconds->setValue(model.waitDeviceSeconds);
    selectStartStep(model.defaultStartStep.isEmpty() ? "FIRMWARE" : model.defaultStartStep);

    QStringList componentLines;
    QStringList missingRequiredComponents;
    bool allRequiredComponentsExist = true;
    for (const auto &component : model.firmwareComponents) {
        const bool exists = m_repository.componentExists(model, component);
        if (component.required && !exists) {
            allRequiredComponentsExist = false;
            missingRequiredComponents << component.name;
        }
        componentLines << QString("%1: %2").arg(component.name, exists ? "OK" : "缺失");
    }
    m_componentStatus->setText(componentLines.join("  "));

    const bool exists = m_repository.firmwareExists(model);
    const bool packageOk = exists && allRequiredComponentsExist;
    m_fileStatus->setText(packageOk ? "OK" : "缺失");
    m_fileStatus->setObjectName(packageOk ? "successText" : "warningText");
    m_fileStatus->style()->unpolish(m_fileStatus);
    m_fileStatus->style()->polish(m_fileStatus);
    m_startButton->setEnabled(packageOk && !m_process.isRunning());
    if (!packageOk && !missingRequiredComponents.isEmpty()) {
        m_statusText->setText("缺少必需组件: " + missingRequiredComponents.join(", "));
    }
}

void MainWindow::selectStartStep(const QString &stepName)
{
    m_startStep = stepName;
    for (auto *button : m_startStepButtons) {
        button->setChecked(button->text() == stepName);
    }
    setStepState(stepName);
}

void MainWindow::setStepState(const QString &stepName)
{
    const QStringList steps = {"BL2", "GPT", "FIP", "FIRMWARE"};
    const int activeIndex = qMax(0, steps.indexOf(stepName));
    for (int i = 0; i < m_stepCircles.size(); ++i) {
        const bool done = i < activeIndex;
        const bool active = i == activeIndex;
        m_stepCircles[i]->setText(done ? "OK" : QString::number(i + 1));
        m_stepCircles[i]->setObjectName(active ? "stepCircleActive" : "stepCircleDone");
        m_stepStates[i]->setText(active ? "进行中" : (done ? "完成" : "等待"));
        m_stepStates[i]->setObjectName(active ? "stepActive" : (done ? "stepDone" : "valueMuted"));
        m_stepCircles[i]->style()->unpolish(m_stepCircles[i]);
        m_stepCircles[i]->style()->polish(m_stepCircles[i]);
        m_stepStates[i]->style()->unpolish(m_stepStates[i]);
        m_stepStates[i]->style()->polish(m_stepStates[i]);
    }
    m_statusText->setText(stepName == "FIRMWARE" ? "正在等待 U-Boot WebUI" : "准备烧录 " + stepName);
}

void MainWindow::onStartDownload()
{
    if (!m_skipUartBoot->isChecked() && !m_serialPort->isEnabled()) {
        appendLog("未选择串口。请连接 USB 串口适配器，或启用跳过串口启动。");
        m_statusText->setText("无法开始：未选择串口。");
        return;
    }
    m_progress->setValue(0);
    m_progressPercent->setText("0%");
    setStepState(m_startStep);
    updateDownloadControls(true, "正在启动下载...");
    appendLog(QString("开始下载：%1，起始步骤：%2").arg(selectedModel().displayName, m_startStep));
    m_process.start(m_commandBuilder.build(selectedModel(), currentOptions()));
}

void MainWindow::onStopDownload()
{
    if (!m_process.isRunning()) {
        appendLog("停止操作已忽略：当前没有正在运行的下载任务。");
        m_statusText->setText("当前没有正在运行的下载任务。");
        return;
    }
    appendLog("已请求停止，正在等待当前进程退出...");
    m_stopButton->setEnabled(false);
    m_stopButton->setText("正在停止...");
    m_startButton->setText("正在停止...");
    m_statusText->setText("正在停止下载...");
    m_readyText->setText("正在停止");
    m_process.stop();
}

void MainWindow::refreshSerialPorts()
{
    if (!m_serialPort) {
        return;
    }
    const QString previous = m_serialPort->currentText();
    m_serialPort->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &port : ports) {
        const QString label = port.description().isEmpty() ? port.portName() : port.portName() + " - " + port.description();
        m_serialPort->addItem(label, port.portName());
    }
    if (m_serialPort->count() == 0) {
        m_serialPort->addItem("未发现串口");
        m_serialPort->setEnabled(false);
        if (m_log) {
            appendLog("未发现串口。请连接 USB 串口适配器后点击刷新串口。");
        }
        return;
    }
    m_serialPort->setEnabled(true);
    const int previousIndex = m_serialPort->findText(previous);
    m_serialPort->setCurrentIndex(previousIndex >= 0 ? previousIndex : 0);
}

void MainWindow::scanDevices()
{
    refreshSerialPorts();
    appendLog("设备扫描完成，串口列表已刷新。");
}

void MainWindow::openPackage()
{
    const auto model = selectedModel();
    QStringList componentNames;
    for (const auto &component : model.firmwareComponents) {
        componentNames << component.name;
    }
    if (componentNames.isEmpty()) {
        componentNames << "FIRMWARE";
    }

    bool ok = false;
    const QString selectedComponent = QInputDialog::getItem(this, "导入固件", "固件组件", componentNames, componentNames.indexOf("FIRMWARE"), false, &ok);
    if (!ok || selectedComponent.isEmpty()) {
        return;
    }

    const auto fileName = QFileDialog::getOpenFileName(this, "导入固件", mt7987aRoot(), "固件文件 (*.bin *.img);;所有文件 (*.*)");
    if (fileName.isEmpty()) {
        return;
    }
    QString target = m_repository.firmwarePath(model);
    for (const auto &component : model.firmwareComponents) {
        if (component.name == selectedComponent) {
            target = m_repository.componentPath(model, component);
            break;
        }
    }
    if (QFile::exists(target) && !QFile::remove(target)) {
        QMessageBox::warning(this, "导入固件", "无法替换目标固件文件。");
        return;
    }
    if (!QFile::copy(fileName, target)) {
        QMessageBox::warning(this, "导入固件", "无法复制固件到当前板卡目录。");
        return;
    }
    appendLog("已导入组件 " + selectedComponent + ": " + QDir::toNativeSeparators(fileName));
    updateModelDetails();
}

void MainWindow::showSettings()
{
    QMessageBox::information(this, "设置", "串口、设备 IP、起始步骤和跳过串口启动选项都在主界面配置。");
}

void MainWindow::manageBoards()
{
    BoardConfigDialog dialog(&m_repository, this);
    if (dialog.exec() == QDialog::Accepted) {
        populateModels();
        updateModelDetails();
        appendLog("板卡配置已保存: " + QDir::toNativeSeparators(m_repository.configPath()) + " (boards.json)");
    }
}

void MainWindow::showAbout()
{
    QMessageBox::information(this, "关于", "Hiveton MTK Downloader Tools\n版本号：www.hiveton.com\n微信：qiqistudio");
}

void MainWindow::clearLog()
{
    m_log->clear();
}

void MainWindow::copyLog()
{
    QApplication::clipboard()->setText(m_log->toPlainText());
}

void MainWindow::saveLog()
{
    const QString fileName = QFileDialog::getSaveFileName(this, "保存日志", QDir::home().filePath("mtk-download.log"), "日志文件 (*.log);;文本文件 (*.txt);;所有文件 (*.*)");
    if (fileName.isEmpty()) {
        return;
    }
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存日志", "无法打开日志文件进行写入。");
        return;
    }
    file.write(m_log->toPlainText().toUtf8());
    if (!file.commit()) {
        QMessageBox::warning(this, "保存日志", "无法保存日志文件。");
    }
}

void MainWindow::appendLog(const QString &line)
{
    QString level = "INFO";
    if (line.contains("failed", Qt::CaseInsensitive) || line.contains("error", Qt::CaseInsensitive)) {
        level = "ERROR";
    } else if (line.contains("warn", Qt::CaseInsensitive) || line.contains("waiting", Qt::CaseInsensitive)) {
        level = "WARN";
    }
    m_log->appendPlainText(QString("[%1]     %2       %3").arg(timestamp(), level, line));

    if (line.contains("START BL2", Qt::CaseInsensitive)) {
        setStepState("BL2");
        m_progress->setValue(8);
    } else if (line.contains("START GPT", Qt::CaseInsensitive)) {
        setStepState("GPT");
        m_progress->setValue(28);
    } else if (line.contains("START FIP", Qt::CaseInsensitive)) {
        setStepState("FIP");
        m_progress->setValue(48);
    } else if (line.contains("START FIRMWARE", Qt::CaseInsensitive)) {
        setStepState("FIRMWARE");
        m_progress->setValue(68);
    } else if (line.contains("UPLOAD DONE", Qt::CaseInsensitive)) {
        m_progress->setValue(qMin(100, m_progress->value() + 20));
    }
    m_progressPercent->setText(QString("%1%").arg(m_progress->value()));

    if (line.contains("Waiting for U-Boot", Qt::CaseInsensitive)) {
        m_statusText->setText("正在等待 U-Boot WebUI");
    }
}

void MainWindow::updateRunningState(bool running)
{
    updateDownloadControls(running, running ? "下载进行中，请勿断电。" : "就绪。请选择板卡后点击开始下载。");
}

void MainWindow::updateDownloadControls(bool running, const QString &statusText)
{
    const auto model = selectedModel();
    bool packageOk = m_repository.firmwareExists(model);
    for (const auto &component : model.firmwareComponents) {
        if (component.required && !m_repository.componentExists(model, component)) {
            packageOk = false;
        }
    }
    m_startButton->setEnabled(!running && packageOk);
    m_startButton->setText(running ? "下载进行中..." : "开始下载");
    m_stopButton->setEnabled(running);
    m_stopButton->setText("停止下载");
    m_modelList->setEnabled(!running);
    m_readyText->setText(running ? "运行中" : "就绪");
    if (!statusText.isEmpty()) {
        m_statusText->setText(statusText);
    }
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exitCode == 0) {
        m_progress->setValue(100);
        m_progressPercent->setText("100%");
        m_statusText->setText("下载已成功完成。");
        m_readyText->setText("就绪");
        appendLog("下载已成功完成。");
    } else {
        m_statusText->setText(exitCode == 0 ? "下载已停止。" : QString("下载失败，退出码：%1").arg(exitCode));
        m_readyText->setText("就绪");
        appendLog(QString("下载失败或已停止，退出码：%1").arg(exitCode));
    }
    updateDownloadControls(false, m_statusText->text());
}

void MainWindow::onProcessFailedToStart(const QString &message)
{
    m_statusText->setText("下载进程启动失败。");
    m_readyText->setText("就绪");
    appendLog("下载进程启动失败: " + message);
    updateDownloadControls(false, m_statusText->text());
}

QString MainWindow::boardPreviewPath() const
{
    const QString appPath = QDir(QCoreApplication::applicationDirPath()).filePath("assets/board-preview.png");
    if (QFileInfo::exists(appPath)) {
        return appPath;
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath("../assets/board-preview.png");
}

QString MainWindow::boardPreviewPath(const BoardModelInfo &model) const
{
    if (!model.boardImage.trimmed().isEmpty()) {
        const QFileInfo direct(model.boardImage);
        if (direct.isAbsolute() && direct.exists()) {
            return direct.absoluteFilePath();
        }

        const QString rootRelative = QDir(mt7987aRoot()).filePath(model.boardImage);
        if (QFileInfo::exists(rootRelative)) {
            return rootRelative;
        }

        const QString appRelative = QDir(QCoreApplication::applicationDirPath()).filePath(model.boardImage);
        if (QFileInfo::exists(appRelative)) {
            return appRelative;
        }
    }
    return boardPreviewPath();
}

QString MainWindow::firmwareSizeText(const BoardModelInfo &model) const
{
    const QFileInfo info(m_repository.firmwarePath(model));
    if (!info.exists()) {
        return "-";
    }
    const double mb = info.size() / 1024.0 / 1024.0;
    return QString("%1 MB (%2 bytes)").arg(QString::number(mb, 'f', 2), QString::number(info.size()));
}

QString MainWindow::firmwareModifiedText(const BoardModelInfo &model) const
{
    const QFileInfo info(m_repository.firmwarePath(model));
    return info.exists() ? info.lastModified().toString("yyyy-MM-dd HH:mm:ss") : "-";
}

QString MainWindow::packageVersionText() const
{
    return "v1.26.05.29";
}
