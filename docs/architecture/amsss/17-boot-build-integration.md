# Boot and Build Integration

Boot order is initial VMM and heap setup, AMSSS initialization, then default reclaim-provider registration. The explicit PowerShell build compiles each AMSSS source and links every AMSSS object into the kernel image.

