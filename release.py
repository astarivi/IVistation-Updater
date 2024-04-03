# Credits to LoveMHz and Rocky5
# https://github.com/LoveMHz/XBEpy/blob/87568a3d2d9c89e80a12f5261331b90e543e5f29/XBE.py

import sys
import struct


class XBE_HEADER():
    def __init__(self, data):
        XOR_EP_DEBUG = 0x94859D4B  # Entry Point (Debug)
        XOR_EP_RETAIL = 0xA8FC57AB  # Entry Point (Retail)
        XOR_KT_DEBUG = 0xEFB1F152  # Kernel Thunk (Debug)
        XOR_KT_RETAIL = 0x5B6D40B6  # Kernel Thunk (Retail)

        self.dwMagic = struct.unpack('4s', data[0:4])[0]  # Magic number [should be "XBEH"]
        self.pbDigitalSignature = struct.unpack('256B', data[4:260])  # Digital signature
        self.dwBaseAddr = struct.unpack('I', data[260:264])[0]  # Base address
        self.dwSizeofHeaders = struct.unpack('I', data[264:268])[0]  # Size of headers
        self.dwSizeofImage = struct.unpack('I', data[268:272])[0]  # Size of image
        self.dwSizeofImageHeader = struct.unpack('I', data[272:276])[0]  # Size of image header
        self.dwTimeDate = struct.unpack('I', data[276:280])[0]  # Timedate stamp
        self.dwCertificateAddr = struct.unpack('I', data[280:284])[0]  # Certificate address
        self.dwSections = struct.unpack('I', data[284:288])[0]  # Number of sections
        self.dwSectionHeadersAddr = struct.unpack('I', data[288:292])[0]  # Section headers address

        # Struct init_flags
        self.dwInitFlags = struct.unpack('I', data[292:296])[0]  # Mount utility drive flag
        self.init_flags_mount_utility_drive = None  # Mount utility drive flag
        self.init_flags_format_utility_drive = None  # Format utility drive flag
        self.init_flags_limit_64mb = None  # Limit development kit run time memory to 64mb flag
        self.init_flags_dont_setup_harddisk = None  # Don't setup hard disk flag
        self.init_flags_unused = None  # Unused (or unknown)
        self.init_flags_unused_b1 = None  # Unused (or unknown)
        self.init_flags_unused_b2 = None  # Unused (or unknown)
        self.init_flags_unused_b3 = None  # Unused (or unknown)

        self.dwEntryAddr = struct.unpack('I', data[296:300])[0]  # Entry point address
        self.dwTLSAddr = struct.unpack('I', data[300:304])[0]  # TLS directory address
        self.dwPeStackCommit = struct.unpack('I', data[304:308])[0]  # Size of stack commit
        self.dwPeHeapReserve = struct.unpack('I', data[308:312])[0]  # Size of heap reserve
        self.dwPeHeapCommit = struct.unpack('I', data[312:316])[0]  # Size of heap commit
        self.dwPeBaseAddr = struct.unpack('I', data[316:320])[0]  # Original base address
        self.dwPeSizeofImage = struct.unpack('I', data[320:324])[0]  # Size of original image
        self.dwPeChecksum = struct.unpack('I', data[324:328])[0]  # Original checksum
        self.dwPeTimeDate = struct.unpack('I', data[328:332])[0]  # Original timedate stamp
        self.dwDebugPathnameAddr = struct.unpack('I', data[332:336])[0]  # Debug pathname address
        self.dwDebugFilenameAddr = struct.unpack('I', data[336:340])[0]  # Debug filename address
        self.dwDebugUnicodeFilenameAddr = struct.unpack('I', data[340:344])[0]  # Debug unicode filename address
        self.dwKernelImageThunkAddr = struct.unpack('I', data[344:348])[0]  # Kernel image thunk address
        self.dwNonKernelImportDirAddr = struct.unpack('I', data[348:352])[0]  # Non kernel import directory address
        self.dwLibraryVersions = struct.unpack('I', data[352:356])[0]  # Number of library versions
        self.dwLibraryVersionsAddr = struct.unpack('I', data[356:360])[0]  # Library versions address
        self.dwKernelLibraryVersionAddr = struct.unpack('I', data[360:364])[0]  # Kernel library version address
        self.dwXAPILibraryVersionAddr = struct.unpack('I', data[364:368])[0]  # XAPI library version address
        self.dwLogoBitmapAddr = struct.unpack('I', data[368:372])[0]  # Logo bitmap address
        self.dwSizeofLogoBitmap = struct.unpack('I', data[372:376])[0]  # Logo bitmap size

        self.dwEntryAddr_f = self.dwEntryAddr ^ XOR_EP_RETAIL  # Entry point address


if __name__ == '__main__':
    target_version = int(sys.argv[1])

    print("Versioning XBE to: " + str(target_version))

    with open("release/update.xbe", "r+b") as xbe_file:
        # Parse the header
        header = XBE_HEADER(xbe_file.read(376))

        # Seek to the version address, which is cert + 0x00AC
        xbe_file.seek(
            (header.dwCertificateAddr - header.dwBaseAddr) + 0x00AC
        )

        xbe_file.write(struct.pack('<I', target_version))
