// Placeholder — Phase 5 will add Avalonia GUI (default) and CLI verb routing.
// For now, just verify the project builds and references the protocol library.

using BootloaderTool.Protocol.Crc;

var crc = new Crc32();
byte[] sample = { 0x12, 0x34, 0x56, 0x78 };
uint result = crc.Compute(sample);
Console.WriteLine($"CRC32(0x12345678) = 0x{result:X8}");
