#ifndef OCCUPANCY_TEST_H
#define OCCUPANCY_TEST_H

#include <QtTest/QtTest>
#include "occupancy_grid.h"

class TestOccupancyGrid: public QObject
{
    Q_OBJECT
private slots:
    void updateOccupancy();
};

#endif // OCCUPANCY_TEST_H
