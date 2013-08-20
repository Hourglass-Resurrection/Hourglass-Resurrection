// Original code by jcoffland, posted here: http://stackoverflow.com/questions/2901694/programatically-detect-number-of-physical-processors-cores-or-if-hyper-threading
// Adapted by Warepire to suit the Hourglass-Ressurection project

#include "CPUinfo.h"

#include <intrin.h>
#include <string>

void CPUInfo(/*optional*/ int* logicalCores, /*optional*/ int* physicalCores, /*optional*/ bool* hyperThreading)
{
	int regs[4];

	// Get vendor
	char vendor[12];
	__cpuid(regs, 0);
	((unsigned *)vendor)[0] = regs[1]; // EBX
	((unsigned *)vendor)[1] = regs[3]; // EDX
	((unsigned *)vendor)[2] = regs[2]; // ECX
	std::string cpuVendor = std::string(vendor, 12);

	// Logical core count per CPU
	__cpuid(regs, 1);
	int cores = (regs[1] >> 16) & 0xff; // EBX[23:16]
	if(logicalCores) *logicalCores = cores;

	int physical;
	if (cpuVendor == "GenuineIntel")
	{
		// Get DCP cache info
		__cpuid(regs, 4);
		physical = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1
		if(physicalCores) *physicalCores = physical;
	}
	else if (cpuVendor == "AuthenticAMD")
	{
		// Get NC: Number of CPU cores - 1
		__cpuid(regs, 0x80000008);
		physical = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
		if(physicalCores) *physicalCores = physical;
	}

	if(hyperThreading)
	{
		// Detect hyper-threads
		__cpuid(regs, 1);
		unsigned cpuFeatures = regs[3]; // EDX
		*hyperThreading = cpuFeatures & (1 << 28) && physical < cores;
	}
}