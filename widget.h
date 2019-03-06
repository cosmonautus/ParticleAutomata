#ifndef WIDGET_H
#define WIDGET_H

#include <QColor>
#include <QPen>
#include <QBrush>
#include <QWidget>

#include <memory>

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  https://habr.com/ru/post/442128/
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const int WIN_W = 1800;
static const int WIN_H = 1000;
static const int NODE_RADIUS = 5;
static const int NODE_COUNT = 800;
static const int MAX_DIST = 100;
static const int MAX_DIST2 = MAX_DIST * MAX_DIST;
static const double SPEED = 4.0;
static const int SKIP_FRAMES = 3;
static const int BORDER = 30;
static const int FIELD_W = WIN_W / MAX_DIST + 1;
static const int FIELD_H = WIN_H / MAX_DIST + 1;
static const double LINK_FORCE = -0.015f;
static const int LINKS[3] = { 1, 3, 2 };
static const double COUPLING[3][3] = {
    {1, 0, -1},
    {1, 1, 1},
    {1, 1, 1}
};
static const int LINKS_POSSIBLE[3][3] = {
        {0, 1, 1},
        {1, 2, 1},
        {1, 1, 2}
};
static const QBrush COLORS[3] = {
        QBrush(QColor(250, 20, 20)),
        QBrush(QColor(200, 140, 100)),
        QBrush(QColor(80, 170, 140))
};
static const QColor COLOR_BG = QColor(20, 55, 75, 255);
static const QPen COLOR_LINK = QPen(QColor(255, 230, 0, 100));

class Particle;

using Link = QPair<Particle*, Particle*>;
using PtrParticle = std::shared_ptr<Particle*>;

class Particle
{
public:
    int type;
    double x;
    double y;
    double sx = 0;
    double sy = 0;
    int links = 0;
    QList<Particle*> bonds;

    Particle(int aType, double ax, double ay) : type(aType), x(ax), y(ay) { }
    void addBond(Particle* p);
    void removeBond(Particle* p);
    bool contains(Particle* p) { return bonds.contains(p); }
    double applyForce(Particle* b);
    void calc();
};

class Field
{
public:
    QList<Particle*> particles;
    void addParticle(Particle* p) { particles.append(p); }
    void removeParticle(Particle* p) { particles.removeOne(p); }
};

class Widget : public QWidget
{
    Q_OBJECT
    QList<Link> links;
    Field fields[FIELD_W][FIELD_H];
    Particle* addParticle(int type, double x, double y);
    void logic();
    void drawScene(QPainter* paint);

public:
    Widget(QWidget *parent = 0);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void timerEvent(QTimerEvent *event) override;
};

#endif // WIDGET_H
