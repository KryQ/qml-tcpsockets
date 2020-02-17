/*
 * Copyright (C) 2016 EPFL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

/**
 * @file TcpSocket.cpp
 * @brief QML wrapper source for QTcpSocket
 * @author Ayberk Özgür
 * @date 2016-11-10
 */

#include "LocalSocket.h"

#include <QHostAddress>

namespace QMLTcpSockets{

LocalSocket::LocalSocket(QQuickItem* parent):
    QQuickItem(parent),
    socket(this)
{
    connect(&socket, SIGNAL(connected()), this, SIGNAL(connected()));
    connect(&socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(emitErrorAsInt(QAbstractSocket::SocketError)));
    connect(&socket, SIGNAL(readyRead()), this, SLOT(publish()));
    peer = "";
}

LocalSocket::LocalSocket(QIntPtr* socketDescriptor, QQuickItem* parent):
    LocalSocket(parent)
{
    socket.setSocketDescriptor(socketDescriptor->ptr);
    socketDescriptor->deleteLater();
    peer = socket.serverName();
}

LocalSocket::~LocalSocket(){
    socket.flush();
}

void LocalSocket::setSocketDescriptor(QIntPtr* socketDescriptor){
    socket.flush();
    socket.disconnectFromServer();

    socket.setSocketDescriptor(socketDescriptor->ptr);
    socketDescriptor->deleteLater();

    QString peer = socket.serverName();
    if(this->peer != peer){
        this->peer = peer;
        emit peerChanged();
    }

    emit connected();
}

/*void LocalSocket::setSocketOption(int option, QVariant value){
    socket.setSocketOption((QAbstractSocket::SocketOption)option, value);
}*/

void LocalSocket::setPeer(QString peer){
    if(peer != this->peer){
        if(socket.state() != QLocalSocket::UnconnectedState)
            qWarning() << "TcpSocket::setPeer(): Can only set peer while disconnected.";
        else{
            this->peer = peer;
            emit peerChanged();
        }
    }
}

void LocalSocket::connectToHost(){
    socket.connectToServer(peer);
}

void LocalSocket::disconnectFromHost(){
    socket.disconnectFromServer();
}

void LocalSocket::emitErrorAsInt(QAbstractSocket::SocketError socketError){
    emit error(socketError);
}

void LocalSocket::publish(){
    QList<int> list;
    QByteArray receivedBytes = socket.readAll();
    for(char c : receivedBytes)
        list << (int)(*((unsigned char*)(&c)));
    emit bytesReceived(list);
}

bool LocalSocket::writeBytes(QList<int> bytes){
    QByteArray container;
    for(int b : bytes){
        if(b < 0 || b > 0xFF){
            qWarning() << "TcpSocket::writeBytes(): bytes contain integer not in [0x00,0xFF], not writing";
            return false;
        }
        else
            container.append((char)b);
    }

    int numBytesToWrite = container.size();
    const char* bytesRaw = container.constData();
    while(numBytesToWrite > 0){
        int bytesWritten = socket.write(bytesRaw, numBytesToWrite);
        if(bytesWritten < 0){
            qWarning() << "TcpSocket::writeBytes(): Bytes were not written: " << socket.errorString();
            return false;
        }
        numBytesToWrite -= bytesWritten;
        bytesRaw += bytesWritten;
    }
    socket.flush();

    return true;
}

}
