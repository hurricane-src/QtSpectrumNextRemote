#include <iostream>
#include <QThread>
#include <QDateTime>
#include "QtSpectrumNextRemoteTask.h"
#include "NEX.h"
#include "RemoteCommand.h"

void QtSpectrumNextRemoteTask::run()
{
    NEX nex;
    if(nex.load(_file))
    {
        _socket.connectToHost(_host, _port);
        if(_socket.waitForConnected(3000))
        {
            static constexpr uint8_t getbanks_message[1] = {(uint8_t)RemoteCommand::GetBanks};
            send(getbanks_message, sizeof(getbanks_message));
            if(recv(_banks, sizeof(_banks)) >= 0) // == sizeof(_banks)
            {
                sendNEX(nex);
                _socket.close();
            }
        }
        else
        {
            std::cerr << "failed to connect to " << _host.toStdString() << ": " << _socket.errorString().toStdString() << std::endl;
        }
    }
    else
    {
        std::cerr << "failed to load '" << _file.toStdString() << "'" << std::endl;
    }

    emit finished();
}

int QtSpectrumNextRemoteTask::send(const void* data, size_t size)
{
    int64_t now = QDateTime::currentMSecsSinceEpoch();
    int64_t dt = now - _flow_epoch;
    if(dt < 1000) // same second
    {
        _flow_bytes_sent += size;
        for(;;)
        {
            double flow = _flow_bytes_sent;
            double dts = dt;
            dts /= 1000;
            flow /= dts;

            if(flow < _flow_kbs)
            {
                qDebug() << "sent=" << _flow_bytes_sent << " dt=" << dt << " flow=" << flow;
                break;
            }

            qDebug() << "sent=" << _flow_bytes_sent << " dt=" << dt << " flow=" << flow << " (sleep)";

            QThread::msleep(50);
            now = QDateTime::currentMSecsSinceEpoch();
            dt = now - _flow_epoch;
        }
    }
    else
    {
        _flow_epoch = now;
        _flow_bytes_sent = size;
    }

    int64_t n = _socket.write((char*)data, size);
    if(n != size)
    {
        // I don't expect this to happen. If it does, please tell me.
        if(n >= 0)
        {
            std::cerr << "unexpected short write: " << n << "/" << size << " bytes sent" << std::endl;
        }
        else
        {
            std::cerr << "error during write: " << _socket.errorString().toStdString() << std::endl;
        }
        return -1;
    }

    if(!_socket.waitForBytesWritten(3000))
    {
        std::cerr << "error sending " << size << " bytes: " << _socket.errorString().toStdString() << std::endl;
        return -1;
    }

    return n; // == size
}

int QtSpectrumNextRemoteTask::recv(void* data, size_t size)
{
    for(;;)
    {
        if(!_socket.waitForReadyRead(3000))
        {
            // timeout, most likely
            std::cerr << "error reading " << size << " bytes: " << _socket.errorString().toStdString() << std::endl;
            return -1;
        }
        if(_socket.bytesAvailable() >= size)
        {
            _socket.read((char*)data, size);
            return size;
        }
        else
        {
            QThread::msleep(100);
        }
    }
}

