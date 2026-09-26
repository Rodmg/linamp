#include "equalizergraph.h"
#include "equalizerbands.h"

#include <QPainter>
#include <QPaintEvent>

#include <algorithm>

namespace {

QColor mixColor(const QColor &a, const QColor &b, double t)
{
    t = std::clamp(t, 0.0, 1.0);
    return QColor(
        int(a.red() + (b.red() - a.red()) * t),
        int(a.green() + (b.green() - a.green()) * t),
        int(a.blue() + (b.blue() - a.blue()) * t));
}

QPointF cubicAt(const QPointF &p0, const QPointF &c1, const QPointF &c2, const QPointF &p1, double t)
{
    const double u = 1.0 - t;
    return (u * u * u) * p0 + (3 * u * u * t) * c1 + (3 * u * t * t) * c2 + (t * t * t) * p1;
}

}

QColor equalizerColorForDb(double db)
{
    db = EqBands::clampDb(db);
    const QColor cut("#00e800");
    const QColor flat("#ffe000");
    const QColor hot("#ff1818");
    if (db < 0.0) {
        return mixColor(flat, cut, -db / EqBands::MaxDb);
    }
    return mixColor(flat, hot, db / EqBands::MaxDb);
}

EqualizerGraph::EqualizerGraph(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(76);
}

void EqualizerGraph::setBandDb(int index, double db)
{
    if (index < 0 || index >= EqBands::Count) {
        return;
    }
    m_bandDb[index] = EqBands::clampDb(db);
    update();
}

void EqualizerGraph::setBands(const double *db, int count)
{
    const int n = std::min(count, EqBands::Count);
    for (int i = 0; i < n; ++i) {
        m_bandDb[i] = EqBands::clampDb(db[i]);
    }
    update();
}

void EqualizerGraph::setPreampDb(double db)
{
    m_preampDb = EqBands::clampDb(db);
    update();
}

void EqualizerGraph::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    constexpr int kOverhang = 8;
    const QRectF plot(kOverhang, 1, width() - kOverhang * 2 - 1, height() - 2);

    auto yFor = [&](double db) {
        const double span = EqBands::MaxDb - EqBands::MinDb;
        const double t = (EqBands::clampDb(db) - EqBands::MinDb) / span;
        return plot.top() + (1.0 - t) * plot.height();
    };
    auto dbForY = [&](double y) {
        const double t = 1.0 - (y - plot.top()) / plot.height();
        return EqBands::MinDb + t * (EqBands::MaxDb - EqBands::MinDb);
    };
    auto xFor = [&](int band) {
        return plot.left() + plot.width() * band / double(EqBands::Count - 1);
    };

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(255, 255, 255, 36), 1));
    for (int i = 0; i < EqBands::Count; ++i) {
        const int x = int(xFor(i));
        painter.drawLine(x, int(plot.top()), x, int(plot.bottom()));
    }
    const int centerY = int(yFor(-m_preampDb));
    painter.setPen(QPen(Qt::white, 1));
    painter.drawLine(0, centerY, width() - 1, centerY);

    QVector<QPointF> points;
    points.reserve(EqBands::Count);
    for (int i = 0; i < EqBands::Count; ++i) {
        points.append(QPointF(xFor(i), yFor(m_bandDb[i])));
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    constexpr int kSteps = 8;
    for (int i = 0; i < points.size() - 1; ++i) {
        const int last = int(points.size()) - 1;
        const QPointF p0 = points[std::max(i - 1, 0)];
        const QPointF p1 = points[i];
        const QPointF p2 = points[i + 1];
        const QPointF p3 = points[std::min(i + 2, last)];
        const QPointF c1 = p1 + (p2 - p0) / 6.0;
        const QPointF c2 = p2 - (p3 - p1) / 6.0;

        QPointF previous = p1;
        for (int step = 1; step <= kSteps; ++step) {
            const QPointF next = cubicAt(p1, c1, c2, p2, step / double(kSteps));
            const double level = dbForY((previous.y() + next.y()) / 2.0);
            painter.setPen(QPen(equalizerColorForDb(level), 2, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));
            painter.drawLine(previous, next);
            previous = next;
        }
    }
}
