#include "LicenseManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QDataStream>
#include <QSettings>
#include <QSysInfo>
#include <QProcess>

// 密钥混淆存储（避免明文出现在二进制中）
static QByteArray getAppSecret()
{
    static const unsigned char xorKey[] = {0xA3, 0x5F, 0x71, 0x2C, 0x9E, 0x4B, 0xD8, 0x60};
    static const unsigned char obfuscated[] = {
        0xF4, 0x2A, 0x1A, 0x43, 0xF0, 0x2C, 0x9E, 0x12,
        0xC6, 0x36, 0x16, 0x44, 0xEA, 0x79, 0xE8, 0x52,
        0x95, 0x1F, 0x07, 0x1D, 0xB0, 0x7B
    };
    QByteArray result;
    int keyLen = sizeof(xorKey);
    for (int i = 0; i < (int)sizeof(obfuscated); ++i) {
        result.append(static_cast<char>(obfuscated[i] ^ xorKey[i % keyLen]));
    }
    return result;
}

static const int TRIAL_DAYS = 7;

// 防系统时间回拨：记录并检查最大已见日期
static QDate getSecureCurrentDate()
{
    QDate today = QDate::currentDate();
    QSettings settings("WukongFreight", "WukongFreight");
    QDate maxDate = QDate::fromString(settings.value("maxSeenDate").toString(), Qt::ISODate);
    if (maxDate.isValid() && today < maxDate) {
        // 系统时间被回拨，使用记录的最大日期
        return maxDate;
    }
    settings.setValue("maxSeenDate", today.toString(Qt::ISODate));
    return today;
}

static QString computeTrialSignature(const QString &trialStart, const QString &fingerprint)
{
    QByteArray data = (trialStart + "|" + fingerprint + "|TRIAL").toUtf8() + getAppSecret();
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex().left(32);
}

LicenseManager::LicenseManager()
{
    loadLicenseFromFile();
}

QString LicenseManager::generateMachineCode()
{
    QString fingerprint = computeHardwareFingerprint();
    if (fingerprint.isEmpty()) {
        // 备用方案：基于随机种子生成固定ID
        fingerprint = QStringLiteral("WKFALLBACK");
    }

    // 取前16位作为机器码显示，但完整指纹用于验证
    QByteArray hash = QCryptographicHash::hash(
        fingerprint.toUtf8(), QCryptographicHash::Sha256
    ).toHex().toUpper();

    // 格式化为 XXXX-XXXX-XXXX-XXXX
    QString code;
    for (int i = 0; i < 16 && i < hash.length(); ++i) {
        if (i > 0 && i % 4 == 0) code += '-';
        code += hash[i];
    }
    return code;
}

QString LicenseManager::computeHardwareFingerprint()
{
    QStringList components;

    QString mac = getMacAddress();
    if (!mac.isEmpty()) components.append(mac);

    QString cpu = getCpuId();
    if (!cpu.isEmpty()) components.append(cpu);

    QString vol = getVolumeUuid();
    if (!vol.isEmpty()) components.append(vol);

    QString board = getBoardSerial();
    if (!board.isEmpty()) components.append(board);

    if (components.isEmpty()) {
        QString fallback = QStringLiteral("%1_%2_%3")
            .arg(QSysInfo::productType())
            .arg(QSysInfo::currentCpuArchitecture())
            .arg(QSysInfo::kernelVersion());
        components.append(fallback);
    }

    QByteArray data = components.join('|').toUtf8();
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

QString LicenseManager::getMacAddress()
{
    QString mac;
    foreach (const QNetworkInterface &iface, QNetworkInterface::allInterfaces()) {
        // 跳过虚拟网卡和回环接口
        if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
        if (iface.name().contains(QStringLiteral("docker"), Qt::CaseInsensitive)) continue;
        if (iface.name().contains(QStringLiteral("vmnet"), Qt::CaseInsensitive)) continue;
        if (iface.name().contains(QStringLiteral("veth"), Qt::CaseInsensitive)) continue;

        QString hwAddr = iface.hardwareAddress();
        if (!hwAddr.isEmpty() && hwAddr != QStringLiteral("00:00:00:00:00:00")) {
            mac = hwAddr.remove(':');
            break;
        }
    }
    return mac;
}

QString LicenseManager::getCpuId()
{
#if defined(Q_OS_MAC)
    QProcess proc;
    proc.start(QStringLiteral("sysctl"), QStringList() << QStringLiteral("-n") << QStringLiteral("machdep.cpu.brand_string"));
    if (proc.waitForFinished(3000)) {
        QString cpu = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!cpu.isEmpty()) return cpu;
    }
    // 备用：获取CPU核心数+架构信息
    proc.start(QStringLiteral("sysctl"), QStringList() << QStringLiteral("-n") << QStringLiteral("hw.physicalcpu"));
    if (proc.waitForFinished(3000)) {
        QString cores = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        return QStringLiteral("MACCPU%1").arg(cores);
    }
#elif defined(Q_OS_WIN)
    QProcess proc;
    proc.start(QStringLiteral("wmic"), QStringList() << QStringLiteral("cpu") << QStringLiteral("get") << QStringLiteral("ProcessorId") << QStringLiteral("/value"));
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QRegularExpression re(QStringLiteral("ProcessorId=(.+)$"));
        auto match = re.match(output);
        if (match.hasMatch()) return match.captured(1).trimmed();
    }
