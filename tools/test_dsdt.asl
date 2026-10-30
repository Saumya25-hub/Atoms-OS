DefinitionBlock ("dsdt.aml", "DSDT", 2, "ATOMS ", "ATOMSVM ", 1)
{
    Scope (\_SB)
    {
        Device (PCI0)
        {
            Name (_HID, EisaId ("PNP0A03"))
            Name (_BBN, Zero)
            Name (_PRT, Package (4)
            {
                Package (4) { 0x0001FFFF, Zero, Zero, 10 },
                Package (4) { 0x0002FFFF, Zero, Zero, 11 },
                Package (4) { 0x0003FFFF, Zero, Zero, 11 },
                Package (4) { 0x0004FFFF, Zero, Zero, 11 }
            })
            Name (_CRS, ResourceTemplate ()
            {
                WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
                    0x0000, 0x0000, 0x00FF, 0x0000, 0x0100)
                IO (Decode16, 0x0CF8, 0x0CF8, 0x01, 0x08)
                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                    0x0000, 0x0000, 0x0CF7, 0x0000, 0x0CF8)
                WordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
                    0x0000, 0x0D00, 0xFFFF, 0x0000, 0xF300)
                DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed, NonCacheable, ReadWrite,
                    0x00000000, 0x80000000, 0xFEBFFFFF, 0x00000000, 0x7EC00000)
            })
        }

        Device (SYSR)
        {
            Name (_HID, EisaId ("PNP0C02"))
            Name (_CRS, ResourceTemplate ()
            {
                IO (Decode16, 0x0600, 0x0600, 0x01, 0x06)
                IO (Decode16, 0x0408, 0x0408, 0x01, 0x04)
            })
        }

        Device (KBD)
        {
            Name (_HID, EisaId ("PNP0303"))
            Name (_STA, 0x0F)
            Name (_CRS, ResourceTemplate ()
            {
                IO (Decode16, 0x0060, 0x0060, 0x01, 0x01)
                IO (Decode16, 0x0064, 0x0064, 0x01, 0x01)
                IRQNoFlags () { 1 }
            })
        }

        Device (COM1)
        {
            Name (_HID, EisaId ("PNP0501"))
            Name (_STA, 0x0F)
            Name (_CRS, ResourceTemplate ()
            {
                IO (Decode16, 0x03F8, 0x03F8, 0x01, 0x08)
                IRQNoFlags () { 4 }
            })
        }
    }
}
