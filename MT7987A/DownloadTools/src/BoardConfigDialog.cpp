#include "BoardConfigDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

BoardConfigDialog::BoardConfigDialog(BoardModelRepository *repository, QWidget *parent)
    : QDialog(parent),
      m_repository(repository)
{
    buildUi();
    loadBoards();
}

void BoardConfigDialog::buildUi()
{
    setWindowTitle("板卡配置");
    resize(980, 620);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(12);

    auto *left = new QVBoxLayout();
    m_boardList = new QListWidget(this);
    auto *addBoardButton = new QPushButton("添加板卡", this);
    auto *removeBoardButton = new QPushButton("删除板卡", this);
    left->addWidget(m_boardList, 1);
    left->addWidget(addBoardButton);
    left->addWidget(removeBoardButton);
    auto *validateButton = new QPushButton("校验配置", this);
    auto *importButton = new QPushButton("导入配置", this);
    auto *exportButton = new QPushButton("导出配置", this);
    left->addWidget(validateButton);
    left->addWidget(importButton);
    left->addWidget(exportButton);
    root->addLayout(left, 1);

    auto *right = new QVBoxLayout();
    auto *form = new QFormLayout();
    m_idEdit = new QLineEdit(this);
    m_nameEdit = new QLineEdit(this);
    m_directoryEdit = new QLineEdit(this);
    auto *directoryRow = new QWidget(this);
    auto *directoryLayout = new QHBoxLayout(directoryRow);
    directoryLayout->setContentsMargins(0, 0, 0, 0);
    auto *browseDirButton = new QPushButton("浏览", directoryRow);
    directoryLayout->addWidget(m_directoryEdit, 1);
    directoryLayout->addWidget(browseDirButton);
    m_photoEdit = new QLineEdit(this);
    auto *photoRow = new QWidget(this);
    auto *photoLayout = new QHBoxLayout(photoRow);
    photoLayout->setContentsMargins(0, 0, 0, 0);
    auto *browsePhotoButton = new QPushButton("选择照片", photoRow);
    photoLayout->addWidget(m_photoEdit, 1);
    photoLayout->addWidget(browsePhotoButton);
    m_photoPreview = new QLabel(this);
    m_photoPreview->setFixedSize(120, 90);
    m_photoPreview->setAlignment(Qt::AlignCenter);
    m_photoPreview->setFrameShape(QFrame::StyledPanel);
    m_ipEdit = new QLineEdit("192.168.1.1", this);
    m_startStepEdit = new QLineEdit("FIRMWARE", this);
    m_waitEdit = new QSpinBox(this);
    m_waitEdit->setRange(1, 3600);
    m_waitEdit->setValue(120);

    form->addRow("板卡 ID", m_idEdit);
    form->addRow("显示名称", m_nameEdit);
    form->addRow("目录", directoryRow);
    form->addRow("板卡照片", photoRow);
    form->addRow("", m_photoPreview);
    form->addRow("默认 IP", m_ipEdit);
    form->addRow("默认起始步骤", m_startStepEdit);
    form->addRow("等待 WebUI 秒数", m_waitEdit);
    right->addLayout(form);

    m_componentTable = new QTableWidget(this);
    m_componentTable->setColumnCount(3);
    m_componentTable->setHorizontalHeaderLabels({"组件", "文件", "必需"});
    m_componentTable->horizontalHeader()->setStretchLastSection(false);
    m_componentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_componentTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_componentTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    right->addWidget(m_componentTable, 1);

    auto *componentButtons = new QHBoxLayout();
    auto *addComponentButton = new QPushButton("添加组件", this);
    auto *removeComponentButton = new QPushButton("删除组件", this);
    auto *browseComponentButton = new QPushButton("选择组件文件", this);
    componentButtons->addWidget(addComponentButton);
    componentButtons->addWidget(removeComponentButton);
    componentButtons->addWidget(browseComponentButton);
    componentButtons->addStretch(1);
    right->addLayout(componentButtons);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText("保存");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    right->addWidget(buttons);
    root->addLayout(right, 3);

    connect(m_boardList, &QListWidget::currentRowChanged, this, &BoardConfigDialog::onBoardSelectionChanged);
    connect(addBoardButton, &QPushButton::clicked, this, &BoardConfigDialog::addBoard);
    connect(removeBoardButton, &QPushButton::clicked, this, &BoardConfigDialog::removeBoard);
    connect(validateButton, &QPushButton::clicked, this, &BoardConfigDialog::validateConfig);
    connect(importButton, &QPushButton::clicked, this, &BoardConfigDialog::importConfig);
    connect(exportButton, &QPushButton::clicked, this, &BoardConfigDialog::exportConfig);
    connect(addComponentButton, &QPushButton::clicked, this, &BoardConfigDialog::addComponent);
    connect(removeComponentButton, &QPushButton::clicked, this, &BoardConfigDialog::removeComponent);
    connect(browseDirButton, &QPushButton::clicked, this, &BoardConfigDialog::browseDirectory);
    connect(browsePhotoButton, &QPushButton::clicked, this, &BoardConfigDialog::browsePhoto);
    connect(m_photoEdit, &QLineEdit::textChanged, this, &BoardConfigDialog::updatePhotoPreview);
    connect(browseComponentButton, &QPushButton::clicked, this, &BoardConfigDialog::browseComponentFile);
    connect(buttons, &QDialogButtonBox::accepted, this, &BoardConfigDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &BoardConfigDialog::reject);
}

