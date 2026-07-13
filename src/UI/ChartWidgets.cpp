#include "UI/ChartWidgets.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QtMath>
#include <QFont>
#include <QLinearGradient>
#include <QRadialGradient>

// ============================================================================
// 调色板 — 精心挑选的 12 种颜色
// ============================================================================
static const QColor g_chartColors[] = {
    QColor("#4a90d9"),  // 蓝
    QColor("#e8a838"),  // 琥珀
    QColor("#27ae60"),  // 绿
    QColor("#e74c3c"),  // 红
    QColor("#8e44ad"),  // 紫
    QColor("#f39c12"),  // 橙
    QColor("#1abc9c"),  // 青
    QColor("#e67e22"),  // 深橙
    QColor("#3498db"),  // 浅蓝
    QColor("#2ecc71"),  // 翠绿
    QColor("#9b59b6"),  // 浅紫
    QColor("#16a085"),  // 深青
};
static constexpr int g_paletteSize = sizeof(g_chartColors) / sizeof(g_chartColors[0]);

// ============================================================================
// StatCardWidget 实现
// ============================================================================
StatCardWidget::StatCardWidget(const QString &title, const QString &value,
                               const QColor &accentColor, QWidget *parent)
    : QWidget(parent), m_title(title), m_value(value), m_accent(accentColor)
{
    setMinimumSize(120, 80);
    setMaximumHeight(100);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void StatCardWidget::setValue(const QString &value)
{
    m_value = value;
    update();
}

void StatCardWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect().adjusted(2, 2, -2, -2);

    // 阴影
    QPainterPath shadowPath;
    shadowPath.addRoundedRect(r.translated(0, 2), 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 25));
    p.drawPath(shadowPath);

    // 背景 — 白色卡片
    QPainterPath cardPath;
    cardPath.addRoundedRect(r, 8, 8);
    p.setBrush(Qt::white);
    p.setPen(QPen(QColor(0xe0, 0xe0, 0xe0), 1));
    p.drawPath(cardPath);

    // 顶部色条
    QPainterPath accentBar;
    accentBar.addRoundedRect(r.adjusted(0, 0, 0, -r.height() + 3), 8, 8);
    // Clip to top area
    QRectF topRect = QRectF(r.topLeft(), QSizeF(r.width(), 3));
    p.setClipRect(topRect);
    p.setBrush(m_accent);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r, 8, 8);
    p.setClipping(false);

    // 标题
    p.setPen(QColor(0x88, 0x88, 0x88));
    QFont titleFont = font();
    titleFont.setPixelSize(11);
    p.setFont(titleFont);
    p.drawText(QRectF(r.left() + 10, r.top() + 6, r.width() - 20, 18),
               Qt::AlignLeft | Qt::AlignVCenter, m_title);

    // 数值
    p.setPen(m_accent);
    QFont valueFont = font();
    valueFont.setPixelSize(22);
    valueFont.setBold(true);
    p.setFont(valueFont);
    p.drawText(QRectF(r.left() + 10, r.top() + 24, r.width() - 20, r.height() - 28),
               Qt::AlignLeft | Qt::AlignVCenter, m_value);
}

// ============================================================================
// DonutChartWidget 实现
// ============================================================================
DonutChartWidget::DonutChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 220);
    initPalette();
}

void DonutChartWidget::initPalette()
{
    m_palette.clear();
    for (int i = 0; i < g_paletteSize; ++i)
        m_palette.append(g_chartColors[i]);
}

void DonutChartWidget::setData(const QList<QPair<QString, double>> &data,
                               const QString &centerLabel)
{
    m_data = data;
    m_centerLabel = centerLabel;
    m_total = 0.0;
    for (const auto &d : m_data)
        m_total += d.second;
    update();
}

QRectF DonutChartWidget::chartRect() const
{
    // 左侧留给饼图
    double side = qMin(width() * 0.55, height() - 20.0);
    double cx = side / 2.0 + 10;
    double cy = height() / 2.0;
    return QRectF(cx - side / 2.0, cy - side / 2.0, side, side);
}

QRectF DonutChartWidget::legendRect() const
{
    double left = chartRect().right() + 20;
    return QRectF(left, 10, width() - left - 10, height() - 20);
}

