#include "datagenerator.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QDebug>
#include <QVector>
#include <QVector3D>
#include <QTransform>
#include <cmath>
#include "logReader.h"
#include "mercator.h"



DataGenerator::DataGenerator(){
    _parkingLots = ParkingLots(762, 1700 - 884);
}

void DataGenerator::generateData()
{
    LogReader logReader(":/log/cars/");
    for (const QString& date: logReader.getAvailableDates())
    {
       QHash<LogReader::LogType, QString> tempHash = logReader.getLogNamesForDate(date);
       qDebug() << "date" << date;
       if (!tempHash.isEmpty())
           getCarsGpsPosFromLogFiles(
                       date,
                       tempHash[LogReader::IMAGES_POS],
                       tempHash[LogReader::CARS_POS]);
    }
    _parkingLots.writeImage(871851, 6077399, 872040, 6077539, "parking_lots");
    _parkingLots.writeGraphFile();
//    getCarsFromLogFiles();
//    emit carsGenerated(_cars);
//    emit gridsGenerated(_grids);
}

void DataGenerator::getCarsFromLogFiles()
{
    QFile * logImagesGps = new QFile(":/log/log_5_12_2013/gps_for_each_image_log.txt");
    QFile * logImagesRects = new QFile(":/log/log_5_12_2013/car_rects_log.dat");
    if (!logImagesGps->open(QIODevice::ReadOnly)
            || !logImagesRects->open(QIODevice::ReadOnly))
    {
        qDebug()<<"no file found";
        return;
    }
    QMap<QString, MyPointF> imagePositionHash;
    QMap<QString, QVector<QVector3D> > carPosHash;

    // reading in all images
    QTextStream inPositions(logImagesGps);
    while(!inPositions.atEnd()) {
        QString line = inPositions.readLine();
        QStringList fields = line.split("\t\t");
        QString name = fields[1].split("=")[1];
        qreal x = fields[2].split("=")[1].toDouble();
        qreal y = fields[3].split("=")[1].toDouble();
        qreal theta = fields[4].split("=")[1].toDouble();
        imagePositionHash[name]=MyPointF(x, y, theta);
    }

    // read in all detected cars' rects
    QTextStream inRects(logImagesRects);
    QVector<QString> allUsedImages;
    while(!inRects.atEnd()) {
        QString line = inRects.readLine();
        if (line.contains("TOTAL_NUM_IMAGES"))
        {
            // no error check.
            // This code is awful. Hope noone ever sees this...
            int num = line.split("\t", QString::SkipEmptyParts)[1].toInt();
            for (int i = 0; i < num; ++i)
            {
                line = inRects.readLine();
                allUsedImages.push_back(line);
            }
            continue;
        }
        QStringList fields = line.split("\t");
        QString name = fields[0].split(":")[1];
        qreal x = fields[1].split(":")[1].toDouble();
        qreal y = fields[2].split(":")[1].toDouble();
        qreal z = fields[3].split(":")[1].toDouble();
        carPosHash[name].append(QVector3D(x, y, z));
    }
    getCarPositionsFromAllData(allUsedImages, carPosHash, imagePositionHash);

    logImagesGps->close();
    logImagesRects->close();
    delete logImagesGps;
    delete logImagesRects;
}

void DataGenerator::getCarsGpsPosFromLogFiles(
        const QString& date,
        const QString& imagesGpsFileName,
        const QString& detectedCarsFileName)
{
    QFile * logImagesGps = new QFile(imagesGpsFileName);
    QFile * logImagesRects = new QFile(detectedCarsFileName);
    if (!logImagesGps->open(QIODevice::ReadOnly)
            || !logImagesRects->open(QIODevice::ReadOnly))
    {
        qDebug()<<"no file here";
        return;
    }
    QMap<QString, MyPointF> imagePositionHash;
    QMap<QString, QVector<QPointF> > carPosHash;

    // reading in all images
    QTextStream inPositions(logImagesGps);
    while(!inPositions.atEnd()) {
        QString line = inPositions.readLine();
        QStringList fields = line.split("\t");
        Q_ASSERT(fields.size() == 5);
        QString name = fields[1].split("=")[1];
        qreal x = fields[2].split("=")[1].toDouble();
        qreal y = fields[3].split("=")[1].toDouble();
        qreal theta = fields[4].split("=")[1].toDouble();
        imagePositionHash[name]=MyPointF(x, y, theta);
    }

    // read in all detected cars' rects
    QTextStream inRects(logImagesRects);
    QVector<QString> allUsedImages;
    while(!inRects.atEnd()) {
        QString line = inRects.readLine();
        if (line.contains("TOTAL_NUM_IMAGES"))
        {
            // no error check.
            // This code is awful. Hope noone ever sees this...
            int num = line.split("\t", QString::SkipEmptyParts)[1].toInt();
            for (int i = 0; i < num; ++i)
            {
                line = inRects.readLine();
                allUsedImages.push_back(line);
            }
            continue;
        }
        QStringList fields = line.split("\t");
        QString name = fields[0].split(":")[1];
        qreal x = fields[2].toDouble();
        qreal y = fields[4].toDouble();
        carPosHash[name].append(QPointF(x, y));
    }
    getCarPositionsFromAllDataLaser(date, allUsedImages, carPosHash, imagePositionHash);

    logImagesGps->close();
    logImagesRects->close();
    delete logImagesGps;
    delete logImagesRects;
}

