// You may need to build the project (run Qt uic code generator) to get "ui_SpectrumNextRemoteForm.h" resolved

#include "SpectrumNextRemoteForm.h"
#include "ui_SpectrumNextRemoteForm.h"
#include <QSettings>
#include <QMimeData>
#include <QDebug>
#include <QThread>
#include <QDateTime>

SpectrumNextRemoteForm::SpectrumNextRemoteForm(QWidget *parent) :
    QWidget(parent), ui(new Ui::SpectrumNextRemoteForm), _socket(nullptr), _nex(nullptr), _connected(false)
{
    ui->setupUi(this);
    QString spectrumNextAddress = _settings.value("SpectrumNextAddress").toString();
    if(!spectrumNextAddress.isEmpty())
    {
        ui->addressLineEdit->setText(spectrumNextAddress);
        ui->connectPushButton->setEnabled(true);
    }
    QString lastProgram = _settings.value("LastProgram").toString();
    if(!lastProgram.isEmpty())
    {
        ui->fileLineEdit->setText(lastProgram);
    }
    _flow_kbs = _settings.value("Flow", 8192).toInt();
    ui->flowHorizontalSlider->setValue(_flow_kbs);
    ui->flowLineEdit->setText(QString::number(_flow_kbs)); // yes, I mean to convert back

    connect(ui->connectPushButton, SIGNAL(clicked()), this, SLOT(connectPressed()));
    connect(ui->sendPushButton, SIGNAL(clicked()), this, SLOT(sendPressed()));
    connect(ui->addressLineEdit, SIGNAL(textEdited(QString)), this, SLOT(addressLineChanged(QString)));
    connect(ui->fileLineEdit, SIGNAL(textEdited(QString)), this, SLOT(fileLineChanged(QString)));
    connect(ui->flowHorizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(flowSliderValueChanged(int)));
    connect(&_timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));

    setAcceptDrops(true);
}

SpectrumNextRemoteForm::~SpectrumNextRemoteForm()
{
    emptyQueue();

    if(_nex != nullptr)
    {
        delete _nex;
    }
    //delete ui;
}

void SpectrumNextRemoteForm::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* mimedata = event->mimeData();

    if(mimedata->hasUrls())
    {
        for(QUrl url : mimedata->urls())
        {
            logLine(url.path());
        }
        event->acceptProposedAction();
    }
}

void SpectrumNextRemoteForm::dropEvent(QDropEvent* event)
{
    const QMimeData* mimedata = event->mimeData();

    if(mimedata->urls().count() == 1)
    {
        QString program = mimedata->urls()[0].path();
        if(loadNEX(program))
        {
            event->acceptProposedAction();
        }
    }
}

bool SpectrumNextRemoteForm::loadNEX(const QString& program)
{
    if(program.endsWith(".nex"))
    {
        NEX *nex = new NEX();
        if(nex->load(program))
        {
            if(_nex != nullptr)
            {
                delete _nex;
            }
            _nex = nex;
            _settings.setValue("LastProgram", program);

            if(_connected)
            {
                ui->sendPushButton->setEnabled(true);
                return true;
            }
        }
        else
        {
            delete nex;
        }
    }

    return false;
}

void SpectrumNextRemoteForm::logLine(const QString& text)
{
    ui->logPlainTextEdit->appendPlainText(text);
}

void SpectrumNextRemoteForm::logFormat(const char *format, ...)
{
    char text_buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(text_buffer, sizeof(text_buffer), format, args);
    logLine(QString(text_buffer));
    va_end(args);
}

void SpectrumNextRemoteForm::connectPressed()
{
    ui->addressLineEdit->setEnabled(false);
    ui->portLineEdit->setEnabled(false);
    ui->connectPushButton->setEnabled(false);

    _settings.setValue("SpectrumNextAddress", ui->addressLineEdit->text());

    connectToHost();
}

void SpectrumNextRemoteForm::getBanks()
{
    remote(new GetBanksMessage(this));
}

void SpectrumNextRemoteForm::onGetBanks(GetBanksMessage& message)
{
    logLine(message.statusText());
    remote(new GetBanksAnswerMessage(this));
}