void BoardConfigDialog::loadBoards()
{
    m_boards = m_repository->models();
    m_boardList->clear();
    for (const auto &board : m_boards) {
        m_boardList->addItem(board.displayName);
    }
    if (!m_boards.isEmpty()) {
        m_boardList->setCurrentRow(0);
    } else {
        setFormEnabled(false);
    }
}

void BoardConfigDialog::onBoardSelectionChanged()
{
    saveCurrentBoardFromForm();
    showBoard(m_boardList->currentRow());
}

void BoardConfigDialog::showBoard(int index)
{
    m_currentIndex = index;
    const bool valid = index >= 0 && index < m_boards.size();
    setFormEnabled(valid);
    if (!valid) {
        return;
    }

    const auto board = m_boards[index];
    m_idEdit->setText(board.id);
    m_nameEdit->setText(board.displayName);
    m_directoryEdit->setText(board.directoryName);
    m_photoEdit->setText(board.boardImage);
    m_ipEdit->setText(board.defaultIp);
    m_startStepEdit->setText(board.defaultStartStep);
    m_waitEdit->setValue(board.waitDeviceSeconds);

    m_componentTable->setRowCount(0);
    for (const auto &component : board.firmwareComponents) {
        const int row = m_componentTable->rowCount();
        m_componentTable->insertRow(row);
        m_componentTable->setItem(row, 0, new QTableWidgetItem(component.name));
        m_componentTable->setItem(row, 1, new QTableWidgetItem(component.file));
        auto *required = new QTableWidgetItem();
        required->setCheckState(component.required ? Qt::Checked : Qt::Unchecked);
        m_componentTable->setItem(row, 2, required);
    }
}

void BoardConfigDialog::saveCurrentBoardFromForm()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_boards.size()) {
        return;
    }
    m_boards[m_currentIndex] = boardFromForm();
    if (auto *item = m_boardList->item(m_currentIndex)) {
        item->setText(m_boards[m_currentIndex].displayName);
    }
}

BoardModelInfo BoardConfigDialog::boardFromForm() const
{
    BoardModelInfo board;
    board.id = m_idEdit->text().trimmed();
    board.displayName = m_nameEdit->text().trimmed();
    board.directoryName = m_directoryEdit->text().trimmed();
    board.boardImage = m_photoEdit->text().trimmed();
    board.defaultIp = m_ipEdit->text().trimmed();
    board.defaultStartStep = m_startStepEdit->text().trimmed().toUpper();
    board.waitDeviceSeconds = m_waitEdit->value();

    for (int row = 0; row < m_componentTable->rowCount(); ++row) {
        FirmwareComponentInfo component;
        component.name = m_componentTable->item(row, 0) ? m_componentTable->item(row, 0)->text().trimmed().toUpper() : QString();
        component.file = m_componentTable->item(row, 1) ? m_componentTable->item(row, 1)->text().trimmed() : QString();
        component.required = !m_componentTable->item(row, 2) || m_componentTable->item(row, 2)->checkState() == Qt::Checked;
        if (!component.name.isEmpty() && !component.file.isEmpty()) {
            board.firmwareComponents.append(component);
            if (component.name == "FIRMWARE") {
                board.firmwareFile = component.file;
            }
        }
    }
    if (board.firmwareFile.isEmpty() && !board.firmwareComponents.isEmpty()) {
        board.firmwareFile = board.firmwareComponents.last().file;
    }
    return board;
}

void BoardConfigDialog::addBoard()
{
    saveCurrentBoardFromForm();
    const int next = m_boards.size() + 1;
    BoardModelInfo board;
    board.id = QString("BOARD%1").arg(next);
    board.displayName = board.id;
    board.directoryName = board.id;
    board.boardImage = "assets/board-preview.png";
    board.firmwareFile = "firmware.bin";
    board.firmwareComponents = {{"BL2", "bl2.img", true}, {"GPT", "gpt.bin", true}, {"FIP", "fip.bin", true}, {"FIRMWARE", "firmware.bin", true}};
    m_boards.append(board);
    m_boardList->addItem(board.displayName);
    m_boardList->setCurrentRow(m_boards.size() - 1);
}

void BoardConfigDialog::removeBoard()
{
    const int row = m_boardList->currentRow();
    if (row < 0 || row >= m_boards.size()) {
        return;
    }
    m_boards.removeAt(row);
    delete m_boardList->takeItem(row);
    m_currentIndex = -1;
    if (!m_boards.isEmpty()) {
        m_boardList->setCurrentRow(qMin(row, m_boards.size() - 1));
    } else {
        setFormEnabled(false);
    }
}

