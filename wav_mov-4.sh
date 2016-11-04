#!/bin/sh

# Go in the proper directory. Modify as needed
#cd /usr/src/linux/

# Wavelan ISA driver
mv drivers/net/i82586.h     drivers/net/wireless/
mv drivers/net/wavelan.h    drivers/net/wireless/
mv drivers/net/wavelan.p.h  drivers/net/wireless/
mv drivers/net/wavelan.c    drivers/net/wireless/

# Wavelan Pcmcia driver
# Warning : Rename headers because of conflict with ISA driver
# We use the same conventions as the ISA driver, xxx.p.h is the private header
mv drivers/net/pcmcia/i82593.h      drivers/net/wireless/
mv drivers/net/pcmcia/wavelan.h     drivers/net/wireless/wavelan_cs.h
mv drivers/net/pcmcia/wavelan_cs.h  drivers/net/wireless/wavelan_cs.p.h
mv drivers/net/pcmcia/wavelan_cs.c  drivers/net/wireless/

# Netwave Pcmcia driver
mv drivers/net/pcmcia/netwave_cs.c  drivers/net/wireless/
