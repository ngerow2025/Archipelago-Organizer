//
// Created by birb on 9/29/26.
//

#pragma once

#include <QPainter>
#include <QSize>
#include <QTimer>
#include <QWidget>
#include <algorithm>

// A classic "fading lines" rotating spinner.
// Header-only: just drop this file into your project.
class LoadingSpinner : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingSpinner(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_color(palette().color(QPalette::Highlight))
    {
        connect(&m_timer, &QTimer::timeout, this, [this] {
            m_head = (m_head + 1) % m_lineCount;
            update();
        });
        updateInterval();
    }

    // --- Control ---
    void start() { m_timer.start(); update(); }
    void stop()  { m_timer.stop();  update(); }
    [[nodiscard]] bool isSpinning() const { return m_timer.isActive(); }

    // --- Appearance ---
    void setColor(const QColor &c)      { m_color = c; update(); }
    void setLineCount(int n)            { m_lineCount = std::max(3, n); updateInterval(); update(); }
    void setRevolutionMs(int ms)        { m_revolutionMs = std::max(100, ms); updateInterval(); }
    void setLineWidthRatio(qreal r)     { m_lineWidthRatio = r; update(); }   // fraction of radius
    void setInnerRadiusRatio(qreal r)   { m_innerRatio = r; update(); }       // fraction of radius

    [[nodiscard]] QSize sizeHint() const override        { return {48, 48}; }
    [[nodiscard]] QSize minimumSizeHint() const override { return {16, 16}; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);

        const qreal radius = std::min(width(), height()) / 2.0;
        const qreal outer  = radius;
        const qreal inner  = radius * m_innerRatio;
        const qreal lineW  = radius * m_lineWidthRatio;

        p.translate(width() / 2.0, height() / 2.0);

        for (int i = 0; i < m_lineCount; ++i) {
            // How far behind the "head" this line is (0 = brightest).
            const int distance = (m_head - i + m_lineCount) % m_lineCount;
            qreal alpha = 1.0 - static_cast<qreal>(distance) / m_lineCount;
            alpha = std::max(alpha, 0.15);

            // When stopped, draw all lines faintly and evenly.
            QColor c = m_color;
            c.setAlphaF(isSpinning() ? alpha : 0.25);
            p.setBrush(c);

            p.save();
            p.rotate(360.0 * i / m_lineCount);
            p.drawRoundedRect(QRectF(-lineW / 2, -outer, lineW, outer - inner),
                              lineW / 2, lineW / 2);
            p.restore();
        }
    }

private:
    void updateInterval() { m_timer.setInterval(std::max(1, m_revolutionMs / m_lineCount)); }

    QTimer m_timer;
    QColor m_color;
    int    m_lineCount      = 12;
    int    m_revolutionMs   = 1000;
    int    m_head           = 0;
    qreal  m_lineWidthRatio = 0.14;
    qreal  m_innerRatio     = 0.50;
};

