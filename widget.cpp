#include "widget.h"
#include <QPainter>

#include <algorithm>

void Particle::addBond(Particle *p)
{
    links++;
    bonds.append(p);
    p->links++;
    p->bonds.append(this);
}

void Particle::removeBond(Particle* p)
{
    bonds.removeOne(p);
    links--;
    p->bonds.removeOne(this);
    p->links--;
}

double Particle::applyForce(Particle *b)
{
    bool canLink = false;
    double d2 = (x - b->x) * (x - b->x) + (y - b->y) * (y - b->y);
    if (d2 < MAX_DIST2)
    {
        double dA = COUPLING[type][b->type] / d2;
        double dB = COUPLING[b->type][type] / d2;
        if (this->links < LINKS[type] && b->links < LINKS[b->type])
        {
            if (d2 < MAX_DIST2 / 4)
            {
                if (!contains(b) && !b->contains(this))
                {
                    int typeCountA = std::count_if(bonds.begin(), bonds.end(), [&](Particle* p) -> bool {
                        return p->type == b->type;
                    });
                    int typeCountB = std::count_if(b->bonds.begin(), b->bonds.end(), [&](Particle* p) -> bool {
                            return p->type == type;
                    });
                    // TODO: particles should connect to closest neighbors not to just first in a list
                    canLink = (typeCountA < LINKS_POSSIBLE[type][b->type] && typeCountB < LINKS_POSSIBLE[b->type][type]);
                }
            }
        }
        else
        {
            if (!contains(b) && !b->contains(this))
            {
                dA = 1 / d2;
                dB = 1 / d2;
            }
        }
        double angle = atan2(y - b->y, x - b->x);
        if (d2 < 1) d2 = 1;
        if (d2 < NODE_RADIUS * NODE_RADIUS * 4)
        {
            dA = 1 / d2;
            dB = 1 / d2;
        }
        sx += cos(angle) * dA * SPEED;
        sy += sin(angle) * dA * SPEED;
        b->sx -= cos(angle) * dB * SPEED;
        b->sy -= sin(angle) * dB * SPEED;
    }
    return canLink ? d2 : -1;
}

void Particle::calc()
{
    x += sx;
    y += sy;
    sx *= 0.98;
    sy *= 0.98;
    // velocity normalization
    // idk if it is still necessary
    double magnitude = sqrt(sx * sx + sy * sy);
    if (magnitude > 1.0)
    {
        sx /= magnitude;
        sy /= magnitude;
    }
    // border repulsion
    if (x < BORDER)
    {
        sx += SPEED * 0.05;
        if (x < 0)
        {
            x = -x;
            sx *= -0.5;
        }
    }
    else
        if (x > (WIN_W - BORDER))
        {
            sx -= SPEED * 0.05;
            if (x > WIN_W)
            {
                x = WIN_W * 2 - x;
                sx *= -0.5;
            }
        }
    if (y < BORDER)
    {
        sy += SPEED * 0.05;
        if (y < 0)
        {
            y = -y;
            sy *= -0.5;
        }
    }
    else
        if (y > (WIN_H - BORDER))
        {
            sy -= SPEED * 0.05;
            if (y > WIN_H)
            {
                y = WIN_H * 2 - y;
                sy *= -0.5;
            }
        }
}

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    resize(WIN_W, WIN_H);
    for (int i = 0; i < NODE_COUNT; i++)
        addParticle(qrand() % 3, qrand() % WIN_W, qrand() % WIN_H);
    startTimer(1);
}


Particle* Widget::addParticle(int type, double x, double y)
{
    Particle* res = new Particle(type, x, y);
    fields[(int)(res->x / MAX_DIST)][(int)(res->y / MAX_DIST)].addParticle(res);
    return res;
}