void BoardConfigDialog::addComponent()
{
    const int row = m_componentTable->rowCount();
    m_componentTable->insertRow(row);
    m_componentTable->setItem(row, 0, new QTableWidgetItem("FIRMWARE"));
    m_componentTable->setItem(row, 1, new QTableWidgetItem("firmware.bin"));
    auto *required = new QTableWidgetItem();
    required->setCheckState(Qt::Checked);
    m_componentTable->setItem(row, 2, required);
}

void BoardConfigDialog::removeComponent()
{
    const int row = m_componentTable->currentRow();
    if (row >= 0) {
        m_componentTable->removeRow(row);
    }
}

void BoardConfigDialog::validateConfig()
{
    saveCurrentBoardFromForm();
    QString errorMessage;
    if (!m_repository->validate(m_boards, &errorMessage)) {
        QMessageBox::warning(this, "板卡配置", errorMessage);
        return;
    }
    QMessageBox::information(this, "板卡配置", "配置校验通过。");
}

void BoardConfigDialog::importConfig()
{
    const QString source = QFileDialog::getOpenFileName(this, "导入配置", QString(), "JSON 配置 (*.json);;所有文件 (*.*)");
    if (source.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_repository->importConfig(source, &errorMessage)) {
        QMessageBox::warning(this, "板卡配置", "无法导入配置: " + errorMessage);
        return;
    }
    loadBoards();
}

void BoardConfigDialog::exportConfig()
{
    saveCurrentBoardFromForm();
    QString errorMessage;
    if (!m_repository->validate(m_boards, &errorMessage)) {
        QMessageBox::warning(this, "板卡配置", errorMessage);
        return;
    }

    const QString target = QFileDialog::getSaveFileName(this, "导出配置", QString(), "JSON 配置 (*.json);;所有文件 (*.*)");
    if (target.isEmpty()) {
        return;
    }
    if (!m_repository->save(m_boards, &errorMessage) || !m_repository->exportConfig(target, &errorMessage)) {
        QMessageBox::warning(this, "板卡配置", "无法导出配置: " + errorMessage);
        return;
    }
}

void BoardConfigDialog::browseDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择板卡目录");
    if (!dir.isEmpty()) {
        m_directoryEdit->setText(QFileInfo(dir).fileName());
    }
}

void BoardConfigDialog::browsePhoto()
{
    const QString file = QFileDialog::getOpenFileName(this, "选择板卡照片", QString(), "图片 (*.png *.jpg *.jpeg *.webp *.bmp);;所有文件 (*.*)");
    if (!file.isEmpty()) {
        m_photoEdit->setText(file);
    }
}

void BoardConfigDialog::updatePhotoPreview()
{
    const QString imagePath = m_photoEdit->text().trimmed();
    QString resolvedPath = imagePath;
    const QFileInfo imageInfo(imagePath);
    if (!imageInfo.isAbsolute() && m_repository) {
        resolvedPath = QFileInfo(m_repository->configPath()).dir().filePath(imagePath);
    }
    QPixmap photo(resolvedPath);
    if (photo.isNull()) {
        photo = QPixmap(QFileInfo(m_repository->configPath()).dir().filePath("DownloadTools/assets/board-preview.png"));
    }
    if (photo.isNull()) {
        m_photoPreview->setText("无照片");
        m_photoPreview->setPixmap(QPixmap());
        return;
    }
    m_photoPreview->setText(QString());
    m_photoPreview->setPixmap(photo.scaled(m_photoPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void BoardConfigDialog::browseComponentFile()
{
    const int row = m_componentTable->currentRow();
    if (row < 0) {
        return;
    }
    const QString file = QFileDialog::getOpenFileName(this, "选择固件组件", QString(), "固件文件 (*.bin *.img);;所有文件 (*.*)");
    if (!file.isEmpty()) {
        m_componentTable->setItem(row, 1, new QTableWidgetItem(QFileInfo(file).fileName()));
    }
}

void BoardConfigDialog::accept()
{
    saveCurrentBoardFromForm();
    for (const auto &board : m_boards) {
        if (board.id.isEmpty() || board.directoryName.isEmpty() || board.firmwareComponents.isEmpty()) {
            QMessageBox::warning(this, "板卡配置", "每块板卡都需要 ID、目录和至少一个固件组件。");
            return;
        }
    }

    QString errorMessage;
    if (!m_repository->validate(m_boards, &errorMessage)) {
        QMessageBox::warning(this, "板卡配置", errorMessage);
        return;
    }
    if (!m_repository->save(m_boards, &errorMessage)) {
        QMessageBox::warning(this, "板卡配置", "无法保存 boards.json: " + errorMessage);
        return;
    }
    m_repository->reload();
    QDialog::accept();
}

void BoardConfigDialog::setFormEnabled(bool enabled)
{
    m_idEdit->setEnabled(enabled);
    m_nameEdit->setEnabled(enabled);
    m_directoryEdit->setEnabled(enabled);
    m_photoEdit->setEnabled(enabled);
    m_photoPreview->setEnabled(enabled);
    m_ipEdit->setEnabled(enabled);
    m_startStepEdit->setEnabled(enabled);
    m_waitEdit->setEnabled(enabled);
    m_componentTable->setEnabled(enabled);
}