void DonutChartWidget::paintEvent(QPaintEvent *)
{
    if (m_data.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF cr = chartRect();
    double outerR = cr.width() / 2.0;
    double innerR = outerR * 0.55;  // 环形内径
    QPointF center = cr.center();

    // --- 绘制环形 ---
    // 阴影
    QRadialGradient shadowGrad(center + QPointF(2, 3), outerR + 3);
    shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 40));
    shadowGrad.setColorAt(0.7, QColor(0, 0, 0, 20));
    shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(shadowGrad);
    p.drawEllipse(QRectF(center.x() - outerR - 3, center.y() - outerR - 3,
                          (outerR + 3) * 2, (outerR + 3) * 2));

    double startAngle = 90.0 * 16;  // 从顶部开始 (Qt: 3点钟方向=0, 逆时针为正)
    for (int i = 0; i < m_data.size(); ++i) {
        double fraction = m_total > 0 ? m_data[i].second / m_total : 0.0;
        double spanAngle = fraction * 360.0 * 16;

        if (spanAngle < 1.0) continue;  // 太小的扇区跳过

        QColor color = m_palette[i % m_palette.size()];

        // 扇区路径
        QPainterPath path;
        QRectF outerRect(center.x() - outerR, center.y() - outerR, outerR * 2, outerR * 2);
        path.arcMoveTo(outerRect, startAngle / 16.0);
        path.arcTo(outerRect, startAngle / 16.0, spanAngle / 16.0);

        QRectF innerRect(center.x() - innerR, center.y() - innerR, innerR * 2, innerR * 2);
        path.arcTo(innerRect, (startAngle + spanAngle) / 16.0, -spanAngle / 16.0);
        path.closeSubpath();

        // 渐变填充
        QLinearGradient grad(center - QPointF(outerR * 0.3, outerR * 0.3),
                             center + QPointF(outerR * 0.3, outerR * 0.3));
        grad.setColorAt(0.0, color.lighter(120));
        grad.setColorAt(1.0, color.darker(110));
        p.setBrush(grad);
        p.setPen(QPen(Qt::white, 2));
        p.drawPath(path);

        startAngle += spanAngle;
    }

    // 中心圆 — 白色背景
    QRadialGradient centerGrad(center, innerR);
    centerGrad.setColorAt(0.0, Qt::white);
    centerGrad.setColorAt(1.0, QColor(0xf5, 0xf6, 0xfa));
    p.setPen(QPen(QColor(0xe0, 0xe0, 0xe0), 1));
    p.setBrush(centerGrad);
    p.drawEllipse(QRectF(center.x() - innerR, center.y() - innerR, innerR * 2, innerR * 2));

    // 中心文字
    if (!m_centerLabel.isEmpty()) {
        QStringList parts = m_centerLabel.split('\n');
        p.setPen(QColor(0x55, 0x55, 0x55));
        QFont smallFont = font();
        smallFont.setPixelSize(10);
        p.setFont(smallFont);
        if (parts.size() >= 2) {
            p.drawText(QRectF(center.x() - innerR, center.y() - innerR * 0.3,
                              innerR * 2, innerR * 0.6),
                       Qt::AlignHCenter | Qt::AlignBottom, parts[0]);
            p.setPen(QColor(0x4a, 0x90, 0xd9));
            QFont bigFont = font();
            bigFont.setPixelSize(16);
            bigFont.setBold(true);
            p.setFont(bigFont);
            p.drawText(QRectF(center.x() - innerR, center.y() - innerR * 0.1,
                              innerR * 2, innerR * 0.6),
                       Qt::AlignHCenter | Qt::AlignTop, parts[1]);
        } else {
            p.setPen(QColor(0x4a, 0x90, 0xd9));
            QFont midFont = font();
            midFont.setPixelSize(13);
            midFont.setBold(true);
            p.setFont(midFont);
            p.drawText(QRectF(center.x() - innerR, center.y() - innerR * 0.5,
                              innerR * 2, innerR),
                       Qt::AlignCenter, m_centerLabel);
        }
    }

    // --- 绘制图例 ---
    QRectF lr = legendRect();
    double itemH = qMin(22.0, (lr.height() - 10) / qMax(m_data.size(), 1));
    double y = lr.top();

    QFont legendFont = font();
    legendFont.setPixelSize(10);
    p.setFont(legendFont);

    for (int i = 0; i < m_data.size(); ++i) {
        if (y + itemH > lr.bottom()) break;

        double fraction = m_total > 0 ? (m_data[i].second / m_total * 100.0) : 0.0;

        // 色块
        QColor color = m_palette[i % m_palette.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRectF(lr.left(), y + 3, 12, 12), 2, 2);

        // 名称 + 百分比
        p.setPen(QColor(0x33, 0x33, 0x33));
        QString label = QStringLiteral("%1  (%2%)")
                            .arg(m_data[i].first)
                            .arg(fraction, 0, 'f', 1);
        p.drawText(QRectF(lr.left() + 18, y, lr.width() - 18, itemH),
                   Qt::AlignLeft | Qt::AlignVCenter, label);

        y += itemH;
    }
}

