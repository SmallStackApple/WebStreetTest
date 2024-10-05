#include <winsock2.h>
#include <iostream>
#include <random>
#include <vector>
#include <winsock.h>
#include <ws2ipdef.h>
//g++ shows IP_HDRINCL isn't define, so I define it manually
#define IP_HDRINCL 2
#pragma comment(lib,"ws2_32.lib")

/*
    csharp
    example:

    using System.Runtime.InteropServices;

    [DllImport("lib/TcpPacketSender.dll", CallingConvention = CallingConvention.Cdecl)]
    static extern IntPtr TcpSynPacketSender_new();

    [DllImport("lib/TcpPacketSender.dll", CallingConvention = CallingConvention.Cdecl)]
    static extern void TcpPacketSender_Send(IntPtr sender, string sourceIp, string destinationIp, ushort sourcePort, ushort destinationPort, ushort flag, string data, uint dataSize);

    [DllImport("lib/TcpPacketSender.dll", CallingConvention = CallingConvention.Cdecl)]
    static extern void TcpPacketSender_delete(IntPtr sender);

    IntPtr sender = TcpPacketSender_new();

    Don't set the srcIp == 127.X.X.X , it won't work.

    TcpPacketSender_Send(sender, "223.5.5.5", "192.168.0.1", 1000, 80, 0x02, null, 0);

    TcpPacketSender_delete(sender);
*/

// Define IP header structure
struct IpHeader {
    unsigned char VersionAndIHL;
    unsigned char TypeOfService;
    unsigned short TotalLength;
    unsigned short Identification;
    unsigned short FlagsAndFragmentOffset;
    unsigned char TimeToLive;
    unsigned char Protocol;
    unsigned short HeaderChecksum;
    unsigned int SourceAddress;
    unsigned int DestinationAddress;
};

// Define TCP header structure
struct TcpHeader {
    unsigned short SourcePort;
    unsigned short DestinationPort;
    unsigned int SequenceNumber;
    unsigned int AcknowledgmentNumber;
    unsigned char DataOffsetAndReserved;
    unsigned char Flags;
    unsigned short WindowSize;
    unsigned short Checksum;
    unsigned short UrgentPointer;
};

class TcpPacketSender {
private:
    SOCKET socket;

    // Convert a structure to a byte sequence
    template <typename T>
    std::vector<unsigned char> GetBytesFromStruct(const T& structure) {
        std::vector<unsigned char> bytes(sizeof(T));
        memcpy(&bytes[0], &structure, sizeof(T));
        return bytes;
    }

