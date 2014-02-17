#include <QCoreApplication>
#include "datagenerator.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    DataGenerator* dataGenerator = new DataGenerator;
    dataGenerator->generateData();
    delete dataGenerator;
    return a.exec();
}