// ============================================================================
// BarChartWidget 实现
// ============================================================================
BarChartWidget::BarChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 200);
    initPalette();
}

void BarChartWidget::initPalette()
{
    m_palette.clear();
    for (int i = 0; i < g_paletteSize; ++i)
        m_palette.append(g_chartColors[i]);
}

void BarChartWidget::setData(const QStringList &categories, const QList<double> &values,
                             const QString &valueSuffix, Orientation orient,
                             ValueMode valueMode)
{
    m_categories = categories;
    m_values = values;
    m_valueSuffix = valueSuffix;
    m_orientation = orient;
    m_valueMode = valueMode;

    m_maxValue = 0.0;
    for (double v : m_values)
        m_maxValue = qMax(m_maxValue, v);
    if (m_maxValue <= 0.0) m_maxValue = 1.0;

    update();
}

void BarChartWidget::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *)
{
    if (m_categories.isEmpty() || m_values.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF area = rect().adjusted(4, 4, -4, -4);

    // 标题
    double titleH = 0;
    if (!m_title.isEmpty()) {
        p.setPen(QColor(0x4a, 0x90, 0xd9));
        QFont titleFont = font();
        titleFont.setPixelSize(14);
        titleFont.setBold(true);
        p.setFont(titleFont);
        p.drawText(QRectF(area.left(), area.top(), area.width(), 24),
                   Qt::AlignLeft | Qt::AlignVCenter, m_title);
        titleH = 26;
    }

    QRectF chartArea = area.adjusted(0, titleH, 0, 0);
    int n = qMin(m_categories.size(), m_values.size());

    if (m_orientation == Horizontal) {
        // === 水平条形图 ===
        double labelW = 0;
        QFont labelFont = font();
        labelFont.setPixelSize(10);
        p.setFont(labelFont);
        QFontMetrics fm(labelFont);
        for (int i = 0; i < n; ++i) {
            labelW = qMax(labelW, (double)fm.horizontalAdvance(m_categories[i]));
        }
        labelW = qMin(labelW + 12, chartArea.width() * 0.3);

        double barAreaLeft = chartArea.left() + labelW;
        double barAreaW = chartArea.width() - labelW - 60;  // 60 for value label
        double barH = qMin(28.0, (chartArea.height() - (n - 1) * 4) / n);
        double totalBarsH = n * barH + (n - 1) * 4;
        double startY = chartArea.top() + (chartArea.height() - totalBarsH) / 2.0;

        for (int i = 0; i < n; ++i) {
            double y = startY + i * (barH + 4);
            QColor color = m_palette[i % m_palette.size()];
            double barW = (m_maxValue > 0) ? (m_values[i] / m_maxValue * barAreaW) : 0;

            // 类别标签
            p.setPen(QColor(0x55, 0x55, 0x55));
            QFont catFont = font();
            catFont.setPixelSize(10);
            p.setFont(catFont);
            QFontMetrics fm(catFont);
            QString elidedCat = fm.elidedText(m_categories[i], Qt::ElideRight, (int)(labelW - 4));
            p.drawText(QRectF(chartArea.left(), y, labelW - 4, barH),
                       Qt::AlignRight | Qt::AlignVCenter, elidedCat);

            // 背景条
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0xf0, 0xf0, 0xf0));
            p.drawRoundedRect(QRectF(barAreaLeft, y + barH * 0.15, barAreaW, barH * 0.7), 3, 3);

            // 数据条 — 带渐变
            if (barW > 2) {
                QLinearGradient barGrad(barAreaLeft, 0, barAreaLeft + barW, 0);
                barGrad.setColorAt(0.0, color);
                barGrad.setColorAt(1.0, color.lighter(130));
                p.setBrush(barGrad);
                p.drawRoundedRect(QRectF(barAreaLeft, y + barH * 0.15, barW, barH * 0.7), 3, 3);

                // 高光条
                p.setBrush(QColor(255, 255, 255, 40));
                p.drawRoundedRect(QRectF(barAreaLeft, y + barH * 0.15, barW, barH * 0.35), 3, 3);
            }

            // 数值标签
            if (m_valueMode == ShowValue) {
                p.setPen(QColor(0x33, 0x33, 0x33));
                QFont valFont = font();
                valFont.setPixelSize(10);
                valFont.setBold(true);
                p.setFont(valFont);
                QString valStr;
                if (m_valueSuffix.isEmpty())
                    valStr = QString::number(m_values[i], 'f', 2);
                else
                    valStr = QString::number(m_values[i], 'f', 2) + m_valueSuffix;
                p.drawText(QRectF(barAreaLeft + barAreaW + 4, y, 56, barH),
                           Qt::AlignLeft | Qt::AlignVCenter, valStr);
            }
        }
    } else {
        // === 垂直柱形图 ===
        double bottomMargin = 30;  // x轴标签
        double topMargin = 20;     // 顶部留白
        double barAreaH = chartArea.height() - bottomMargin - topMargin;
        double barAreaTop = chartArea.top() + topMargin;
        double barW = qMin(50.0, (chartArea.width() - (n - 1) * 8) / n);
        double totalBarsW = n * barW + (n - 1) * 8;
        double startX = chartArea.left() + (chartArea.width() - totalBarsW) / 2.0;

        // 网格线
        int gridLines = 5;
        p.setPen(QPen(QColor(0xe8, 0xe8, 0xe8), 1, Qt::DashLine));
        for (int g = 0; g <= gridLines; ++g) {
            double gy = barAreaTop + barAreaH * g / gridLines;
            p.drawLine(QPointF(chartArea.left(), gy), QPointF(chartArea.right() - 20, gy));

            // Y轴标签
            double val = m_maxValue * (gridLines - g) / gridLines;
            p.setPen(QColor(0x99, 0x99, 0x99));
            QFont yFont = font();
            yFont.setPixelSize(9);
            p.setFont(yFont);
            QString yLabel;
            if (m_valueSuffix.isEmpty())
                yLabel = QString::number(val, 'f', 1);
            else
                yLabel = QString::number(val, 'f', 1) + m_valueSuffix;
            p.drawText(QRectF(chartArea.right() - 20, gy - 10, 20, 20),
                       Qt::AlignLeft | Qt::AlignVCenter, yLabel);
            p.setPen(QPen(QColor(0xe8, 0xe8, 0xe8), 1, Qt::DashLine));
        }

        // 柱形
        for (int i = 0; i < n; ++i) {
            double barH = (m_maxValue > 0) ? (m_values[i] / m_maxValue * barAreaH) : 0;
            double x = startX + i * (barW + 8);
            double y = barAreaTop + barAreaH - barH;

            QColor color = m_palette[i % m_palette.size()];

            // 阴影
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, 15));
            p.drawRoundedRect(QRectF(x + 1, y + 2, barW, barH), 4, 4);

            // 柱体渐变
            QLinearGradient barGrad(x, y, x, y + barH);
            barGrad.setColorAt(0.0, color.lighter(120));
            barGrad.setColorAt(0.5, color);
            barGrad.setColorAt(1.0, color.darker(115));
            p.setBrush(barGrad);
            p.drawRoundedRect(QRectF(x, y, barW, barH), 4, 4);

            // 顶部高光
            if (barH > 8) {
                p.setBrush(QColor(255, 255, 255, 50));
                p.drawRoundedRect(QRectF(x + 2, y + 2, barW - 4, qMin(barH * 0.4, 20.0)), 3, 3);
            }

            // 数值标签（柱顶）
            if (m_valueMode == ShowValue) {
                p.setPen(QColor(0x33, 0x33, 0x33));
                QFont valFont = font();
                valFont.setPixelSize(9);
                valFont.setBold(true);
                p.setFont(valFont);
                QString valStr;
                if (m_valueSuffix.isEmpty())
                    valStr = QString::number(m_values[i], 'f', 2);
                else
                    valStr = QString::number(m_values[i], 'f', 2) + m_valueSuffix;
                p.drawText(QRectF(x - 4, y - 18, barW + 8, 16),
                           Qt::AlignHCenter | Qt::AlignBottom, valStr);
            }

            // X轴标签
            p.setPen(QColor(0x55, 0x55, 0x55));
            QFont xFont = font();
            xFont.setPixelSize(9);
            p.setFont(xFont);
            QFontMetrics xfm(xFont);
            QString elidedText = xfm.elidedText(m_categories[i], Qt::ElideRight, barW + 16);
            p.drawText(QRectF(x - 8, barAreaTop + barAreaH + 4, barW + 16, bottomMargin - 4),
                       Qt::AlignHCenter | Qt::AlignTop, elidedText);
        }
    }
}
