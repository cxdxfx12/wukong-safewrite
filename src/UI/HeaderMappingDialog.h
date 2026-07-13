#ifndef HEADERMAPPINGDIALOG_H
#define HEADERMAPPINGDIALOG_H

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include "DataModel/MappingTemplate.h"

class HeaderMappingDialog : public QDialog {
    Q_OBJECT
public:
    explicit HeaderMappingDialog(const QStringList &excelHeaders,
                                  const QMap<QString, int> &currentMapping,
                                  const QString &filePath,
                                  MappingTemplateManager *tplManager,
                                  QWidget *parent = nullptr);

    QMap<QString, int> mapping() const;

private slots:
    void onTemplateSelected(int index);
    void onSaveTemplate();
    void onDeleteTemplate();
    void onResetDefault();
    void onApply();

private:
    void setupUI();
    void populatePreviewTable();
    void populateMappingCombos();
    void refreshTemplateCombo();

    QStringList m_headers;
    QMap<QString, int> m_autoMapping;   // 自动检测的原始结果（永不改变）
    QMap<QString, int> m_defaultMapping; // 当前映射（模板/手动调整会改变）
    QMap<QString, int> m_currentMapping; // 用户确认后的最终映射
    QString m_filePath;
    MappingTemplateManager *m_tplManager;

    // === UI 控件 ===
    QComboBox *m_templateCombo;
    QTableWidget *m_previewTable;
    QMap<QString, QComboBox*> m_comboMap;  // 3个关键字段下拉

    struct FieldDef {
        QString key;
        QString label;
        bool required;
    };
    static QList<FieldDef> fieldDefs();
};

#endif // HEADERMAPPINGDIALOG_H