void SpectrumNextRemoteForm::onGetBanksAnswer(GetBanksAnswerMessage& message)
{
    logLine(message.statusText());
    if(message.statusCode() >= 0)
    {
        _timer.stop();

        if(_banks == nullptr)
        {
            _banks = new uint8_t[8];
        }
        memcpy(_banks, message.data(), 8);
        logFormat("banks: %02x %02x %02x %02x %02x %02x %02x %02x",
            _banks[0],_banks[1],_banks[2],_banks[3],
            _banks[4],_banks[5],_banks[6],_banks[7]);

        if(_nex == nullptr)
        {
            loadNEX(ui->fileLineEdit->text());
        }

        if(_nex != nullptr)
        {
            ui->sendPushButton->setEnabled(true);
        }
    }
}

void SpectrumNextRemoteForm::sendPressed()
{
    sendNEX();
}

void SpectrumNextRemoteForm::addressLineChanged(const QString& text)
{
    ui->connectPushButton->setEnabled(!text.isEmpty());
}

void SpectrumNextRemoteForm::fileLineChanged(const QString& text)
{
    if(loadNEX(text))
    {
        _settings.setValue("LastProgram", text);
    }
}

void SpectrumNextRemoteForm::flowSliderValueChanged(int)
{
    _flow_kbs = ui->flowHorizontalSlider->value();
    ui->flowLineEdit->setText(QString::number(_flow_kbs));
    _settings.setValue("Flow", _flow_kbs);
}

