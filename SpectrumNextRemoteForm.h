//
// Created by ediazfer on 15/08/24.
//

#ifndef QTSPECTRUMNEXTREMOTE_SPECTRUMNEXTREMOTEFORM_H
#define QTSPECTRUMNEXTREMOTE_SPECTRUMNEXTREMOTEFORM_H

#include <QWidget>
#include <QSettings>
#include <QTcpSocket>
#include <QQueue>
#include <QMutex>
#include <QTimer>
#include "NEX.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class SpectrumNextRemoteForm;
}
QT_END_NAMESPACE

class SpectrumNextRemoteForm : public QWidget
{
Q_OBJECT

    enum class RemoteCommand : uint8_t
    {
        Ping = 0,       //
        GetBanks = 1,   // page[0..8]
        Set8KBank = 2,  // page, bank
        WriteAt = 3,    // offset, len10, bytes
        JumpTo = 4,     // offset
        Echo = 5,
        Zero = 6,       // offset, len10
    };

    enum class MessageType
    {
        Terminate = 0,
        Connect,
        Disconnect,
        IsConnected,
        Send,
        Recv
    };

    class Message
    {
    public:
        Message(MessageType mt, SpectrumNextRemoteForm* form):
            _mt(mt),_form(form),_status_code{0} {}
        virtual ~Message() {}

        virtual void execute() {}
        virtual void callback() {}

        MessageType messageType() const { return _mt; }
        int32_t statusCode() const { return _status_code; }
        QString statusText() const { return _status_text; }
        void setStatusCode(int32_t code) { _status_code = code; }
        void setStatusText(QString text) { _status_text = text; }

    protected:
        SpectrumNextRemoteForm* _form;
        QString _status_text;
        int32_t _status_code;
        const MessageType _mt;
    };

    class SendMessage:public Message
    {
    public:
        SendMessage(SpectrumNextRemoteForm* form, const void* data, size_t size):
            Message(MessageType::Send, form), _offset(0), _size(size)
        {
            _data = new uint8_t[size];
            memcpy(_data, data, size);
        }
        ~SendMessage() { delete[] _data; }

        void execute() override
        {
            _form->logFormat("write %i bytes", _size - _offset);
            _form->socket()->write((char*)&_data[_offset], _size - _offset);
        }

        void* data() const { return _data; }
        off_t offset() const { return _offset; }
        size_t size() const { return _size; }
        bool progress(size_t n)
        {
            _offset += n;
            return _offset == _size;
        }

    private:
        uint8_t* _data;
        off_t _offset;
        size_t _size;
    };

    class RecvMessage:public Message
    {
    public:
        RecvMessage(SpectrumNextRemoteForm* form, size_t size):
            Message(MessageType::Recv, form), _data(new uint8_t[size]), _offset(0), _size(size) {}
        ~RecvMessage() { delete[] _data; }

        void* data() const { return _data; }
        off_t offset() const { return _offset; }
        size_t size() const { return _size; }
        size_t remain() const { return _size - _offset; }
        bool progress(QTcpSocket* socket, size_t n)
        {
            _form->logFormat("read %i bytes", n);
            assert(n <= remain());
            socket->read((char*)&_data[_offset], n);
            _offset += n;
            return _offset == _size;
        }

    private:
        uint8_t* _data;
        off_t _offset;
        size_t _size;
    };

    class DisconnectMessage:public Message
    {
    public:
        DisconnectMessage(SpectrumNextRemoteForm* form):
            Message(MessageType::Disconnect, form) {}
        ~DisconnectMessage() { }

        void execute() override
        {
            QTcpSocket* socket = _form->socket();
            if(socket != nullptr)
            {
                socket->close();
            }
        }
    };

    class GetBanksMessage: public SendMessage
    {
        static constexpr uint8_t _data[1] = {(uint8_t)RemoteCommand::GetBanks};
    public:
        GetBanksMessage(SpectrumNextRemoteForm* form):
            SendMessage(form, (void*)_data, sizeof(_data)) {}

        void callback() override
        {
            _form->onGetBanks(*this);
        }
    };

    class GetBanksAnswerMessage: public RecvMessage
    {
    public:
        GetBanksAnswerMessage(SpectrumNextRemoteForm* form):
            RecvMessage(form, 8) {}

        void callback() override
        {
            _form->onGetBanksAnswer(*this);
        }
    };

public:
    explicit SpectrumNextRemoteForm(QWidget *parent = nullptr);

    ~SpectrumNextRemoteForm() override;

protected:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    bool loadNEX(const QString& program);
    void sendNEX();
    void getBanks();

    void remote(Message* message);
    void remoteSendProgress(int n);
    void remoteRecvProgress(int n);

    void logLine(const QString& text);
    void logFormat(const char *format, ...);

    void connectToHost();
    void disconnectFromHost();
    void send(void *data, size_t size);
    void emptyQueue();

public:
    void onGetBanks(GetBanksMessage& message);
    void onGetBanksAnswer(GetBanksAnswerMessage& message);
    QTcpSocket* socket() { return _socket; }

private slots:
    void connectPressed();
    void sendPressed();
    void addressLineChanged(const QString& text);
    void fileLineChanged(const QString& text);

    void flowSliderValueChanged(int);

    void remoteConnected();
    void remoteDisconnected();
    void remoteBytesWritten(quint64 bytes);
    void remoteReadyRead();
    void remoteErrorOccurred(QAbstractSocket::SocketError);

    void timerTimeout();

private:
    Ui::SpectrumNextRemoteForm *ui;
    QSettings _settings;
    QTimer _timer;

    QTcpSocket* _socket;
    QRecursiveMutex _message_queue_mtx;
    QQueue<Message*> _message_queue;
    QQueue<Message*> _recv_message_queue;

    int64_t _flow_bytes_sent;
    int64_t _flow_epoch;

    NEX* _nex;
    uint8_t* _banks;

    int _flow_kbs;

    bool _connected;
};

#endif //QTSPECTRUMNEXTREMOTE_SPECTRUMNEXTREMOTEFORM_H