#elif defined(Q_OS_LINUX)
    QFile file(QStringLiteral("/proc/cpuinfo"));
    if (file.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(file.readAll());
        QRegularExpression re(QStringLiteral("serial\\s*:\\s*(.+)$"), QRegularExpression::MultilineOption);
        auto match = re.match(content);
        if (match.hasMatch()) return match.captured(1).trimmed();

        // 备用：使用model name
        re.setPattern(QStringLiteral("model name\\s*:\\s*(.+)$"));
        match = re.match(content);
        if (match.hasMatch()) return match.captured(1).trimmed();
    }
#endif
    return QString();
}

QString LicenseManager::getVolumeUuid()
{
#if defined(Q_OS_MAC)
    QProcess proc;
    proc.start(QStringLiteral("diskutil"), QStringList() << QStringLiteral("info") << QStringLiteral("/"));
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        QRegularExpression re(QStringLiteral("Volume UUID:\\s*(.+)$"), QRegularExpression::MultilineOption);
        auto match = re.match(output);
        if (match.hasMatch()) return match.captured(1).trimmed();
    }
#elif defined(Q_OS_WIN)
    QProcess proc;
    proc.start(QStringLiteral("wmic"), QStringList() << QStringLiteral("path") << QStringLiteral("win32_computersystemproduct") << QStringLiteral("get") << QStringLiteral("UUID") << QStringLiteral("/value"));
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QRegularExpression re(QStringLiteral("UUID=(.+)$"));
        auto match = re.match(output);
        if (match.hasMatch()) {
            QString uuid = match.captured(1).trimmed();
            if (uuid != QStringLiteral("FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"))
                return uuid;
        }
    }
#elif defined(Q_OS_LINUX)
    QProcess proc;
    proc.start(QStringLiteral("blkid"), QStringList() << QStringLiteral("-s") << QStringLiteral("UUID") << QStringLiteral("-o") << QStringLiteral("value") << QStringLiteral("/"));
    if (proc.waitForFinished(3000)) {
        QString uuid = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!uuid.isEmpty()) return uuid;
    }
#endif
    return QString();
}

QString LicenseManager::getBoardSerial()
{
#if defined(Q_OS_MAC)
    QProcess proc;
    proc.start(QStringLiteral("ioreg"), QStringList() << QStringLiteral("-l") << QStringLiteral("-p") << QStringLiteral("IODeviceTree"));
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        QRegularExpression re(QStringLiteral("\"IOPlatformSerialNumber\"\\s*=\\s*\"([^\"]+)\""));
        auto match = re.match(output);
        if (match.hasMatch()) {
            QString serial = match.captured(1);
            if (serial != QStringLiteral("System Serial Number") &&
                serial != QStringLiteral("Not Available"))
                return serial;
        }
    }
