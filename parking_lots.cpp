#include "parking_lots.h"
#include <QDebug>
#include <QImage>
#include <QColor>
#include <QFile>
#include <QCoreApplication>
#include <QImageWriter>
#include <iostream>
#include <iomanip>

ParkingLots::ParkingLots()
{
}

ParkingLots::ParkingLots(const qreal &leftCorner, const qreal &bottomCorner)
{
    QPointF tempPoint;
    QPointF leftBottomCorner(leftCorner, bottomCorner);
    OccupancyCell emptyCell;
    QPointF leftBottomUtm(871831, 6077369);
    qDebug() << "num of rows"<<_numOfRows;
    qDebug() << "num of cols"<<_numOfCols;
    for (int colNum = 0; colNum < _numOfCols; ++colNum)
    {
        for (int rowNum = 0; rowNum < _numOfRows; ++rowNum)
        {
            tempPoint.ry() =
                    leftBottomCorner.y() * _metersInPx + leftBottomUtm.y() +
                    rowNum * _height * _metersInPx +
                    (_height / 2) * _metersInPx;
            tempPoint.rx() =
                    leftBottomCorner.x() * _metersInPx + leftBottomUtm.x() +
                    colNum * _width * _metersInPx +
                    (_width / 2) * _metersInPx;
            // the rows are as follows:
            // 01___23___45___67___ ...
            // so we need to account for the gap
            tempPoint.rx() = tempPoint.x() + (colNum / _gap_every) * (_gap) * _metersInPx;
            _centers.append(tempPoint);
            _occupancy.append(emptyCell);
            _currentOccupancy.append(emptyCell);
        }
    }
}

void ParkingLots::reInitCurrentOccupancy()
{
    _currentOccupancy.clear();
    OccupancyCell emptyCell;
    for (int i = 0; i < _occupancy.size(); ++i)
    {
        _currentOccupancy.append(emptyCell);
    }
}

void ParkingLots::updateClosest(const QPointF &detection)
{
    qreal minDist = 1E+20; //should be big enough
    int minIndex = -1;
    for (int i = 0; i < _centers.size(); ++i)
    {
        qreal currentDist = sqrt(QPointF::dotProduct(_centers[i] - detection, _centers[i] - detection));
        if (minDist > currentDist)
        {
            minDist = currentDist;
            minIndex = i;
        }
    }
    if (minIndex < 0)
    {
        qDebug()<<"parking lots update failed for" << detection;
        return;
    }
    _occupancy[minIndex].occupied++;
    _currentOccupancy[minIndex].occupied++;
    _tempUpdatedIndeces.append(minIndex);
    _realDetections.append(detection);
}

void ParkingLots::updateLeftFree(const QString& date)
{
    for (int i = 0; i < _occupancy.size(); ++i)
    {
        if (!_tempUpdatedIndeces.contains(i))
        {
            _occupancy[i].free++;
            _currentOccupancy[i].free++;
        }
    }
    _tempUpdatedIndeces.clear();
    writeGraphFile(date, &_currentOccupancy);
    reInitCurrentOccupancy();
}

void ParkingLots::update(const QVector<QPointF> &cars)
{
    for (const QPointF& car: cars)
    {
        this->updateClosest(car);
    }
}


void ParkingLots::writeGraphFile(const QString& type, const QVector<OccupancyCell> *occupancy) const
{
    QFile file("/home/igor/Work/Thesis/MiscCode/OccupancyAnalyzer/grids/" + type + "_parkings_graph.dat");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    if (occupancy == nullptr)
    {
        occupancy = &_occupancy;
    }
    QTextStream out(&file);
    out.setRealNumberPrecision(10);
    out << "rows: " << _numOfRows << "\t";
    out << "cols: " << _numOfCols << "\t";
    out << "gap_every: " << _gap_every << endl;
    for (int i = 0; i < _centers.size(); ++i)
    {
        out << "x: " << _centers[i].x() << " y: " << _centers[i].y();
        out << " free_prob: " << occupancy->at(i).getFreeProb() << endl;
    }
    qDebug() << "file written";
}

void ParkingLots::writeImage(
        int xMin,
        int yMin,
        int xMax,
        int yMax,
        QString imageName) const
{
    int width = xMax - xMin;
    int height = yMax - yMin;
    int cellSize = 10; //px
    QImage image(width * cellSize, height * cellSize, QImage::Format_RGB32);
    qDebug() << xMin << xMax << yMin << yMax;
    QColor colorGray = QColor(50, 50, 50);

    for (int i = 0; i < _occupancy.size(); ++i)
    {
        int currentCenterX = floor((_centers[i].x() - xMin) * cellSize);
        int currentCenterY = floor((_centers[i].y() - yMin) * cellSize);
        for (int xx = currentCenterX - 2 * cellSize; xx < currentCenterX + 2 * cellSize; ++xx)
        {
           for (int yy = currentCenterY - cellSize; yy < currentCenterY + cellSize; ++yy)
           {
               OccupancyCell cell = _occupancy[i];
               if (cell.free + cell.occupied < 1)
               {
                    image.setPixel(xx, image.height() - yy, colorGray.rgb());
               }
               // amount of green color from 0 to 1
               qreal greenAmount = (qreal) cell.free / (cell.free + cell.occupied);
               greenAmount*=255;
               QColor color = QColor(255 - greenAmount, greenAmount, 0);
               image.setPixel(xx, image.height() - yy, color.rgb());
           }
        }
    }

    QString dirPath = QCoreApplication::applicationDirPath();
    dirPath = dirPath.left(dirPath.lastIndexOf('/')) + "/OSMViewer/grids/";
    qDebug() << "saving!!!";
    qDebug() << image.size();
    QImageWriter writer("/home/igor/Work/Thesis/MiscCode/OccupancyAnalyzer/grids/" + imageName + ".jpg");
    if (!writer.write(image))
    {
        qDebug() << writer.errorString();
    }
}