void SpectrumNextRemoteForm::sendNEX()
{
    static constexpr int BLOCK_SIZE = 0x100;

    if(_nex == nullptr)
    {
        return;
    }

    if((_banks != nullptr) && _nex->uses8KBank(_banks[0]))
    {
        logLine("Bank conflict with the server: Can't load this NEX");
        //MessageBox.Show("Bank conflict with the server", "Can't load this NEX", MessageBoxButtons.OK, MessageBoxIcon.Error);
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
        logFormat("Page %i", page_index);
        qDebug() << "Page " << page_index;

        int page = page_index;

        if(page == 2)
        {
            continue; // skip the page that covers 4 & 5
        }

        if(page == 112)
        {
            page = 2;
        }

        uint8_t* data = _nex->getPage(page);

        if(data == nullptr)
        {
            continue;
        }

        logFormat("Setting bank 4@%i 5@%i", page * 2, page * 2 + 1);
        qDebug() << "Setting bank 4@" << page * 2 << " 5@" << page * 2 + 1;

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

            logFormat("Sending %04x %04x with 4@%i 5@%i", address, offset, page * 2, page * 2 + 1);

            qDebug() << "Sending "  << address << ' ' << offset << " with 4@" << page * 2 << " 5@" << page * 2 + 1;

            if(!zeroes)
            {
                //logger.AppendHex(write_bytes_command);
                send(write_bytes_command, sizeof(write_bytes_command));

                logFormat("Sending %i bytes", BLOCK_SIZE);
                qDebug() << "Sending " << BLOCK_SIZE << " bytes";
                //logger.AppendHex(buffer);
                send(current_slice, BLOCK_SIZE);
            }
            else
            {
                zeroes_slice[1] = address & 0xff;
                zeroes_slice[2] = (address >> 8) & 0xff;

                logFormat("Sending %i zeroes", BLOCK_SIZE);
                qDebug() << "Sending " << BLOCK_SIZE << " zeroes";
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
        (uint8_t)RemoteCommand::JumpTo, 0xf0, 0xff                                          // 16
    };

    int sp = _nex->SP();
    int pc = _nex->PC();

    int bootstrap_address = sp - sizeof(bootstrap_command) - 2;

    logFormat("Bootstrapping PC=%04x SP=%04x @=%04x", pc, sp, bootstrap_address);
    qDebug() << "Bootstrapping PC=" << pc << " SP=" << sp << " @=" << bootstrap_address;

    bootstrap_command[1] = ((bootstrap_address) & 0xff); // technically I don't need to mask ...
    bootstrap_command[2] = ((bootstrap_address >> 8) & 0xff);
    bootstrap_command[11] = ((sp) & 0xff);
    bootstrap_command[12] = ((sp >> 8) & 0xff);
    bootstrap_command[14] = ((pc) & 0xff);
    bootstrap_command[15] = ((pc >> 8) & 0xff);
    bootstrap_command[17] = bootstrap_command[1];
    bootstrap_command[18] = bootstrap_command[2];

    // I should put the bootstrap right before SP

    logLine("Bootstrapping");
    qDebug() << "Bootstrapping";
    //logger.AppendHex(bootstrap_command);

    send(bootstrap_command, sizeof(bootstrap_command));

    // disconnection should be a message so it's done when the queue is processed.
    // not really important though

    logLine("Disconnecting");
    disconnectFromHost();
}

void SpectrumNextRemoteForm::remote(Message* message)
{
    _message_queue_mtx.lock();

    switch(message->messageType())
    {
        case MessageType::Send:
        {
            SendMessage* send_message = dynamic_cast<SendMessage*>(message);
            qDebug() << "remote enqueue: send, size " << send_message->size();
            break;
        }
        case MessageType::Recv:
        {
            RecvMessage* recv_message = dynamic_cast<RecvMessage*>(message);
            qDebug() << "remote enqueue: recv, size " << recv_message->size();
            break;
        }
        case MessageType::Disconnect:
        {
            qDebug() << "remote enqueue: disconnect";
            break;
        }
        default:
        {
            qDebug() << "remote enqueue: " << (int)message->messageType();
            break;
        }
    }
    _message_queue.enqueue(message);
    if(_message_queue.count() == 1)
    {
        qDebug() << "remote enqueue: executing " << (int)message->messageType();
        message->execute();
    }
    _message_queue_mtx.unlock();
}

void SpectrumNextRemoteForm::remoteSendProgress(int n)
{
    logFormat("remoteSendProgress(%i)", n);
    qDebug() << "remoteSendProgress(" << n << ")";
    //QThread::msleep(25);

    int64_t now = QDateTime::currentMSecsSinceEpoch();
    int64_t dt = now - _flow_epoch;
    if(dt < 1000) // same second
    {
        _flow_bytes_sent += n;
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
        _flow_bytes_sent = n;
    }

    _message_queue_mtx.lock();
    if(!_message_queue.isEmpty())
    {
        Message* msg = _message_queue.head();
        qDebug() << "remoteSendProgress(" << n << ") type " << (int)msg->messageType();
        SendMessage* message = dynamic_cast<SendMessage*>(msg);
        if(message == nullptr)
        {
            qDebug() << "remoteSendProgress(" << n << ") bad cast";
            _message_queue_mtx.unlock();
            return;
        }
        qDebug() << "remoteSendProgress(" << n << ") offset " << message->offset() << " size " << message->size();
        if(!message->progress(n))
        {
            qDebug() << "remoteSendProgress(" << n << ") not finished";
            //message->execute();
        }
        else
        {
            qDebug() << "remoteSendProgress(" << n << ") callback";
            message->callback();
            qDebug() << "remoteSendProgress(" << n << ") callback done";
            msg = _message_queue.dequeue();
            qDebug() << "remoteSendProgress(" << n << ") dequeued " << (int)msg->messageType();
            delete message;

            if(!_message_queue.isEmpty())
            {
                Message* next = _message_queue.head();
                qDebug() << "remoteSendProgress(" << n << ") executing " << (int)next->messageType();
                next->execute();
            }
        }
    }
    _message_queue_mtx.unlock();
}

void SpectrumNextRemoteForm::remoteRecvProgress(int n)
{
    logFormat("remoteRecvProgress(%i)", n);
    qDebug() << "remoteRecvProgress(" << n << ")";
    _message_queue_mtx.lock();
    if(!_message_queue.isEmpty())
    {
        Message* msg = _message_queue.head();
        qDebug() << "remoteRecvProgress(" << n << ") type " << (int)msg->messageType();
        RecvMessage* message = dynamic_cast<RecvMessage*>(msg);
        if(message == nullptr)
        {
            qDebug() << "remoteRecvProgress(" << n << ") bad cast";
            _message_queue_mtx.unlock();
            return;
        }
        qDebug() << "remoteRecvProgress(" << n << ") offset " << message->offset() << " size " << message->size();
        size_t remain = message->remain();
        qDebug() << "remoteRecvProgress(" << n << ") remain " << remain;
        if(n > remain)
        {
            n = remain;
        }
        qDebug() << "remoteRecvProgress(" << n << ") progress " << n;
        if(!message->progress(_socket, n))
        {
            qDebug() << "remoteRecvProgress(" << n << ") not finished";
            //message->execute();
        }
        else
        {
            qDebug() << "remoteRecvProgress(" << n << ") callback";
            message->callback();
            qDebug() << "remoteRecvProgress(" << n << ") callback done";
            msg = _message_queue.dequeue();
            qDebug() << "remoteRecvProgress(" << n << ") dequeued " << (int)msg->messageType();
            delete message;

            if(!_message_queue.isEmpty())
            {
                Message* next = _message_queue.head();
                qDebug() << "remoteRecvProgress(" << n << ") executing " << (int)next->messageType();
                next->execute();
            }
        }
    }
    _message_queue_mtx.unlock();
}

void SpectrumNextRemoteForm::connectToHost()
{
    if(_socket != nullptr)
    {
        return;
    }

    _socket = new QTcpSocket(this);

    connect(_socket, SIGNAL(connected()), this, SLOT(remoteConnected()));
    connect(_socket, SIGNAL(disconnected()), this, SLOT(remoteDisconnected()));
    connect(_socket, SIGNAL(readyRead()), this, SLOT(remoteReadyRead()));
    // connect(_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(remoteBytesWritten(qint64))); // Doesn't work but ...
    QObject::connect(_socket, &QTcpSocket::bytesWritten, this, &SpectrumNextRemoteForm::remoteBytesWritten); // ... does
    connect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(remoteErrorOccurred(QAbstractSocket::SocketError)));
    logLine("connecting");

    _socket->connectToHost(ui->addressLineEdit->text(), ui->portLineEdit->text().toInt());
    _timer.start(5000);
}