    // Calculate checksum
    unsigned short ComputeChecksum(const std::vector<unsigned char>& data) {
        int sum = 0;
        for (size_t i = 0; i < data.size(); i += 2) {
            sum += (data[i] << 8) | data[i + 1];
        }
        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        return (unsigned short)~sum;
    }

public:
    TcpPacketSender() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        socket = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL, 0, 0);
        int flag = 1;
        setsockopt(socket, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(flag));
    }

    ~TcpPacketSender() {
        closesocket(socket);
        WSACleanup();
    }

    void Send(const char* sourceIp, const char* destinationIp, unsigned short sourcePort, unsigned short destinationPort, unsigned short flag, const unsigned char* data, size_t dataSize) {
        std::vector<unsigned char> packet = BuildTcpSynPacket(sourceIp, destinationIp, sourcePort, destinationPort, flag, data, dataSize);

        sockaddr_in destAddr;
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(destinationPort);
        destAddr.sin_addr.s_addr = inet_addr(destinationIp);

        sendto(socket, (const char*)&packet[0], packet.size(), 0, (sockaddr*)&destAddr, sizeof(destAddr));
    }

    std::vector<unsigned char> BuildTcpSynPacket(const char* sourceIp, const char* destinationIp, unsigned short sourcePort, unsigned short destinationPort, unsigned short flag, const unsigned char* data, size_t dataSize) {
        // Initialize IP header
        IpHeader ipHeader;
        ipHeader.VersionAndIHL = (4 << 4) | 5;
        ipHeader.TypeOfService = 0;
        ipHeader.TotalLength = htons(20 + 20);
        ipHeader.Identification = htons(rand());
        ipHeader.FlagsAndFragmentOffset = 0;
        ipHeader.TimeToLive = 64;
        ipHeader.Protocol = 6;
        ipHeader.HeaderChecksum = 0;
        ipHeader.SourceAddress = inet_addr(sourceIp);
        ipHeader.DestinationAddress = inet_addr(destinationIp);

        // Calculate checksum
        ipHeader.HeaderChecksum = ComputeChecksum(GetBytesFromStruct(ipHeader));
        ipHeader.HeaderChecksum = ~ipHeader.HeaderChecksum;

        // Initialize TCP header
        TcpHeader tcpHeader;
        tcpHeader.SourcePort = htons(sourcePort);
        tcpHeader.DestinationPort = htons(destinationPort);
        tcpHeader.SequenceNumber = htonl(rand());
        tcpHeader.AcknowledgmentNumber = 0;
        tcpHeader.DataOffsetAndReserved = (5 << 4);
        tcpHeader.Flags = flag;
        tcpHeader.WindowSize = htons(65535);
        tcpHeader.Checksum = 0;
        tcpHeader.UrgentPointer = 0;

        // Create pseudo header
        unsigned int pseudoHeaderSourceAddress = ipHeader.SourceAddress;
        unsigned int pseudoHeaderDestinationAddress = ipHeader.DestinationAddress;
        std::vector<unsigned char> pseudoHeader(12);
        memcpy(&pseudoHeader[0], &pseudoHeaderSourceAddress, 4);
        memcpy(&pseudoHeader[4], &pseudoHeaderDestinationAddress, 4);
                pseudoHeader[8] = 0;
        pseudoHeader[9] = ipHeader.Protocol;
        unsigned short tcpLength = htons(20);
        memcpy(&pseudoHeader[10], &tcpLength, 2);

        // Calculate TCP header checksum
        std::vector<unsigned char> tcpHeaderBytes = GetBytesFromStruct(tcpHeader);
        std::vector<unsigned char> checksumData = pseudoHeader;
        checksumData.insert(checksumData.end(), tcpHeaderBytes.begin(), tcpHeaderBytes.end());
        tcpHeader.Checksum = ComputeChecksum(checksumData);
        tcpHeader.Checksum = ~tcpHeader.Checksum;

        // Create packet
        std::vector<unsigned char> ipHeaderBytes = GetBytesFromStruct(ipHeader);
        std::vector<unsigned char> packet;
        packet.insert(packet.end(), ipHeaderBytes.begin(), ipHeaderBytes.end());
        packet.insert(packet.end(), tcpHeaderBytes.begin(), tcpHeaderBytes.end());

        // Insert data at the end of the packet if it's not null
        if (data != nullptr && dataSize > 0) {

            packet.insert(packet.end(), data, data + dataSize);
            
            // Update the IP header TotalLength field
            ipHeader.TotalLength = htons(ntohs(ipHeader.TotalLength) + dataSize);
            // Recalculate the IP header checksum
            ipHeader.HeaderChecksum = ComputeChecksum(GetBytesFromStruct(ipHeader));
            ipHeader.HeaderChecksum = ~ipHeader.HeaderChecksum;
            // Update the packet with the new IP header bytes
            ipHeaderBytes = GetBytesFromStruct(ipHeader);
            std::copy(ipHeaderBytes.begin(), ipHeaderBytes.end(), packet.begin());

        }
        return packet;
    }
};


extern "C" {
    __declspec(dllexport) TcpPacketSender* TcpPacketSender_new() {
        return new TcpPacketSender();
    }

    __declspec(dllexport) void TcpPacketSender_Send(TcpPacketSender* sender, const char* sourceIp, const char* destinationIp, unsigned short sourcePort, unsigned short destinationPort, unsigned short flag, const unsigned char* data, size_t dataSize) {
        sender->Send(sourceIp, destinationIp, sourcePort, destinationPort, flag, data, dataSize);
    }

    __declspec(dllexport) void TcpPacketSender_delete(TcpPacketSender* sender) {
        delete sender;
    }
}