#elif defined(Q_OS_WIN)
    QProcess proc;
    proc.start(QStringLiteral("wmic"), QStringList() << QStringLiteral("baseboard") << QStringLiteral("get") << QStringLiteral("SerialNumber") << QStringLiteral("/value"));
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QRegularExpression re(QStringLiteral("SerialNumber=(.+)$"));
        auto match = re.match(output);
        if (match.hasMatch()) return match.captured(1).trimmed();
    }
#endif
    return QString();
}

bool LicenseManager::validateLicense(const QString &licenseKey)
{
    if (licenseKey.isEmpty()) return false;

    QString machineCode = generateMachineCode();

    if (verifyLicenseKey(licenseKey, machineCode)) {
        LicenseInfo info;
        if (parseLicenseKey(licenseKey, info)) {
            if (info.expireDate < getSecureCurrentDate()) {
                return false;
            }
            info.isValid = true;
            info.machineCode = machineCode;
            info.licenseKey = licenseKey;
            m_license = info;
            saveLicenseToFile(licenseKey);
            return true;
        }
    }
    return false;
}

bool LicenseManager::verifyLicenseKey(const QString &licenseKey, const QString &machineCode)
{
    QStringList parts = licenseKey.split('-');
    if (parts.size() != 7) return false;
    if (parts[0] != QStringLiteral("WKF")) return false;

    QString keyMachineCode = parts.mid(1, 4).join('-');
    if (keyMachineCode != machineCode) return false;

    QString signData = parts.mid(1, 5).join('-');
    QByteArray expectedHash = QCryptographicHash::hash(
        (signData + getAppSecret()).toUtf8(), QCryptographicHash::Sha256
    ).toHex().toUpper();
    QString expectedSign = expectedHash.left(32);

    return parts[6] == expectedSign;
}

bool LicenseManager::parseLicenseKey(const QString &licenseKey, LicenseInfo &info)
{
    QStringList parts = licenseKey.split('-');
    if (parts.size() != 7) return false;

    // parts[5] 是过期日期编码 (YYMMDD)
    QString dateCode = parts[5];
    if (dateCode == QStringLiteral("LIFE")) {
        info.expireDate = QDate(2099, 12, 31);
    } else if (dateCode.length() == 6) {
        int year = dateCode.mid(0, 2).toInt() + 2000;
        int month = dateCode.mid(2, 2).toInt();
        int day = dateCode.mid(4, 2).toInt();
        info.expireDate = QDate(year, month, day);
    } else {
        return false;
    }

    info.activateDate = getSecureCurrentDate().toString(QStringLiteral("yyyy-MM-dd"));
    return true;
}

bool LicenseManager::loadLicenseFromFile()
{
    QString path = licenseFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        initTrialIfNeeded();
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        initTrialIfNeeded();
        return false;
    }

    QJsonObject obj = doc.object();
    QString licenseKey = obj.value(QStringLiteral("licenseKey")).toString();
    QString storedFingerprint = obj.value(QStringLiteral("fingerprint")).toString();

    QString currentFingerprint = computeHardwareFingerprint();

    if (storedFingerprint.isEmpty() || storedFingerprint != currentFingerprint) {
        m_license.isValid = false;
        initTrialIfNeeded();
        return false;
    }

    if (!licenseKey.isEmpty()) {
        if (validateLicense(licenseKey)) {
            return true;
        }
        initTrialIfNeeded();
        return false;
    }

    QString storedSig = obj.value(QStringLiteral("sig")).toString();
    QString trialStartStr = obj.value(QStringLiteral("trialStart")).toString();
    if (!trialStartStr.isEmpty()) {
        QString expectedSig = computeTrialSignature(trialStartStr, currentFingerprint);
        if (storedSig != expectedSig) {
            initTrialIfNeeded();
            return false;
        }
    }

    QDate trialStart = QDate::fromString(trialStartStr, Qt::ISODate);
    if (trialStart.isValid()) {
        m_license.isTrial = true;
        m_license.expireDate = trialStart.addDays(TRIAL_DAYS);
        m_license.hardwareFingerprint = currentFingerprint;
    }

    initTrialIfNeeded();
    return false;
}

