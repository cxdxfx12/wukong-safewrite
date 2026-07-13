#include "BannerWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QGradient>
#include <QLinearGradient>

BannerWidget::BannerWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentIndex(0)
    , m_interval(3000)
{
    m_textLabel = new QLabel(this);
    m_textLabel->setAlignment(Qt::AlignCenter);
    m_textLabel->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: orange;"
    );

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_textLabel, 0, Qt::AlignCenter);
    layout->setContentsMargins(16, 4, 16, 4);

    setStyleSheet(
        "BannerWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "        stop:0 #4a90d9, stop:0.3 #3a7bc8, stop:0.7 #2d6bb3, stop:1 #1e58a0);"
        "    border-radius: 8px;"
        "}"
    );

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &BannerWidget::showNextText);
}

void BannerWidget::setTexts(const QStringList &texts)
{
    m_texts = texts;
    m_currentIndex = 0;
    updateDisplay();
}

void BannerWidget::setInterval(int milliseconds)
{
    m_interval = milliseconds;
}

void BannerWidget::showNextText()
{
    if (m_texts.isEmpty())
        return;

    m_currentIndex = (m_currentIndex + 1) % m_texts.size();
    updateDisplay();
}

void BannerWidget::updateDisplay()
{
    if (!m_texts.isEmpty()) {
        m_textLabel->setText(m_texts[m_currentIndex]);
        m_timer->start(m_interval);
    }
}