void DataGenerator::updateOccupancy(
        const QString& date,
        const QPointF& thisPointInMeters,
        const qreal &angleOfThisGpsPointSystem,
        QVector<QVector3D>& carPositions,
        const QString &name)
{
    QVector<QPointF> carCorrectPositions;
    qreal maxDist = 30;
    qreal fieldOfView = M_PI * 97. / 180.; // in radians
    QPointF leftMostPoint(maxDist, maxDist * tan(-fieldOfView / 2));
    QPointF rightMostPoint(maxDist, maxDist * tan(fieldOfView / 2));
    QVector<QPointF> marginPoints;
    QTransform transform;
    transform.translate(thisPointInMeters.x(), thisPointInMeters.y());
    transform.rotate(angleOfThisGpsPointSystem);
    marginPoints.push_back(transform.map(leftMostPoint));
    marginPoints.push_back(transform.map(rightMostPoint));
    for (auto carPos: carPositions)
    {
        QPointF carInCameraViewPosition(carPos.z(), carPos.x());
        if (sqrt(QPointF::dotProduct(carInCameraViewPosition, carInCameraViewPosition)) > 20)
        {
            continue;
        }
        QPointF carGlobalPos = transform.map(carInCameraViewPosition);
        carCorrectPositions.push_back(carGlobalPos);
        _cars.push_back(carGlobalPos);
    }
//    _grids[date].add(thisPointInMeters,
//              carCorrectPositions,
//              marginPoints);
//    _parkingLots.update(carCorrectPositions);
}

void DataGenerator::updateOccupancyLaser(
        const QString& date,
        const QPointF& thisPointInMeters,
        const qreal &angleOfThisGpsPointSystem,
        QVector<QPointF>& carPositions,
        const QString &name)
{
    QVector<QPointF> carCorrectPositions;
    qreal maxDist = 20;
    qreal fieldOfView = M_PI * 97. / 180.; // in radians
    QPointF leftMostPoint(maxDist, maxDist * tan(-fieldOfView / 2));
    QPointF rightMostPoint(maxDist, maxDist * tan(fieldOfView / 2));
    QVector<QPointF> marginPoints;
    QTransform transform;
    transform.translate(thisPointInMeters.x(), thisPointInMeters.y());
    transform.rotate(angleOfThisGpsPointSystem);
    marginPoints.push_back(transform.map(leftMostPoint));
    marginPoints.push_back(transform.map(rightMostPoint));
    for (const auto& carPos: carPositions)
    {
        carCorrectPositions.push_back(carPos);
        _cars.push_back(carPos);
    }
    _path.append(thisPointInMeters);
    _grids[date].add(thisPointInMeters,
              carCorrectPositions,
              marginPoints);
    _parkingLots.update(carCorrectPositions);
}

void DataGenerator::getCarPositionsFromAllData(
        const QVector<QString> &allImageNames,
        const QMap<QString, QVector<QVector3D> > &carPosHash,
        const QMap<QString, MyPointF> &imageGpsHash)
{
    _cars.clear();
    int counter = 0;
    for (const auto& name: allImageNames)
    {
        MyPointF thisPoint = imageGpsHash.value(name);
        qreal angleOfThisGpsPointSystem = thisPoint.theta() * 180 / M_PI;
        QVector<QVector3D> carPositions = carPosHash.value(name);
        updateOccupancy(
                    "hack",
                    QPointF(thisPoint.x(), thisPoint.y()),
                    angleOfThisGpsPointSystem,
                    carPositions, name);
        QString name;
        name.setNum(++counter);
        name  = "mymap" + name;
    }
}

void DataGenerator::getCarPositionsFromAllDataLaser(
        const QString& date,
        const QVector<QString> &allImageNames,
        const QMap<QString, QVector<QPointF> > &carPosHash,
        const QMap<QString, MyPointF> &imageGpsHash)
{
    _cars.clear();
    int counter = 0;
    for (const auto& name: allImageNames)
    {
        MyPointF thisPoint = imageGpsHash.value(name);
        qreal angleOfThisGpsPointSystem = thisPoint.theta() * 180 / M_PI;
        QVector<QPointF> carPositions = carPosHash.value(name);
        updateOccupancyLaser(
                    date,
                    QPointF(thisPoint.x(), thisPoint.y()),
                    angleOfThisGpsPointSystem,
                    carPositions,
                    name);
        QString name;
        name.setNum(++counter);
        name  = "mymap" + name;
//        qDebug() << name;
    }
    _parkingLots.updateLeftFree();
}
