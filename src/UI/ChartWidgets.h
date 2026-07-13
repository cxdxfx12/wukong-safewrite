#ifndef CHARTWIDGETS_H
#define CHARTWIDGETS_H

#include <QWidget>
#include <QString>
#include <QList>
#include <QPair>
#include <QColor>

// ============================================================================
// StatCardWidget — 单个统计卡片
// ============================================================================
class StatCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatCardWidget(const QString &title, const QString &value,
                            const QColor &accentColor, QWidget *parent = nullptr);

    void setValue(const QString &value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QString m_value;
    QColor m_accent;
};

// ============================================================================
// DonutChartWidget — 环形图（带图例）
// ============================================================================
class DonutChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit DonutChartWidget(QWidget *parent = nullptr);

    void setData(const QList<QPair<QString, double>> &data, const QString &centerLabel = QString());

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<QPair<QString, double>> m_data;
    QString m_centerLabel;
    QList<QColor> m_palette;
    double m_total = 0.0;

    void initPalette();
    QRectF chartRect() const;
    QRectF legendRect() const;
};

// ============================================================================
// BarChartWidget — 柱形图 / 条形图
// ============================================================================
class BarChartWidget : public QWidget {
    Q_OBJECT
public:
    enum Orientation { Horizontal, Vertical };
    enum ValueMode { ShowValue, ShowBarOnly };

    explicit BarChartWidget(QWidget *parent = nullptr);

    void setData(const QStringList &categories, const QList<double> &values,
                 const QString &valueSuffix = QString(),
                 Orientation orient = Horizontal,
                 ValueMode valueMode = ShowValue);

    void setTitle(const QString &title);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList m_categories;
    QList<double> m_values;
    QString m_valueSuffix;
    Orientation m_orientation = Horizontal;
    ValueMode m_valueMode = ShowValue;
    QString m_title;
    QList<QColor> m_palette;
    double m_maxValue = 0.0;

    void initPalette();
};

#endif // CHARTWIDGETS_H