void SpectrumNextRemoteForm::disconnectFromHost()
{
    remote(new DisconnectMessage(this));
}

void SpectrumNextRemoteForm::send(void *data, size_t size)
{
    remote(new SendMessage(this, data, size));
}

void SpectrumNextRemoteForm::emptyQueue()
{
    _message_queue_mtx.lock();
    while(!_message_queue.isEmpty())
    {
        Message* msg = _message_queue.dequeue();
        delete msg;
    }
    _message_queue_mtx.unlock();
}

void SpectrumNextRemoteForm::remoteConnected()
{
    logLine("remoteConnected");

    _connected = true;
    getBanks();
}

void SpectrumNextRemoteForm::remoteDisconnected()
{
    if(_socket != nullptr)
    {
        _socket->deleteLater();
        _socket = nullptr;
    }

    emptyQueue();

    ui->connectPushButton->setEnabled(true);
    ui->addressLineEdit->setEnabled(true);
    ui->sendPushButton->setEnabled(false);
    _connected = false;
}

void SpectrumNextRemoteForm::remoteBytesWritten(quint64 bytes)
{
    remoteSendProgress(bytes);
}

void SpectrumNextRemoteForm::remoteReadyRead()
{
    remoteRecvProgress(_socket->bytesAvailable());
}

void SpectrumNextRemoteForm::remoteErrorOccurred(QAbstractSocket::SocketError err)
{
    ui->addressLineEdit->setEnabled(true);
    ui->portLineEdit->setEnabled(true);
    ui->connectPushButton->setEnabled(true);

    if(_socket != nullptr)
    {
        logLine("error: " + _socket->errorString());
        qDebug() << "error: " << _socket->errorString();
        _socket->close();
    }
    else
    {
        logFormat("error: code %i",  err);
    }

    emptyQueue();
}

void SpectrumNextRemoteForm::timerTimeout()
{
    qDebug() << "timeout";
    if(!_connected)
    {
        remoteDisconnected();
    }
    _timer.stop();
    emptyQueue();
    disconnectFromHost();
}
