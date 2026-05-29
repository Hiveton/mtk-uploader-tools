#pragma once

#include "BoardModel.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QTableWidget>

class BoardConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit BoardConfigDialog(BoardModelRepository *repository, QWidget *parent = nullptr);

private slots:
    void onBoardSelectionChanged();
    void addBoard();
    void removeBoard();
    void addComponent();
    void removeComponent();
    void validateConfig();
    void importConfig();
    void exportConfig();
    void browseDirectory();
    void browsePhoto();
    void browseComponentFile();
    void accept() override;

private:
    void buildUi();
    void loadBoards();
    void saveCurrentBoardFromForm();
    void showBoard(int index);
    BoardModelInfo boardFromForm() const;
    void setFormEnabled(bool enabled);
    void updatePhotoPreview();

    BoardModelRepository *m_repository = nullptr;
    QVector<BoardModelInfo> m_boards;
    int m_currentIndex = -1;

    QListWidget *m_boardList = nullptr;
    QLineEdit *m_idEdit = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_directoryEdit = nullptr;
    QLineEdit *m_photoEdit = nullptr;
    QLabel *m_photoPreview = nullptr;
    QLineEdit *m_ipEdit = nullptr;
    QLineEdit *m_startStepEdit = nullptr;
    QSpinBox *m_waitEdit = nullptr;
    QTableWidget *m_componentTable = nullptr;
};
