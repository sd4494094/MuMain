#pragma once
struct TestConnection
{
    int closes = 0;
    std::vector<std::wstring> commands;
    void SendCloseNpcRequest()
    {
        ++closes;
    }
    void SendPublicChatMessage(const wchar_t*, const wchar_t* command)
    {
        commands.emplace_back(command);
    }
};
struct TestSocket
{
    TestConnection connection;
    TestConnection* ToGameServer()
    {
        return &connection;
    }
};
inline TestSocket socket;
inline TestSocket* SocketClient = &socket;