bool LicenseManager::saveLicenseToFile(const QString &licenseKey)
{
    QString path = licenseFilePath();
    QDir().mkpath(QFileInfo(path).path());

    QJsonObject obj;
    obj[QStringLiteral("licenseKey")] = licenseKey;
    obj[QStringLiteral("fingerprint")] = computeHardwareFingerprint();
    obj[QStringLiteral("activateDate")] = getSecureCurrentDate().toString(Qt::ISODate);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    return true;
}

void LicenseManager::initTrialIfNeeded()
{
    if (!m_license.isValid && m_license.expireDate.isNull()) {
        QString path = licenseFilePath();
        QFile file(path);

        QJsonObject obj;
        QString currentFingerprint = computeHardwareFingerprint();
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) obj = doc.object();
        }

        QString trialStartStr = obj.value(QStringLiteral("trialStart")).toString();
        QDate trialStart = QDate::fromString(trialStartStr, Qt::ISODate);
        QString storedSig = obj.value(QStringLiteral("sig")).toString();

        bool validTrial = false;
        if (trialStart.isValid() && !storedSig.isEmpty()) {
            QString expectedSig = computeTrialSignature(trialStartStr, currentFingerprint);
            if (storedSig == expectedSig) {
                validTrial = true;
            }
        }

        // 防试用重置：检查系统级备份（QSettings），用户删除license.dat无法重置试用期
        QSettings settings("WukongFreight", "WukongFreight");
        QString settingsTrialStart = settings.value("trialStart").toString();
        if (!settingsTrialStart.isEmpty()) {
            QDate settingsDate = QDate::fromString(settingsTrialStart, Qt::ISODate);
            if (settingsDate.isValid()) {
                if (!validTrial || settingsDate < trialStart) {
                    // 系统备份有更早的试用起始日期，防止删除文件重置试用
                    trialStart = settingsDate;
                    trialStartStr = trialStart.toString(Qt::ISODate);
                    validTrial = true;
                }
            }
        }

        if (!validTrial) {
            trialStart = getSecureCurrentDate();
            trialStartStr = trialStart.toString(Qt::ISODate);
            obj[QStringLiteral("trialStart")] = trialStartStr;
            obj[QStringLiteral("fingerprint")] = currentFingerprint;
            obj[QStringLiteral("sig")] = computeTrialSignature(trialStartStr, currentFingerprint);

            QDir().mkpath(QFileInfo(path).path());
            QFile out(path);
            if (out.open(QIODevice::WriteOnly)) {
                out.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
            }
            // 同步到系统级备份
            settings.setValue("trialStart", trialStartStr);
        }

        m_license.isTrial = true;
        m_license.expireDate = trialStart.addDays(TRIAL_DAYS);
        m_license.hardwareFingerprint = currentFingerprint;
    }
}

bool LicenseManager::isAuthorized() const
{
    return m_license.isValid || isTrialValid();
}

bool LicenseManager::isTrialValid() const
{
    if (!m_license.isTrial) return false;
    return getSecureCurrentDate() <= m_license.expireDate;
}

int LicenseManager::trialDaysLeft() const
{
    if (!m_license.isTrial) return 0;
    return qMax(0, getSecureCurrentDate().daysTo(m_license.expireDate));
}

QString LicenseManager::statusText() const
{
    if (m_license.isValid) {
        if (m_license.expireDate.year() >= 2099) {
            return QStringLiteral("永久授权");
        }
        return QStringLiteral("授权有效期至: %1").arg(m_license.expireDate.toString(QStringLiteral("yyyy-MM-dd")));
    }
    if (isTrialValid()) {
        return QStringLiteral("试用期剩余 %1 天").arg(trialDaysLeft());
    }
    return QStringLiteral("未授权");
}

bool LicenseManager::clearLicense()
{
    m_license = LicenseInfo();
    QString path = licenseFilePath();
    QFile file(path);
    if (file.exists()) {
        file.remove();
    }
    // 保留系统级试用备份（QSettings），防止注销后重新获得试用期
    return true;
}

QString LicenseManager::licenseFilePath()
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty()) appDataDir = QDir::homePath() + QStringLiteral("/.WukongFreight");
    return appDataDir + QStringLiteral("/license.dat");
}