void QtSpectrumNextRemoteTask::sendNEX(NEX& nex)
{
    static constexpr int BLOCK_SIZE = 0x100;
    
    if(nex.uses8KBank(_banks[0]))
    {
        std::cerr << "Bank conflict with the server: Can't load this NEX" << std::endl;
        return;
    }

    uint8_t set_banks_command[6] =
    {
        (uint8_t)RemoteCommand::Set8KBank, 4, 0, // index 2
        (uint8_t)RemoteCommand::Set8KBank, 5, 0  // index 5
    };

    uint8_t write_bytes_command[5] =
    {
        (uint8_t)RemoteCommand::WriteAt, // 1KB
        0, 0,
        (BLOCK_SIZE & 0xff), ((BLOCK_SIZE >> 8) & 0xff)
    };

    uint8_t zeroes_slice[5] =
    {
        (uint8_t)RemoteCommand::Zero,
        0, 0,
        (BLOCK_SIZE & 0xff), ((BLOCK_SIZE >> 8) & 0xff)
    };

    // all is good: upload
    // bank 2 will be loaded last as it's the one I'm using for transfers

    for(int page_index = 0; page_index <= 112; page_index++)
    {
        std::cout << "Page " << page_index;

        int page = page_index;

        if(page == 2)
        {
            continue; // skip the page that covers 4 & 5
        }

        if(page == 112)
        {
            page = 2;
        }

        uint8_t* data = nex.getPage(page);

        if(data == nullptr)
        {
            continue;
        }

        std::cout << "Setting bank 4@" << page * 2 << " 5@" << page * 2 + 1 << std::endl;

        // set the bank


        set_banks_command[2] = page * 2;
        set_banks_command[5] = page * 2 + 1;

        send(set_banks_command, sizeof(set_banks_command));

        // load the bank

        // upload by 1KB

        int address = 0x8000;
        for(int offset = 0x0000; offset < 0x4000; offset += BLOCK_SIZE, address += BLOCK_SIZE)
        {
            write_bytes_command[1] = address & 0xff;
            write_bytes_command[2] = (address >> 8) & 0xff;

            uint8_t *current_slice = &data[offset];

            bool zeroes = true;
            for(int i = 0; i < BLOCK_SIZE; ++i)
            {
                if(current_slice[i] != 0)
                {
                    zeroes = false;
                    break;
                }
            }

            std::cout << "Sending "  << address << ' ' << offset << " with 4@" << page * 2 << " 5@" << page * 2 + 1 << std::endl;

            if(!zeroes)
            {
                //logger.AppendHex(write_bytes_command);
                send(write_bytes_command, sizeof(write_bytes_command));

                std::cout << "Sending " << BLOCK_SIZE << " bytes" << std::endl;
                //logger.AppendHex(buffer);
                send(current_slice, BLOCK_SIZE);
            }
            else
            {
                zeroes_slice[1] = address & 0xff;
                zeroes_slice[2] = (address >> 8) & 0xff;

                std::cout << "Sending " << BLOCK_SIZE << " zeroes" << std::endl;
                send(zeroes_slice, sizeof(zeroes_slice));
            }
        }
    }

    // pages have been uploaded

    /*
        F3                 di
        ED 91 50 FF        nextreg $50, $ff
        31 FE FF           ld sp, $fffe
        C3 00 80           jp $8000
    */

    // writes the bootstrap code at $fff0 and jumps
    uint8_t bootstrap_command[]=
    {
        (uint8_t)RemoteCommand::WriteAt, /* address = */ 0xf0, 0xff, /*len = */ 0x0b, 0x00, // 0
        0xf3,                                                                               // 5
        0xed, 0x91, 0x50, 0xff, // restores the ROM                                         // 6
        0x31, 0xfe, 0xff, // SP                                                             // 10
        0xc3, 0x00, 0x80, // PC                                                             // 13
        (uint8_t)RemoteCommand::CloseAndJumpTo, 0xf0, 0xff                                  // 16
    };

    int sp = nex.SP();
    int pc = nex.PC();

    int bootstrap_address = sp - sizeof(bootstrap_command) - 2;

    std::cout << "Bootstrapping PC=" << pc << " SP=" << sp << " @=" << bootstrap_address << std::endl;

    bootstrap_command[1] = ((bootstrap_address) & 0xff); // technically I don't need to mask ...
    bootstrap_command[2] = ((bootstrap_address >> 8) & 0xff);
    bootstrap_command[11] = ((sp) & 0xff);
    bootstrap_command[12] = ((sp >> 8) & 0xff);
    bootstrap_command[14] = ((pc) & 0xff);
    bootstrap_command[15] = ((pc >> 8) & 0xff);
    bootstrap_command[17] = bootstrap_command[1];
    bootstrap_command[18] = bootstrap_command[2];

    // I should put the bootstrap right before SP

    std::cout << "Bootstrapping" << std::endl;
    //logger.AppendHex(bootstrap_command);

    send(bootstrap_command, sizeof(bootstrap_command));

    // disconnection should be a message, so it's done when the queue is processed.
    // not really important though
}
