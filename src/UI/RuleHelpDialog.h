#ifndef RULEHELPDIALOG_H
#define RULEHELPDIALOG_H

#include <QDialog>

namespace Ui { class RuleHelpDialog; }

class RuleHelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit RuleHelpDialog(QWidget *parent = nullptr);
    ~RuleHelpDialog();

private:
    Ui::RuleHelpDialog *ui;
    void setupContent();
    QString generateHelpContent();
};

#endif // RULEHELPDIALOG_H
