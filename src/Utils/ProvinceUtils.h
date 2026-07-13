#ifndef PROVINCEUTILS_H
#define PROVINCEUTILS_H

#include <QString>
#include <QStringList>

/**
 * @brief 省份名称处理工具类
 * 
 * 统一处理省份名称的标准化、匹配和模糊搜索
 * 消除代码中重复的省份处理逻辑
 */
class ProvinceUtils {
public:
    static QString normalize(const QString &province);
    static QString extractTwoCharPrefix(const QString &province);
    static bool matches(const QString &input, const QString &target);
    static const QStringList &getSuffixes();

    /**
     * @brief ★ 标准化为报价表中的省份简称
     *        新疆维吾尔自治区 → 新疆, 上海市 → 上海
     * @return 标准简称，未匹配则返回 normalize() 的结果
     */
    static QString standardize(const QString &province);
};

#endif // PROVINCEUTILS_H
