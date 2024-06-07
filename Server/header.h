#ifndef HEADER_H
#define HEADER_H

#include <QString>
#include <QList>
#include <QtCore>

#define HEADER_SIZE 128
#define START_BYTE 0x1F

enum MessageType
{
    Connection,
    Disconnection,
    Text,
    FileInfo,
    FileData
};

struct Header
{
private:
    // Maps for converting MessageType to QString and vice versa
    std::pmr::map<MessageType, QString> MessageTypeToString = {
        {Connection, "Connection"},
        {Disconnection, "Disconnection"},
        {Text, "Text"},
        {FileInfo, "FileInfo"},
        {FileData, "FileData"}
    };

    std::pmr::map<QString, MessageType> StringToMessageType = {
        {"Connection", Connection},
        {"Disconnection", Disconnection},
        {"Text", Text},
        {"FileInfo", FileInfo},
        {"FileData", FileData}
    };

public:
    MessageType type;
    QString fileName;
    int dataSize;
    int totalPacket;
    int no;

    Header() {
        this->type = Text;
        this->fileName = "null";
        this->dataSize = 0;
        this->totalPacket = 0;
        this->no = 0;
    }

    Header(MessageType type, int dataSize, int totalPacket, int no)
    {
        this->type = type;
        this->fileName = "null";
        this->dataSize = dataSize;
        this->totalPacket = totalPacket;
        this->no = no;
    }

    Header(MessageType type, QString fileName, int dataSize, int totalPacket, int no)
    {
        this->type = type;
        this->fileName = fileName;
        this->dataSize = dataSize;
        this->totalPacket = totalPacket;
        this->no = no;
    }

    Header(QByteArray headerData)
    {
        QString headerDataStr = headerData;

        // Parse header data
        this->type = StringToMessageType[headerDataStr.split(",")[0].split(':')[1]];
        this->fileName = headerDataStr.split(",")[1].split(':')[1];
        this->dataSize = headerDataStr.split(",")[2].split(':')[1].toInt();
        this->totalPacket = headerDataStr.split(",")[3].split(':')[1].toInt();
        this->no = headerDataStr.split(",")[4].split(':')[1].split('\0')[0].toInt();
    }

    QString toString()
    {
        return "Type:" + MessageTypeToString[type] + ",Name:" + fileName + ",Size:" + QString::number(dataSize) + ",Packet:"
               + QString::number(totalPacket) + ",No:" + QString::number(no) + "\0";
    }

    QByteArray toByteArray()
    {
        QByteArray headerData = toString().toUtf8();
        headerData.prepend(START_BYTE);
        headerData.resize(HEADER_SIZE);
        return headerData;
    }
};

#endif // HEADER_H
