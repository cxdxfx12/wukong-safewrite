#ifndef LICENSEGENERATOR_H
#define LICENSEGENERATOR_H

#include <QWidget>

namespace Ui { class LicenseGenerator; }

class LicenseGenerator : public QWidget {
    Q_OBJECT
public:
    explicit LicenseGenerator(QWidget *parent = nullptr);
    ~LicenseGenerator();

private slots:
    void onGenerateClicked();
    void onCopyClicked();

private:
    Ui::LicenseGenerator *ui;
    QString generateLicenseKey(const QString &machineCode, const QString &expiryDate);
    QString computeSignature(const QString &signData);
};

#endif // LICENSEGENERATOR_H
