#ifndef HISTORYDIALOG_H
#define HISTORYDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QList>
#include "DataModel/CalculationHistory.h"

class HistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit HistoryDialog(const QString &historyFilePath, QWidget *parent = nullptr);

private slots:
    void onItemSelected(int row);
    void onClearHistory();
    void onDeleteSelected();

private:
    QListWidget *m_listWidget;
    QTextEdit *m_detailView;
    QPushButton *m_clearBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_closeBtn;
    QLabel *m_countLabel;
    QList<CalculationHistoryRecord> m_history;
    QString m_filePath;

    void setupUi();
    void loadHistory();
    void saveHistory();
    void refreshList();
    QString formatRecordDetail(const CalculationHistoryRecord &rec) const;
};

#endif // HISTORYDIALOG_H
