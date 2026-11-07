#pragma once
struct StatResetTestConnection
{
    int closeRequests = 0;
    int resetRequests = 0;
    void SendCloseNpcRequest()
    {
        ++closeRequests;
    }
    void SendResetCharacterPointRequest()
    {
        ++resetRequests;
    }
};
struct StatResetTestSocket
{
    StatResetTestConnection connection;
    StatResetTestConnection* ToGameServer()
    {
        return &connection;
    }
};
inline StatResetTestSocket statResetTestSocket;
inline StatResetTestSocket* SocketClient = &statResetTestSocket;
