using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WebStreetTest
{
    // Represents the flags that can be set on a TCP packet
    public enum TcpFlags
    {
        SYN = 0x02,
        ACK = 0x10,
        FIN = 0x01
    }
    // Represents the mode for generating fake source IP addresses
    public enum FakeSrcIPAndPortMode
    {
        Random,
        Fixed,
        None
    }
    // Represents the type of data to send in a TCP packet
    public enum DataType
    {
        Random,
        Fixed,
    }
   
    
    public class TcpPacketSender
    {
        //Imports the C++ functions from the DLL
        [DllImport("lib/TcpPacketSender.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern IntPtr TcpPacketSender_new();

        [DllImport("lib/TcpPacketSender.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern void TcpPacketSender_Send(IntPtr sender, string sourceIp, string destinationIp, ushort sourcePort, ushort destinationPort, ushort flag, string data, uint dataSize);

        [DllImport("lib/TcpPacketSender.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern void TcpPacketSender_delete(IntPtr sender);

        private bool isRunning = false;
        private FakeSrcIPAndPortMode fakeSrcIPAndPortMode = FakeSrcIPAndPortMode.Random;
        private string SrcIP = "192.168.0.1";
        private string DstIP = "172.247.189.55";
        private ushort SrcPort = 12345;
        private ushort DstPort = 80;
        private uint ThreadNum = (uint)Environment.ProcessorCount;
        private Thread[] threads = new Thread[Environment.ProcessorCount];
        private DataType dataType = DataType.Random;
        private string ?data = null;
        private uint dataSize = 0;
        private Random random = new Random();
        private TcpFlags flags;
        private readonly object syncLock = new object();
        public ulong syncUlong { get; set; }

        private string GenerateRandomData(uint dataSize)
        {
            byte[] data = new byte[dataSize];
            random.NextBytes(data);
            return data.ToString();
        }

        private string GenerateRandomIP()
        {
            int[] ip = new int[4];
            for (int i = 0; i < 4; i++)
            {
                ip[i] = random.Next(256);
            }
            return ip[0] + "." + ip[1] + "." + ip[2] + "." + ip[3];
        }

        private void SendPacket()
        {
            IntPtr ptr = TcpPacketSender_new();
            if (this.fakeSrcIPAndPortMode == FakeSrcIPAndPortMode.Random)
            {
                string IP;
                if (DataType.Random == this.dataType)
                {
                    while (isRunning)
                    {
                        IP = GenerateRandomIP();
                        TcpPacketSender_Send(ptr, IP, DstIP, (ushort)random.Next(ushort.MaxValue), DstPort, (ushort)flags, GenerateRandomData(dataSize), dataSize);
                        lock(syncLock)
                        {
                            syncUlong++;
                        }
                    }
                }
                else
                {
                    while (isRunning)
                    {
                        IP = GenerateRandomIP();
                        TcpPacketSender_Send(ptr, IP, DstIP, (ushort)random.Next(ushort.MaxValue), DstPort, (ushort)flags, data, dataSize);
                        lock (syncLock)
                        {
                            syncUlong++;
                        }
                    }
                }                
            }
            else
            {
                if (this.dataType == DataType.Random)
                {
                    while (isRunning)
                    {
                        TcpPacketSender_Send(ptr, SrcIP, DstIP, SrcPort, DstPort, (ushort)flags, GenerateRandomData(dataSize), dataSize);
                        lock (syncLock)
                        {
                            syncUlong++;
                        }
                    }
                }
                else
                {
                    while (isRunning)
                    {
                        TcpPacketSender_Send(ptr, SrcIP, DstIP, SrcPort, DstPort, (ushort)flags, data, dataSize);
                        lock (syncLock)
                        {
                            syncUlong++;
                        }
                    }
                }
            }
            TcpPacketSender_delete(ptr);
        }

        public void Build(string DstIP, ushort DstPort, string data, uint dataSize, uint ThreadNum, TcpFlags flags, DataType type)
        {
            this.DstIP = DstIP;
            this.DstPort = DstPort;
            this.data = data;
            if(data == null && DataType.Random != type)
            {
                this.dataSize = (uint)this.data.Length;
            }
            else
            {
                this.dataSize = dataSize;
            }
            this.fakeSrcIPAndPortMode = FakeSrcIPAndPortMode.Random;
            this.ThreadNum = ThreadNum;
            this.dataType = type;
            this.flags = flags;
            if (this.threads == null)
            {
                threads = new Thread[this.ThreadNum];
            }
        }

        public void Build(string SrcIP, ushort SrcPort, string DstIP, ushort DstPort, string data, uint dataSize, uint ThreadNum, TcpFlags flags, DataType type, FakeSrcIPAndPortMode mode = FakeSrcIPAndPortMode.Fixed)
        {
            this.SrcIP = SrcIP;
            this.SrcPort = SrcPort;
            this.DstIP = DstIP;
            this.DstPort = DstPort;
            this.data = data;
            if (data == null && DataType.Random != type)
            {
                this.dataSize = (uint)this.data.Length;
            }
            else
            {
                this.dataSize = dataSize;
            }
            this.fakeSrcIPAndPortMode = mode;
            this.ThreadNum = ThreadNum;
            this.dataType = type;
            this.flags = flags;
            if (this.threads == null)
            {
                threads = new Thread[this.ThreadNum];
            }
        }

        public void Run()
        {
            for (int i = 0; i < this.ThreadNum; i++)
            {
                threads[i] = new Thread(new ThreadStart(SendPacket));
                isRunning = true;
                threads[i].Start();
            }

        }

        public void Stop()
        {
            isRunning = false;
        }
    }
}