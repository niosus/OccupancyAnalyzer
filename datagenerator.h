#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include <QHash>
#include <QXmlStreamReader>
#include <QPolygonF>
#include "occupancy_grid.h"
#include "point_with_rot.h"
#include "logReader.h"
#include "parking_lots.h"

class DataGenerator: public QObject
{
    Q_OBJECT

public:
    DataGenerator();
    void generateData();

private:
    QVector<QPointF> _cars;
    QPolygonF _path;
    ParkingLots _parkingLots;
    QHash<QString, OccupancyGrid> _grids;

    void getCarsFromLogFiles();
    void getCarsGpsPosFromLogFiles(const QString &date,
            const QString &imagesGpsFileName,
            const QString &detectedCarsFileName);
    void getCarPositionsFromAllData(
            const QVector<QString> &allImageNames,
            const QMap<QString, QVector<QVector3D> > &carPosHash,
            const QMap<QString, MyPointF> &imageGpsHash);
    void getCarPositionsFromAllDataLaser(
            const QString &date,
            const QVector<QString> &allImageNames,
            const QMap<QString, QVector<QPointF> > &carPosHash,
            const QMap<QString, MyPointF> &imageGpsHash);
    QPointF getPrevGpsPoint(const QString &name,
            const QMap<QString, MyPointF> &imagePositionHash);
    void updateOccupancy(const QString &date,
            const QPointF& thisPointInMeters,
            const qreal &angleOfThisGpsPointSystem,
            QVector<QVector3D> &carPositions,
            const QString &name);
    void updateOccupancyLaser(const QString &date,
            const QPointF& thisPointInMeters,
            const qreal &angleOfThisGpsPointSystem,
            QVector<QPointF>& carPositions,
            const QString &name);

signals:
    void carsGenerated(QVector<QPointF> &cars);
    void pathGenerated(QPolygonF &path);
    void gridsGenerated(QHash<QString, OccupancyGrid>& grid);

};

#endif // DATAGENERATOR_H
