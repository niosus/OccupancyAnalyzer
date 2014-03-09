#ifndef OCCUPANCY_CELL_H
#define OCCUPANCY_CELL_H

#include <QDebug>

class OccupancyCell {
public:
    int occupied;
    int free;
    OccupancyCell()
    {
        occupied = 0;
        free = 0;
    }
    OccupancyCell(int occ, int fr)
    {
        occupied = occ;
        free = fr;
    }

    qreal getFreeProb() const
    {
        return ((qreal) free) / (free + occupied);
    }
};

inline QDebug& operator<< (QDebug& os, OccupancyCell& val)
{
    os<<"free ="<<val.free<<"\n"<<"occupied ="<<val.occupied;
    return os;
}

#endif // OCCUPANCY_CELL_H