void Widget::logic()
{
    for (auto &column : fields)
        for (auto &field : column)
            for (auto a : field.particles)
                a->calc();

    for (auto link : links)
    {
        Particle* a = link.first;
        Particle* b = link.second;
        float d2 = (a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y);
        if (d2 > MAX_DIST2 / 4)
        {
            a->removeBond(b);
            links.removeOne(link);
       }
        else
        {
            if (d2 > NODE_RADIUS * NODE_RADIUS * 4)
            {
                double angle = atan2(a->y - b->y, a->x - b->x);
                a->sx += cos(angle) * LINK_FORCE * SPEED;
                a->sy += sin(angle) * LINK_FORCE * SPEED;
                b->sx -= cos(angle) * LINK_FORCE * SPEED;
                b->sy -= sin(angle) * LINK_FORCE * SPEED;
            }
        }
    }
    // moving particle to another field
    for (int i = 0; i < FIELD_W; i++)
        for (int j = 0; j < FIELD_H; j++)
        {
            Field& field = fields[i][j];
            for(auto a : field.particles)
            {
                if(((int)(a->x / MAX_DIST) != i) || ((int)(a->y / MAX_DIST) != j))
                {
                    field.removeParticle(a);
                    fields[(int)(a->x / MAX_DIST)][(int)(a->y / MAX_DIST)].addParticle(a);
                }
            }
        }
    // dividing scene into parts to reduce complexity
    for (int i = 0; i < FIELD_W; i++)
    {
        for (int j = 0; j < FIELD_H; j++)
        {
            Field& field = fields[i][j];
            for (int i1 = 0; i1 < field.particles.size(); i1++)
            {
                Particle *a = field.particles[i1];
                Particle *particleToLink = nullptr;
                double particleToLinkMinDist2 = (WIN_W + WIN_H) * (WIN_W + WIN_H);
                for (int j1 = i1 + 1; j1 < field.particles.size(); j1++)
                {
                    Particle *b = field.particles[j1];
                    double d2 = a->applyForce(b);
                    if (d2 != -1 && d2 < particleToLinkMinDist2)
                    {
                        particleToLinkMinDist2 = d2;
                        particleToLink = b;
                    }
                }
                if (i < FIELD_W - 1)
                {
                    for (auto b : fields[i + 1][j].particles)
                    {
                        double d2 = a->applyForce(b);
                        if (d2 != -1 && d2 < particleToLinkMinDist2)
                        {
                            particleToLinkMinDist2 = d2;
                            particleToLink = b;
                        }
                    }
                }
                if (j < FIELD_H - 1)
                {
                    for (auto b : fields[i][j + 1].particles)
                    {
                        double d2 = a->applyForce(b);
                        if (d2 != -1 && d2 < particleToLinkMinDist2)
                        {
                            particleToLinkMinDist2 = d2;
                            particleToLink = b;
                        }
                    }
                    if (i < FIELD_W - 1)
                    {
                        for (auto b : fields[i + 1][j + 1].particles)
                        {
                            double d2 = a->applyForce(b);
                            if (d2 != -1 && d2 < particleToLinkMinDist2)
                            {
                                particleToLinkMinDist2 = d2;
                                particleToLink = b;
                            }
                        }
                    }
                }
                if (particleToLink != nullptr)
                {
                    a->addBond(particleToLink);
                    links.append(Link(a, particleToLink));
                }
            }
        }
    }
}

void Widget::drawScene(QPainter *paint)
{
    paint->fillRect(rect(), COLOR_BG);
    for (auto &column : fields)
        for (auto &field : column)
            for (auto a : field.particles)
            {
                paint->setBrush(COLORS[a->type]);
                paint->setPen(Qt::NoPen);
                paint->drawEllipse((int) a->x - NODE_RADIUS, (int) a->y - NODE_RADIUS, NODE_RADIUS * 2, NODE_RADIUS * 2);
                paint->setPen(COLOR_LINK);
                for (auto b : a->bonds)
                    paint->drawLine((int) a->x, (int) a->y, (int) b->x, (int) b->y);
            }
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter paint(this);
    paint.setRenderHint(QPainter::HighQualityAntialiasing, true);
    drawScene(&paint);
}

void Widget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    repaint();
    for (int i = 0; i < SKIP_FRAMES; i++) logic();
}



