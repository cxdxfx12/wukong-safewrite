#ifndef BANNERWIDGET_H
#define BANNERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QString>

class QLabel;
class QVBoxLayout;

class BannerWidget : public QWidget {
    Q_OBJECT
public:
    explicit BannerWidget(QWidget *parent = nullptr);

    void setTexts(const QStringList &texts);
    void setInterval(int milliseconds);

private slots:
    void showNextText();

private:
    void updateDisplay();

    QLabel *m_textLabel;
    QTimer *m_timer;
    QStringList m_texts;
    int m_currentIndex;
    int m_interval;
};

#endif // BANNERWIDGET_H