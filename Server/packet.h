#ifndef PACKET_H
#define PACKET_H

#include <QString>
#include <QByteArray>

#include "header.h"

#define DATA_SIZE 1024
#define TAIL_SIZE 4

class Packet
{
public:
    Header header;
    QByteArray data;

    Packet() {
        this->header = Header();
        this->data = QByteArray(DATA_SIZE + TAIL_SIZE, 0);
    };

    Packet(QByteArray rawData)
    {
        this->header = Header(rawData.left(HEADER_SIZE));
        this->data = rawData.mid(HEADER_SIZE, this->header.dataSize);
    };

    Packet(Header header, QByteArray data)
    {
        this->header = header;
        this->data = data;
    };

    Packet(Header header, QString message)
    {
        this->header = header;
        this->data = message.toUtf8();
    };

    ~Packet() {};

    QByteArray toByteArray()
    {
        // Create a raw data array and append the header to it
        QByteArray rawData;
        rawData.append(this->header.toByteArray());

        // If the data size is not equal to DATA_SIZE, resize the data array
        // Then append the data to the raw data array
        if(this->data.size() != DATA_SIZE)
        {
            QByteArray resizedData = this->data;
            resizedData.resize(DATA_SIZE + TAIL_SIZE);
            rawData.append(resizedData);
        }
        else
        {
            rawData.append(this->data);
        }

        return rawData;
    };
};

#endif // PACKET_H
