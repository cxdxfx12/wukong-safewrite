#include "LicenseGenerator.h"
#include "ui_LicenseGenerator.h"

#include <QClipboard>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QApplication>

// 与 LicenseManager.cpp 中的密钥一致
static const QByteArray APP_SECRET = QByteArrayLiteral("WukongFreight2026@v1.0");

LicenseGenerator::LicenseGenerator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LicenseGenerator)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("悟空运费结算 - 许可证生成器"));
    setMinimumSize(600, 400);

    // 设置默认值
    ui->expiryCombo->setCurrentText(QStringLiteral("永久"));
    ui->machineCodeEdit->setPlaceholderText(QStringLiteral("输入机器码，格式: XXXX-XXXX-XXXX-XXXX"));
}

LicenseGenerator::~LicenseGenerator()
{
    delete ui;
}

void LicenseGenerator::onGenerateClicked()
{
    QString machineCode = ui->machineCodeEdit->text().trimmed();
    QString expiryText = ui->expiryCombo->currentText();

    // 验证机器码格式
    if (machineCode.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请输入机器码"));
        return;
    }

    // 移除所有分隔符
    QString cleanCode = machineCode;
    cleanCode.remove('-');
    cleanCode.remove(' ');

    // 验证长度（必须是16个十六进制字符）
    if (cleanCode.length() != 16) {
        QMessageBox::warning(this, QStringLiteral("错误"),
            QStringLiteral("机器码格式错误，应为16个十六进制字符（当前: %1 个）").arg(cleanCode.length()));
        return;
    }

    // 验证十六进制
    QRegularExpression hexRegex(QStringLiteral("^[0-9A-Fa-f]{16}$"));
    if (!hexRegex.match(cleanCode).hasMatch()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("机器码只能包含十六进制字符 (0-9, A-F)"));
        return;
    }

    // 格式化为 XXXX-XXXX-XXXX-XXXX
    QString formattedCode;
    for (int i = 0; i < 16; i++) {
        if (i > 0 && i % 4 == 0) formattedCode += '-';
        formattedCode += cleanCode[i].toUpper();
    }

    // 过期日期编码
    QString dateCode;
    if (expiryText == QStringLiteral("永久")) {
        dateCode = QStringLiteral("LIFE");
    } else if (expiryText == QStringLiteral("30天")) {
        QDate expiry = QDate::currentDate().addDays(30);
        dateCode = expiry.toString(QStringLiteral("yyMMdd"));
    } else if (expiryText == QStringLiteral("90天")) {
        QDate expiry = QDate::currentDate().addDays(90);
        dateCode = expiry.toString(QStringLiteral("yyMMdd"));
    } else if (expiryText == QStringLiteral("365天")) {
        QDate expiry = QDate::currentDate().addDays(365);
        dateCode = expiry.toString(QStringLiteral("yyMMdd"));
    } else if (expiryText == QStringLiteral("自定义")) {
        dateCode = ui->customDateEdit->date().toString(QStringLiteral("yyMMdd"));
    }

    // 生成许可证密钥
    QString licenseKey = generateLicenseKey(formattedCode, dateCode);

    ui->licenseKeyEdit->setText(licenseKey);

    // 显示信息
    QString info = QStringLiteral("许可证生成成功！\n\n"
        "机器码: %1\n"
        "过期日期: %2\n"
        "许可证: %3\n\n"
        "点击复制按钮复制到剪贴板").arg(formattedCode, expiryText, licenseKey);

    QMessageBox::information(this, QStringLiteral("成功"), info);
}

void LicenseGenerator::onCopyClicked()
{
    QString licenseKey = ui->licenseKeyEdit->text();
    if (licenseKey.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先生成许可证"));
        return;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(licenseKey);
    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("许可证已复制到剪贴板"));
}

QString LicenseGenerator::generateLicenseKey(const QString &machineCode, const QString &expiryDate)
{
    // 机器码格式: XXXX-XXXX-XXXX-XXXX
    // 许可证格式: WKF-XXXX-XXXX-XXXX-XXXX-日期-签名

    // 计算签名
    // signData = 机器码(4段) + "-" + 日期
    QString signData = machineCode + '-' + expiryDate;
    QString signature = computeSignature(signData);

    // 组装许可证
    QString licenseKey = QStringLiteral("WKF-%1-%2-%3")
        .arg(machineCode, expiryDate, signature);

    return licenseKey;
}

QString LicenseGenerator::computeSignature(const QString &signData)
{
    // 与 LicenseManager::verifyLicenseKey 中的签名算法一致
    QByteArray hash = QCryptographicHash::hash(
        (signData + APP_SECRET).toUtf8(),
        QCryptographicHash::Sha256
    ).toHex().toUpper();

    return hash.left(16);
}
