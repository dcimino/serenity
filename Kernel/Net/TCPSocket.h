#pragma once

#include <Kernel/Net/IPv4Socket.h>

class TCPSocket final : public IPv4Socket {
public:
    static NonnullRefPtr<TCPSocket> create(int protocol);
    virtual ~TCPSocket() override;

    enum class State {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting,
    };

    State state() const { return m_state; }
    void set_state(State state) { m_state = state; }

    void set_ack_number(u32 n) { m_ack_number = n; }
    void set_sequence_number(u32 n) { m_sequence_number = n; }
    u32 ack_number() const { return m_ack_number; }
    u32 sequence_number() const { return m_sequence_number; }

    void send_tcp_packet(u16 flags, const void* = nullptr, int = 0);

    static Lockable<HashMap<u16, TCPSocket*>>& sockets_by_port();
    static TCPSocketHandle from_port(u16);

private:
    explicit TCPSocket(int protocol);
    virtual const char* class_name() const override { return "TCPSocket"; }

    static NetworkOrdered<u16> compute_tcp_checksum(const IPv4Address& source, const IPv4Address& destination, const TCPPacket&, u16 payload_size);

    virtual int protocol_receive(const ByteBuffer&, void* buffer, size_t buffer_size, int flags) override;
    virtual int protocol_send(const void*, int) override;
    virtual KResult protocol_connect(FileDescription&, ShouldBlock) override;
    virtual int protocol_allocate_local_port() override;
    virtual bool protocol_is_disconnected() const override;
    virtual KResult protocol_bind() override;

    u32 m_sequence_number { 0 };
    u32 m_ack_number { 0 };
    State m_state { State::Disconnected };
};

class TCPSocketHandle : public SocketHandle {
public:
    TCPSocketHandle() {}

    TCPSocketHandle(RefPtr<TCPSocket>&& socket)
        : SocketHandle(move(socket))
    {
    }

    TCPSocketHandle(TCPSocketHandle&& other)
        : SocketHandle(move(other))
    {
    }

    TCPSocketHandle(const TCPSocketHandle&) = delete;
    TCPSocketHandle& operator=(const TCPSocketHandle&) = delete;

    TCPSocket* operator->() { return &socket(); }
    const TCPSocket* operator->() const { return &socket(); }

    TCPSocket& socket() { return static_cast<TCPSocket&>(SocketHandle::socket()); }
    const TCPSocket& socket() const { return static_cast<const TCPSocket&>(SocketHandle::socket()); }
};
